// $Id: EscaladeDrive.cpp,v 1.17 2003/12/23 22:10:07 msmith Exp $

//
// EscaladeDrive
//

#include "Escalade.h"

// convenience
#define super	IOBlockStorageDevice
#define self	EscaladeDrive

OSDefineMetaClassAndStructors(self, super);

////////////////////////////////////////////////////////////////////////////////
// Attach method
//
bool
self::attach(IOService *provider)
{
    OSDictionary	*aDictionary;
    EscaladeController	*withController;
    
    if ((withController = OSDynamicCast(EscaladeController, provider)) == NULL) {
	error("attach by wrong controller/provider");
	return(false);
    }
    
    if (!super::attach(provider))
	return(false);
    
    controller = withController;

    // set interconnect properties
    if ((aDictionary = OSDictionary::withCapacity(2)) != NULL) {
	aDictionary->setObject(kIOPropertyPhysicalInterconnectTypeKey,
			controller->getProperty(kIOPropertyPhysicalInterconnectTypeKey));
	aDictionary->setObject(kIOPropertyPhysicalInterconnectLocationKey,
			controller->getProperty(kIOPropertyPhysicalInterconnectLocationKey));
	setProperty(kIOPropertyProtocolCharacteristicsKey, aDictionary);
	aDictionary->release();
    }

    return(true);
}

////////////////////////////////////////////////////////////////////////////////
// Detach method
//
void
self::detach(IOService *provider)
{
    if (controller == provider) {
	controller = NULL;
	super::detach(provider);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Parameter accessors
//
void
self::setUnit(int newUnit)
{
    unitNumber = newUnit;
    setProperty("IOUnit", (UInt64)newUnit, 32);
}

void
self::setSize(UInt32 newSize)
{
    unitSize = newSize;
}

void
self::setConfiguration(char *config)
{
    OSDictionary	*aDictionary;
    OSString		*aString;
    char		stringBuf[80];
    
    sprintf(unitDescription, "%s %s", controller->getControllerName(), config);

    // set device properties
    if ((aDictionary = OSDictionary::withCapacity(2)) != NULL) {
	if ((aString = OSString::withCString("3ware")) != NULL) {
	    aDictionary->setObject(kIOPropertyVendorNameKey, aString);
	    aString->release();
	}
	sprintf(stringBuf, "%s %s", controller->getControllerName(), config);
	if ((aString = OSString::withCString(stringBuf)) != NULL) {
	    aDictionary->setObject(kIOPropertyProductNameKey, aString);
	    aString->release();
	}
	// for completeness, not inserting controller revision here
	if ((aString = OSString::withCString("")) != NULL) {
	    aDictionary->setObject(kIOPropertyProductRevisionLevelKey, aString);
	    aString->release();
	}
	setProperty(kIOPropertyDeviceCharacteristicsKey, aDictionary);
    }
}

////////////////////////////////////////////////////////////////////////////////
// IOBlockStorageDevice protocol
//

IOReturn
self::doAsyncReadWrite(IOMemoryDescriptor *buffer, UInt64 block, UInt64 nblks,
                       IOStorageAttributes *attributes __unused, IOStorageCompletion *completion)
{
    // check for termination in progress
    if (isInactive())
	return(kIOReturnNotAttached);
    
    return(controller->doAsyncReadWrite(unitNumber, buffer, block, nblks, *completion));
}

#ifdef __LP64__
IOReturn
self::getWriteCacheState(bool *enabled)
{
    return(kIOReturnUnsupported);
}

OSMetaClassDefineReservedUsed(IOBlockStorageDevice, 1);

IOReturn
self::setWriteCacheState(bool enabled)
{
    return(kIOReturnUnsupported);
}
#endif

IOReturn
self::doEjectMedia(void)
{
    return(kIOReturnUnsupported);
}

IOReturn
self::doFormatMedia(__unused UInt64 byteCapacity)
{
    return(kIOReturnUnsupported);
}

UInt32
self::doGetFormatCapacities(UInt64 *capacities,  __unused UInt32 capacitiesMaxCount) const
{
    *capacities = (UInt64)unitSize * TWE_SECTOR_SIZE;
    return(1);
}

IOReturn
self::doLockUnlockMedia(__unused bool doLock)
{
    return(kIOReturnUnsupported);
}

IOReturn
self::doSynchronizeCache(void)
{
    // check for termination in progress
    if (isInactive())
	return(kIOReturnNotAttached);

    return(controller->doSynchronizeCache(unitNumber));
}

char *
self::getVendorString(void)
{
    return("3ware");
}

char *
self::getProductString(void)
{
    return(unitDescription);
}

char *
self::getRevisionString(void)
{
    return(controller->getControllerVersion());
}

char *
self::getAdditionalDeviceInfoString(void)
{
    return(unitDescription);
}

IOReturn
self::reportBlockSize(UInt64 *blockSize)
{
    *blockSize = TWE_SECTOR_SIZE;
    return(kIOReturnSuccess);
}

IOReturn
self::reportEjectability(bool *isEjectable)
{
    *isEjectable = false;
    return(kIOReturnSuccess);
}

IOReturn
self::reportLockability(bool *isLockable)
{
    *isLockable = false;
    return(kIOReturnSuccess);
}

IOReturn
self::reportMaxReadTransfer(__unused UInt64 blockSize, UInt64 *max)
{
    *max = TWE_MAX_IO_LENGTH * TWE_SECTOR_SIZE;
    return(kIOReturnSuccess);
}

IOReturn
self::reportMaxWriteTransfer(__unused UInt64 blockSize, UInt64 *max)
{
    *max = TWE_MAX_IO_LENGTH * TWE_SECTOR_SIZE;
    return(kIOReturnSuccess);
}

IOReturn
self::reportMaxValidBlock(UInt64 *maxBlock)
{
    *maxBlock = unitSize - 1;		// last addressable block
    return(kIOReturnSuccess);
}

IOReturn
self::reportMediaState(bool *mediaPresent, bool *changedState)
{
    *mediaPresent = true;
    *changedState = true;
    return(kIOReturnSuccess);
}

IOReturn
self::reportPollRequirements(bool *pollRequired, bool *pollIsExpensive)
{
    *pollRequired = false;
    *pollIsExpensive = false;
    return(kIOReturnSuccess);
}

IOReturn
self::reportRemovability(bool *isRemovable)
{
    *isRemovable = false;	// not *strictly* correct
    return(kIOReturnSuccess);
}

IOReturn
self::reportWriteProtection(bool *isWriteProtected)
{
    *isWriteProtected = false;
    return(kIOReturnSuccess);
}
