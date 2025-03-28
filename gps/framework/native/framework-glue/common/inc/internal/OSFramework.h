/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

GENERAL DESCRIPTION
  OSFramework Class definition

  Copyright (c) 2015-2017 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.
=============================================================================*/

#ifndef __IZAT_MANAGER_OSFRAMEWORK_H__
#define __IZAT_MANAGER_OSFRAMEWORK_H__

#include <MsgTask.h>
#include <IOSFramework.h>
#include <IIzatManager.h>
#include <platform_lib_log_util.h>

using namespace izat_manager;

class OSFramework : public IOSFramework {

public:
    static OSFramework* getOSFramework();
    static void destroyInstance();

    // IOSFramework overrides
    IDataItemSubscription* getDataItemSubscriptionObj();
    IFrameworkActionReq * getFrameworkActionReqObj();
    IDataItemQuery* getDataItemQueryObj();
    IOSLocationProvider* getOSNP();
    IOSFrameworkCleaner* getOSFrameworkCleaner();
    LocMsg* getTaskSetupJob();

protected:

private:
    static OSFramework *mOSFramework;

    OSFramework();
    ~OSFramework();
};

#endif // #ifndef __IZAT_MANAGER_OSFRAMEWORK_H__
