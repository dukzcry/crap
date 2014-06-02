// $Id: EscaladeController.h,v 1.21 2003/12/23 22:18:40 msmith Exp $

//
// EscaladeController
//
// Class which owns the controller.
//

#ifndef ESCALADECONTROLLER_H
#define ESCALADECONTROLLER_H

// Maximum number of outstanding commands we allow.
// The controller has a hard limit of 255.
// It is implicitly assumed that if we can get a command,
// there is room for it in the controller.
#define TWE_MAX_COMMANDS	128

// Command pool identifiers.
#define EC_COMMAND_IO		0	// freeCommands pool
#define EC_COMMAND_ADMIN	1	// adminCommand pool
//#define EC_COMMAND_USERCLIENT	2	// userClientCommand pool

// Timeout rate (seconds)
// The timeout fires once (per controller) every TWE_TIMEOUT_RATE
// seconds, and the support thread runs to check for timed-out
// commands.  This is also the 'credit' deducted from each command's
// timeout credit every time timeouts are checked.
#define TWE_TIMEOUT_INTERVAL	10000	// milliseconds
#define TWE_TIMEOUT_COST	10	// credits to deduct

// Commands have a 30-second timeout by default; the ATA spec
// requires a drive to respond within 30 seconds even from a cold
// start.
// Commands submitted by the userclient get a longer timeout, since
// they may involve controller work as well.
#define TWE_TIMEOUT_DEFAULT	30
//#define TWE_TIMEOUT_USERCLIENT	60

// Support thread work request bits
#define ST_DO_AEN	(1<<0)		// request clearing/processing of AEN queue
#define ST_DO_RESET	(1<<1)		// request controller reset
#define ST_DO_TIMEOUTS	(1<<2)		// check commands for timeout
#define ST_DO_IDLED	(1<<3)		// active command queue has gone empty
#define ST_DO_RESCAN	(1<<4)		// rescan all units for changes

// Support thread power state transition request bits
#define ST_DO_POWER_STANDBY	(1<<24)
#define ST_DO_POWER_ACTIVE	(2<<24)
#define ST_DO_POWER_MASK	(3<<24)

// Terminate the support thread
#define ST_DO_TERMINATE	(1<<31)		// kill thread

#define ST_WORK_MASK    (ST_DO_AEN		| \
			 ST_DO_RESET		| \
			 ST_DO_TIMEOUTS		| \
			 ST_DO_IDLED		| \
			 ST_DO_RESCAN		| \
			 ST_DO_POWER_MASK	| \
			 ST_DO_TERMINATE)


class EscaladeController : public IOService
{
    OSDeclareDefaultStructors(EscaladeController);

public:

    //
    // Command pool interface
    //
    virtual EscaladeCommand	*getCommand(int commandType = EC_COMMAND_IO);
    virtual void		_returnCommand(EscaladeCommand *cmd);
    virtual void		_queueActive(EscaladeCommand *cmd);
    virtual void		_dequeueActive(EscaladeCommand *cmd);

    IOThread IOCreateThread(IOThreadFunc, void *);
    // superclass overrides
    virtual bool		start(IOService *provider);
    virtual void		stop(IOService *provider);
    virtual IOWorkLoop		*getWorkLoop(void);

    //
    // EscaladeCommand interface
    //
    virtual void		wakeCommand(EscaladeCommand *ec);
    virtual bool		postCommand(EscaladeCommand *ec);
    virtual void		requestReset(void) { supportThreadAddWork(ST_DO_RESET); };

    //
    // EscaladeDrive interface
    //
    virtual char		*getControllerName(void);
    virtual char		*getControllerVersion(void);
    virtual IOReturn		doAsyncReadWrite(int unit, IOMemoryDescriptor *buffer,
				       UInt32 block, UInt32 nblks, IOStorageCompletion completion);
    virtual IOReturn		doSynchronizeCache(int unit);

#if 0
    //
    // EscaladeUserClient interface
    //
    virtual int			getControllerArchitecture(void);
    virtual UInt16		getAEN(void);
#endif
    virtual IOReturn		doRemoveUnit(int unit, bool force = false);
    virtual IOReturn		doAddUnit(int unit);

    //
    // External command interface
    //
    virtual bool		runSynchronousCommand(EscaladeCommand *ec);
    virtual bool		runAsynchronousCommand(EscaladeCommand *ec);
    
    //
    // Power management interface.
    //
    virtual void		addFullPowerConsumer(void);
    virtual void		removeFullPowerConsumer(void);
    
    // interrupt handling
    virtual void		handleInterrupt(void);
    
    // create a new command
    static IODMACommand	*factory(void);

private:

    // attached drives
    EscaladeDrive		*logicalUnit[TWE_MAX_UNITS];
	
    // controller state
    IOPhysicalAddress           memBasePhysical;
    IOPCIDevice                 *pciNub;
    IOWorkLoop                  *workLoop;
    IOCommandGate		*commandGate;
    IOInterruptEventSource      *interruptSrc;
    IOTimerEventSource          *timerSrc;
    IOMemoryMap                 *registerMap;
    char			controllerName[32];
    char			controllerVersion[18];
    
