// $Id: EscaladeDrive.h,v 1.10 2003/12/23 21:51:19 msmith Exp $

//
// EscaladeDrive
//
// This class provides a shim between the IOBlockStorage class and the
// controller.  We keep per-drive private state in this object as well.
//

#ifndef ESCALADEDRIVE_H
#define ESCALADEDRIVE_H


class EscaladeController;

class EscaladeDrive : public IOBlockStorageDevice
{
    OSDeclareDefaultStructors(EscaladeDrive);

public:
    // superclass overrides
    virtual bool	attach(IOService *provider);
    virtual void	detach(IOService *provider);
    
    // parameter interface
    virtual void	setUnit(int unit);
    virtual void	setSize(UInt32 size);
    virtual void	setConfiguration(char *config);
    
    // IOBlockStorageDevice protocol
    virtual IOReturn	doAsyncReadWrite(IOMemoryDescriptor *buffer,
				      UInt64 block, UInt64 nblks,
                    IOStorageAttributes *attributes, IOStorageCompletion *completion);
#ifdef __LP64__
    virtual IOReturn	getWriteCacheState(bool *enabled);
    virtual IOReturn	setWriteCacheState(bool enabled);
#endif
    virtual IOReturn	doEjectMedia(void);
    virtual IOReturn	doFormatMedia(UInt64 byteCapacity);
    virtual UInt32	doGetFormatCapacities(UInt64 *capacities,  UInt32 capacitiesMaxCount) const;
    virtual IOReturn	doLockUnlockMedia(bool doLock);
    virtual IOReturn	doSynchronizeCache(void);
    virtual char	*getVendorString(void);
    virtual char	*getProductString(void);
    virtual char	*getRevisionString(void);
    virtual char 	*getAdditionalDeviceInfoString(void);
    virtual IOReturn	reportBlockSize(UInt64 *blockSize);
    virtual IOReturn	reportEjectability(bool *isEjectable);
    virtual IOReturn	reportLockability(bool *isLockable);
    virtual IOReturn	reportMaxReadTransfer (UInt64 blockSize, UInt64 *max);
    virtual IOReturn	reportMaxWriteTransfer(UInt64 blockSize, UInt64 *max);
    virtual IOReturn	reportMaxValidBlock(UInt64 *maxBlock);
    virtual IOReturn	reportMediaState(bool *mediaPresent, bool *changedState);
    virtual IOReturn	reportPollRequirements(bool *pollRequired, bool *pollIsExpensive);
    virtual IOReturn	reportRemovability(bool *isRemovable);
    virtual IOReturn	reportWriteProtection(bool *isWriteProtected);

private:
    EscaladeController	*controller;		// controller that holds us
    int			unitNumber;		// our unit number on the controller
    UInt32		unitSize;		// unit size in blocks
    char		unitDescription[80];	// product description string
};


#endif // ESCALADEDRIVE_H
