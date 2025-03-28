/*==========================================================================
Description
  This file taps regular BT UART data (hci command, event, ACL packets)
  and writes into QXDM. It also writes the controller logs (controller's
  printf strings, LMP RX and TX data) received from BT SOC over the UART.

# Copyright (c) 2013, 2016 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc..

===========================================================================*/
#include "main.h"
#include "bt_qxdmlog.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cutils/properties.h>
#include <fcntl.h>
#include <linux/stat.h>
#include <sys/time.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "utils.h"

#define UART_IPC_LOG_COLLECTION_OLD_PATH  "/sys/kernel/debug/ipc_logging/msm_serial_hs/log"
#define UART_IPC_LOG_COLLECTION_NEW_ROME_DIR  "/sys/kernel/debug/ipc_logging/7570000"
#define UART_IPC_LOG_COLLECTION_NEW_CHEROKEE_DIR  "/sys/kernel/debug/ipc_logging/c171000"
#define UART_IPC_LOG_TX_FILE_NAME    ".uart_tx/log"
#define UART_IPC_LOG_RX_FILE_NAME    ".uart_rx/log"
#define UART_IPC_LOG_STATE_FILE_NAME    ".uart_state/log"
#define UART_IPC_LOG_PWR_FILE_NAME    ".uart_pwr/log"
#define UART_TX_LOG_COLLECTION_PATH        "/sys/kernel/debug/msm_serial_hs/tx_data.0"
#define UART_RX_LOG_COLLECTION_PATH        "/sys/kernel/debug/msm_serial_hs/rx_data.0"
#define MAX_NUM_OF_UART_IPC_LOGS (20)
#define UART_IPC_PATH_BUF_SIZE   (255)

#ifdef DIAG_ENABLED
#ifdef LOG_BT_ENABLE
#include "diag_lsm.h"
#include "msgcfg.h"
#include "diagi.h"
#include "diaglogi.h"
#include "diagpkt.h"
#include "diagcmd.h"
#ifdef WCNSS_IBS_ENABLED
#include "wcnss_ibs.h"
#endif
#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "WCNSS_FILTER"

#define LLM_DEBUG_LE_CONTROL_PDU_RX(ppkt)\
        MSG_SPRINTF_3(MSG_SSID_BT,MSG_LEGACY_MED,"LE CPDU RX: %s, c: 0x%2X, len:%d", psLE_ControlPDU[( (((ppkt)[4]) >= MAX_LE_CONTROL_PDU_SIZE) ?  (MAX_LE_CONTROL_PDU_SIZE-1) :  ((ppkt)[4]))], ppkt[4],(ppkt)[3]);

#define LLM_DEBUG_LE_CONTROL_PDU_TX(ppkt)\
        MSG_SPRINTF_3(MSG_SSID_BT,MSG_LEGACY_MED,"LE CPDU TX: %s, c: 0x%2X, len:%d", psLE_ControlPDU[( (((ppkt)[4]) >= MAX_LE_CONTROL_PDU_SIZE) ?  (MAX_LE_CONTROL_PDU_SIZE-1) :  ((ppkt)[4]))], ppkt[4],(ppkt)[3]);

#define LLM_DEBUG_LINK_MANAGER_STATE(ppkt)\
        MSG_SPRINTF_3(MSG_SSID_BT,MSG_LEGACY_HIGH,"LLM LM  h:%d, st: %s, ev:%s", (ppkt)[2],psLMstate[(ppkt)[0]], psLMevent[(ppkt)[1]]);

#define LLM_DEBUG_CONNECTION_MANAGER_STATE(ppkt)\
        MSG_SPRINTF_3(MSG_SSID_BT,MSG_LEGACY_HIGH,"LLM CM  h:%d, st: %s, ev:%s", (ppkt)[2],psCMstate[(ppkt)[0]], psCMevent[(ppkt)[1]]);

#define LLM_DEBUG_SECURITY_STATE(ppkt)\
        MSG_SPRINTF_3(MSG_SSID_BT,MSG_LEGACY_HIGH,"LLM SEQ h:%d, st: %s, ev:%s", (ppkt)[2],psSECstate[(ppkt)[0]], psSECevent[(ppkt)[1]]);

#define LLM_DEBUG_LE_CONNECTION_MANAGER_STATE(ppkt)\
        MSG_SPRINTF_3(MSG_SSID_BT,MSG_LEGACY_HIGH,"LLM-LE CM   h:%d, st: %s, ev:%s", (ppkt)[2],psCMstate_LE[(ppkt)[0]], psCMevent[(ppkt)[1]]);

#define LLM_DEBUG_LE_CHANNEL_MAP_UPDATE_STATE(ppkt)\
        MSG_SPRINTF_3(MSG_SSID_BT,MSG_LEGACY_HIGH,"LLM-LE CH_MAP_UPD  h:%d, st: %s, ev:%s", (ppkt)[2],psLE_ChMapUpdateState[(0x1F&((ppkt)[0]))], psCMevent[(ppkt)[1]]);

#define LLM_DEBUG_LE_ENCRYTION_STATE(ppkt)\
        MSG_SPRINTF_3(MSG_SSID_BT,MSG_LEGACY_HIGH,"LLM-LE ENCR h:%d, st: %s, ev:%s", (ppkt)[2],psLE_EncryptionState[(ppkt)[0]], psCMevent[(ppkt)[1]]);

#define MAX_LE_CONTROL_PDU_SIZE (0x16)
#define DIAG_SSR_BT_CMD     0x0007
#define PRINT_BUF_SIZE      (260*3 +2)
#define ROME_CRASH_RAMDUMP_PATH        "/data/misc/bluetooth"
#define MAX_CRASH_DUMP_COUNT  10
#define CRASH_DUMP_PATH_BUF_SIZE 50
#define LAST_SEQUENCE_NUM 0xFFFF
#define PROC_PANIC_PATH     "/proc/sysrq-trigger"
#define CRASH_DUMP_SIGNATURE_SIZE 24
#define MAGIC_CANNOT_OPEN_SRC_PATH (-99)

static char *get_reset_reason_str(Reset_reason_e reason);
static void handle_event(uint8 *eventBuf, int length);
static void send_str_log(uint8 *strBuf);
static void send_cntlr_log(uint8 *cntlrBuf, int length);
static void format_lmp(uint8 *dst, uint8 *src, int32 pktLen);
static void send_log(uint8 *Buf, int length, int type);
void *ssr_bt_handle(void *req_pkt, uint16 pkt_len);
static int dump_UART_logs(char* spath, char* dpath, int buffer_size, int water_mark);
int bt_ssr_level ();
extern int fd_transport;
#ifdef PANIC_ON_SOC_CRASH
extern int fd_sysrq;
#endif //PANIC_ON_SOC_CRASH
extern g_crash_dump_started;
int g_timer_stopped_instance = false;

static bool enable_force_hcidump = false;
static bool enable_unified_logging = false;
static bool enable_snoop_log = false;
static bool enable_crashdump = true;

static unsigned char dump_cnt =0;
static const diagpkt_user_table_entry_type ssr_bt_tbl[] =
{ /* susbsys_cmd_code lo = 7 , susbsys_cmd_code hi = 7, call back function */
    {DIAG_SSR_BT_CMD, DIAG_SSR_BT_CMD, ssr_bt_handle},
};

static Reason_map_st reset_reason [] = {
    { BT_CRASH_REASON_UNKNOWN        ,    "Unknown"},
    { BT_CRASH_REASON_SW_REQUESTED   ,    "SW Requested"},
    { BT_CRASH_REASON_STACK_OVERFLOW ,    "Stack Overflow"},
    { BT_CRASH_REASON_EXCEPTION      ,    "Exception"},
    { BT_CRASH_REASON_ASSERT         ,    "Assert"},
    { BT_CRASH_REASON_TRAP           ,    "Trap"},
    { BT_CRASH_REASON_OS_FATAL       ,    "OS Fatal"},
    { BT_CRASH_REASON_HCI_RESET      ,    "HCI Reset"},
    { BT_CRASH_REASON_PATCH_RESET    ,    "Patch Reset"},
    { BT_CRASH_REASON_POWERON        ,    "Power ON"},
    { BT_CRASH_REASON_WATCHDOG       ,    "Watchdog"},
};

char * psLE_ControlPDU[MAX_LE_CONTROL_PDU_SIZE] = {
    "LL_CONNECTION_UPDATE_REQ", /*0x00*/
    "LL_CHANNEL_MAP_REQ",       /*0x01*/
    "LL_TERMINATE_IND",         /*0x02*/
    "LL_ENC_REQ",               /*0x03*/
    "LL_ENC_RSP",               /*0x04*/
    "LL_START_ENC_REQ",         /*0x05*/
    "LL_START_ENC_RSP",         /*0x06*/
    "LL_UNKNOWN_RSP",           /*0x07*/
    "LL_FEATURE_REQ",           /*0x08*/
    "LL_FEATURE_RSP",           /*0x09*/
    "LL_PAUSE_ENC_REQ",         /*0x0A*/
    "LL_PAUSE_ENC_RSP",         /*0x0B*/
    "LL_VERSION_IND",           /*0x0C*/
    "LL_REJECT_IND",            /*0x0D*/
    "LL_SLAVE_FEATURE_REQ",     /*0x0E*/
    "LL_CONNECTION_PARAM_REQ",  /*0x0F*/
    "LL_CONNECTION_PARAM_RSP",  /*0x10*/
    "LL_REJECT_IND_EXT",        /*0x11*/
    "LL_PING_REQ",              /*0x12*/
    "LL_PING_RSP",              /*0x13*/
    "LL_LENGTH_REQ",            /*0x14*/
    "LL_LENGTH_RSP",            /*0x15*/
};