    // command pools and indexing
    IOCommandPool		*freeCommands;
    IOCommandPool		*adminCommands;
    //IOCommandPool		*userClientCommands;
    EscaladeCommand		*emergencyCommand;
    EscaladeCommand		*commandLookup[TWE_MAX_COMMANDS];
    virtual bool		createCommands(int count);
    virtual void		destroyCommands(void);
    virtual EscaladeCommand	*findCommand(UInt8 tag);
    queue_head_t		activeCommands;

    // driver state
    bool			cold;		// driver still coming up
    bool			resetting;	// reset in progress
    bool			suspending;	// suspend in progress/active
    bool			pmEnabled;	// power management enabled

    // AEN queue
    UInt16			aenBuffer[TWE_MAX_AEN_BUFFER];
    int				aenBufHead;
    int				aenBufTail;
    virtual void		addAEN(UInt16 aenCode);
    static IOReturn		addAENGateway(OSObject *owner, void *arg0, void *arg1, void *arg2, void *arg3);
    static IOReturn		getAENGateway(OSObject *owner, void *arg0, void *arg1, void *arg2, void *arg3);

    // support thread
    IOThread			supportThread;
    static void			startSupportThread(void *arg);
    virtual void		supportThreadMain(void);
    UInt32			supportThreadInbox;
    virtual void		supportThreadAddWork(UInt32 workBits);
    static IOReturn		supportThreadGetWork(OSObject *owner, void *arg0, void *arg1, void *arg2, void *arg3);
    static IOReturn		supportThreadAddWorkGateway(OSObject *owner, void *arg0, void *arg1, void *arg2, void *arg3);
    static IOReturn		supportThreadCheckTimeouts(OSObject *owner, void *arg0, void *arg1, void *arg2, void *arg3);
    static IOReturn		setSuspending(OSObject *owner, void *arg0, void *arg1, void *arg2, void *arg3);

    static bool         outputEscaladeSegment(IODMACommand *, IODMACommand::Segment64, void *, UInt32);
    
    // event handlers
    static void			interruptOccurred(OSObject *owner, IOInterruptEventSource *src, int count);
    static void			timeoutOccurred(OSObject *owner, IOTimerEventSource *src);

    // power management
    int				powerState;
    bool			drivePowerState;
    bool			needReInit;
    int				fullPowerConsumers;
    virtual void		initPowerManagement(IOService *provider);
    virtual unsigned long		initialPowerStateForDomainState(IOPMPowerFlags flags);
    virtual IOReturn		setAggressiveness(UInt32 type, UInt32 minutes);
    virtual void		checkPowerState(void);
    static IOReturn		checkPowerStateLocked(OSObject *owner, void *arg0, void *arg1, void *arg2, void *arg3);
    virtual IOReturn		setPowerState(unsigned long state, IOService *whichDevice);
    virtual void		setPowerStandby(void);
    virtual void		setPowerActive(void);
    virtual void		setDrivePower(bool powerOn);

    IOReturn            changeFullPowerConsumer(int delta);
    IONotifier			*powerDownNotifier;
    static IOReturn		powerDownHandler(void *target, void *refCon, UInt32 messageType, IOService * service,
						 void* messageArgument, vm_size_t argSize);

    // debugging
    virtual void		getSettings(void);
    
    // command procedures
    virtual bool		initController(void);
    virtual bool		initConnection(int credits);
    virtual void		shutdownController(void);
    virtual bool		findLogicalUnits(void);
    virtual bool		getParam(int table, int param, void *vp, size_t size);
    virtual bool		getParam(int table, int param, UInt8 *vp);
    virtual bool		getParam(int table, int param, UInt16 *vp);
    virtual bool		getParam(int table, int param, UInt32 *vp);
    virtual void		setProperties(void);
    virtual void		resetController(void);
    static IOReturn		resetControllerGateway(OSObject *owner, void *arg0, void *arg1, void *arg2, void *arg3);
    
    // low-level controller I/O (EscaladeControllerIO module)
    virtual void		setControlReg(UInt32 val);
    virtual volatile UInt32	getStatusReg(void);
    virtual UInt8		getResponseReg(void);
    virtual void		enableInterrupts(void);
    virtual void		disableInterrupts(void);
    virtual bool		drainResponseQueue(void);
    virtual void		softReset(void);
    virtual bool		waitStatusBits(UInt32 bits, int waittime);
    virtual bool		checkControllerErrors(void);
    virtual bool		warnControllerStatus(UInt32 status);
    static IOReturn		synchronousCommandGateway(OSObject *owner, void *arg0, void *arg1, void *arg2, void *arg3);
    static IOReturn		asynchronousCommandGateway(OSObject *owner, void *arg0, void *arg1, void *arg2, void *arg3);
};

__private_extern__ int verbose;

// debugging
#define debug(level, fmt, args...)                      \
    do {                                                \
	if (verbose > (level))                          \
	    IOLog("%s: " fmt "\n", __func__ , ##args);  \
    } while (0);
    
#define error(fmt, args...)                             \
    do {                                                \
	IOLog("%s: " fmt "\n", __func__ , ##args);      \
    } while (0);

#endif // ESCALADECONTROLLER_H
