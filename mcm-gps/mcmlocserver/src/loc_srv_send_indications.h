//=============================================================================
// FILE: loc_srv_send_send_indications.h
//
// DESCRIPTION:
// Send Indications
//
// Copyright (c) 2013-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
// Qualcomm Technologies Proprietary and Confidential.
//=================================================i============================

#ifndef LOC_SRV_SEND_INDICATIONS_H
#define LOC_SRV_SEND_INDICATIONS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <loc_extended.h>

//=============================================================================
// FUNCTION: loc_srv_send_location_ind
//
// DESCRIPTION:
// Indication with Location information
//
// @return
//       void
//=============================================================================
void loc_srv_send_location_ind (LocGpsLocation *location);

//=============================================================================
// FUNCTION: loc_srv_send_status_ind
//
// DESCRIPTION:
// Indication with GPS Status information
//
// @return
//       void
//=============================================================================
void loc_srv_send_status_ind (LocGpsStatus *status);

//=============================================================================
// FUNCTION: loc_srv_send_sv_status_ind
//
// DESCRIPTION:
// Indication with Satellites in View information
//
// @return
//       void
//=============================================================================
void loc_srv_send_sv_status_ind (LocGpsSvStatus *sv_info);

//=============================================================================
// FUNCTION: loc_srv_send_nmea_ind
//
// DESCRIPTION:
// Indication with NMEA information
//
// @return
//       void
//=============================================================================
void loc_srv_send_nmea_ind (LocGpsUtcTime timestamp, const char* nmea, int length);

//=============================================================================
// FUNCTION: loc_srv_send_set_capabilities_ind
//
// DESCRIPTION:
// Indication with capabilities information
//
// @return
//       void
//=============================================================================
void loc_srv_send_set_capabilities_ind (uint32_t capabilities);

//=============================================================================
// FUNCTION: loc_srv_send_utc_time_req_ind
//
// DESCRIPTION:
// Indication with UTC time Request
//
// @return
//       void
//=============================================================================
void loc_srv_send_utc_time_req_ind ();

//=============================================================================
// FUNCTION: loc_srv_send_gps_xtra_data_req_ind
//
// DESCRIPTION:
// Indication with XTRA Data Request
//
// @return
//       void
//=============================================================================
void loc_srv_send_gps_xtra_data_req_ind ();

//=============================================================================
// FUNCTION: loc_srv_send_gps_xtra_report_server_ind
//
// DESCRIPTION:
// Indication with reported XTRA Server URLs
//
// @return
//       void
//=============================================================================
void loc_srv_send_gps_xtra_report_server_ind (const char* server1,
                                              const char* server2,
                                              const char* server3);

//=============================================================================
// FUNCTION: loc_srv_send_agps_status_ind
//
// DESCRIPTION:
// Indication with Agps Status information
//
// @return
//       void
//=============================================================================
void loc_srv_send_agps_status_ind (AGpsExtStatus * status);

//=============================================================================
// FUNCTION: loc_srv_send_gps_ni_ind
//
// DESCRIPTION:
// Indication with NI Notification
//
// @return
//       void
//=============================================================================
void loc_srv_send_gps_ni_ind (LocGpsNiNotification *notification);

//=============================================================================
// FUNCTION: loc_srv_send_gps_acquire_wakelock
//
// DESCRIPTION:
// GPS Acquire Wakelock
//
// @return
//       void
//=============================================================================
void loc_srv_send_gps_acquire_wakelock_ind ();

//=============================================================================
// FUNCTION: loc_srv_send_gps_release_wakelock
//
// DESCRIPTION:
// GPS Release Wakelock
//
// @return
//       void
//=============================================================================
void loc_srv_send_gps_release_wakelock_ind ();

//=============================================================================
// FUNCTION: loc_srv_send_gps_create_thread
//
// DESCRIPTION:
// GPS Create thread
//
// @return
//       void
//=============================================================================
void loc_srv_send_gps_create_thread_ind ();

#ifdef __cplusplus
}
#endif


#endif // LOC_SRV_SEND_INDICATIONS_H
