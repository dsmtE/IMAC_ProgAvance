//-----------------------------------------------------------------------------------
//
// Boids Tower Defense (based on Paratrooper code)
// Copyright (C) 2007-2008  Sukender
// For more information, contact us : sukender@free.fr
//
// All rights reserved
//
//-----------------------------------------------------------------------------------

#include "UserSwitches.h"

namespace UserSwitches {

const char * switchesNames[] = {
	"MOVE",
	"STOP",
	"BUILD_BASIC_TURRET"
};

BOOST_STATIC_ASSERT(static_cast<int>(MAX_USER_SWITCH) <= ControlState::MAX_SWITCH);		// Static cast is used to avoid a warning
BOOST_STATIC_ASSERT(sizeof(switchesNames) / sizeof(char *) == MAX_USER_SWITCH - ControlState::USER_SWITCH_0);		// If failed, then there is a mistake is the above array.

}	// namespace UserSwitches
