//
//  BatteryTracker.cpp
//  ACPIBatteryManager
//
//  Created by RehabMan on 12/3/13.
//
//

#include "BatteryTracker.h"

OSDefineMetaClassAndStructors(BatteryTracker, IOService)

bool BatteryTracker::init(OSDictionary* dict)
{
    DEBUG_LOG("BatteryTracker::start: entering init\n");
    
    if (!IOService::init(dict))
    {
        IOLog("BatteryTracker: IOService::init failed!\n");
        return false;
    }
    
    m_pBatteryList = NULL;
    m_pLock = NULL;
    
    return true;
}

bool BatteryTracker::start(IOService* provider)
{
    DEBUG_LOG("BatteryTracker::start: entering start\n");
    
    if (!IOService::start(provider))
    {
        IOLog("BatteryTracker: IOService::start failed!\n");
        return false;
    }
    
	DEBUG_LOG("ACPIBatteryManager: Version 1.50 starting BatteryTracker.\n");
    
    m_pBatteryList = OSArray::withCapacity(2);
    m_pLock = IOLockAlloc();
    
    registerService();
    
    return true;
}

void BatteryTracker::stop(IOService* provider)
{
    DEBUG_LOG("BatteryTracker::stop: entering stop\n");
    
    OSSafeReleaseNULL(m_pBatteryList);
    if (NULL != m_pLock)
    {
        IOLockFree(m_pLock);
        m_pLock = NULL;
    }
    IOService::stop(provider);
}

bool BatteryTracker::addBatteryManager(AppleSmartBatteryManager* pManager)
{
    DEBUG_LOG("BatteryTracker::addBatteryManager: entering addBatteryManager(%p)\n", pManager);
    
    bool result = false;
    
    IOLockLock(m_pLock);
    unsigned count = m_pBatteryList->getCount();
    unsigned i = 0;
    for (; i < count; ++i)
    {
        OSObject* pTemp = m_pBatteryList->getObject(i);
        if (pTemp == pManager)
            break;
        if (pTemp == NULL)
        {
            result = true;
            m_pBatteryList->setObject(i, pManager);
            break;
        }
    }
    if (i == count)
    {
        m_pBatteryList->setObject(pManager);
        result = true;
    }
    IOLockUnlock(m_pLock);
    
    return result;
}

bool BatteryTracker::removeBatteryManager(AppleSmartBatteryManager* pManager)
{
    DEBUG_LOG("BatteryTracker::removeBatteryManager: entering removeBatteryManager(%p)\n", pManager);
    
    bool result = false;
    
    IOLockLock(m_pLock);
    unsigned count = m_pBatteryList->getCount();
    for (unsigned i = 0; i < count; ++i)
    {
        if (m_pBatteryList->getObject(i) == pManager)
        {
            m_pBatteryList->setObject(i, NULL);
            result = true;
            break;
        }
    }
    IOLockUnlock(m_pLock);
    
    return result;
}

void BatteryTracker::notifyBatteryManagers(bool connected)
{
    DEBUG_LOG("BatteryTracker::notifyBatteryManagers: entering notifyBatteryManager(%s)\n", connected ? "connected" : "disconnected");
    
    IOLockLock(m_pLock);
    unsigned count = m_pBatteryList->getCount();
    for (unsigned i = 0; i < count; ++i)
    {
        AppleSmartBatteryManager* pManager = static_cast<AppleSmartBatteryManager*>(m_pBatteryList->getObject(i));
        if (NULL != pManager)
            pManager->notifyConnectedState(connected);
    }
    IOLockUnlock(m_pLock);
}

