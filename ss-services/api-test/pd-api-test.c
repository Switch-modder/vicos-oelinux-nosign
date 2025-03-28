/*
 * Copyright (c) 2015-2017 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include "libpdmapper.h"
#include "libpdnotifier.h"

#ifndef ANDROID_BUILD
#include <sys/syslog.h>
#define ALOGI(fmt, arg...) syslog (LOG_INFO, fmt, ##arg)
#define ALOGE(fmt, arg...) syslog (LOG_ERR, fmt, ##arg)
#define ALOGW(fmt, arg...) syslog (LOG_WARNING, fmt, ##arg)
#define ALOGD(fmt, arg...) syslog (LOG_DEBUG, fmt, ##arg)
#else //ANDROID_BUILD
#define LOG_TAG "pd-api-test"
#include <cutils/log.h>
#endif //ANDROID_BUILD

void pd_notifier_test_cb(void *data, enum pd_event event)
{
	ALOGE("Recieved event : %d. %s", event, (char *)data);
}
int main(int argc, char *argv[])
{
	SR_DL_Handle *handle = NULL;
	int num_providers;
	int instance_id;
	int service_data_valid;
	int service_data;
	char *entry = NULL;
	PD_Notifier_Handle *notifier = NULL;
	enum SR_Result_Enum rc;
	enum pd_rcode pd_rc;
	char *notifier_string = NULL;
	uint32_t i;
	pd_state state;
	char *client_name = "pd_api_test_client";
	if (argc < 2) {
		ALOGE("Incorrect number of arguments passed");
		goto error;
	}
	if (argv[1] == NULL) {
		ALOGE("Invalid argument passed");
		goto error;
	}
	handle = servreg_alloc_DLHandle();
	if (!handle) {
		ALOGE("Failed to allocate dlhandle");
		goto error;
	}
	rc = servreg_get_domainlist(argv[1], handle);
	if (rc != SR_RESULT_SUCCESS) {
		ALOGE("servreg_get_domainlist failed with error: %d",rc);
		goto error;
	}
	num_providers = servreg_get_numentries(handle);
	if ( num_providers < 0) {
		ALOGE("Error while getting num providers for %s", argv[1]);
		goto error;
	}
	ALOGI("%s is provided by %d domains", argv[1], num_providers);
	for (i = 0; i < num_providers; i++) {
		//Only need to call this if you care about the 'service
		//data'. This is a optional field that services that
		//are present at multiple locations can use to distinguish
		//themselves.
		rc = servreg_get_entry(handle,
				&entry,
				&instance_id,
				&service_data_valid,
				&service_data,
				i);
		if (rc != SR_RESULT_SUCCESS){
			ALOGE("Failed to get domain %d : rcode : %d", i,rc);
			goto error;
		}
		//Unless you care about the service data fields you can
		//just directly call the below api to get the
		//values necessary to register with the pd-notifier
		rc = servreg_get_data_for_notifier(handle,
				&notifier_string,
				&instance_id,
				i);
		if (rc != SR_RESULT_SUCCESS){
			ALOGE("Failed to get data for notifier");
			goto error;
		}
		ALOGI("Notifier_string is : :%s:",
				notifier_string);
		if (!entry) {
			//Something went terribly wrong
			ALOGE("get_entry returned NULL");
			goto error;
		}
		ALOGI("PD: %s\ninstance id %u\ndata_valid:%u\nservice_data:%u",
				entry,
				instance_id,
				service_data_valid,
				service_data);
		notifier = pd_notifier_alloc(notifier_string,
				client_name,
				instance_id,
				pd_notifier_test_cb,
				entry);
		if (!notifier) {
			ALOGE("Failed to allocate notifier handle");
			goto error;
		}
		pd_rc = pd_notifier_register(notifier, &state);
		free(notifier_string);
		notifier_string = NULL;
		if (pd_rc != PD_NOTIFIER_SUCCESS) {
			ALOGE("Failed to register for notifications: %d",rc);
			goto error;
		} else {
			ALOGI("Successfully registered.Curr state: %d",
					state);
		}
	}
	servreg_free_DLHandle(handle);
	do {
		ALOGE("Waiting forever..Kill process to end test");
		sleep(5000);
	} while(1);
	return 0;
error:
	if (handle)
		servreg_free_DLHandle(handle);
	if (notifier)
		pd_notifier_free(notifier);
	if (notifier_string)
		free(notifier_string);
	return 1;
}