char * psLMstate[] = {
    "LMsRESET_PND=0",
    "LMsREADY",
    "LMsLC_PAGE",
    "LMsLC_PAGE_CANCEL",
    "LMsLC_ROLE_SWITCH",
    "LMsACL_CONNECTION_SETUP",
    "LMsREMOTE_ACL_SETUP_W4_HOST",
    "LMsMASTER_LINK_KEY_W4_KEY_SIZE_MASK_REQ",
   "INVALID_LM_STATE",
   "INVALID_LM_STATE",
   "INVALID_LM_STATE",
   "INVALID_LM_STATE"
};

char * psLMevent[] = {
    "LMeHCI_INQUIRY",
    "LMeHCI_INQUIRY_CANCEL",
    "LMeHCI_CREATE_CONNECTION",
    "LMeHCI_CREATE_CONNECTION_CANCEL",

    "LMeHCI_ACCEPT_CONNECTION_REQUEST",
    "LMeHCI_REJECT_CONNECTION_REQUEST",

    "LMeCONNECTION_REQUEST",
    "LMeCONNECTION_COMPLETE",
    "LMeCONNECTION_TERMINATED",
    "LMePERIODIC_INQUIRY",
    "LMeHOST_TO",
    "LMeRMC_INQUIRY_RESULT",
    "LMeRMC_INQUIRY_COMPLETE",
    "LMeRMC_PAGE_COMPLETE",
    "LMeRMC_TOKEN_LMs_RETURN",

    "LMeRMC_ROLE_SWITCH_COMPLETE",
    "LMeROLE_SWITCH_ORIGINATE",
    "LMeROLE_SWITCH_REQUESTED",
    "LMeROLE_SWITCH_COMPLETE",
    "LMeHCI_MASTER_LINK_KEY",
    "LMeKEY_SIZE_MASK_RES",
    "LMeNOT_ACCEPTED_KEY_SIZE_MASK_REQ",
    "LMeKEY_SIZE_MASK_RES_TO",

    "LMeLM_TIMEOUT_PAGE",

    "LMeHCI_INQUIRY_TIMEOUT",
    "LMePAGESCAN_CONNECTION_COLLISION",

    "LMeNULL",
    "INVALID_LM_EVENT",
    "INVALID_LM_EVENT",
    "INVALID_LM_EVENT",
    "INVALID_LM_EVENT",
    "INVALID_LM_EVENT"
};

char * psCMstate[] = {
    "CMsIDLE",
    "CMsW4_LMS_DISCONNECT",
    "CMsCONNTERM",
    "CMsM_LM_SETUP",
    "CMsM_CONNSETUP",
    "CMsS_LM_SETUP",
    "CMsS_CONNSETUP",
    "CMsCONN_ACTIVE",
    "CMsCONN_HOLD",
    "CMsCONN_HOLD_EXIT_DISCONNECT_PEND",
    "CMsCONN_SNIFF",
    "CMsCONN_SNIFF_EXIT_DISCONNECT_PEND",
    "CMsCONN_PARK",
    "CMsCONN_UNPARK_PERIODIC",
    "CMsCONN_UNPARK_PEND",
    "CMsCONN_UNPARK_DISCONNECT_PEND",
    "CMsCONN_VS_TEST_ACTIVE",
    "INVALID_CM_STATE",
    "INVALID_CM_STATE",
    "INVALID_CM_STATE",
    "INVALID_CM_STATE"
};

char * psCMevent[] = {
    "CMePAGE_SUCESS",
    "CMePAGESCAN_SUCESS",
    "CMeHOST_CONN_ACCEPT",
    "CMeHOST_CONN_REJECT",
    "CMeHOST_DISCONNECT",
    "CMeAUTHENTICATION_COMPLETE",
    "CMePAIRING_COMPLETE",
    "CMeAUTHENTION_FAILURE",
    "CMeENCRYPTION_COMPLETE",
    "CMeENCRYPTION_SETUP_FAILURE",
    "CMeKEY_CHANGE_FAILURE",
    "CMeLMS_LMP_ACK",
    "CMeLMS_DISCONNECT_COMPLETE",
    "CMeROLE_CHANGE_COMPLETE",
    "CMeHOST_INITIATE_HOLD",
    "CMeENTER_HOLD",
    "CMeEXIT_HOLD",
    "CMeHOST_INITIATE_SNIFF",
    "CMeENTER_SNIFF",
    "CMeEXIT_SNIFF",
    //"CMeEXIT_SNIFF_DISCONNECT",
    "CMeEXIT_SNIFF_TO",

    "CMeHOST_INITIATE_PARK",

    "CMeENTER_PARK_DONE",

    "CMeBB_UNPARK_DONE",
    "CMeBB_ACCESS_REQUEST_FAILURE",
    "CMeUNPARK_TO",
    "CMeEXIT_PARK_DONE",

    "CMeUNPARK_HOST",
    "CMeUNPARK_PERIODIC",
    "CMeUNPARK_DISCONNECT",
    "CMeUNPARK_ACCESS_REQUEST",

    "CMeLMP_UNPARK_REQ",

    "CMeUNPARK_PERIODIC_FAIL",

    "CMeHOST_CHANGE_PACKET_TYPE",
    "CMeHOST_SET_CONN_ENCRYPTION",
    "CMeHOST_CHANGE_CONN_LINK_KEY",

    "CMeLMP_NAME_REQ",
    "CMeLMP_NAME_REQ_NOT_ACCEPTED",
    "CMeLMP_NAME_RES",
    "CMeLMP_ACCEPTED",
    "CMeLMP_NOT_ACCEPTED",
    "CMeLMP_CLKOFFSET_REQ",
    "CMeLMP_CLKOFFSET_RES",
    "CMeLMP_DETACH",
    "CMeLMP_IN_RAND",
    "CMeLMP_COMB_KEY",
    "CMeLMP_UNIT_KEY",
    "CMeLMP_AU_RAND",
    "CMeLMP_SRES",
    "CMeLMP_TEMP_RAND",
    "CMeLMP_TEMP_KEY",
    "CMeLMP_ENCRYPTION_MODE_REQ ",
    "CMeLMP_ENCRYPTION_KEY_SIZE_REQ",
    "CMeLMP_SWITCH_REQ ",
    "CMeLMP_SNIFF_REQ",
    "CMeLMP_PARK_REQ",
    //"CMeLMP_SET_BROADCAST_SCAN_WINDOW",
    //"CMeLMP_MODIFY_BEACON",
    "CMeLMP_AUTO_RATE",
    "CMeLMP_PREFERRED_RATE",
    "CMeLMP_VERSION_REQ",
    "CMeLMP_VERSION_RES",
    "CMeLMP_FEATURES_REQ",
    "CMeLMP_FEATURES_RES",
    "CMeLMP_QUALITY_OF_SERVICE ",
    "CMeLMP_QUALITY_OF_SERVICE_REQ",
    "CMeLMP_MAX_SLOT",
    "CMeLMP_MAX_SLOT_REQ",
    "CMeLMP_SETUP_COMPLETE",
    "CMeLMP_USE_SEMI_PERMANENT_KEY ",
    "CMeLMP_HOST_CONNECTION_REQ ",
    "CMeLMP_SLOT_OFFSET",
    "CMeLMP_PAGE_SCAN_MODE_REQ",
    "CMeLMP_SUPERVISION_TIMEOUT ",
    "CMeLMP_PAGE_SCAN_MODE_REQ_ACCEPTED",
    "CMeLMP_PAGE_MODE_REQ_ACCEPTED",
    "CMeLMP_PAGE_SCAN_MODE_REQ_NOT_ACCEPTED",
    "CMeHCI_WRITE_PAGE_SCAN_MODE",
    "CMeABORT_CONNECTION",
    "CMeLMP_QUALITY_OF_SERVICE_REQ_ACCEPTED",
    "CMeLMP_QUALITY_OF_SERVICE_REQ_NOT_ACCEPTED",
    "CMeLMP_ACCEPTED_TEST_ACTIVATE",
    "CMeLMP_ACCEPTED_TEST_CONTROL",
    "CMeVS_TEST_COMPLETE",
    "CMeLMP_NOT_ACCEPTED_TEST_ACTIVATE",
    "CMeLMP_NOT_ACCEPTED_TEST_CONTROL",
    "CMeLMP_FEATURES_EXT_PAGE_1",
    "CMeLMP_FEATURES_EXT_PAGE_2",

    /* LE events */
    "CMeLE_CONNECT_REQ",
    "CMeLE_TERMINATE_CONNECTION_LOCAL",
    "CMeLE_TERMINATE_CONNECTION_REMOTE",
    "CMeLE_TERMINATE_CONNECTION_ACK",

    "CMeLE_CHANNEL_MAP_UPDATE_LOCAL",
    "CMeLE_CHANNEL_MAP_UPDATE_REMOTE",
    "CMeLE_CHANNEL_MAP_UPDATE_CONN_EVENT",

    "CMeLE_CONN_PARAMS_UPDATE_LOCAL",
    "CMeLE_CONN_PARAMS_UPDATE_REMOTE",
    "CMeLE_CONN_PARAMS_UPDATE_CONN_EVENT",

    "CMeLE_CTRL_SEQ_TX_INDICATION",
    "CMeLE_CTRL_SEQ_TIMEOUT",

    "CMeLE_ENCR_EVENT_START_ENCRYPTION",
    "CMeLE_ENCR_EVENT_RX_ENC_REQ",
    "CMeLE_ENCR_EVENT_RX_ENC_RSP",
    "CMeLE_ENCR_EVENT_RX_START_ENC_REQ",
    "CMeLE_ENCR_EVENT_RX_START_ENC_RSP",
    "CMeLE_ENCR_EVENT_RX_LTK",
    "CMeLE_ENCR_EVENT_TIMEOUT",
    "CMeLE_ENCR_EVENT_RX_PAUSE_ENC_RSP",
    "CMeLE_ENCR_EVENT_RX_PAUSE_ENC_RSP_TX_ACK",
    "CMeLE_ENCR_EVENT_RX_PAUSE_ENC_REQ",

    "CMeLE_UNKNOWN_RSP_RCVD",

    "CMeLE_FEATURE_REQ_LOCAL",
    "CMeLE_FEATURE_REQ_REMOTE",
    "CMeLE_FEATURE_RSP",
    "CMeLE_VERSION_REQ",
    "CMeLE_VERSION_IND_RCVD",

    "CMeLE_REJECT_IND_RCVD",
    "CMeLE_ENCR_EVENT_NO_LTK",
    "CMeLE_ENCR_EVENT_RX_REJECT_IND_ACK",
    "CMeLE_TERMINATE_LAST_ACK_RCVD",
    "CMeLE_TERMINATE_ACK_TX",
    "CMeLE_PING_REQ_LOCAL",
    "CMeLE_PING_REQ_REMOTE",
    "CMeLE_PING_RSP",

    "CMeLE_DATA_LEN_CHANGE_REQ_LOCAL",
    "CMeLE_DATA_LEN_CHANGE_REQ_REMOTE",
    "CMeLE_DATA_LEN_CHANGE_RSP",

    "CMeLE_CONN_PARAMS_REQ_REMOTE",
    "CMeLE_CONN_PARAMS_RSP_REMOTE",
    "CMeLE_CONN_PARAMS_HCI_REPLY",
    "CMeLE_CONN_PARAMS_HCI_NEG_REPLY",
    "CMeLE_CONN_PARAMS_REJECT_RCVD",

    "INVALID_CM_EVENT",
    "INVALID_CM_EVENT",
    "INVALID_CM_EVENT",
    "INVALID_CM_EVENT"
};


