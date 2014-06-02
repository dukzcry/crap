// $Id: EscaladeControllerIO.cpp,v 1.18 2003/12/23 22:06:32 msmith Exp $

//
// Low-level controller I/O primitives and procedures.
//

#include "Escalade.h"

// convenience
#define super IOService
#define self EscaladeController

////////////////////////////////////////////////////////////////////////////////
// Register accessors
//

//
// Set the control register
//
void
self::setControlReg(UInt32 val)
{
    pciNub->ioWrite32(0, val, registerMap);
}

//
// Read the status register
//
volatile UInt32
self::getStatusReg(void)
{
    return(pciNub->ioRead32(4, registerMap));
}

//
// Read the head of the response queue, return the tag that's there.
//
// Note that this value is only valid if the response queue is not
// empty.
//
UInt8
self::getResponseReg(void)
{
    UInt32	response_reg;
    
    response_reg = pciNub->ioRead32(12, registerMap);
    return((response_reg >> 4) & 0xff);
}

//
// Post a request, returns false if the controller is busy.
//
bool
self::postCommand(EscaladeCommand *ec)
{
    UInt32	status_reg;

    // check that the command queue is not full
    status_reg = getStatusReg();
    if (status_reg & TWE_STATUS_COMMAND_QUEUE_FULL)
	return(false);

    // and post the command
    pciNub->ioWrite32(8, (UInt32)ec->commandPhys, registerMap);

    // mark as posted
    ec->wasPosted();
    return(true);
}

//
// Enable interrupts
//
void
self::enableInterrupts(void)
{
    setControlReg(TWE_CONTROL_CLEAR_ATTENTION_INTERRUPT |
	TWE_CONTROL_UNMASK_RESPONSE_INTERRUPT |
	TWE_CONTROL_ENABLE_INTERRUPTS);
}

//
// Disable interrupts
//
void
self::disableInterrupts(void)
{
    setControlReg(TWE_CONTROL_DISABLE_INTERRUPTS);
}

////////////////////////////////////////////////////////////////////////////////
// Utility functions
//

//
// Wait up to waittime seconds for the status bits to be set.
//
// Note that this spins, and should be avoided when possible.
//
bool
self::waitStatusBits(UInt32 bits, int waittime)
{
    UInt32	status_reg = 0;
    int		i;

    for (i = 0; i < (waittime * 100); i++) {
	status_reg = getStatusReg();
	if ((status_reg & bits) == bits)
	    return(true);
	IODelay(10000);
    }
    debug(2, "timed out waiting for status bits 0x%08x, have status 0x%08x",
	  (uint)bits, (uint)status_reg);
    return(false);
}

//
// Drain the response queue, discard all the responses
//
bool
self::drainResponseQueue(void)
{
    UInt32		status_reg;
    //UInt8		tag;

    // spin pulling responses until none left
    for (;;) {
	// check controller status
	status_reg = getStatusReg();
	if (!warnControllerStatus(status_reg)) {
	    error("unexpected controller status 0x%08x", (uint)status_reg);
	    return(false);
	}

	// check for more status
	if (status_reg & TWE_STATUS_RESPONSE_QUEUE_EMPTY)
	    break;
	//tag = getResponseReg();
    }
    return(true);
}

//
// Perform a controller soft reset
//
void
self::softReset(void)
{
    setControlReg(TWE_SOFT_RESET_BITS);
}

//
// Check for controller errors
//
bool
self::checkControllerErrors(void)
{
    UInt32	status_reg;

    status_reg = getStatusReg();
    if (TWE_STATUS_ERRORS(status_reg) || !warnControllerStatus(status_reg))
	return(false);
    return(true);
}

//
// Emit a diagnostic if controller status is abnormal
//
bool
self::warnControllerStatus(UInt32 status)
{
    if (!((status & TWE_STATUS_EXPECTED_BITS) == TWE_STATUS_EXPECTED_BITS)) {
	error("controller status 0x%x missing expected bits 0x%x",
	      (uint)status, TWE_STATUS_EXPECTED_BITS);
	return(false);
    }
    if ((status & TWE_STATUS_UNEXPECTED_BITS) != 0) {
	error("controller status 0x%x contains unexpected bits from 0x%x",
	      (uint)status, TWE_STATUS_UNEXPECTED_BITS);
	return(false);
    }
    return(true);
}

////////////////////////////////////////////////////////////////////////////////
// Interrupt handler
//
void
self::interruptOccurred(OSObject *owner, __unused IOInterruptEventSource *src, __unused int count)
{
    self        *sp;

    // get instance against which the interrupt has occurred
    if ((sp = OSDynamicCast(self, owner)) == NULL)
	return;

    sp->handleInterrupt();
}


