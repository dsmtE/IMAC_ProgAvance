//-----------------------------------------------------------------------------------
//
// Boids Tower Defense (based on Paratrooper code)
// Copyright (C) 2007-2008  Sukender
// For more information, contact us : sukender@free.fr
//
// All rights reserved
//
//-----------------------------------------------------------------------------------

///\file
/// Game global constants, such as version/revision, and so on.

#ifndef USER_SWITCHES_H
#define USER_SWITCHES_H

#include <PVLE/Input/Control.h>

namespace UserSwitches {

enum ESwichesId {
	ORDER_MOVE = ControlState::USER_SWITCH_0,
	ORDER_STOP,
	ORDER_BUILD_BASIC_TURRET,

	MAX_USER_SWITCH
};

extern const char * switchesNames[];

const unsigned int BINDINGS_FILE_VERSION = 1;		// Increment this when user switches change.

}	// namespace UserSwitches

#endif	// USER_SWITCHES_H