char * psSECstate[] = {
   "LMP_PROCsIDLE",
   "LMP_PROCsAUTHEN_W4_HOST_LINK_KEY",
   "LMP_PROCsAUTHEN_W4_SRES",

   "LMP_PROCsPAIRING_W4_PIN",
   "LMP_PROCsPAIRING_W4_INIT_KEY_RESP",
   "LMP_PROCsPAIRING_W4_NEW_KEY",
   "LMP_PROCsPAIRING_W4_MUTUAL_AUTH_REQ",

   "LMP_PROCsENCRYPTION_W4_MODE",
   "LMP_PROCsENCRYPTION_W4_MASK_REQ",
   "LMP_PROCsENCRYPTION_KEY_SIZE_NEGOTIATION",
   "LMP_PROCsENCRYPTION_W4_START_ENCRYPTION",
   "LMP_PROCsENCRYPTION_STOP_W4_STOP",
   "LMP_PROCsENCRYPTION_STOP_W4_MODE",

   "LMP_PROCsKEY_CHANGE_W4_KEY_TYPE",
   "LMP_PROCsKEY_CHANGE_W4_C_NEW_KEY_AUTH_REQ",

   "LMP_PROCsROLE_SWITCH_W4_LC_ROLE_SWITCH",

   "LMP_PROCsHOLD_W4_NEGOTIATION_OR_SYNC",

   "LMP_PROCsSNIFF_W4_NEGOTIATION",
   "LMP_PROCsSNIFF_W4_LMS_ACK",

    "LMP_PROCsUNSNIFF_INITIATE_W4_ACCEPT_ACK",
    "LMP_PROCsUNSNIFF_REQ_W4_ACK",

   "LMP_PROCsMULTISLOT_W4_RESP",

   "LMP_PROCsTESTCONTROL_SETUP",

   "LMP_PROCsSCO_PARAM_CHANGE_W4_RESP",
   "LMP_PROCsADD_SCO",

   "LMP_PROCs_eSCO_PARAM_CHANGE_W4_RESP",
   "LMP_PROCsADD_eSCO",

   "LMP_PROCsLINK_KEY_TYPE_CHANGE",
   "LMP_PROCs_rxDETACH_W4_TIMER",
   "LMP_PROCs_txDETACH",

   "LMP_PROCsQOS_W4_RES",

   "LMP_PROCsPARK_W4_RESP",
   "LMP_PROCsENTER_PARK",
   "LMP_PROCsUNPARK_W4_RESP",
   //"LMP_PROCsENTER_UNPARK",
   //"LMP_PROCsSLAVE_UPARK_W4_ACCESS_WIN",
    "LMP_PROCsMED_RATE_W4_CNTRL",

   "LMP_PROCsENCRYPTION_W4_PAUSE_REQ_I",
   "LMP_PROCsENCRYPTION_W4_PAUSE_REQ_RI",

   "LMP_PROCsSNIFF_W4_SUB_RATE_NEGOTIATION",
   "LMP_PROCsSNIFF_W4_SUB_RATE_RES_TX_CONFIRMATION",

   "LMP_PROCs_eSCO_CQDDR_PARAM_CHANGE_W4_RESP",

   "LMP_PROCsTxENCAP_LMP_HDR_PEND",
   "LMP_PROCsTxENCAP_LMP_PYLD_PEND",
   "LMP_PROCsRxENCAP_LMP_PEND",

   "LMP_PROCsSPAIRING_INIT_W4_HCI_IO_CAPABILITIES",
   "LMP_PROCsSPAIRING_W4_HCI_IO_CAPABILITIES",

   "LMP_PROCsSPAIRING_W4_LMP_IO_CAPABILITIES",

   "LMP_PROCsSPAIRING_INITIATE_PUBLIC_KEY",
   "LMP_PROCsSPAIRING_W4_PUBLIC_KEY",

   "LMP_PROCsSPAIRING_AUTHEN_STAGE1",
   "LMP_PROCsSPAIRING_W4_CONFIRM",
   "LMP_PROCsSPAIRING_W4_NONCE",
   "LMP_PROCsSPAIRING_W4_HCI_CONFIRM",

   "LMP_PROCsSPAIRING_DHKEY_W4_STATUS",
   "LMP_PROCsSPAIRING_DHKEY_W4_DHKEY_CHECK",

   "LMP_PROCsSPAIRING_W4_HCI_PASSKEY",
   "LMP_PROCsSPAIRING_W4_HCI_REMOTE_OOB_DATA",
   "LMP_PROCsSPAIRING_W4_NONCE_ERROR",

   "LMP_PROCsEDR_W4_TX_ACK",

   "LMP_PROCsLSTO_CHANGE_PND",
   "LMP_PROCsAUTHEN_SECURE_W4_AU_RAND",
   "LMP_PROCsAUTHEN_SECURE_W4_SRES",

   "LMP_PROCsROLE_SWITCH_W4_ENCR_RESTART",
   "INVALID_SEC_STATE",
   "INVALID_SEC_STATE",
   "INVALID_SEC_STATE",
   "INVALID_SEC_STATE"
};


