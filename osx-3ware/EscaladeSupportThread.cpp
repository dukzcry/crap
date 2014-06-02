// $Id: EscaladeSupportThread.cpp,v 1.7 2003/12/23 21:51:20 msmith Exp $

//
// Support thread for the controller.
//
// - Handles AENs
// - Handles timeout scans
// - Handles reset requests
// - Handles suspend/resume
//
// Note: Where possible, avoid sleeping in any work routine here to avoid
//       the possibility of deadlock when a reset is requested.
//

#include "Escalade.h"

// convenience
#define super IOService
#define self EscaladeController

//////////////////////////////////////////////////////////////////////////////
// Thread entrypoint
//
void
self::startSupportThread(void *arg)
{
    EscaladeController	*sp = (EscaladeController *)arg;

    // Keep a reference on the class while we are executing
    debug(3, "support thread started");
    sp->retain();    
    sp->supportThreadMain();
    sp->supportThread = NULL;	// just for cleanliness' sake
    sp->release();
    debug(3, "support thread exiting");
    thread_terminate(*globalThreadPtr);
}

//////////////////////////////////////////////////////////////////////////////
// Thread body
//
// The thread detects work by means of bits set in the supportThreadInbox.
// These bits are copied into the local variable workBits, and the thread
// loops until the set of bits are cleared through local actions.
//
void
self::supportThreadMain(void)
{
    UInt32		workBits;
    UInt16		aen;
    UInt32		desiredPowerState;

    workBits = 0;
    desiredPowerState = 0;

#define WB_TEST(x)	(workBits & (x))
#define WB_POWER(x)	((workBits & ST_DO_POWER_MASK) == (x))
#define WB_CLEAR(x)	workBits &= ~(x)

    for (;;) {

	//
	// Wait for work to be submitted.
	// Note that we only wait if we have no work to be going on with.
	//
	if (workBits == 0) {
	    if (commandGate->runAction(supportThreadGetWork, &workBits)) {
		error("error getting work, aborting");
		return;
	    }
	}

	//
	// Termination request.
	//
	if (WB_TEST(ST_DO_TERMINATE))
	    return;

	//
	// Controller reset request.
	//
	if (WB_TEST(ST_DO_RESET)) {
	    debug(2, "controller reset requested");
	    resetController();
	    WB_CLEAR(ST_DO_RESET);	// one-shot request (could check reset success?)
	}

	//
	// Check for timeouts.
	//
	if (WB_TEST(ST_DO_TIMEOUTS)) {
	    // this may set ST_DO_RESET if a timed-out command is found
	    if (commandGate->runAction(supportThreadCheckTimeouts, &workBits)) {
		error("error checking timeouts, aborting");
		return;
	    }
	    WB_CLEAR(ST_DO_TIMEOUTS);
	}

	//
	// Check for an AEN.
	// Note that if multiple AENs are outstanding, we will pick them up
	// one at a time around the main thread loop.
	//
	// Note that we are violating the "do not sleep" recommendation here;
	// if an AEN and a controller reset collide, we may deadlock (assuming
	// the AEN request is not completed by the controller).
	//
	if (WB_TEST(ST_DO_AEN)) {
	    if (!getParam(TWE_PARAM_AEN, TWE_PARAM_AEN_UnitCode, &aen)) {
		error("error fetching AEN");	// error, stop polling for now
		WB_CLEAR(ST_DO_AEN);
	    } else if (aen == TWE_AEN_QUEUE_EMPTY) {
		debug(3, "AEN queue emptied");
		WB_CLEAR(ST_DO_AEN);		// nothing more to fetch
	    } else {
		debug(3, "fetched AEN %x", (uint)aen);
		addAEN(aen);			// don't clear bit, we will run again
	    }
	}

	//
	// Unit rescan, something may have changed unit state(s).
	//
	if (WB_TEST(ST_DO_RESCAN)) {
	    findLogicalUnits();
	    WB_CLEAR(ST_DO_RESCAN);
	}
	
	//
	// Power state transitions.
	//
	if (WB_TEST(ST_DO_IDLED)) {
	    // restore the pending power state bits
	    WB_CLEAR(ST_DO_POWER_MASK | ST_DO_IDLED);
	    if (desiredPowerState & ST_DO_POWER_MASK) {
		debug(3, "resuming deferred power transition to %d", (int)desiredPowerState);
		workBits |= desiredPowerState;
	    }
	    desiredPowerState = 0;
	}
	if (WB_POWER(ST_DO_POWER_STANDBY)) {
	    debug(3, "power state transition to standby requested");
	    // Set the 'suspending' flag.  If there are no active commands,
	    // we can go straight to standby.
	    // If there are, we defer the power state transition and wait for
	    // an ST_DO_IDLED message.
	    if (!commandGate->runAction(setSuspending)) {
		setPowerStandby();
	    } else {
		desiredPowerState = ST_DO_POWER_STANDBY;
	    }
	    WB_CLEAR(ST_DO_POWER_STANDBY);
	}
    }
}

