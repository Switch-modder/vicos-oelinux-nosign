/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

GENERAL DESCRIPTION
  DataItemConcreteTypeDefaultValues

  Copyright (c) 2015-2016 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.
=============================================================================*/

#ifndef __IZAT_MANAGER_DATAITEMCONCRETETYPEDEFAULTVALUES_H__
#define __IZAT_MANAGER_DATAITEMCONCRETETYPEDEFAULTVALUES_H__

#define AIRPLANEMODE_DEFAULT_MODE false

#define ENH_DEFAULT_ENABLED false

#define GPSSTATE_DEFAULT_ENABLED false

#define NLPSTATUS_DEFAULT_ENABLED false

#define WIFIHARDWARESTATE_DEFAULT_ENABLED false

#define SCREEN_STATE_DEFAULT_ENABLED false
#define POWER_CONNECT_STATE_DEFAULT_ENABLED false

#define TIME_DEFAULT_CURRTIME 0
#define TIMEZONE_DEFAULT_RAWOFFSET 0
#define TIMEZONE_DEFAULT_DSTOFFSET 0

#define SHUTDOWN_DEFAULT_STATE false

#define ASSISTED_GPS_DEFAULT_ENABLED false

#define NETWORKINFO_DEFAULT_TYPE 300
#define NETWORKINFO_DEFAULT_TYPENAME ""
#define NETWORKINFO_DEFAULT_SUBTYPENAME ""
#define NETWORKINFO_DEFAULT_AVAILABLE false
#define NETWORKINFO_DEFAULT_CONNECTED false
#define NETWORKINFO_DEFAULT_ROAMING false

#define SERVICESTATUS_DEFAULT_STATE 3 /// OOO

#define MODEL_DEFAULT_NAME ""

#define MANUFACTURER_DEFAULT_NAME ""

#define RTLLSERVICEINFO_DEFAULT_VALID_MASK            0x00000000
#define RTLLSERVICEINFO_DEFAULT_AIRIF_TYPE            0x01 // LOC_RILAIRIF_CDMA
#define RTLLSERVICEINFO_DEFAULT_CARRIER_AIRIF_TYPE    0x01 // LOC_RILAIRIF_CDMA
#define RTLLSERVICEINFO_DEFAULT_CARRIER_MCC           0
#define RTLLSERVICEINFO_DEFAULT_CARRIER_MNC           0
#define RTLLSERVICEINFO_DEFAULT_CARRIER_NAMELEN       0
#define RTLLSERVICEINFO_DEFAULT_CARRIER_NAME          ""

#define RILLCELLINFO_DEFAULT_VALID_MASK               0x00000000
#define RILLCELLINFO_DEFAULT_NETWORK_STATUS           3
#define RILLCELLINFO_DEFAULT_RIL_TECH_TYPE            0x1 // LOC_RIL_TECH_CDMA
#define RILLCELLINFO_DEFAULT_CDMA_CINFO_MCC           0
#define RILLCELLINFO_DEFAULT_CDMA_CINFO_SID           0
#define RILLCELLINFO_DEFAULT_CDMA_CINFO_NID           0
#define RILLCELLINFO_DEFAULT_CDMA_CINFO_BSID          0
#define RILLCELLINFO_DEFAULT_CDMA_CINFO_BSLAT         100
#define RILLCELLINFO_DEFAULT_CDMA_CINFO_BSLON         100
#define RILLCELLINFO_DEFAULT_CDMA_CINFO_LOCAL_TIME_ZONE_OFFSET_FROM_UTC        5
#define RILLCELLINFO_DEFAULT_CDMA_CINFO_LOCAL_TIME_ZONE_ON_DAYLIGHT_SAVINGS    false
#define RILLCELLINFO_DEFAULT_GSM_CINFO_MCC            0
#define RILLCELLINFO_DEFAULT_GSM_CINFO_MNC            0
#define RILLCELLINFO_DEFAULT_GSM_CINFO_LAC            0
#define RILLCELLINFO_DEFAULT_GSM_CINFO_CID            0
#define RILLCELLINFO_DEFAULT_WCDMA_CINFO_MCC          0
#define RILLCELLINFO_DEFAULT_WCDMA_CINFO_MNC          0
#define RILLCELLINFO_DEFAULT_WCDMA_CINFO_LAC          0
#define RILLCELLINFO_DEFAULT_WCDMA_CINFO_CID          0
#define RILLCELLINFO_DEFAULT_LTE_CINFO_MCC            0
#define RILLCELLINFO_DEFAULT_LTE_CINFO_MNC            0
#define RILLCELLINFO_DEFAULT_LTE_CINFO_TAC            0
#define RILLCELLINFO_DEFAULT_LTE_CINFO_PCI            0
#define RILLCELLINFO_DEFAULT_LTE_CINFO_CID            0

#define PIP_USER_SETTING_DEFAULT_ENABLED false

#define WIFI_SUPPLICANT_DEFAULT_STATE                 0

#define TAC_DEFAULT_NAME ""
#define MCCMNC_DEFAULT_NAME ""

#endif // __IZAT_MANAGER_DATAITEMCONCRETETYPEDEFAULTVALUES_H__