char * psSECevent[] = {
   "LMP_PROCeNONE",
   "LMP_PROCeSTART_AUTH_NEW_CONN",
   "LMP_PROCeSTART_AUTHENTICATION",
   "LMP_PROCeSTART_KEY_CHANGE",
   "LMP_PROCeLMP_IN_RAND",
   "LMP_PROCeLMP_ACCEPT_IN_RAND",
   "LMP_PROCeLMP_NOT_ACCEPT_IN_RAND",
   "LMP_PROCeLMP_AU_RAND",
   "LMP_PROCeLMP_ACCEPT_AU_RAND",
   "LMP_PROCeLMP_NOT_ACCEPT_AU_RAND",
   "LMP_PROCeLMP_SRES",
   "LMP_PROCeLMP_UNIT_KEY",
   "LMP_PROCeLMP_COMB_KEY",
   "LMP_PROCeSECURITY_SEQ_TIMER",
   "LMP_PROCeLMP_DETACH",
   "LMP_PROCeDETACH_TI_EXPIRE",
   "LMP_PROCeINITIATE_LMP_DETACH",
   "LMP_PROCeLMS_LMP_DETACH_ACK",

   "LMP_PROCeHCI_PIN_RESPONSE",
   "LMP_PROCeHCI_NO_PIN",
   "LMP_PROCeHCI_HOST_RESPONSE_TO",
   "LMP_PROCeHCI_NO_LINK_KEY",
   "LMP_PROCeHCI_VALID_LINK_KEY",

   "LMP_PROCeHCI_ADD_SCO_CONNECTION",
   "LMP_PROCeHCI_ACCEPT_CONNECTION_REQUEST",
   "LMP_PROCeHCI_REJECT_CONNECTION_REQUEST",
   "LMP_PROCeHCI_CHANGE_SCO_PACKET_TYPE",

    "LMP_PROCeHCI_ENHANCED_SETUP_SYNCHRONOUS_CONNECTION",
    "LMP_PROCeHCI_ENHANCED_SETUP_SYNCHRONOUS_CONNECTION_CHANGE",
    "LMP_PROCeHCI_ENHANCED_ACCEPT_SYNCHRONOUS_CONNECTION_REQ",

   "LMP_PROCeHCI_SETUP_SYNCHRONOUS_CONNECTION",
   "LMP_PROCeHCI_SETUP_SYNCHRONOUS_CONNECTION_CHANGE",

   "LMP_PROCeHCI_ACCEPT_SYNCHRONOUS_CONNECTION_REQ",
   "LMP_PROCeHCI_REJECT_SYNCHRONOUS_CONNECTION_REQ",

   "LMP_PROCeLMP_eSCO_LINK_REQ",
   "LMP_PROCeLMP_ACCEPT_eSCO_LINK_REQ",
   "LMP_PROCeLMP_NOT_ACCEPT_eSCO_LINK_REQ",

   "LMP_PROCeSTART_ENCRYPTION",
   "LMP_PROCeRESTART_ENCRYPTION",
   "LMP_PROCeLMP_ENCRYPTION_MODE_REQ",
   "LMP_PROCeLMP_ACCEPT_ENCRYPTION_MODE",
   "LMP_PROCeLMP_NOT_ACCEPT_ENCRYPTION_MODE",
   "LMP_PROCeLMP_ENCRYPTION_KEY_SIZE_REQ",
   "LMP_PROCeLMP_ACCEPT_ENCRYPTION_KEY_SIZE",
   "LMP_PROCeLMP_NOT_ACCEPT_ENCRYPTION_KEY_SIZE",
   "LMP_PROCeLMP_START_ENCRYPTION_REQ",
   "LMP_PROCeLMP_ACCEPT_START_ENCRYPTION",
   "LMP_PROCeLMP_NOT_ACCEPT_START_ENCRYPTION",
   "LMP_PROCeLMP_STOP_ENCRYPTION_REQ",
   "LMP_PROCeSTOP_ENCRYPTION",
   "LMP_PROCeLMP_ACCEPT_STOP_ENCRYPTION",
   "LMP_PROCeLMP_NOT_ACCEPT_STOP_ENCRYPTION",

   "LMP_PROCeLMP_ENCRYPTION_KEY_SIZE_MASK_RES",
   "LMP_PROCeLMP_NOT_ACCEPT_ENCRYPTION_KEY_SIZE_MASK_REQ",

   "LMP_PROCeLMP_NOT_ACCEPT_UNIT_KEY",
   "LMP_PROCeLMP_NOT_ACCEPT_COMB_KEY",

   "LMP_PROCeO_ROLE_SWITCH",
   "LMP_PROCeR_NC_ROLE_SWITCH",
   "LMP_PROCeLMP_NOT_ACCEPT_SLOT_OFFSET",
   "LMP_PROCeLMP_SWITCH_REQ",
   "LMP_PROCeLMP_ACCEPT_SWITCH_REQ",
   "LMP_PROCeLMP_NOT_ACCEPT_SWITCH_REQ",
   "LMP_PROCeLMS_LMP_ACK",
   "LMP_PROCeLMS_RX_FAILURE_DETECT",
   "LMP_PROCe_LC_ROLE_SWITCH_READY",

   "LMP_PROCeLMS_PICONET_SWITCH_COMPLETE",

   "LMP_PROCeINITIATE_HOLD",
   "LMP_PROCeLMP_HOLD",
   "LMP_PROCeLMP_HOLD_REQ",
   "LMP_PROCeLMP_ACCEPT_HOLD_REQ",
   "LMP_PROCeLMP_NOT_ACCEPT_HOLD",

   "LMP_PROCeINITIATE_SNIFF",
   "LMP_PROCeLMP_SNIFF_REQ",
   "LMP_PROCeLMP_ACCEPT_SNIFF_REQ",
   "LMP_PROCeLMP_NOT_ACCEPT_SNIFF",

   "LMP_PROCeHOST_TERMINATE_SNIFF",
   "LMP_PROCeLMP_UNSNIFF_REQ",
   "LMP_PROCeLMP_ACCEPT_UNSNIFF_REQ",
   //"LMP_PROCeLMP_NOT_ACCEPT_UNSNIFF_REQ",
   "LMP_PROCeLMP_UNSNIFF_REQ_ACK",
   "LMP_PROCeLMP_ACCEPT_UNSNIFF_REQ_ACK",
   "LMP_PROCeEXIT_SNIFF_TO",

   "LMP_PROCeINITIATE_PARK",
   "LMP_PROCeLMP_ACCEPT_PARK_REQ",
   "LMP_PROCeLMP_NOT_ACCEPT_PARK_REQ",
   "LMP_PROCePARK_REQ",
   "LMP_PROCePARK_TIMER",
   "LMP_PROCePARK_LMS_ACK",
   "LMP_PROCeTERMINATE_PARK",
   "LMP_PROCeTERMINATE_PARK_ACCESS_REQ",
   "LMP_PROCeUNPARK_REQ",
   "LMP_PROCeLMP_ACCEPT_UNPARK_PMADDR_REQ",
   "LMP_PROCeLMP_ACCEPT_UNPARK_BDADDR_REQ",
   //"LMP_PROCeLMP_ACCEPT_UNPARK_REQ_ACK",
   "LMP_PROCeBB_EXIT_PARK",

   "LMP_PROCeHOST_NEWCONNECTION",
   "LMP_PROCeHOST_CHANGE_PACKET_TYPE",
   "LMP_PROCeLMP_EVAL_MAX_SLOTS",
   "LMP_PROCeLMP_MAX_SLOT",
   "LMP_PROCeLMP_ACCEPT_SLOT_REQ",
   "LMP_PROCeLMP_NOT_ACCEPT_SLOT_REQ",
   "LMP_PROCeEVAL_MULTISLOT_CTRL_REQ",

   "LMP_PROCeLMP_TESTCONTROL_SETUP",

   "LMP_PROCeLMP_ACCEPT_SCO_LINK_REQ",
   "LMP_PROCeLMP_NOT_ACCEPT_SCO_LINK_REQ",

   "LMP_PROCeLMP_SCO_LINK_REQ",

   "LMP_PROCeLMP_USE_SEMI_PERMANENT_KEY_ACCEPTED",
   "LMP_PROCeLMP_USE_SEMI_PERMANENT_KEY_NOT_ACCEPTED",
   "LMP_PROCeSEMI_PERMANENT_KEY",
   "LMP_PROCeR_SEMI_PERMANENT_KEY",
   "LMP_PROCeMASTER_LINK_KEY",
   "LMP_PROCeR_MASTER_LINK_KEY",
   "LMP_PROCeINITIATE_QUALITY_OF_SERVICE_REQ",
   "LMP_PROCeLMP_ACCEPT_QUALITY_OF_SERVICE_REQ",
   "LMP_PROCeLMP_NOT_ACCEPT_QUALITY_OF_SERVICE_REQ",
   "LMP_PROCeLMP_NOT_ACCEPT_CREATE_CONNECTION_REQ",
   "LMP_PROCe_MED_RATE_CNTRL_ENABLE_1_M",
   "LMP_PROCe_MED_RATE_CNTRL_ENABLE_2_3_M",
   "LMP_PROCeLMP_PACKET_TABLE_REQ",
   "LMP_PROCeLMP_ACCEPT_LMP_PACKET_TABLE_REQ",
   "LMP_PROCeLMP_ACCEPT_LMP_PACKET_TABLE_REQ_ACK",
   "LMP_PROCeLMP_NOT_ACCEPT_LMP_PACKET_TABLE_REQ",

   "LMP_PROCeSTOP_ENCRYPTION_STOP_ACL_U",
   "LMP_PROCeLMP_PAUSE_ENCRYPTION_REQ",
   "LMP_PROCeLMP_RESUME_ENCRYPTION_REQ",
   "LMP_PROCeLMP_NOT_ACCEPT_PAUSE_ENCRYPTION_REQ",

   "LMP_PROCeSTART_ENCRYPTION_START_ACL_U",

   "LMP_PROCeLMP_SNIFF_SUB_RATE_REQ",
   "LMP_PROCeLMP_SNIFF_SUB_RATE_RES",
   "LMP_PROCeLMP_NOT_ACCEPT_SNIFF_SUBRATE_REQ",
   "LMP_PROCeHOST_SNIFF_SUBRATE_PARAMS",

   "LMP_PROCeLMP_SRES_TX_NOTIFY",

   "LMP_PROCeEDR_ESCO_CQDDR_CHANGE",

    /* LMP Encapsulated Payload */
   "LMP_PROCeLMP_ENCAP_PDU_TX_COMPLETE",
   "LMP_PROCeLMP_ENCAP_PDU_TX_FAILURE",

   "LMP_PROCeINITIATE_LMP_ENCAP_PDU",
   "LMP_PROCeLMP_ACCEPTED_ENCAP_HEADER",
   "LMP_PROCeLMP_NOT_ACCEPTED_ENCAP_HEADER",
   "LMP_PROCeLMP_ENCAPSULATED_HEADER",
   "LMP_PROCeLMP_ACCEPTED_ENCAP_PAYLOAD",
   "LMP_PROCeLMP_NOT_ACCEPTED_ENCAP_PAYLOAD",
   "LMP_PROCeLMP_ENCAPSULATED_PAYLOAD",
   "LMP_PROCeLMP_RX_ENCAP_SEQ_TIMEOUT",

    /* Simple Pairing */
   "LMP_PROCeHCI_IO_CAPABILITY_RESPONSE",
   "LMP_PROCeLMP_IO_CAPABILITY_REQ",
   "LMP_PROCeLMP_NOT_ACCEPT_IO_CAPABILITY_REQ",
   "LMP_PROCeLMP_IO_CAPABILITY_RES",
   "LMP_PROCeLMP_ENCAP_PUBLIC_KEY",

   "LMP_PROCeSPAIRING_AUTHEN_S1_INIT",

   "LMP_PROCeLMP_SIMPLE_PAIRING_CONFIRM",
   "LMP_PROCeLMP_SIMPLE_PAIRING_NUMBER",
   "LMP_PROCeLMP_ACCEPTED_SIMPLE_PAIRING_NUMBER",
   "LMP_PROCeLMP_NOT_ACCEPTED_SIMPLE_PAIRING_NUMBER",

   "LMP_PROCeHCI_CONFIRM_REQUEST_REPLY",
   "LMP_PROCeHCI_CONFIRM_REQUEST_NEGATIVE_REPLY",

   "LMP_PROCeLMP_DHKEY_CHECK",
   "LMP_PROCeLMP_ACCEPTED_DHKEY_CHECK",
   "LMP_PROCeLMP_NOT_ACCEPTED_DHKEY_CHECK",

   "LMP_PROCeLMP_NUMERIC_COMPARISON_FAILED",

   "LMP_PROCeHCI_USER_PASSKEY_REQUEST_REPLY",
   "LMP_PROCeHCI_USER_PASSKEY_REQUEST_NEGATIVE_REPLY",

   "LMP_PROCeLMP_PASSKEY_ENTRY_FAILED",

   "LMP_PROCeHCI_REMOTE_OOB_DATA_REQUEST_REPLY",

   "LMP_PROCeLMP_OOB_FAILED",
   "LMP_PROCeHCI_REMOTE_OOB_DATA_REQUEST_NEGATIVE_REPLY",
   "LMP_PROCeHCI_SEND_KEYPRESS_NOTIFICATION",
   "LMP_PROCeLMP_KEYPRESS_NOTIFICATION",

   "LMP_PROCeENCRYPTION_KEY_REFRESH",

   "LMP_PROCeHCI_IO_CAPABILITY_REQUEST_NEGATIVE_REPLY",

   "LMP_PROCeHCI_WRITE_LINK_SUPERVISION_TIMEOUT",
   "LMP_PROCeEVAL_SSR_STATUS",
   "LMP_PROCeENCRYPTION_RESUME_TIMEOUT",

   "LMP_PROCeLMP_PAUSE_ENCRYPTION_AES_REQ",
   "LMP_PROCeHCI_REMOTE_OOB_EXTENDED_DATA_REQUEST_REPLY",

   "LMP_PROCeAFH_ACTION_MASTER",
   "LMP_PROCeINVALID_EVENT",
   "LMP_PROCeINVALID_EVENT"
};

