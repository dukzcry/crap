// $Id: EscaladeController.cpp,v 1.35 2003/12/23 22:00:13 msmith Exp $

//
// Implementation of EscaladeController
//

#include "Escalade.h"

// convenience
#define super IOService
#define self EscaladeController

// default metaclass, structors
OSDefineMetaClassAndStructors(self, super);

////////////////////////////////////////////////////////////////////////////////
// IOService protocol
//

IOThread self::IOCreateThread(IOThreadFunc fcn, void *arg)
{
    thread_t thread;
    
    if (kernel_thread_start((thread_continue_t) fcn, arg, &thread) != KERN_SUCCESS)
        return NULL;
    
    thread_deallocate(thread);
    return thread;
}

//
// Our hardware was detected, set up to talk to it.
//
bool
self::start(IOService *provider)
{
    //const OSSymbol *userClient;
    bool	result;
    int		i;
    
    //
    // Initialise instance variables.
    //
    cold = true;
    resetting = false;
    pciNub = NULL;
    commandGate = NULL;
    interruptSrc = NULL;
    timerSrc = NULL;
    workLoop = NULL;
    freeCommands = NULL;
    emergencyCommand = NULL;
    aenBufHead = 0;
    aenBufTail = 0;
    queue_init(&activeCommands);
    for (i = 0; i < TWE_MAX_UNITS; i++)
	logicalUnit[i] = NULL;
    supportThread = NULL;
    supportThreadInbox = 0;
    pmEnabled = false;
    powerDownNotifier = NULL;

    //
    // Connect to our provider and establish resources.
    //
    
    // start the superclass first
    if (!super::start(provider)) {
	error("provider failed to start");
	goto fail;
    }

    // fetch settings from the plist
    getSettings();
    debug(1, "EscaladeController");

    // cache our PCI nub and open it
    if ((pciNub = OSDynamicCast(IOPCIDevice, provider)) == NULL) {
	error("could not get PCI nub");
	goto fail;
    }
    pciNub->retain();
    result = pciNub->open(this);
    pciNub->release();
    if (!result) {
	pciNub = NULL;
	error("could not open our PCI nub");
	goto fail;
    }

    // ensure busmaster and IO access is enabled
    pciNub->setBusMasterEnable(true);
    pciNub->setIOEnable(true, false);	// not exclusive

    // get a handle on our register map
    if ((registerMap = pciNub->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0)) == NULL) {
	error("could not map registers");
	goto fail;
    }

    //
    // Create the workloop and associated event sources.
    //
    
    // create our own workloop
    workLoop = IOWorkLoop::workLoop();
    if (workLoop == NULL) {
	error("can't create workloop");
	goto fail;
    }
    
    // create and register an interrupt source
    interruptSrc = IOInterruptEventSource::interruptEventSource(this,
	    (IOInterruptEventSource::Action)interruptOccurred, pciNub, 0);
    if (interruptSrc == NULL) {
    	error("could not create interrupt source");
	goto fail;
    }
    if (workLoop->addEventSource(interruptSrc) != kIOReturnSuccess) {
	error("could not add interrupt source to workloop");
	goto fail;
    }
    interruptSrc->enable();

    // create and register our command gate
    commandGate = IOCommandGate::commandGate(this);
    if (commandGate == NULL) {
    	error("could not create command gate");
	goto fail;
    }
    if (workLoop->addEventSource(commandGate) != kIOReturnSuccess) {
	error("could not add command gate to workloop");
	goto fail;
    }

    // register our timer event source
    timerSrc = IOTimerEventSource::timerEventSource(this,
						    (IOTimerEventSource::Action)timeoutOccurred);
    if (timerSrc == NULL) {
	error("could not create timer source");
	goto fail;
    }
    if (workLoop->addEventSource(timerSrc) != kIOReturnSuccess) {
	error("could not add timer source to workloop");
	goto fail;
    }
    // set initial timeout
    timerSrc->setTimeoutMS(TWE_TIMEOUT_INTERVAL);

#if 0
    //
    // Set up the UserClient.
    //
    
    // set up our userClientClass name, used when user clients
    // request a connection to the driver
    userClient = OSSymbol::withCStringNoCopy("com_3ware_EscaladeUserClient");
    if (userClient == NULL) {
	error("could not allocate userClient name");
	goto fail;
    }
    setProperty(gIOUserClientClassKey, (OSObject *)userClient);
    userClient->release();

    // Register ourselves for UserClient connectivity.
    registerService();
