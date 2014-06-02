// $Id: EscaladeUserClient.h,v 1.6 2003/12/23 21:51:20 msmith Exp $

//
// UserClient class
//

#ifndef ESCALADEUSERCLIENT_H
#define ESCALADEUSERCLIENT_H

class EscaladeUserClient : public IOUserClient
{
    OSDeclareDefaultStructors(EscaladeUserClient);

protected:
    static const IOExternalMethod	sMethods[kEscaladeUserClientMethodCount];
    task_t		fTask;		// currently active user task
    EscaladeController	*fProvider;	// our provider
    void		*fSecurity;     // the user task's credentials

    // generic interface
    virtual bool	initWithTask(task_t owningTask, void *security_id, UInt32 type);
    virtual bool	start(IOService *provider);
    virtual IOExternalMethod *getTargetAndMethodForIndex(IOService **target, UInt32 index);
    virtual IOReturn	open(void);
    virtual IOReturn	close(void);
    virtual IOReturn	clientClose(void);
    virtual IOReturn	clientDied(void);
    
    // our work methods
    virtual IOReturn	getControllerArchitecture(int *outArchitecture);
    virtual IOReturn	doRemoveUnit(int inUnit);
    virtual IOReturn	doAddUnit(int inUnit);
    virtual IOReturn	doCommand(EscaladeUserCommand *inCommand, EscaladeUserCommand *outCommand,
			       IOByteCount inCount, IOByteCount *outCount);
    virtual IOReturn	getAEN(int *outAEN);
    virtual IOReturn	doReset(void);
};

#endif // ESCALADEUSERCLIENT