bool qxdm_property_init()
{
    int ret =0;
    char value[PROPERTY_VALUE_MAX] = {'\0'};

    property_get_bt("wc_transport.force_hcidump", value, "false");
    enable_force_hcidump = (strncasecmp(value, "true",strlen("true")) ==0) ? TRUE : FALSE;
    ALOGV("%s: wc_transport.force_hcidump: %s, enabled: %d ",
            __func__, value, enable_force_hcidump);

    property_get_bt("unified_bt_logging", value, "false");
    enable_unified_logging = (strncasecmp(value, "true",strlen("true")) ==0) ? TRUE: FALSE;
    ALOGV("%s: unified_bt_logging: %s, enabled: %d ", __func__, value, enable_unified_logging);

    property_get_bt("persist.service.bdroid.snooplog", value, "false");
    enable_snoop_log = (strncasecmp(value, "true",strlen("true")) ==0) ? TRUE : FALSE;
    ALOGV("%s: persist.service.bdroid.snooplog: %s, enabled: %d ",
            __func__, value, enable_snoop_log);
    //crash dump enable by default
    property_get_bt("wc_transport.crashdump", value, NULL);
    enable_crashdump = (strncasecmp(value, "false",strlen("false")) ==0) ? FALSE : TRUE;
    ALOGV("%s: wc_transport.crashdump: %s, enabled: %d ",
            __func__, value, enable_crashdump);

    return 1;
}

bool diag_init()
{
  bool status;
  ALOGI("Init diag for BT log packets");

  status = Diag_LSM_Init(NULL);

  if(!status)
  {
    ALOGE("Failed to init diag");
    return 0;
  }

  /* Register callback for QXDM input data */
  DIAGPKT_DISPATCH_TABLE_REGISTER(DIAG_SUBSYS_BT, ssr_bt_tbl);

  return 1;
}

bool is_force_hcidump_enabled() {
    return enable_force_hcidump;
}

void diag_deinit()
{
   Diag_LSM_DeInit();
}

void send_btlog_pkt(uint8 *pBtPaktBuf, int packet_len, int direction)
{
  short int log_type = 0;
  uint8 *logBuf;

  if (pBtPaktBuf == NULL || packet_len <= 0)
    return;

  logBuf = pBtPaktBuf;
  switch(logBuf[0])
  {
     case LOG_BT_CMD_PACKET_TYPE:
         log_type = LOG_BT_HCI_CMD_C;
     break;

     case LOG_BT_ACL_PACKET_TYPE:
         if (direction == LOG_BT_HOST_TO_SOC) {
            log_type = LOG_BT_HCI_TX_ACL_C;
         } else if(direction == LOG_BT_SOC_TO_HOST) {
            log_type = LOG_BT_HCI_RX_ACL_C;
         }
     break;

     case LOG_BT_EVT_PACKET_TYPE:
          log_type = LOG_BT_HCI_EV_C;
          handle_event(logBuf, packet_len);
          return;
     case LOG_ANT_CTL_PACKET_TYPE:
         if (direction == LOG_BT_HOST_TO_SOC) {
            log_type = LOG_ANT_HCI_CMD_C;
         } else if(direction == LOG_BT_SOC_TO_HOST) {
            log_type = LOG_ANT_HCI_EV_C;
         }
     break;

     case LOG_ANT_DATA_PACKET_TYPE:
         if (direction == LOG_BT_HOST_TO_SOC) {
            log_type = LOG_ANT_HCI_DATA_TX_C;
         } else if(direction == LOG_BT_SOC_TO_HOST) {
            log_type = LOG_ANT_HCI_DATA_RX_C;
         }
         return;

     default:
         ALOGI("%s: Unexpeced packet type - %#x", __func__, logBuf[0]);
     break;
  }

  send_log(&logBuf[1], packet_len-1, log_type);
}

static void get_log_type(int type, char* type_str) {
    switch(type) {
           case LOG_BT_HCI_CMD_C: strlcpy(type_str, "CMD", sizeof(type_str)); break;
           case LOG_BT_HCI_TX_ACL_C: strlcpy(type_str,"ACL TX", sizeof(type_str)); break;
           case LOG_BT_HCI_RX_ACL_C: strlcpy(type_str,"ACL RX", sizeof(type_str)); break;
           case LOG_BT_HCI_EV_C: strlcpy(type_str,"EVT", sizeof(type_str)); break;
           case LOG_BT_DIAG_LMP_RX_ID: strlcpy(type_str,"LMP_RX", sizeof(type_str)); break;
           case LOG_BT_DIAG_LMP_TX_ID: strlcpy(type_str,"LMP_TX", sizeof(type_str)); break;
           default: ALOGE("No log type!!");
    }
}

static int is_unified_logging_enabled() {
    return enable_unified_logging;
}

void ts_get_conv (void *timestamp, int type, char *log_buf, int len)
{
    struct timeval tv;
    char *temp1;
    char *temp2;
    long long int seconds, microseconds;
    unsigned int temp, second, minute, hour;
    char log_type[10];
    int i;
    ALOGI("%s:len=%d",__func__,len);
    memset(log_type, '\0', sizeof(log_type));
    get_log_type(type, log_type);

    gettimeofday(&tv, NULL);
    temp = tv.tv_sec;
    second = temp%60;
    temp /= 60;
    minute = temp%60;
    temp /= 60;
    hour = temp%24;

    if (is_unified_logging_enabled()) {
        switch (type) {
        case LOG_BT_HCI_CMD_C:
            ALOGI("<<< ts <%02d:%02d:%02d:%06d> type <%s> opcode: <%0x> subopcode: <%0x>",
                hour, minute, second, (unsigned int)tv.tv_usec, log_type, log_buf[0], log_buf[1]);
            break;
        case LOG_BT_HCI_TX_ACL_C:
        case LOG_BT_HCI_RX_ACL_C:
            ALOGI("<<< ts <%02d:%02d:%02d:%06d> type <%s> conn handle: <%0x %0x>",
                hour, minute, second, (unsigned int)tv.tv_usec, log_type, log_buf[0], log_buf[1]);
            break;
        case LOG_BT_HCI_EV_C:
            ALOGI("<<< ts <%02d:%02d:%02d:%06d> type <%s> opcode: <%0x>",
                hour, minute, second, (unsigned int )tv.tv_usec, log_type, log_buf[0]);
            break;
        default:
            ALOGI("<<< ts <%02d:%02d:%02d:%06d> type <%s> ", hour, minute, second, (unsigned int)tv.tv_usec, log_type);
        }
    }

    seconds = (long long int)tv.tv_sec;

    seconds = seconds * 1000;
    microseconds = (long long int)tv.tv_usec;
    microseconds = microseconds/1000;
    seconds = seconds + microseconds;
    seconds = seconds*4;
    seconds = seconds/5;
    seconds = seconds << 16;
    temp1 = (char *) (timestamp);
    temp2 = (char *) &(seconds);

    /*
     * This is assuming that you have 8 consecutive
     * bytes to store the data.
     */
    for (i=0;i<8;i++)
        *(temp1+i) = *(temp2+i);
}

