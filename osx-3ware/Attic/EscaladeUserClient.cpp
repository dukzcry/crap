// $Id: EscaladeUserClient.cpp,v 1.12 2003/12/23 21:51:20 msmith Exp $

//
// Userland interface to the Escalade driver.
//

#include "Escalade.h"

#define super IOUserClient
#define self EscaladeUserClient

OSDefineMetaClassAndStructors(self, super);

//////////////////////////////////////////////////////////////////////////////
// Return the controller architecture
//
IOReturn
self::getControllerArchitecture(int *outArchitecture)
{
    // provider terminated?
    if (isInactive())
	return(kIOReturnNotAttached);
        
    *outArchitecture = fProvider->getControllerArchitecture();
    return(kIOReturnSuccess);
}

//////////////////////////////////////////////////////////////////////////////
// Request a unit be terminated.
//
IOReturn
self::doRemoveUnit(int inUnit)
{
    IOReturn    ret;

    // provider terminated?
    if (isInactive())
	return(kIOReturnNotAttached);
    
    if ((ret = clientHasPrivilege(fSecurity, kIOClientPrivilegeAdministrator)) != kIOReturnSuccess)
	return(ret);
    
    return(fProvider->doRemoveUnit(inUnit));
}

//////////////////////////////////////////////////////////////////////////////
// Request a unit be scanned and added.
//
IOReturn
self::doAddUnit(int inUnit)
{
    IOReturn    ret;
    
    // provider terminated?
    if (isInactive())
	return(kIOReturnNotAttached);
    
    if ((ret = clientHasPrivilege(fSecurity, kIOClientPrivilegeAdministrator)) != kIOReturnSuccess)
	return(ret);
    
    return(fProvider->doAddUnit(inUnit));
}

//////////////////////////////////////////////////////////////////////////////
// Execute a command on the controller
//
IOReturn
self::doCommand(EscaladeUserCommand *inCommand, __unused EscaladeUserCommand *outCommand, 
                __unused IOByteCount inCount, __unused IOByteCount *outCount)
{
    EscaladeUserCommand	*uc;
    EscaladeCommand	*cmd;
    IOMemoryDescriptor	*cmdBuf, *dataBuf;
    IOReturn		result;
    IOByteCount		packetSize;

    // provider terminated?
    if (isInactive())
	return(kIOReturnNotAttached);
    
    if ((result = clientHasPrivilege(fSecurity, kIOClientPrivilegeAdministrator)) != kIOReturnSuccess)
	return(result);
    
    debug(0, "called");
    uc = (EscaladeUserCommand *)inCommand;
    cmd = NULL;
    cmdBuf = NULL;
    dataBuf = NULL;

    // validate input
    if ((uc->data_buffer != 0) &&
	((uc->data_buffer % TWE_ALIGNMENT) || 
  	 (uc->data_size == 0) ||
	 (uc->data_size % TWE_ALIGNMENT))) {
	    error("user client supplied unaligned data buffer 0x%x/0x%x", uc->data_buffer, uc->data_size);
	    result = kIOReturnNotAligned;
	    goto out;
    }
	
    // Get a command and map the command packet from user space.
    debug(0, "get command");
    cmd = fProvider->getCommand(EC_COMMAND_USERCLIENT);
    cmd->setTimeout(TWE_TIMEOUT_USERCLIENT);
    packetSize = sizeof(TWE_Command);
    cmdBuf = IOMemoryDescriptor::withAddress(uc->command_packet,
					     packetSize,
					     kIODirectionOutIn,
					     fTask);
    if (cmdBuf == NULL) {
	error("could not map command packet at 0x%08x", (uint)uc->command_packet);
	result = kIOReturnVMError;
	goto out;
    }
    cmdBuf->prepare();
    
    // If we have a data buffer, get an appropriate IOMemoryDescriptor.
    debug(0, "get data");
    if ((uc->data_buffer != 0) && (uc->data_size > 0)) {
	dataBuf = IOMemoryDescriptor::withAddress(uc->data_buffer,
					   (IOByteCount)uc->data_size,
					   kIODirectionOutIn,
					   fTask);
	if (dataBuf == NULL) {
	    error("could not create memory descriptor for 0x%08x/%d", (uint)uc->data_buffer, uc->data_size);
	    result = kIOReturnVMError;
	    goto out;
	}
	dataBuf->prepare();
    }
    
    // Copy in the command packet and set the command up.
    // Note that we will fix up the command tag and scatter/gather length
    // later.
    debug(0, "copy command");
    cmdBuf->readBytes(0, (void *)cmd->commandPtr, packetSize);
    if (dataBuf != NULL)
	cmd->setDataBuffer(dataBuf);

    // Run the command
    debug(0, "run command %x", cmd->commandPtr->generic.opcode);
    if (fProvider->runSynchronousCommand(cmd)) {
	error("command completed");
	result = kIOReturnSuccess;
	// Copy the command packet back out to return status, etc.
	// XXX if the caller is frugal, we may overwrite something here...
	cmdBuf->writeBytes(0, (void *)cmd->commandPtr, packetSize);
    } else {
	error("timeout submitting user command");
	result = kIOReturnTimeout;
    }

out:
    debug(0, "cleanup");
    // clean up/tear down
    if (cmdBuf != NULL) {
	cmdBuf->complete();
	cmdBuf->release();
    }
    if (dataBuf != NULL) {
	if (cmd != NULL)
	    cmd->releaseDataBuffer();
	dataBuf->complete();
	dataBuf->release();
    }
    if (cmd != NULL)
	cmd->returnCommand();
    
    return(result);
}