#endif

    //
    // Initialise commands and their requisite support.
    //

    // create our command pools
    if (!createCommands(TWE_MAX_COMMANDS)) {
	error("could not create command pool");
	goto fail;
    }

    //
    // Kick off the support thread.
    //
    supportThread = IOCreateThread(startSupportThread, (void *)this);
    globalThreadPtr = &supportThread;

    //
    // Initialise power management.  This must be done before
    // we can handle any I/O.
    //
    initPowerManagement(provider);

    //
    // Initialise the controller and find attached units.
    //
    if (!initController()) {
	error("controller init failed");
	goto fail;
    }

    // set some informative IORegistry properties
    setProperties();

    // detect and attach units
    if (!findLogicalUnits()) {
	error("probe for drives failed");
	goto fail;
    }

    //
    // Ready for normal operation.
    //
    debug(1, "Controller ready for I/O");
    cold = false;
    return(true);
    
fail:
    // XXX cleanup here?
    error("Escalade start failed");
    super::stop(provider);
    return(false);
}

//
// We're asked to stop, in preparation for being unloaded.
//
void
self::stop(IOService *provider)
{

    // disable power management
    if (pmEnabled)
	PMstop();

    // disconnect from the controller
    shutdownController();

    // turn off the disks
    IOSleep(500);	// XXX necessary?
    setDrivePower(false);

    // kill off the support thread
    if (supportThread)
	supportThreadAddWork(ST_DO_TERMINATE);

    // delete all our commands
    destroyCommands();

    // deregister the power-down handler
    if (powerDownNotifier)
	powerDownNotifier->release();
    
    // clean up the workloop
    if (commandGate != NULL) {
	workLoop->removeEventSource(commandGate);
	commandGate->release();
    }
    if (interruptSrc != NULL) {
	workLoop->removeEventSource(interruptSrc);
	interruptSrc->release();
    }
    if (timerSrc != NULL) {
	timerSrc->cancelTimeout();
	workLoop->removeEventSource(timerSrc);
	timerSrc->release();
    }
    if (workLoop != NULL) {
	workLoop->release();
    }
    
    // release the register map
    if (registerMap != NULL)
	registerMap->release();

    // must test since we may be called before the nub even exists
    if (pciNub != NULL)
	pciNub->close(this);
    
    // stop our provider (is this required?)
    super::stop(provider);
}

//
// Return our workloop, rather than the default
//
IOWorkLoop *
self::getWorkLoop(void)
{
    return(workLoop);
}

////////////////////////////////////////////////////////////////////////////////
// Initialisation glue

int verbose;