static void send_log (uint8 *Buf, int length, int type)
{
   int result = 0;
   int ret;
   char timebuf[8];
   // Send logs only if the particular log type is enabled in QXDM
   result = log_status(type);
   //Get the time for log packet
   ts_get_conv(timebuf, type, (char *)Buf, length);
   if (result || is_force_hcidump_enabled())
   {
     ALOGV("Dumping to diag>>");
     bt_log_pkt*  bt_log_pkt_ptr = NULL;
     bt_log_pkt_ptr = (bt_log_pkt *) log_alloc(type, LOG_BT_HEADER_SIZE + length);
     if (bt_log_pkt_ptr != NULL)
     {
       // There is a two byte length alignment is required for LMP packets
       if (type == LOG_BT_DIAG_LMP_RX_ID || type == LOG_BT_DIAG_LMP_TX_ID)
       {
         format_lmp(bt_log_pkt_ptr->data, Buf, length);
       }
       else
       {
         memcpy((void *)bt_log_pkt_ptr->data,(void *)Buf, length);
       }
       ((log_header_type *)&(bt_log_pkt_ptr->hdr))->ts_lo = *(uint32 *)&timebuf[0];
       ((log_header_type *)&(bt_log_pkt_ptr->hdr))->ts_hi = *(uint32 *)&timebuf[4];

       log_commit(bt_log_pkt_ptr);
     }
   }
}

/************************************************************************
 * Dump the log from UART log path to bluetooth log path.
 * Return value:
 *   > 0    succeed
 *   <=0    failed
 *   MAGIC_CANNOT_OPEN_SRC_PATH failed to open the source file path
 ************************************************************************/
static int dump_UART_logs(char* spath, char* dpath, int buffer_size, int water_mark) {
    int ret = 0;
    int sfd, dfd;
    //UART debug fs should be be of 48K buffer
    char *buf;
    int buf_cnt = 0;

    ALOGI("%s: Dumping UART logs in %s", __func__, dpath);

    buf = malloc(buffer_size);
    if (!buf) {
        ALOGE("%s: Error allocating memory %d bytes for log buffer", __func__,buffer_size );
        return ret;
    }

    sfd = open(spath, O_RDONLY | O_NONBLOCK);
    if (sfd < 0) {
        ALOGE("%s: Error opening source file: %d (%s)", __func__, sfd, strerror(errno) );
        free(buf);
        return MAGIC_CANNOT_OPEN_SRC_PATH;
    }
    dfd = open(dpath, O_CREAT| O_SYNC | O_WRONLY,  S_IRUSR |S_IWUSR |S_IRGRP );
    if (dfd < 0) {
        ALOGE("%s: Error opening destination file %d (%s)", __func__, dfd, strerror(errno) );
        free(buf);
        close(sfd);
        return ret;
    }

    ALOGI("%s: UART_DUMP: START", __func__);
    do {
        ret = read (sfd, buf, buffer_size);
        if (ret <= 0) {
            ALOGE("%s: Finish reading s file: %d (%s)", __func__, ret, strerror(errno) );
            break;
        }
        ALOGV("%s: Success reading s file: %d (%s)", __func__, ret, strerror(errno) );

        ret = write (dfd, buf, ret);
        if (ret <= 0) {
            ALOGE("%s: Error writing d file: %d (%s)", __func__, ret, strerror(errno) );
            break;
        }

        ALOGV("%s: Success writing s file: %d (%s)", __func__, ret, strerror(errno) );
        buf_cnt+= ret;
        if (buf_cnt >= water_mark) {
           /* As UART IPC log size can be at most 12 pages (48KB) in size
            * will stop after pulling the latest 20 KB.
            * this serve as termination condition
            */
           ALOGE("%s: Have pulled enough UART IPC logs", __func__);
           break;
        }
    } while (1);

    ALOGI("%s: UART_DUMP: DONE", __func__);

    free(buf);
    close(sfd);
    close(dfd);
    return ret;
}

void  dump_uart_ipc_logs(void) {
    int ret = 0;
    //UART debug fs should be be of 48K buffer
    char dpath[UART_IPC_PATH_BUF_SIZE];
    char spath[UART_IPC_PATH_BUF_SIZE];
    int soc_type = get_bt_soc_type();
    char* ipc_log_dir = (BT_SOC_CHEROKEE == soc_type) ?
         UART_IPC_LOG_COLLECTION_NEW_CHEROKEE_DIR : UART_IPC_LOG_COLLECTION_NEW_ROME_DIR;
    int inx_scan;
    int dfd;

    ALOGI("%s", __func__);

    if (is_debug_out_ring_buf_enabled()) {
        commit_out_ring_buffer ();
    }

    if (is_debug_in_ring_buf_enabled()) {
        commit_in_ring_buffer ();
    }

#ifdef DUMP_IPC_LOG
    /* Scan for uart ipc dump file */
    for (inx_scan= 0; inx_scan<MAX_NUM_OF_UART_IPC_LOGS;inx_scan++)
    {
         snprintf(dpath, UART_IPC_PATH_BUF_SIZE,
                "/data/misc/bluetooth/uart_ipc_txlogs-%.02d.txt", inx_scan);
         ALOGV("%s: try to open file : %s", __func__, dpath);
         dfd = open(dpath, O_RDONLY);
         if(dfd < 0 && (errno == ENOENT) ) {
             ALOGV("%s: file( %s) can be used for uart ipc", __func__, dpath);
             break;
         }
         close(dfd);
    }

    strlcpy(spath, ipc_log_dir, sizeof(spath));
    strlcat(spath, UART_IPC_LOG_TX_FILE_NAME, sizeof(spath));
    ret = dump_UART_logs(spath, dpath, 1024*30, 1024*12);

    snprintf(dpath, UART_IPC_PATH_BUF_SIZE,
             "/data/misc/bluetooth/uart_ipc_rxlogs-%.02d.txt", inx_scan);
    strlcpy(spath, ipc_log_dir, sizeof(spath));
    strlcat(spath, UART_IPC_LOG_RX_FILE_NAME, sizeof(spath));
    ret = dump_UART_logs(spath, dpath, 1024*30, 1024*12);

    snprintf(dpath, UART_IPC_PATH_BUF_SIZE,
             "/data/misc/bluetooth/uart_ipc_statelogs-%.02d.txt", inx_scan);
    strlcpy(spath, ipc_log_dir, sizeof(spath));
    strlcat(spath, UART_IPC_LOG_STATE_FILE_NAME, sizeof(spath));
    ret = dump_UART_logs(spath, dpath, 1024*30, 1024*8);

    snprintf(dpath, UART_IPC_PATH_BUF_SIZE,
             "/data/misc/bluetooth/uart_ipc_pwrlogs-%.02d.txt", inx_scan);
    strlcpy(spath, ipc_log_dir, sizeof(spath));
    strlcat(spath, UART_IPC_LOG_PWR_FILE_NAME, sizeof(spath));
    ret = dump_UART_logs(spath, dpath, 1024*30, 1024*8);

    if (MAGIC_CANNOT_OPEN_SRC_PATH == ret)
    {
        dump_UART_logs(UART_IPC_LOG_COLLECTION_OLD_PATH, dpath, 1024*50, 1024*22);
    }

    snprintf(dpath, UART_IPC_PATH_BUF_SIZE, "/data/misc/bluetooth/uart_txlogs-%.02d.txt", inx_scan);
    dump_UART_logs(UART_TX_LOG_COLLECTION_PATH, dpath, 4000, 4000);

    snprintf(dpath, UART_IPC_PATH_BUF_SIZE, "/data/misc/bluetooth/uart_rxlogs-%.02d.txt", inx_scan);
    dump_UART_logs(UART_RX_LOG_COLLECTION_PATH, dpath, 4000, 4000);
#endif // DUMP_IPC_LOG
    return;
}

#ifdef PANIC_ON_SOC_CRASH
void bt_kernel_panic(void)
{
    char panic_set ='c';

    ALOGI("%s", __func__  );
    if (fd_sysrq < 0) {
        ALOGE("%s: fd_sysrq has not been allocated", __func__);
        return;
    }

    if (write(fd_sysrq, &panic_set, 1) < 0) {
        ALOGE("%s: write (%s) fail - %s (%d)", __func__, PROC_PANIC_PATH, strerror(errno), errno);
    }
}
#endif //PANIC_ON_SOC_CRASH