//////////////////////////////////////////////////////////////////////////////
// Fetch the next AEN off the controller queue
//
IOReturn
self::getAEN(int *outAEN)
{

    // provider terminated?
    if (isInactive())
	return(kIOReturnNotAttached);
    
    *outAEN = (int)fProvider->getAEN();
    return(kIOReturnSuccess);
}

//////////////////////////////////////////////////////////////////////////////
// Reset the controller
//
IOReturn
self::doReset(void)
{
    IOReturn    ret;
    
    // provider terminated?
    if (isInactive())
	return(kIOReturnNotAttached);
    
    if ((ret = clientHasPrivilege(fSecurity, kIOClientPrivilegeAdministrator)) != kIOReturnSuccess)
	return(ret);
    
    fProvider->requestReset();
    return(kIOReturnSuccess);
}


//////////////////////////////////////////////////////////////////////////////
// Method lookup
//
// NOTE:  This must stay in synch with the enum in EscaladeUserClientInterface.h
IOExternalMethod *
self::getTargetAndMethodForIndex(IOService **target, UInt32 index)
{
    static const IOExternalMethod sMethods[kEscaladeUserClientMethodCount] = {
    	{   // kEscaladeUserClientOpen
	    NULL,				// IOService
	    (IOMethod) &self::open,
	    kIOUCScalarIScalarO,
	    0,					// no input
	    0					// no output
	},
    	{   // kEscaladeUserClientClose
	    NULL,				// IOService
	    (IOMethod) &self::close,
	    kIOUCScalarIScalarO,
	    0,					// no input
	    0					// no output
	},
    	{   // kEscaladeUserClientControllerArchitecture
	    NULL,				// IOService
	    (IOMethod) &self::getControllerArchitecture,
	    kIOUCScalarIScalarO,
	    0,					// no input
	    1					// one scalar out
	},
	{   // kEscaladeUserClientRemoveUnit
	    NULL,				// IOService
	    (IOMethod) &self::doRemoveUnit,
	    kIOUCScalarIScalarO,
	    1,					// one scalar in
	    0					// no output
	},
    	{   // kEscaladeUserClientAddUnit
	    NULL,				// IOService
	    (IOMethod) &self::doAddUnit,
	    kIOUCScalarIScalarO,
	    1,					// one scalar in
	    0					// no output
	},
    	{   // kEscaladeUserClientCommand
	    NULL,				// IOService
	    (IOMethod) &self::doCommand,
	    kIOUCStructIStructO,		// want StructINothingO but doesn't exist!
	    sizeof(EscaladeUserCommand),	// command in
	    sizeof(EscaladeUserCommand)		// command out
	},
	{   // kEscaladeUserClientGetAEN
	    NULL,				// IOService
	    (IOMethod) &self::getAEN,
	    kIOUCScalarIScalarO,
	    0,					// no input
	    1					// one scalar out
	},
	{   // kEscaladeUserClientReset
	    NULL,				// IOService
	    (IOMethod) &self::doReset,
	    kIOUCScalarIScalarO,
	    0,					// no input
	    0					// no output
	},
    };

    // range check
    if (index < kEscaladeUserClientMethodCount) {
	*target = this;
	return((IOExternalMethod *)&sMethods[index]);
    }

    // invalid request
    error("request for invalid method %d", (int)index);
    return(NULL);
};

//////////////////////////////////////////////////////////////////////////////
// Generic interface
//
bool
self::initWithTask(task_t owningTask, void *security_id, UInt32 type)
{
    // XXX should check security token here?
    
    if (!super::initWithTask(owningTask, security_id , type))
        return false;

    if (!owningTask)
	return false;

    fTask = owningTask;
    fProvider = NULL;
    fSecurity = security_id;

    return true;
}

bool
self::start(IOService *provider)
{
    // generic start
    if (!super::start(provider))
	return(false);

    // Must be associated with the right driver
    if ((fProvider = OSDynamicCast(EscaladeController, provider)) == NULL)
	return(false);

    return(true);
}

IOReturn
self::open(void)
{
    
    // provider terminated?
    if (isInactive())
	return(kIOReturnNotAttached);
    
    // Open the provider.
    // Note that this must guarantee exclusive UserClient access to the controller.
    if (!fProvider->open(this))
	return(kIOReturnExclusiveAccess);

    // Don't allow the array/system to sleep while open
    fProvider->addFullPowerConsumer();
    return(kIOReturnSuccess);
}

IOReturn
self::close(void)
{

    // provider terminated?
    if (isInactive())
	return(kIOReturnNotAttached);
    
    if (fProvider->isOpen(this)) {
	fProvider->removeFullPowerConsumer();
	fProvider->close(this);
    }

    return(kIOReturnSuccess);
}

IOReturn
self::clientClose(void)
{
    // close parent if required
    close();

    if (fTask)
	fTask = NULL;
    fProvider = NULL;
    terminate();

    return(kIOReturnSuccess);
}

