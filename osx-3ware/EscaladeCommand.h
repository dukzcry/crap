// $Id: EscaladeCommand.h,v 1.16 2003/12/23 21:51:19 msmith Exp $

//
// EscaladeCommand
//
// Defines a command to be issued to the controller and all related
// housekeeping structures.
//

#ifndef ESCALADECOMMAND_H
#define ESCALADECOMMAND_H

class EscaladeController;

class EscaladeCommand : public IOCommand
{
    OSDeclareDefaultStructors(EscaladeCommand);
    
public:

    //
    // Factory method; creates a number command and inserts it into
    // the specified IOCommandPool.  
    //
    static EscaladeCommand *withAttributes(int withType, EscaladeController *withController, UInt8 withTag);

    //
    // Prepare/run the command.
    //
    virtual IOReturn	prepare(bool waiting);
    virtual void	complete(bool postSuccess = true);
    virtual bool	getResult(void);	// return values?
    virtual void	wasPosted(void);

    //
    // Command queueing
    //
    virtual void	_wasGotten(void);
    virtual void	returnCommand(void);
    virtual void	queueActive(void)   { controller->_queueActive(this); };
    virtual void	dequeueActive(void) { controller->_dequeueActive(this); };

    //
    // Command mutators
    //
    virtual void	makeInitConnection(UInt32 credits);
    virtual void	makeGetParam(int table, int param, size_t size);
    virtual void	makeSetParam(int table, int param, UInt8 value);
    virtual void	makeSetParam(int table, int param, UInt16 value);
    virtual void	makeSetParam(int table, int param, UInt32 value);
    virtual void	makeSetParam(int table, int param, void *vp, size_t vsize);
    virtual void	makeFlush(int unit);
    virtual void	makeReadWrite(int unit, IOMemoryDescriptor *buf, UInt32 block, UInt32 nblks,
			       IOStorageCompletion withCompletion);
    virtual void	makePowerSave(int state);	// 0 = spin down

    //
    // Timeout handling
    //
    virtual void	setTimeout(int seconds);
    virtual bool	isTimedOut(void);
    virtual bool	checkTimeout(void);
    
    //
    // Extract the results of a GetParam operation
    //
    virtual void	getParamResult(UInt8 *vp);
    virtual void	getParamResult(UInt16 *vp);
    virtual void	getParamResult(UInt32 *vp);
    virtual void	getParamResult(void *vp, size_t vsize);

    // public so reachable by controller
    IOPhysicalAddress	commandPhys;	// always contiguous
    int			commandType;	// pool from which the command came
    
    // public for UserClient
    TWE_Command		*commandPtr;
    virtual void	setDataBuffer(IOMemoryDescriptor *dp);
    virtual void	releaseDataBuffer(void);

    // superclass overrides
    virtual bool	init(int withType, EscaladeController *withController, UInt8 withTag);
    virtual void	free(void);

    // debugging
    virtual void	print(void);
    
private:
    virtual void	makeSetParam(int table, int param);
    virtual void	getParamBuffer(void);
    virtual void	releaseParamBuffer(void);

    // permanent fields
    EscaladeController		*controller;
    IOBufferMemoryDescriptor	*command;
    UInt8			commandTag;

    // data buffers, set each time command is used
    IOMemoryDescriptor		*dataBuffer;
    IOBufferMemoryDescriptor	*paramBuffer;
    TWE_Param			*paramPtr;
    IODMACommand        *dmaCommand;

    // completion, set by ::makeReadWrite
    IOStorageCompletion		completion;

    // set in ::prepare
    bool			waitingOwner;
    UInt64			byteCount;	// for reporting success

    // set in ::complete
    bool			postResult;	// false if error in posting command

    // timeout handling
    int				timeoutCredits;	// set when the command is mutated
    bool			timedOut;	// have we timed out?
};

#endif // ESCALADECOMMAND_H