static void handle_event(uint8 *eventBuf, int length)
{
  static int dumpFd = -1;
  unsigned char inx_scan;
  static char path[CRASH_DUMP_PATH_BUF_SIZE];
  char nullBuff[255] = {0,};
  int ssrlevel;

  if (eventBuf[1] == LOG_BT_EVT_VENDOR_SPECIFIC)
  {
    // It is a VS event for logs. Check if it is a controller log packet
    if (eventBuf[3] == LOG_BT_CONTROLLER_LOG)
    {
      switch(eventBuf[4])
      {
         case LOG_BT_MESSAGE_TYPE_VSTR:
           send_str_log(eventBuf);
         break;

         case LOG_BT_MESSAGE_TYPE_MEM_DUMP:
         {
            static unsigned int dump_size =0, total_size =0;
            unsigned short seq_num =0;
            static unsigned short seq_num_cnt =0;
            unsigned char *dump_ptr = NULL, param_len=0, packet_len =0;
            static unsigned char *temp_buf, *p;

            param_len = eventBuf[2];
            dump_ptr = &eventBuf[8];
            seq_num = eventBuf[5] | (eventBuf[6] << 8);

            ALOGV("%s: LOG_BT_MESSAGE_TYPE_MEM_DUMP (%d) ", __func__, seq_num);

            if (seq_num == 0 && is_debug_force_special_bytes()) {
                /* Crash happend stop the timer if active*/
               bool was_active = soc_crash_wait_timer_stop();
               if (was_active) {
                   ALOGI("%s: Timer was active in this instance", __func__);
               }

               g_crash_dump_started = true;
            }

            /* Check if crashdump is enabled */
            if( !is_crashdump_enabled() )
                goto skip_crashdump;

            if((seq_num != seq_num_cnt) && seq_num !=LAST_SEQUENCE_NUM)
            {
                ALOGE("%s: current sequence number: %d, expected seq num: %d ", __func__,
                        seq_num, seq_num_cnt);
            }

            if (seq_num == 0x0000)
            {
#ifdef WCNSS_IBS_ENABLED
                /* Stop sending any pending Tx data/Tx idle timer */
                wcnss_ibs_cleanup();
#endif

                dump_size = (unsigned int)
                    (eventBuf[8]|(eventBuf[9]<<8)|(eventBuf[10]<<16)|(eventBuf[11]<<24));
                dump_ptr = &eventBuf[12];
                total_size = seq_num_cnt = 0;

                memset(path, 0, CRASH_DUMP_PATH_BUF_SIZE);
                /* first pack has total ram dump size (4 bytes) */
                param_len -= 4;
                ALOGD("%s: Crash Dump Start - total Size: %d ", __func__, dump_size);

                p = temp_buf = (unsigned char *) malloc(dump_size);
                if (p != NULL) {
                  memset(p, 0, dump_size);
                } else {
                  ALOGE("Failed to allocate mem for the crash dump size: %d", dump_size);
                }

                /* Scan for crash dump file */
                for (inx_scan= 0; inx_scan<MAX_CRASH_DUMP_COUNT;inx_scan++)
                {
                    snprintf(path, CRASH_DUMP_PATH_BUF_SIZE, "%s/bt_fw_crashdump-%.02d.bin",
                                ROME_CRASH_RAMDUMP_PATH,inx_scan);
                    ALOGV("%s: try to open file : %s", __func__, path);
                    dumpFd = open(path, O_RDONLY);
                    if(dumpFd < 0 && (errno == ENOENT) ) {
                        ALOGV("%s: file( %s) can be used", __func__, path);
                        break;
                    }
                    close(dumpFd);
                }

                if(inx_scan >= MAX_CRASH_DUMP_COUNT)
                {
                    ALOGE("%s: Bluetooth Firmware Crash dump file reaches max count (%d)!\n"
                                  "\t\t\tPlease delete file in %s\n"
                                  "\t\t\tbt_fw_crashdump-last.bin file created",
                                  __func__, inx_scan, ROME_CRASH_RAMDUMP_PATH);
                    snprintf(path, CRASH_DUMP_PATH_BUF_SIZE, "%s/bt_fw_crashdump-last.bin",
                                ROME_CRASH_RAMDUMP_PATH );
                }

                dumpFd = open(path, O_CREAT| O_SYNC | O_WRONLY,  S_IRUSR |S_IWUSR |S_IRGRP );
                if(dumpFd < 0)
                {
                    ALOGE("%s: File open (%s) failed: errno: %d", __func__, path, errno);
                    seq_num_cnt++;
                    return;
                }
                ALOGV("%s: File Open (%s) successfully ", __func__, path);
            }

            packet_len = param_len - 5;
            if(dumpFd >= 0) {
                for ( ; (seq_num > seq_num_cnt) && (seq_num !=LAST_SEQUENCE_NUM) ; seq_num_cnt++)
                {
                    ALOGE("%s: controller missed packet : %d, write null (%d) into file ",
                            __func__, seq_num_cnt, packet_len);

                    if (p != NULL) {
                       memcpy(temp_buf, nullBuff, packet_len);
                       temp_buf = temp_buf + packet_len;
                   }
                }

                if (p != NULL) {
                   memcpy(temp_buf, dump_ptr, packet_len);
                   temp_buf = temp_buf + packet_len;
                }
                total_size += packet_len;
                ALOGV("%s: File Write : total_size:%d,seq_num:%d (seq_num_cnt:%d),"
                  "packet_len: %d", __func__, total_size, seq_num, seq_num_cnt, packet_len);
            }

            seq_num_cnt++;
            if (seq_num == LAST_SEQUENCE_NUM && p != NULL) {
               ALOGI("Writing crash dump of size %d bytes", total_size);
               write(dumpFd, p, total_size);
               free(p);
            }

skip_crashdump:
            if (seq_num == LAST_SEQUENCE_NUM)
            {
               if(dumpFd >= 0) {
                   if ( fsync(dumpFd) < 0 ) {
                        ALOGE("%s: File flush (%s) failed: %s, errno: %d", __func__,
                              path, strerror(errno), errno);
                   }
                   close(dumpFd);
                   dumpFd = -1;
                   ALOGI("%s: Write crashdump successfully : \n"
                                        "\t\tFile: %s\n\t\tdump_size: %d\n\t\twritten_size: %d",
                                __func__, path, dump_size, total_size );
                   struct stat stbuf;
                   int cntr = 0;
                   size_t readsize;
                   unsigned char *dumpinfo, *tempptr;
                   uint32  ucFilename;
                   uint32 uLinenum;
                   uint32 uPCAddr;
                   uint32 uResetReason;
                   uint32 uBuildVersion;
                   uint32 uReserved;
                   dumpFd = -1;
                   int i = 0;
                   char filenameBuf [CRASH_SOURCE_FILE_PATH_LEN];
                   memset(filenameBuf, 0, CRASH_SOURCE_FILE_PATH_LEN);
                   if (-1 != (dumpFd = open(path, O_RDONLY))) {
                       if (NULL != (dumpinfo = (unsigned char*)malloc(CRASH_DUMP_SIGNATURE_SIZE))){
                           tempptr = dumpinfo;
                           lseek(dumpFd, 0xFEE0, SEEK_SET);
                           readsize = CRASH_DUMP_SIGNATURE_SIZE;
                           while(readsize) {
                                cntr = read(dumpFd, (void*)tempptr, readsize);
                                tempptr += cntr;
                                readsize -= cntr;
                            }

                            tempptr = dumpinfo;
                            memcpy(&ucFilename,tempptr, 4); tempptr += 4;
                            memcpy(&uLinenum, tempptr, 4); tempptr += 4;
                            memcpy(&uPCAddr, tempptr, 4); tempptr += 4;
                            memcpy(&uResetReason, tempptr, 4); tempptr += 4;
                            memcpy(&uBuildVersion, tempptr, 4); tempptr += 4;

                            if (0 != ucFilename)
                            {
                                lseek(dumpFd,(off_t) ucFilename, SEEK_SET);
                                cntr = read(dumpFd, (void*)filenameBuf, CRASH_SOURCE_FILE_PATH_LEN);
                                while(i < CRASH_SOURCE_FILE_PATH_LEN && filenameBuf[i++] != '\0' );
                                if (i < CRASH_SOURCE_FILE_PATH_LEN)
                                {
                                    ALOGE("Filename::%s\n", filenameBuf);
                                }
                            }
                            ALOGE("Linenum::%d\n", uLinenum);
                            ALOGE("PC Addr::0x%x\n", uPCAddr);
                            ALOGE("Reset reason::%s\n", get_reset_reason_str(uResetReason));
                            ALOGE("Build Version::0x%x\n", uBuildVersion);
                        }
                        if (NULL != dumpinfo)free(dumpinfo);
                        close(dumpFd);
                   }
               }

               // By default kernel panic is enabled upon SSR command. Set 'btssrlevel' property
               // to '3' to disable kernel panic and test BT SSR
               ssrlevel = bt_ssr_level();
#ifdef PANIC_ON_SOC_CRASH
               if (ssrlevel == 1 || ssrlevel == 2) {
                  /* call kernel panic to report it to crashscope */
                  bt_kernel_panic();
               } else if (ssrlevel == 3){
#endif //PANIC_ON_SOC_CRASH
                   ALOGI("%s: Report SoC failure to bluedroid to trigger SSR", __func__);
                   dump_uart_ipc_logs();
                   report_soc_failure();
#ifdef PANIC_ON_SOC_CRASH
               }
#endif //PANIC_ON_SOC_CRASH
            }
         }
         break;

         case LOG_BT_MESSAGE_TYPE_PACKET:
           send_cntlr_log(eventBuf, length);
         break;

         default:
           ALOGI("%s: Unexpected message type - %#x", __func__, eventBuf[4]);
         break;
      }
    }
    else
    {
      //Event for regular vendor specific command
      send_log(&eventBuf[1], length-1, LOG_BT_HCI_EV_C);
    }
  }
  else
  {
    //Event for HCI command.
    //Packet type is not required. Ignoring 1st byte.
    send_log(&eventBuf[1], length-1, LOG_BT_HCI_EV_C);
  }
}

static void send_str_log(uint8 *strBuf)
{
  switch(strBuf[8])
  {
    case LOG_BT_VSTR_ERROR:
      MSG_SPRINTF_1(MSG_SSID_BT, MSG_LEGACY_ERROR, "%s", &strBuf[9]);
      if (is_unified_logging_enabled()) {
          ALOGI("<<< BTSOC_ERR: %s",  &strBuf[9]);
      }
    break;

    case LOG_BT_VSTR_HIGH:
      MSG_SPRINTF_1(MSG_SSID_BT, MSG_LEGACY_HIGH, "%s", &strBuf[9]);
      if (is_unified_logging_enabled()) {
          ALOGI("<<< BTSOC_HIGH: %s",  &strBuf[9]);
      }
    break;

    case LOG_BT_VSTR_LOW:
      MSG_SPRINTF_1(MSG_SSID_BT, MSG_LEGACY_LOW, "%s", &strBuf[9]);
      if (is_unified_logging_enabled()) {
          ALOGI("<<< BTSOC_LOW: %s",  &strBuf[9]);
      }
    break;

    default:
      ALOGI("%s: Unexpected message type string - %#x", __func__, strBuf[8]);
    break;
  }
}

static void format_lmp(uint8 *dst, uint8 *src, int32 pktLen)
{
    dst[LOG_BT_QXDM_PKT_LENGTH_POS] = src[LOG_BT_DBG_PKT_LENGTH_POS];
    dst[LOG_BT_QXDM_PKT_LENGTH2_POS] = 0x0;

    dst[LOG_BT_QXDM_DEVICE_IDX_POS] = src[LOG_BT_DBG_DEVICE_IDX_POS];

    memcpy( (dst+LOG_BT_QXDM_PKT_POS), (src+LOG_BT_DBG_PKT_POS), (pktLen-LOG_BT_QXDM_PKT_POS) );
}


