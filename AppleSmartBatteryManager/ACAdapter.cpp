//
//  ACAdapter.cpp
//  ACPIBatteryManager
//
//  Created by RehabMan on 11/16/13.
//
//  Code/ideas liberally borrowed from VoodooBattery...
//

#include "ACAdapter.h"
#include <IOKit/pwr_mgt/RootDomain.h>
#include <IOKit/pwr_mgt/IOPMPowerSource.h>
#include "IOPMPrivate.h"

OSDefineMetaClassAndStructors(ACPIACAdapter, IOService)

bool ACPIACAdapter::init(OSDictionary* dict)
{
    if (!IOService::init(dict))
        return false;
    
    fTracker = NULL;
    fProvider = NULL;
    
    return true;
}

IOService* ACPIACAdapter::probe(IOService* provider, SInt32* score)
{
    if (IOService::probe(provider, score) != this)
        return 0;

    if (score)
        *score = -1000;
    
    IOACPIPlatformDevice* acpi = OSDynamicCast(IOACPIPlatformDevice, provider);
    if (!acpi /*|| acpi->validateObject("_PRW")*/)
    {
        IOLog("acadapt: returing 0 from probe due to _PRW object existing\n");
        return 0;
    }

    //*score = 1000;

    IOLog("acadapt: returning success from probe\n");
    
    return this;
}

bool ACPIACAdapter::start(IOService* provider)
{
    IOLog("acadapt: entering start\n");

    if (!IOService::start(provider))
    {
        IOLog("acadapt: IOService::start failed!\n");
        return false;
    }
    
	IOLog("ACPIBatteryManager: Version 1.50 starting ACPIACAdapter\n");
    
    fProvider = OSDynamicCast(IOACPIPlatformDevice, provider);
    if (!fProvider)
    {
        IOLog("acadapt: provider not IOACPIPlatformDevice\n");
        return false;
    }
    fProvider->retain();
    
    // get tracker for notifications
    fTracker = OSDynamicCast(BatteryTracker, waitForMatchingService(serviceMatching(kBatteryTrackerService)));
    
    return true;
}

void ACPIACAdapter::stop(IOService* provider)
{
    IOLog("acadapt: entering stop\n");

    OSSafeReleaseNULL(fProvider);
    OSSafeReleaseNULL(fTracker);
    
    IOService::stop(provider);
}

void ACPIACAdapter::free(void)
{
    IOService::free();
}

IOReturn ACPIACAdapter::setPowerState(unsigned long state, IOService* device)
{
    DEBUG_LOG("ACPIACAdapter::setPowerState: state: %u, device: %s\n", (unsigned int) state, device->getName());
    
    return kIOPMAckImplied;
}

IOReturn ACPIACAdapter::message(UInt32 type, IOService* provider, void* argument)
{
    IOLog("ACPIACAdapter::message: type: %08X provider: %s\n", (unsigned int)type, provider->getName());
    
    if (type == kIOACPIMessageDeviceNotification && fProvider)
    {
        UInt32 acpi = 0;
        if (kIOReturnSuccess == fProvider->evaluateInteger("_PSR", &acpi))
        {
            IOLog("ACPIACAdapter: setting AC %s\n", (acpi ? "connected" : "disconnected"));
            
            // notify system of change in AC state
            if (IOPMrootDomain* root = getPMRootDomain())
                root->receivePowerNotification(kIOPMSetACAdaptorConnected | acpi ? kIOPMSetValue : 0);
            else
                IOLog("ACPIACAdapter: couldn't notify OS about AC status\n");
            
            // notify battery managers of change in AC state
            if (NULL != fTracker)
                fTracker->notifyBatteryManagers(acpi);
        }
        else
        {
            IOLog("ACPIACAdapter: _PSR failed\n");
        }
    }
    return kIOReturnSuccess;
}

