// $Id: EscaladeControllerPM.cpp,v 1.8 2003/12/23 22:08:05 msmith Exp $

//
// Power management.
//

#include "Escalade.h"

// convenience
#define super IOService
#define self EscaladeController

//
// We define three power states; sleep, idle and active
//
#define EC_POWER_OFF		0
#define EC_POWER_STANDBY	1
#define EC_POWER_ACTIVE		2
#define EC_POWER_NSTATES	3

static IOPMPowerState powerStates[] = {
    // power-off, required to support system sleep
    {kIOPMPowerStateVersion1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    // standby, drives powered down
    {kIOPMPowerStateVersion1, 0, IOPMPowerOn, IOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0},
    // active
    {kIOPMPowerStateVersion1, (IOPMDeviceUsable | kIOPMPreventIdleSleep), IOPMPowerOn, IOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0}
};


//////////////////////////////////////////////////////////////////////////////
// Set up our PM code.
//
void
self::initPowerManagement(IOService *provider)
{
    debug(3, "called");


    PMinit();
    provider->joinPMtree(this);
    setIdleTimerPeriod(5 * 60);		// default idle timeout
    makeUsable();
    pmEnabled = true;
    fullPowerConsumers = 0;

    // power driver init
    registerPowerDriver(this, powerStates, EC_POWER_NSTATES);
    drivePowerState = true;

    // register our power-down handler
    powerDownNotifier = registerPrioritySleepWakeInterest((IOServiceInterestHandler)powerDownHandler, this);
    debug(3, "done");
}

//////////////////////////////////////////////////////////////////////////////
// Power Controller
//

//
// Report the controller's initial power state
//
// XXX is this useful/called?
unsigned long
self::initialPowerStateForDomainState(__unused IOPMPowerFlags flags)
{
    debug(3, "power state queried");
    return(EC_POWER_ACTIVE);
}

//
// Implement power state changes.
//
IOReturn
self::setPowerState(unsigned long state, IOService *junk __unused)
{
    debug(3, "called in state %d to set state %d", getPowerState(), (int)state);

    switch(state) {
	case EC_POWER_OFF:
	    // We do not actually allow ourselves to turn off; just treat
	    // this like a duplicate request for standby.
	    // Real power-off detection is handled by the sleep/wake interest
	    // handler.
	case EC_POWER_STANDBY:
	    supportThreadAddWork(ST_DO_POWER_STANDBY);
	    return(60 * 1000 * 1000);	// one minute timeout if we fail to comply
	case EC_POWER_ACTIVE:
        // The active transition is simpler, since there can be no active
        // commands if we are not already active.
        debug(3, "power state transition to active requested");
        setPowerActive();
        break;
	default:
	    return(IOPMNoSuchState);
    }
    return kIOPMAckImplied;
}

//
// Callback from the support thread to put the drives to sleep.
//
void
self::setPowerStandby(void)
{
    debug(3, "called in state %d", getPowerState());

    // If we were powered off, first reset/reinit the controller.
    // It doesn't make a lot of sense to go from off to standby,
    // and if we're in fact in active, this is us preparing to
    // go to sleep.
    if (needReInit && getPowerState() != EC_POWER_ACTIVE) {
        debug(3, "bring controller up");
        initController();
    }

    // If the drives are spinning, turn them off.
    if (drivePowerState) {
        debug(3, "stopping disks");
        setDrivePower(false);
    }

    // transition complete
    acknowledgeSetPowerState();
    debug(3, "transition to STANDBY complete");
}

//
// Callback from the support thread to bring the controller and drives back
// up.
//
void
self::setPowerActive(void)
{
    debug(3, "called in state %d", getPowerState());

    // If we were powered off, first reset/reinit the controller.
    if (needReInit) {
        debug(3, "bring controller up");
        initController();
    }

    // If the drives might be off, bring them back
    if (getPowerState() <= EC_POWER_STANDBY) {
        debug(3, "starting disks");
        setDrivePower(true);
    }

    debug(3, "transition to ACTIVE complete");
}


//////////////////////////////////////////////////////////////////////////////
// Policy Maker
//

//
// Intercept kPMMinutesToSpinDown and set our internal spin-down timer.
//
IOReturn
self::setAggressiveness(UInt32 type, UInt32 minutes)
{
    if (type == kPMMinutesToSpinDown) {
	debug(3, "setting spindown timer to %d minutes", (int)minutes);
	setIdleTimerPeriod(minutes * 60);
    }
    return(super::setAggressiveness(type, minutes));
}

//
// Elaborate dance to make sure that drives are spun up before actuallly
// attempting to issue an I/O.
//
void
self::checkPowerState(void)
{
    // Notify the idle timer of activity.  If this returns false,
    // we've dropped to a lower power state and have to wait for
    // it to come back up.
    if (activityTickle(kIOPMSuperclassPolicy1, EC_POWER_ACTIVE))
        return;

    // Wait for power to be restored
    commandGate->runAction(checkPowerStateLocked);
}

IOReturn
self::checkPowerStateLocked(OSObject *owner, __unused void *arg0, __unused void *arg1, __unused void *arg2, __unused void *arg3)
{
    self	*sp;

    // get instance against which the interrupt has occurred
    if ((sp = OSDynamicCast(self, owner)) == NULL)
	return(kIOReturnError);

    // wait for us to switch to the required state
    debug(3, "waiting for power to be restored");
    while ((sp->powerState = sp->getPowerState()) != EC_POWER_ACTIVE)
        sp->commandGate->commandSleep(&sp->powerState);

    return(kIOReturnSuccess);
}

//////////////////////////////////////////////////////////////////////////////
// Start or stop the drives attached to the controller
//
// Note that this is implicitly serialised by using the 'admin'
// command.
void
self::setDrivePower(bool desiredState)
{
    EscaladeCommand *ec;

    ec = getCommand(EC_COMMAND_ADMIN);

    // If we have the command and drive power is still in the wrong
    // state, make the switch. If we have raced with ourselves, just return.
    if (drivePowerState != desiredState) {
	ec->makePowerSave(desiredState ? 1 : 0);
	if (!runSynchronousCommand(ec) || !ec->getResult()) {
	    error("could not spin drives %s", desiredState ? "up" : "down");
	} else {
	    debug(3, "spun drives %s", desiredState ? "up" : "down");
	    drivePowerState = desiredState;
	}
    }
    ec->returnCommand();
}

//////////////////////////////////////////////////////////////////////////////
// Handle system power transitions.
//
//
IOReturn
self::powerDownHandler(void *target, __unused void *refCon, UInt32 messageType, __unused IOService *provider,
		       __unused void* messageArgument, __unused vm_size_t argSize)
{
    EscaladeController	*sp;

    sp = (EscaladeController *)target;

    debug(3, "got power message 0x%x", (int)messageType);

    switch(messageType) {

	// The system is about to sleep, which means that when we are powered
	// back up, the controller has to be re-initialised.
	case kIOMessageSystemWillSleep:
	    debug(3, "system sleep notified, will re-init controller before I/O");
	    sp->shutdownController();
	    sp->needReInit = true;
	    // It's possible for us to be forced to sleep even though we have a
	    // background operation in progress.  When we're woken and reinit
	    // the controller, the background operations should resume, so
	    // to keep the refcounting correct here we should take the
	    // fullPowerConsumers count to zero.
	    // ... yes, this could race.
	    while (sp->fullPowerConsumers > 0)
		sp->removeFullPowerConsumer();
	    break;
	case kIOMessageSystemWillNotSleep:
	    // since we have shut down, we still need to re-init - could do that here.
	    debug(3, "system sleep abort notified");
	    break;
	    
	// Note that at this point, all filesystems should be unmounted so there
	// will be no I/O going on.
	case kIOMessageSystemWillPowerOff:
	case kIOMessageSystemWillRestart:
	    sp->shutdownController();
	    IOSleep(500);	// XXX necessary?
	    sp->setDrivePower(false);
	    debug(3, "notified controller of shutdown");
	    break;
    }
    // XXX how long after shutdown before controller is stable?
    return(kIOReturnSuccess);
}

//////////////////////////////////////////////////////////////////////////////
// Handle clamping power on while background operations are in progress.
//
void
self::addFullPowerConsumer(void)
{
    changeFullPowerConsumer(1);
}

void
self::removeFullPowerConsumer(void)
{
    changeFullPowerConsumer(-1);
}

IOReturn
self::changeFullPowerConsumer(int delta)
{
    // handle transitions to/from 0
    if ((delta == 1) && (fullPowerConsumers == 0)) {
	// clamp power to maximum while we need it
        changePowerStateTo(EC_POWER_ACTIVE);
        debug(3, "clamping power while background operation in progess");
    } else if (delta == -1) {
	if (fullPowerConsumers == 1) {
	    // relinquish control over our power.
	    changePowerStateTo(0);
	    debug(3, "background operations complete, returning to standard PM policy");
	}
	if (fullPowerConsumers < 1) {
	    error("too many removeFullPowerConsumer calls!");
	    return(kIOReturnError);
	}
    } else if ((delta != -1) && (delta != 1)) {
        error("bad delta %d", delta);
        return(kIOReturnError);
    }
    fullPowerConsumers += delta;
    return(kIOReturnSuccess);
}