//
// fetch adjustable settings
//
void
self::getSettings(void)
{
    verbose = 0;
#if 0
    OSNumber	*numObj;
    
    // XXX this doesn't work - why?
    numObj = OSDynamicCast(OSNumber, getProperty("verbose"));
    if (numObj) {
	verbose = numObj->unsigned32BitValue();
	IOLog("got verbose = %d\n", verbose);
    } else {
	IOLog("couldn't find verbose\n");
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////
// Command pool handling

//
// Initialise the free pool
//
bool
self::createCommands(int count)
{
    EscaladeCommand *ec;
    int		tag;

    // initialise the command pools
    if ((freeCommands = IOCommandPool::withWorkLoop(workLoop)) == NULL)
	return(false);
    if ((adminCommands = IOCommandPool::withWorkLoop(workLoop)) == NULL)
	return(false);
    /*if ((userClientCommands = IOCommandPool::withWorkLoop(workLoop)) == NULL)
	return(false);*/
    bzero(commandLookup, sizeof(commandLookup));

    // create commands initially as IO commands
    for (tag = 1; tag < count; tag++) {
	if ((ec = EscaladeCommand::withAttributes(EC_COMMAND_IO, this, tag)) == NULL) {
	    error("could not create command %d", tag);
	    return(false);
	}
	commandLookup[tag] = ec;	// save for lookup
    }

    // get one regular and one emergency admin command
    ec = getCommand();
    ec->commandType = EC_COMMAND_ADMIN;
    ec->returnCommand();
    ec = getCommand();
    ec->commandType = EC_COMMAND_ADMIN;
    emergencyCommand = ec;

#if 0
    // get a userclient command
    ec = getCommand();
    ec->commandType = EC_COMMAND_USERCLIENT;
    ec->returnCommand();
#endif

    return(true);
}

//
// Destroy the free pool
//
// XXX needs to know about outstanding commands and wait for them
//     to complete, or something else needs to ensure that the
//     controller is quiescent
//
void
self::destroyCommands(void)
{
    EscaladeCommand	*ec;
    int			i;

    // can't use normal getters here as they block...
    i = 1;
    if (freeCommands != NULL) {
	while ((ec = (EscaladeCommand *)freeCommands->getCommand(false)) != NULL) {
	    i++;
	    ec->release();
	}
    }
#if 0
    if (userClientCommands != NULL) {
	while ((ec = (EscaladeCommand *)userClientCommands->getCommand(false)) != NULL) {
	    i++;
	    ec->release();
	}
    }
#endif
    if (adminCommands != NULL) {
	while ((ec = (EscaladeCommand *)adminCommands->getCommand(false)) != NULL) {
	    i++;
	    ec->release();
	}
    }
    if (emergencyCommand != NULL)
	emergencyCommand->release();

    if (i != TWE_MAX_COMMANDS)
	error("missing %d commands (still in use?)", TWE_MAX_COMMANDS - i);
    if (freeCommands) freeCommands->release();
    //userClientCommands->release();
    if (adminCommands) adminCommands->release();
}

//
// Get a command from the free pool; block until one is available.
//
EscaladeCommand *
self::getCommand(int commandType)
{
    EscaladeCommand	*cmd;

    switch(commandType) {
	case EC_COMMAND_IO:
	    cmd = (EscaladeCommand *)freeCommands->getCommand(true);
	    break;
	case EC_COMMAND_ADMIN:
	    cmd = (EscaladeCommand *)adminCommands->getCommand(true);
	    break;
#if 0
	case EC_COMMAND_USERCLIENT:
	    cmd = (EscaladeCommand *)userClientCommands->getCommand(true);
	    break;
#endif
	default:
	    return(NULL);
    }
    
    cmd->_wasGotten();
    return(cmd);
}

//
// Return a command to the free pool
//
void
self::_returnCommand(EscaladeCommand *cmd)
{
    switch(cmd->commandType) {
	case EC_COMMAND_IO:
	    freeCommands->returnCommand(cmd);
	    break;
	case EC_COMMAND_ADMIN:
	    adminCommands->returnCommand(cmd);
	    break;
#if 0
	case EC_COMMAND_USERCLIENT:
	    userClientCommands->returnCommand(cmd);
	    break;
#endif
	default:
	    error("command with unknown type (%d) freed", cmd->commandType);
	    cmd->release();	// try not to leak it...
    }
}

//
// Find a command given its tag
//
EscaladeCommand *
self::findCommand(UInt8 tag)
{
    if ((tag >= TWE_MAX_COMMANDS) || (tag == 0)) {
	error("looked up tag %d out of range", tag);
	return(NULL);
    }
    return(commandLookup[tag]);
}

//
// Maintain a list of active commands
//
// Note that these must be called with the gate held.
//
void
self::_queueActive(EscaladeCommand *cmd)
{
    queue_enter(&activeCommands, cmd, EscaladeCommand *, fCommandChain);
}

void
self::_dequeueActive(EscaladeCommand *cmd)
{
    queue_remove(&activeCommands, cmd, EscaladeCommand *, fCommandChain);

    // If we are waiting for the active commands queue to drain to empty,
    // that condition has been met.  Kick off the second part of the process.
    if (suspending && queue_empty(&activeCommands))
	supportThreadAddWork(ST_DO_IDLED);
}

bool
self::outputEscaladeSegment(IODMACommand * __unused,
                            IODMACommand::Segment64 segment,
                            void *segments,
                            UInt32 segmentIndex)
{
    TWE_SG_Entry	*sg;
    
    sg = (TWE_SG_Entry *)segments;

    sg[segmentIndex].address = OSSwapHostToLittleInt32(segment.fIOVMAddr);
    sg[segmentIndex].length = OSSwapHostToLittleInt32(segment.fLength);
    
    return true;
}

// The Escalade controller requires all data to be 512-aligned.
IODMACommand *
self::factory(void)
{
    // initialise command
    return IODMACommand::withSpecification(self::outputEscaladeSegment, 32, 0xfffffe0,
                                              IODMACommand::kMapped, 0, TWE_ALIGNMENT);
}

////////////////////////////////////////////////////////////////////////////////
// Timeout event handler
//
void
self::timeoutOccurred(OSObject *owner, __unused IOTimerEventSource *src)
{
    EscaladeController	*sp;

    if ((sp = OSDynamicCast(EscaladeController, owner)) == NULL) {
	error("timeout event not signalled by EscaladeController");
	return;
    }

    // kick the support thread
    sp->supportThreadAddWork(ST_DO_TIMEOUTS);

    // reschedule ourselves
    sp->timerSrc->setTimeoutMS(TWE_TIMEOUT_INTERVAL);
}

////////////////////////////////////////////////////////////////////////////////
// Reset and initialise the controller.
//
bool
self::initController(void)
{
    EscaladeCommand *ec;
    bool	result;

    debug(1, "called");
    
    result = false;
    ec = NULL;
    
    // disable interrupts
    disableInterrupts();

    //
    // Try to reset the controller
    //
    
    // soft reset first
    softReset();

    // wait for the attention interrupt that signals the reset completed
    if (!waitStatusBits(TWE_STATUS_ATTENTION_INTERRUPT, 30)) {
	error("microcontroller did not come out of reset");
	goto out;
    }
    setControlReg(TWE_CONTROL_CLEAR_ATTENTION_INTERRUPT);

    // enable interrupts again
    enableInterrupts();

    // check for errors
    if (!checkControllerErrors()) {
	error("controller errors after reset");
	goto out;
    }

    //
    // Empty the response queue
    //
    if (!drainResponseQueue()) {
	error("could not drain the response queue");
	goto out;
    }

    //
    // Perform initconnection
    //
    if (!initConnection(TWE_INIT_MESSAGE_CREDITS))
	goto out;

    //
    // Power up drives.
    //
    setDrivePower(true);
    
    //
    // Tell the controller that we support shutdown notification
    //
    ec = getCommand(EC_COMMAND_ADMIN);
    ec->makeSetParam(TWE_PARAM_FEATURES, TWE_PARAM_FEATURES_DriverShutdown, (UInt8)1);
    if (!runSynchronousCommand(ec) || !ec->getResult()) {
	error("could not configure for shutdown notification");
	goto out;	// XXX not really an error, but should always work regardless
    }

    //
    // Collect AENs from the queue
    //
    supportThreadAddWork(ST_DO_AEN);
    
    result = true;
    needReInit = false;
    debug(1, "done");
out:
    if (ec)
	ec->returnCommand();
    return(result);
}

////////////////////////////////////////////////////////////////////////////////
// Run the INITCONNNECTION command
//
bool
self::initConnection(int credits)
{
    EscaladeCommand	*ec;
    bool		result;
    
    ec = getCommand(EC_COMMAND_ADMIN);
    ec->makeInitConnection(credits);
    if (!runSynchronousCommand(ec) || !ec->getResult()) {
	error("could not initialise controller connection");
	result = false;
    } else {
	result = true;
    }
    ec->returnCommand();
    return(result);
}

////////////////////////////////////////////////////////////////////////////////
// Reset the controller, resubmit all outstanding commands.
//
// Must not be called on the workloop thread.
//
IOReturn
self::resetControllerGateway(OSObject *owner, __unused void *arg0, __unused void *arg1, __unused void *arg2, __unused void *arg3)
{
    self	*sp;

    // get instance against which the interrupt has occurred
    if ((sp = OSDynamicCast(self, owner)) == NULL)
	return(kIOReturnError);

    sp->resetController();
    return(kIOReturnSuccess);
}

void
self::resetController(void)
{
    EscaladeCommand	*ec;
    queue_head_t	pendingCommands;

    // we can't run on the workloop thread, as synchronous commands
    // will fail
    if (workLoop->onThread()) {
	error("called on workloop thread - cannot reset");
	return;
    }

    // if we're not on the workloop, go there
    if (!workLoop->inGate()) {
	commandGate->runAction(resetControllerGateway);
	return;
    }

    // mark controller as resetting
    if (resetting) {
	error("multiple resets signalled!");
	return;
    }
    resetting = true;
    error("controller reset in progress...");
    
    // get all the currently-outstanding commands
    // XXX could use queue_new_head except that it's buggy
    queue_init(&pendingCommands);
    while (!queue_empty(&activeCommands)) {
	queue_remove_first(&activeCommands, ec, EscaladeCommand *, fCommandChain);
	queue_enter(&pendingCommands, ec, EscaladeCommand *, fCommandChain);
    }

    // If the admin command pool is empty, we have to supply an 'emergency'
    // command so that the reset can complete.
    if ((ec = (EscaladeCommand *)adminCommands->getCommand(false)) == NULL) {
	ec = emergencyCommand;
	ec->commandType = EC_COMMAND_ADMIN;	// pretend it's an admin command
	debug(3, "using emergency command");
    }
    ec->returnCommand();
    emergencyCommand = NULL;
    
    // initialise the controller
    if (!initController())			// use in-reset mode
	error("controller reset failed!");	// XXX should give up completely here?

    // recover emergency command if we used it
    if (emergencyCommand == NULL) {
	emergencyCommand = (EscaladeCommand *)adminCommands->getCommand(false);
	if (emergencyCommand == NULL)
	    error("could not recover emergency command!");
    }
    
    // resubmit outstanding commands
    // XXX now out of order?
    while (!queue_empty(&pendingCommands)) {
	queue_remove_first(&pendingCommands, ec, EscaladeCommand *, fCommandChain);
	if (ec->isTimedOut()) {
	    error("terminating timed-out command %p", ec);
	    ec->complete();
	}
	if (!postCommand(ec)) {
	    error("could not re-submit command %p", ec);
	    ec->complete(false);
	}
    }

    resetting = false;
    commandGate->commandWakeup(&resetting);
    error("controller reset done.");
}


////////////////////////////////////////////////////////////////////////////////
// Notify the controller of impending shutdown.
//
void
self::shutdownController(void)
{
    if (cold)
	return;

    initConnection(TWE_SHUTDOWN_MESSAGE_CREDITS);
}

////////////////////////////////////////////////////////////////////////////////
// Locate logical drives on the controller.
//

bool
self::findLogicalUnits(void)
{
    UInt8		summary[TWE_MAX_UNITS];
    int			i;

    debug(1, "called");
    
    if (!getParam(TWE_PARAM_UNITSUMMARY, TWE_PARAM_UNITSUMMARY_Status, summary, TWE_MAX_UNITS)) {
	error("could not get unit summary information");
	return(false);
    }

    // process individual unit status
    for (i = 0; i < TWE_MAX_UNITS; i++) {

	// new unit?
	if ((logicalUnit[i] == NULL) &&
	    (summary[i] & TWE_PARAM_UNITSTATUS_Online)) {

	    // new unit found
	    doAddUnit(i);	// ignore result, since we don't care

	} else {
	    // existing unit has gone offline
	    if (logicalUnit[i] != NULL) {
		// it would be nice to have some user notification here
		error("unit %d has gone offline, forcibly removing", i);
		doRemoveUnit(i, true);
	    }
	}
    }

    debug(1, "done");
    return(true);
}

////////////////////////////////////////////////////////////////////////////////
// Read a parmeter from the controller
//
bool
self::getParam(int table, int param, void *vp, size_t size)
{
    EscaladeCommand	*ec;
    bool		result;

    result = true;
    ec = getCommand(EC_COMMAND_ADMIN);
    ec->makeGetParam(table, param, size);
    if (!runSynchronousCommand(ec) || !ec->getResult()) {
	result = false;
    } else {
	ec->getParamResult(vp, size);
    }
    ec->returnCommand();
    return(result);
}

bool
self::getParam(int table, int param, UInt8 *vp)
{
    EscaladeCommand	*ec;
    bool		result;

    result = true;
    ec = getCommand(EC_COMMAND_ADMIN);
    ec->makeGetParam(table, param, sizeof(*vp));
    if (!runSynchronousCommand(ec) || !ec->getResult()) {
	result = false;
    } else {
	ec->getParamResult(vp);
    }
    ec->returnCommand();
    return(result);
}

bool
self::getParam(int table, int param, UInt16 *vp)
{
    EscaladeCommand	*ec;
    bool		result;

    result = true;
    ec = getCommand(EC_COMMAND_ADMIN);
    ec->makeGetParam(table, param, sizeof(*vp));
    if (!runSynchronousCommand(ec) || !ec->getResult()) {
	result = false;
    } else {
	ec->getParamResult(vp);
    }
    ec->returnCommand();
    return(result);
}

bool
self::getParam(int table, int param, UInt32 *vp)
{
    EscaladeCommand	*ec;
    bool		result;

    result = true;
    ec = getCommand(EC_COMMAND_ADMIN);
    ec->makeGetParam(table, param, sizeof(*vp));
    if (!runSynchronousCommand(ec) || !ec->getResult()) {
	result = false;
    } else {
	ec->getParamResult(vp);
    }
    ec->returnCommand();
    return(result);
}

////////////////////////////////////////////////////////////////////////////////
// Establish a set of properties describing the controller and its configuration
void
self::setProperties(void)
{
    char	buf[22];
    UInt8	count8;
    UInt16	count16;

    // trim trailing space, nul-terminate
#define EATSPACE(p, n)				\
    do {					\
	int i = (n);				\
	do {					\
	    (p)[i--] = 0;			\
	} while ((i >= 0) && ((p)[i] == ' '));	\
    } while(0)

    // controller version numbers
    if (!getParam(TWE_PARAM_VERSION, TWE_PARAM_VERSION_FW, controllerVersion, 16)) {
	error("could not get firmware version");
	controllerVersion[0] = 0;
    } else {
	EATSPACE(controllerVersion, 16);
	setProperty("TWRE,FWVer", (const char *)controllerVersion);
    }
    if (!getParam(TWE_PARAM_VERSION, TWE_PARAM_VERSION_PCB, buf, 8)) {
	error("could not get PCB version");
    } else {
	EATSPACE(buf, 8);
	setProperty("TWRE,PCBAVer", (const char *)buf);
    }
    if (!getParam(TWE_PARAM_VERSION, TWE_PARAM_VERSION_ATA, buf, 8)) {
	error("could not get Achip version");
    } else {
	EATSPACE(buf, 8);
	setProperty("TWRE,ATAVer", (const char *)buf);
    }
    if (!getParam(TWE_PARAM_VERSION, TWE_PARAM_VERSION_PCI, buf, 8)) {
	error("could not get Pchip version");
    } else {
	EATSPACE(buf, 8);
	setProperty("TWRE,PCIVer", (const char *)buf);
    }
    if (!getParam(TWE_PARAM_VERSION, TWE_PARAM_VERSION_CtrlModel, buf, 16)) {
	error("could not get controller model");
	controllerName[0] = 0;
    } else {
	EATSPACE(buf, 16);
	sprintf(controllerName, "Escalade %s", buf);
	setProperty("TWRE,CtrlModel", (const char *)buf);
    }
    if (!getParam(TWE_PARAM_VERSION, TWE_PARAM_VERSION_CtrlSerial, buf, 20)) {
	error("could not get controller serial number");
    } else {
	EATSPACE(buf, 20);
	setProperty("TWRE,CtrlSerial", (const char *)buf);
    }

    if (!getParam(TWE_PARAM_CONTROLLER, TWE_PARAM_CONTROLLER_PortCount, &count8)) {
	error("could not get port count");
    } else {
	setProperty("TWRE,PortCount", (UInt64)count8, 64);
    }
    if (!getParam(TWE_PARAM_DRIVESUMMARY, TWE_PARAM_DRIVESUMMARY_Num, &count16)) {
	error("could not get drive count");
    } else {
	setProperty("TWRE,NumDrives", (UInt64)count16, 64);
    }
    if (!getParam(TWE_PARAM_UNITSUMMARY, TWE_PARAM_UNITSUMMARY_Num, &count16)) {
	error("could not get unit count");
    } else {
	setProperty("TWRE,NumUnits", (UInt64)count16, 64);
    }

    // These will be inherited by all our logical units.
    // Note that the maximum block count is arbitrary, the
    // controller's theoretical limit is 2^^32.
    setProperty(kIOMaximumBlockCountReadKey, (UInt64)TWE_MAX_IO_LENGTH, 64);
    setProperty(kIOMaximumBlockCountWriteKey, (UInt64)TWE_MAX_IO_LENGTH, 64);
    setProperty(kIOMaximumSegmentCountReadKey, (UInt64)TWE_MAX_SGL_LENGTH, 64);
    setProperty(kIOMaximumSegmentCountWriteKey, (UInt64)TWE_MAX_SGL_LENGTH, 64);
    // The Escalade controller requires all data to be 512-multiple sized,
    // bouncing is made up by IODMACommand
    setProperty(kIOMinimumSegmentAlignmentByteCountKey, (UInt64)TWE_SECTOR_SIZE, 64);

    // physical interconnect data
    // XXX need to detemrine whether ATA or SATA
    setProperty(kIOPropertyPhysicalInterconnectTypeKey, "ATA");
    setProperty(kIOPropertyPhysicalInterconnectLocationKey, kIOPropertyInternalKey);
}

////////////////////////////////////////////////////////////////////////////////
// Command/controller interface
//

//
// Wake up a caller asleep on this command.
//
void
self::wakeCommand(EscaladeCommand *ec)
{
    commandGate->commandWakeup(ec, false);
}

////////////////////////////////////////////////////////////////////////////////
// EscaladeDrive interface
//

//
// Return the controller's model name/version
//
char *
self::getControllerName(void)
{
    return(controllerName);
}

char *
self::getControllerVersion(void)
{
    return(controllerVersion);
}

//
// Do actual I/O
//
IOReturn
self::doAsyncReadWrite(int unit,
		       IOMemoryDescriptor *buffer,
		       UInt32 block,
		       UInt32 nblks,
		       IOStorageCompletion completion)
{
    EscaladeCommand	*ec;
    bool		result;

    // check that drives are spun up
    checkPowerState();

    // do I/O
    ec = getCommand();
    ec->makeReadWrite(unit, buffer, block, nblks, completion);
    result = runAsynchronousCommand(ec);
    if (!result)
	ec->returnCommand();

    return(result ? kIOReturnSuccess : kIOReturnIOError);
}

//
// Flush the cache for the given unit
//
IOReturn
self::doSynchronizeCache(int unit)
{
    EscaladeCommand	*ec;
    bool		result;

    // check that drives are spun up
    checkPowerState();

    // run a flush command
    ec = getCommand();
    ec->makeFlush(unit);
    result = runSynchronousCommand(ec);
    ec->returnCommand();

    return(result ? kIOReturnSuccess : kIOReturnIOError);
}

#if 0
////////////////////////////////////////////////////////////////////////////////
// EscaladeUserClient interface
//

//
// Return the controller version code
//
int
self::getControllerArchitecture(void)
{
    // XXX there is probably a better way to do this
    switch (controllerName[9]) {
	case '5':
	    return(TW_API_5000_ARCHITECTURE);
	case '6':
	    return(TW_API_6000_ARCHITECTURE);
	case '7':
	    return(TW_API_7000_ARCHITECTURE);
	case '8':
	    return(TW_API_8000_ARCHITECTURE);
    }
    return(TW_API_UNKNOWN_ARCHITECTURE);
}
#endif

//
// Request termination of a given unit
//
IOReturn
self::doRemoveUnit(int unit, bool force)
{
    EscaladeDrive	*pUnit;

    // find unit and validate
    pUnit = logicalUnit[unit];
    if (!pUnit)
	return(kIOReturnNoDevice);
    if (!force && pUnit->isOpen()) {
	error("attempt to remove busy unit %d", unit);
	return(kIOReturnBusy);
    }

    // valid, terminate unit
    debug(3, "unit %d unbusy, terminating", unit);
    pUnit->terminate();
    logicalUnit[unit] = NULL;
    return(kIOReturnSuccess);
}

//
// Request scan/attach of a given unit.
//
// Also used at driver startup time.
//

static struct {
    int		key;
    char	*name;
} unit_type[] = {
    {TWE_UD_CONFIG_RAID0,    "RAID0"},
    {TWE_UD_CONFIG_RAID1,    "RAID1"},
    {TWE_UD_CONFIG_TwinStor, "TwinStor"},
    {TWE_UD_CONFIG_RAID5,    "RAID5"},
    {TWE_UD_CONFIG_RAID10,   "RAID10"},
    {TWE_UD_CONFIG_CBOD,     "CBOD"},
    {TWE_UD_CONFIG_SPARE,    "SPARE"},
    {TWE_UD_CONFIG_SUBUNIT,  "SUBUNIT"},
    {TWE_UD_CONFIG_JBOD,     "JBOD"},
    {-1,                     "Uknown type"}
};

IOReturn
self::doAddUnit(int unit)
{
    int			j, table;
    UInt32		size;
    UInt16		dsize;
    TWE_Unit_Descriptor	*ud;
    char		*unitType;
    
    // sanity
    if (logicalUnit[unit] != NULL)
	return(kIOReturnStillOpen);

    // common table for this unit
    table = TWE_PARAM_UNITINFO + unit;

    // get unit capacity (blocks)
    if (!getParam(table, TWE_PARAM_UNITINFO_Capacity, &size)) {
	error("could not get capacity for unit %d", unit);
	return(kIOReturnNoDevice);
    }
    // if size is zero, we're not interested
    if (size == 0)
	return(kIOReturnNoDevice);

    // get Unit Descriptor table size
    if (!getParam(table, TWE_PARAM_UNITINFO_DescriptorSize, &dsize)) {
	error("could not get descriptor size for unit %d", unit);
	return(kIOReturnIOError);
    }

    // allocate and fetch table
    dsize -= 3;
    ud = (TWE_Unit_Descriptor *)IOMalloc(dsize);
    if (!getParam(table, TWE_PARAM_UNITINFO_Descriptor, (void *)ud, dsize)) {
	error("could not get unit descriptor for unit %d", unit);
	IOFree(ud, dsize);
	return(kIOReturnIOError);
    }
    for (j = 0; unit_type[j].key != -1; j++)
	if (unit_type[j].key == ud->configuration)
	    break;
    unitType = unit_type[j].name;
    IOFree(ud, dsize);

    // instantiate the drive
    if ((logicalUnit[unit] = new EscaladeDrive) == NULL) {
	error("could not instantiate EscaladeDrive for unit %d", unit)
	return(kIOReturnNoMemory);
    }
    logicalUnit[unit]->init(NULL);
    if (!logicalUnit[unit]->attach(this)) {
	error("could not attach EscaladeDrive for unit %d", unit);
	logicalUnit[unit]->release();
	logicalUnit[unit] = NULL;
	return(kIOReturnError);
    }
    logicalUnit[unit]->setUnit(unit);
    logicalUnit[unit]->setSize(size);
    logicalUnit[unit]->setConfiguration(unitType);

    // all done, register unit for discovery
    logicalUnit[unit]->registerService();

    // The IORegistry has retained the unit, so we don't here.
    // It might make sense for us to keep a reference on it
    // just in case...
    logicalUnit[unit]->release();
    
    return(kIOReturnSuccess);
}

#if 0
//
// Fetch an AEN from the queue.
//
IOReturn
self::getAENGateway(OSObject *owner, void *arg0, __unused void *arg1, __unused void *arg2, __unused void *arg3)
{
    self	*sp;
    UInt16	*aenCode;

    // get instance against which the interrupt has occurred
    if ((sp = OSDynamicCast(self, owner)) == NULL)
	return(kIOReturnError);

    aenCode = (UInt16 *)arg0;

    *aenCode = sp->getAEN();
    return(kIOReturnSuccess);
}
    
UInt16
self::getAEN(void)
{
    UInt16	aenCode;

    if (!workLoop->inGate()) {
	commandGate->runAction(getAENGateway, (void *)&aenCode);
	return(aenCode);
    }

    // queue empty?
    if (aenBufHead == aenBufTail) {
	aenCode = TWE_AEN_QUEUE_EMPTY;
    } else {
	// fetch off the ring
	aenCode = aenBuffer[aenBufTail];
	aenBufTail = (aenBufTail + 1) % TWE_MAX_AEN_BUFFER;
    }
    return(aenCode);
}
#endif

//
// Add an AEN to the queue.
//
IOReturn
self::addAENGateway(OSObject *owner, void *arg0, __unused void *arg1, __unused void *arg2, __unused void *arg3)
{
    self	*sp;
    UInt16	aenCode;

    // get instance against which the interrupt has occurred
    if ((sp = OSDynamicCast(self, owner)) == NULL)
	return(kIOReturnError);

    aenCode = *(UInt16 *)arg0;

    sp->addAEN(aenCode);
    return(kIOReturnSuccess);
}

void
self::addAEN(UInt16 aenCode)
{
    int		nextHead;

    if (!workLoop->inGate()) {
	commandGate->runAction(addAENGateway, (void *)&aenCode);
	return;
    }
	
    nextHead = (aenBufHead + 1) % TWE_MAX_AEN_BUFFER;

    // policy dictates we drop old AENs
    if (nextHead == aenBufTail)
	aenBufTail = (aenBufTail + 1) % TWE_MAX_AEN_BUFFER;

    aenBuffer[aenBufHead] = aenCode;
    aenBufHead = nextHead;

    //
    // Act on AENs reporting a change in required power.
    //
    switch(TWE_AEN_CODE(aenCode)) {
	case TWE_AEN_REBUILD_STARTED:
	case TWE_AEN_INIT_STARTED:
	case TWE_AEN_VERIFY_STARTED:
	    addFullPowerConsumer();
	    break;

	case TWE_AEN_REBUILD_FAIL:
	case TWE_AEN_REBUILD_DONE:
	case TWE_AEN_INIT_DONE:
	case TWE_AEN_VERIFY_FAILED:
	case TWE_AEN_VERIFY_COMPLETE:
	    removeFullPowerConsumer();
	    break;
    }

    //
    // Act on AENs that may have resulted in unit change or failure.
    //
    // XXX prune this list?
    //
    switch(TWE_AEN_CODE(aenCode)) {
	case TWE_AEN_DEGRADED_MIRROR:
	case TWE_AEN_CONTROLLER_ERROR:
	case TWE_AEN_UNIT_DELETED:
	case TWE_AEN_INCOMP_UNIT:
	case TWE_AEN_APORT_TIMEOUT:
	case TWE_AEN_DRIVE_ERROR:
	    // request a rescan of all units
	    supportThreadAddWork(ST_DO_RESCAN);
    }
}
