// $Id: Escalade.h,v 1.13 2003/12/23 21:51:19 msmith Exp $

//
// Master include file
//
// The include dependancies for this project are terribly incestuous, so
// in the name of simplicity everything is included here, and then each
// source file just includes us once.
//

#ifndef ESCALADE_H
#define ESCALADE_H

//////////////////////////////////////////////////////////////////////////////
// Tunables

// AEN's we will buffer
#define TWE_MAX_AEN_BUFFER	64


// System headers.	XXX prune!
#include <mach/vm_types.h>
#include <kern/clock.h>
#include <sys/cdefs.h>

#include <IOKit/assert.h>
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IOCommand.h>
#include <IOKit/IOCommandPool.h>
#include <IOKit/IOInterruptEventSource.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOLib.h>
#include <IOKit/IODMACommand.h>
#include <IOKit/IOReturn.h>
#include <IOKit/IOService.h>
#include <IOKit/IOTimerEventSource.h>
//#include <IOKit/IOUserClient.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/storage/IOBlockStorageDevice.h>
#include <IOKit/storage/IOStorage.h>
#include <IOKit/storage/IOStorageDeviceCharacteristics.h>
#include <IOKit/pwr_mgt/RootDomain.h>

#if defined PPC
#include <libkern/OSByteOrder.h>
#else
/* For users of private headers */
#undef OSSwapHostToLittleInt16
#undef OSSwapHostToLittleInt32
#undef OSSwapLittleToHostInt16
#undef OSSwapLittleToHostInt32
/* */
#define OSSwapHostToLittleInt16(x) (x)
#define OSSwapHostToLittleInt32(x) (UInt32) (x)
#define OSSwapLittleToHostInt16(x) (x)
#define OSSwapLittleToHostInt32(x) (x)
#endif

static IOThread *globalThreadPtr;
// Forward-delcare all our classes.
class com_3ware_EscaladeController;
class com_3ware_EscaladeCommand;
class com_3ware_EscaladeDrive;
//class com_3ware_EscaladeUserClient;
// shorthand for sanity's sake
#define EscaladeController	com_3ware_EscaladeController
#define EscaladeCommand		com_3ware_EscaladeCommand
#define EscaladeDrive		com_3ware_EscaladeDrive
//#define EscaladeUserClient	com_3ware_EscaladeUserClient

// Local headers, do not reorder.
//#include "EscaladeUserClientInterface.h"
//#include "EscaladeUserClient.h"
#include "EscaladeRegisters.h"
#include "EscaladeController.h"
#include "EscaladeCommand.h"
#include "EscaladeDrive.h"


#endif // ESCALADE_H
