//-----------------------------------------------------------------------------------
//
// Boids Tower Defense (based on Paratrooper code)
// Copyright (C) 2007-2008  Sukender
// For more information, contact us : sukender@free.fr
//
// All rights reserved
//
//-----------------------------------------------------------------------------------

#include "OptionsConstants.h"
#include <boost/static_assert.hpp>

const char * const OPTION_NAME[/*MAX_OPTIONS*/] = {
	"version",
	"help",

	"FileVersion",

	// Display
	"ResolutionX",
	"ResolutionY",
	"FullScreen",
	"WideShrink",
	"ScreenNum",

	// Sound
	"SoundFXVolume",
	"MusicVolume",

	// Network
	"ProxyAddress",
	"ProxyType",
	"ProxyPort",

	// Map parameters
	"Map",
	"MapZ",
	"MapXY",
	"Boids",
	"Obstacles",
	"Clouds",

	// Other
	"StressTest"
};

BOOST_STATIC_ASSERT(sizeof(OPTION_NAME) / sizeof(char *) == MAX_OPTIONS);
