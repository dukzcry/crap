// $Id: EscaladeUserClientInterface.h,v 1.6 2003/12/23 21:51:20 msmith Exp $

//
// This file defines the API between the Escalade UserClient and
// a userland client.
//

#ifndef ESCALADEUSERCLIENTINTERFACE_H
#define ESCALADEUSERCLIENTINTERFACE_H

enum {
    // Obtain exclusive access to the controller
    kEscaladeUserClientOpen,			// -

    // Release exclusive access to the controller
    kEscaladeUserClientClose,			// -

    // Return the controller architecture
    kEscaladeUserClientControllerArchitecture,	// ScalarO

    // Terminate and detach the numbered unit
    kEscaladeUserClientRemoveUnit,		// ScalarI

    // Probe and (possibly) attach the numbered unit
    kEscaladeUserClientAddUnit,			// ScalarI

    // Execute a command on the controller
    kEscaladeUserClientCommand,			// StructureI

    // Fetch the next AEN from the controller queue
    kEscaladeUserClientGetAEN,			// ScalarO

    // Request a controller reset
    kEscaladeUserClientReset,			// -

    // enum range limit
    kEscaladeUserClientMethodCount
};

// Controller architecture definitions
#ifndef TW_API_UNKNOWN_ARCHITECTURE
# define TW_API_5000_ARCHITECTURE                       0x0001
# define TW_API_6000_ARCHITECTURE                       0x0002
# define TW_API_7000_ARCHITECTURE                       0x0003
# define TW_API_8000_ARCHITECTURE                       0x0203
# define TW_API_UNKNOWN_ARCHITECTURE                        -1
#endif


// User command structure
typedef struct
{
    int			driver_opcode;
    vm_address_t	command_packet;
    vm_address_t	data_buffer;
    vm_offset_t		data_size;
} EscaladeUserCommand;

#endif ESCALADEUSERCLIENTINTERFACE_H