//////////////////////////////////////////////////////////////////////////////
// Check the active commands list for timed-out commands
//
IOReturn
self::supportThreadCheckTimeouts(OSObject *owner, void *arg0, void *arg1 __unused, void *arg2 __unused, void *arg3 __unused)
{
    EscaladeCommand	*ec;
    self		*sp;
    UInt32		*workBits;

    // get instance against which the interrupt has occurred
    if ((sp = OSDynamicCast(self, owner)) == NULL)
	return(kIOReturnError);

    workBits = (UInt32 *)arg0;

    queue_iterate(&sp->activeCommands, ec, EscaladeCommand *, fCommandChain) {
	if (ec->checkTimeout()) {
	    error("command %p timed out", ec);
	    *workBits |= ST_DO_RESET;
	}
    }
    return(kIOReturnSuccess);
}
    
//////////////////////////////////////////////////////////////////////////////
// Wait for someone to ask us to do work
//
IOReturn
self::supportThreadGetWork(OSObject *owner, void *arg0, __unused void *arg1, __unused void *arg2, __unused void *arg3)
{
    IOReturn	ret;
    self	*sp;
    UInt32	*workBits;

    // get instance against which the interrupt has occurred
    if ((sp = OSDynamicCast(self, owner)) == NULL)
	return(kIOReturnError);

    workBits = (UInt32 *)arg0;
    
    // Spin waiting for work
    while (sp->supportThreadInbox == 0) {
	ret = sp->commandGate->commandSleep(&sp->supportThreadInbox);
	switch(ret) {
	    case THREAD_AWAKENED:	// completed as planned
		goto got_work;
	    case THREAD_RESTART:	// harmless, spin.
		break;
	    default:			// all others are fatal
		debug(3, "woke with unexpected result %d", ret);
		return(kIOReturnError);
	}
    }
got_work:
    *workBits = sp->supportThreadInbox & ST_WORK_MASK;
    sp->supportThreadInbox = 0;
    return(kIOReturnSuccess);
}

//////////////////////////////////////////////////////////////////////////////
// Add more work for the support thread
//
IOReturn
self::supportThreadAddWorkGateway(OSObject *owner, void *arg0, __unused void *arg1, __unused void *arg2, __unused void *arg3)
{
    self	*sp;
    UInt32	*workBits;

    // get instance against which the interrupt has occurred
    if ((sp = OSDynamicCast(self, owner)) == NULL)
	return(kIOReturnError);

    workBits = (UInt32 *)arg0;
    sp->supportThreadAddWork(*workBits);
    return(kIOReturnSuccess);
}

void
self::supportThreadAddWork(UInt32 workBits)
{
    if (!workLoop->inGate()) {
	commandGate->runAction(supportThreadAddWorkGateway, &workBits);
	return;
    }

    // post new bits and post a wakeup
    supportThreadInbox |= workBits;
    commandGate->commandWakeup(&supportThreadInbox);
}

//////////////////////////////////////////////////////////////////////////////
// Suspend the controller
//
// Note that we have to run with the gate to avoid racing around the
// suspending flag.
//
IOReturn
self::setSuspending(OSObject *owner, void *arg0 __unused, void *arg1 __unused, void *arg2 __unused, void *arg3 __unused)
{
    self	*sp;

    // get instance against which the interrupt has occurred
    if ((sp = OSDynamicCast(self, owner)) == NULL)
	return(kIOReturnError);

    debug(3, "called with suspending %d", sp->suspending);
    
    // already in progress?
    if (sp->suspending)
	return(kIOReturnSuccess);
    sp->suspending = 1;
    
    // If there is no work in progress, we can go straight to the next stage
    if (queue_empty(&sp->activeCommands)) {
	debug(3, "suspend OK now");
	return(kIOReturnSuccess);
    }

    // Defer and wait for the active queue to drain
    debug(3, "wait for active commands to drain");
    return(kIOReturnBusy);
}