static void dump_to_lcat(uint8* ppkt, uint8 type) {
   if (is_unified_logging_enabled()) {
       switch (type) {
       case LOG_BT_RX_LE_CTRL_PDU:
           // Actual data starts at buf[10]
           ALOGI("<<< LE CPDU RX: %s, c: 0x%2X, len:%d", psLE_ControlPDU[( (((ppkt)[4]) >= MAX_LE_CONTROL_PDU_SIZE) ?  (MAX_LE_CONTROL_PDU_SIZE-1) :  ((ppkt)[4]))], ppkt[4],(ppkt)[3]);
           break;

       case LOG_BT_TX_LE_CTRL_PDU:
           ALOGI("<<< LE CPDU TX: %s, c: 0x%2X, len:%d", psLE_ControlPDU[( (((ppkt)[4]) >= MAX_LE_CONTROL_PDU_SIZE) ?  (MAX_LE_CONTROL_PDU_SIZE-1) :  ((ppkt)[4]))], ppkt[4],(ppkt)[3]);
           break;

       case LOG_BT_TX_LE_CONN_MNGR:
           break;

       case LOG_BT_LINK_MANAGER_STATE:
           ALOGI("<<< LLM LM  h:%d, st: %s, ev:%s", (ppkt)[2],psLMstate[(ppkt)[0]], psLMevent[(ppkt)[1]]);
           break;

       case LOG_BT_CONN_MANAGER_STATE:
           ALOGI("<<< LLM CM  h:%d, st: %s, ev:%s", (ppkt)[2],psCMstate[(ppkt)[0]], psCMevent[(ppkt)[1]]);
           break;

       case LOG_BT_SECURITY_STATE:
           ALOGI("<<< LLM SEQ h:%d, st: %s, ev:%s", (ppkt)[2],psSECstate[(ppkt)[0]], psSECevent[(ppkt)[1]]);
           break;

       case LOG_BT_LE_CONN_MANAGER_STATE:
          break;

       case LOG_BT_LE_CHANNEL_MAP_STATE:
          break;

       case LOG_BT_LE_ENCRYPTION_STATE:
          break;
       default:
          ALOGI("Unknown log type : %d\n", ppkt[8]);
       }
   }
}

static void send_cntlr_log(uint8 *cntlrBuf, int length)
{
   int type = 0;
   int len = 0;
   uint8 *Buf;
   Buf = cntlrBuf;
    ALOGI("%s: Inparamter length= %d",__func__,length);
   switch (cntlrBuf[8])
   {
     // HCI commands and events are already tapped from the UART in regular flow.
     // So ignore if it comes as part of event log packet.
     case LOG_BT_HCI_CMD:
     case LOG_BT_HCI_EVENT:
       ALOGI("HCI cmd/event log. Ignore it: %#.2x", cntlrBuf[8]);
     return;

     case LOG_BT_RX_LMP_PDU:
       len = cntlrBuf[9];
       //Add extra byte for new LMP style, containing extra length byte
       len++;
       type = LOG_BT_DIAG_LMP_RX_ID;
     break;

     case LOG_BT_TX_LMP_PDU:
       len = cntlrBuf[9];
       //Add extra byte for new LMP style, containing extra length byte
       len++;
       type = LOG_BT_DIAG_LMP_TX_ID;
     break;

     case LOG_BT_RX_LE_CTRL_PDU:
       // Actual data starts at buf[10]
       Buf = Buf + 10;
       LLM_DEBUG_LE_CONTROL_PDU_RX(Buf);
       dump_to_lcat(Buf, cntlrBuf[8]);
     return;

     case LOG_BT_TX_LE_CTRL_PDU:
       Buf = Buf + 10;
       LLM_DEBUG_LE_CONTROL_PDU_TX(Buf);
       dump_to_lcat(Buf, cntlrBuf[8]);
     return;

     case LOG_BT_TX_LE_CONN_MNGR:
       //LLM_DEBUG_LE_CONNECTION_MANAGER_STATE(Buf);
       //dump_to_lcat(Buf);
     return;

     case LOG_BT_LINK_MANAGER_STATE:
       Buf = Buf + 10;
       LLM_DEBUG_LINK_MANAGER_STATE(Buf);
       dump_to_lcat(Buf, cntlrBuf[8]);
     return;

     case LOG_BT_CONN_MANAGER_STATE:
       Buf = Buf + 10;
       LLM_DEBUG_CONNECTION_MANAGER_STATE(Buf);
       dump_to_lcat(Buf, cntlrBuf[8]);
     return;

     case LOG_BT_SECURITY_STATE:
       Buf = Buf + 10;
       LLM_DEBUG_SECURITY_STATE(Buf);
       dump_to_lcat(Buf, cntlrBuf[8]);
     return;

     case LOG_BT_LE_CONN_MANAGER_STATE:
       //LLM_DEBUG_LE_CONNECTION_MANAGER_STATE(ppkt);
       //dump_to_lcat(Buf);
     return;

     case LOG_BT_LE_CHANNEL_MAP_STATE:
       //LLM_DEBUG_LE_CHANNEL_MAP_UPDATE_STATE(Buf);
       //dump_to_lcat(Buf);
     return;

     case LOG_BT_LE_ENCRYPTION_STATE:
       //LLM_DEBUG_LE_ENCRYTION_STATE(Buf);
       //dump_to_lcat(Buf);
     return;

     default:
       ALOGE("%s control log packet. Not handled - %#x", __func__, cntlrBuf[8]);
     return;
   }
   send_log (&cntlrBuf[10], len, type);
}

int is_snoop_log_enabled ()
{
    return enable_snoop_log;
}

/*
    SSR trigger command from QXDM

    1) software Error Fatal(hardfault)
       send_data 75 3 7 0 1 0C FC  01 20
       send_data 75 3 7 0 1 12 252 01 32

    2) Software Exception (Div by 0)
       send_data 75 3 7 0 1 0C FC  01 21
       send_data 75 3 7 0 1 12 252 01 33

    3) Software Exception (NULL ptr)
       send_data 75 3 7 0 1 0C FC  01 22
       send_data 75 3 7 0 1 12 252 01 34

    75 - DIAG_SUBSYS_CMD_F
    3 - DIAG_SUBSYS_BT
    0007 - DIAG_SSR_BT_CMD
    1 - HCI CMD type
    FC00 - VS Command OpCode
*/
void *ssr_bt_handle(void *req_pkt, uint16 pkt_len)
{
    unsigned char *pkt_ptr = (unsigned char *)req_pkt + 4;
    int i, retval =0;
    unsigned short p_len, p_opcode;
    char data_buf[PRINT_BUF_SIZE]={0,};

    void *rsp = NULL;
    ALOGI("%s : pkt_len: %d \n", __func__, pkt_len );

    /* Allocate the same length as the request. */
    rsp = diagpkt_subsys_alloc( DIAG_SUBSYS_BT, DIAG_SSR_BT_CMD, pkt_len);

    if (rsp != NULL) {

            p_len = *(pkt_ptr+3); /* VS Command packet length */
            p_opcode = (*(pkt_ptr+2) << 8) | *(pkt_ptr+1);

            ALOGI("%s : p_len: %d, pkt_len -8: %d, p_opcode:%.04x \n", __func__,
                  p_len, pkt_len -8, p_opcode);

            if(p_len !=(pkt_len - 8) || ( p_opcode != 0xFC00 && p_opcode != 0xFC0C))
            {
                return rsp;
            }

            for(i =0;(i<(p_len + 4) && (i*3 + 2) <PRINT_BUF_SIZE);i++)
                snprintf((char *)data_buf,PRINT_BUF_SIZE,"%s %.02x ",
                         (char *)data_buf,*(pkt_ptr + i));

            ALOGI("Received data : %s", data_buf);
            /* Send VS Command from QXDM input */
#ifdef WCNSS_IBS_ENABLED
            /* Send wake up byte to controller */
            wcnss_wake_assert();
            /* Tx idle timeout causes UART clock off during crash dump */
            wcnss_stop_idle_timeout_timer();
#endif
            retval = write(fd_transport, pkt_ptr, (p_len + 4));
            if (retval < 0) {
                ALOGE("%s:error in writing buf: %d: %s", __func__, retval, strerror(errno));
            }
            memcpy(rsp, req_pkt, pkt_len);
        } else {
        ALOGI("Allocate response buffer error" );
    }
    return rsp;
}

bool is_crashdump_enabled(void)
{
    return enable_crashdump;
}

static char *get_reset_reason_str(Reset_reason_e reason)
{
    int i = 0;
    char *str = NULL;
    for(i = 0; i < (int)(sizeof(reset_reason)/sizeof(Reason_map_st)); i++)
        if (reset_reason[i].reason == reason)
            return reset_reason[i].reasonStr;
    return NULL;
}
#endif
#endif//DIAG_ENABLED

int bt_ssr_level ()
{
   char value[PROPERTY_VALUE_MAX] = {'\0'};
   int soc_type = get_bt_soc_type();
#ifdef ENABLE_DBG_FLAGS
   char* default_value = (soc_type == BT_SOC_CHEROKEE) ? "1" : "3";
#else
   char* default_value = "3";
#endif

#ifdef ANDROID
   property_get("persist.service.bdroid.ssrlvl", value, default_value);
#else
   property_get_bt("persist.service.bdroid.ssrlvl", value, default_value);
#endif
   return atoi(value);
}