////////////////////////////////////////////////////////////////////////////////
// Handle a controller interrupt.
//
// We are on the workloop.
//
void
self::handleInterrupt(void)
{
    EscaladeCommand *ec;
    UInt32	status_reg;
    UInt8	tag;

    status_reg = getStatusReg();

    // Host interrupt - not used
    if (status_reg & TWE_STATUS_HOST_INTERRUPT) {
	debug(4, "host interrupt");
	setControlReg(TWE_CONTROL_CLEAR_HOST_INTERRUPT);
    }
    // Command interrupt - not currently used
    if (status_reg & TWE_STATUS_COMMAND_INTERRUPT) {
	debug(4, "command interrupt");
	setControlReg(TWE_CONTROL_MASK_COMMAND_INTERRUPT);
    }
    // Attention interrupt - one or more AENs to be fetched
    if (status_reg & TWE_STATUS_ATTENTION_INTERRUPT) {
	setControlReg(TWE_CONTROL_CLEAR_ATTENTION_INTERRUPT);
	supportThreadAddWork(ST_DO_AEN);
    }
    // Completion interrupt - work done
    if (status_reg & TWE_STATUS_RESPONSE_INTERRUPT) {
	while (!(getStatusReg() & TWE_STATUS_RESPONSE_QUEUE_EMPTY)) {
	    tag = getResponseReg();
	    ec = findCommand(tag);
	    if (ec == NULL) {		// bogus completion
		error("completed invalid tag %d", tag);
		continue;
	    }
	    // Perform command completion
	    // Note that once this is called, the command
	    // must not be touched here as it may have been
	    // invalidated.
	    ec->complete();
	}
    }
}


////////////////////////////////////////////////////////////////////////////////
// Run a command synchronously
//
// XXX implement timeout
IOReturn
self::synchronousCommandGateway(OSObject *owner, void *arg0, __unused void *arg1, __unused void *arg2, __unused void *arg3)
{
    self		*sp;
    EscaladeCommand	*ec = (EscaladeCommand *)arg0;

    // get our instance
    if ((sp = OSDynamicCast(self, owner)) == NULL)
	return(kIOReturnError);

    return(sp->runSynchronousCommand(ec) ? kIOReturnSuccess : kIOReturnError);
}

bool
self::runSynchronousCommand(EscaladeCommand *ec)
{
    IOReturn	ret;

    // if we don't have the workloop lock already, get it
    if (!workLoop->inGate()) {
	ret = commandGate->runAction(synchronousCommandGateway, (void *)ec);
	return((ret == kIOReturnSuccess) ? true : false);
    }

    // We can't run on the workloop thread, since once we block we
    // will never be able to process the interrupt.
    if (workLoop->onThread()) {
	error("synchronous command dispatched while on workloop thread");
	return(false);
    }
    
    ret = ec->prepare(true);
    // check that prepare() succeeded
    if (ret != kIOReturnSuccess) {
	error("prepare() failed");
	ec->complete(false);
	return(false);
    }

    // post the command
    if (!postCommand(ec)) {
	error("failed to post command");
	ec->complete(false);
	return(false);
    }

    // wait for completion
    // XXX timeout!
    for (;;) {
	// Uniterruptible because getting the command back out of the controller
	// would involve a complete reset.
	ret = commandGate->commandSleep(ec, THREAD_UNINT);
	switch (ret) {
	    case THREAD_AWAKENED:	// completed as planned
		return(true);
	    case THREAD_RESTART:	// harmless, spin.
		break;
	    default:
		error("woke with unexpected result %d", ret);
		return(false);		// this is bad...
	}
    }
}

////////////////////////////////////////////////////////////////////////////////
// Run a command asynchronously
//
IOReturn
self::asynchronousCommandGateway(OSObject *owner, void *arg0, __unused void *arg1, __unused void *arg2, __unused void *arg3)
{
    self		*sp;
    EscaladeCommand	*ec = (EscaladeCommand *)arg0;

    // get our instance
    if ((sp = OSDynamicCast(self, owner)) == NULL)
	return(kIOReturnError);

    return(sp->runAsynchronousCommand(ec) ? kIOReturnSuccess : kIOReturnError);
}

bool
self::runAsynchronousCommand(EscaladeCommand *ec)
{
    IOReturn	ret;

    // if not on the workloop, get ourselves there
    if (!workLoop->inGate()) {
	ret = commandGate->runAction(asynchronousCommandGateway, (void *)ec);
	return((ret == kIOReturnSuccess) ? true : false);
    }

    // hold if controller in reset
    while(resetting) {
	ret = commandGate->commandSleep(&resetting, THREAD_UNINT);
	switch (ret) {
	    case THREAD_AWAKENED:	// completed as planned
		goto reset_done;
	    case THREAD_RESTART:	// harmless, spin.
		break;
	    default:
		debug(3, "woke with unexpected result %d", ret);
		return(false);
	}
    }
reset_done:
    
    // we will not be waiting on this command
    ret = ec->prepare(false);
    // check that prepare() succeeded
    if (ret != kIOReturnSuccess) {
	ec->complete(false);
	return(false);
    }

    // post the actual command
    if (!postCommand(ec)) {
	error("failed to post command");
	ec->complete(false);
	return(false);
    }

    // command will complete asynchronously once we're off the workloop
    return(true);
}

