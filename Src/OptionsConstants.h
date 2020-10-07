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
/// Constants for game options (command line and options file).

#ifndef OPTIONS_CONSTANTS_H
#define OPTIONS_CONSTANTS_H

//#include <string>

enum EOptionsConstants {
	OPT_VERSION,
	OPT_HELP,

	OPT_FILE_VERSION,

	// Display
	OPT_RESOLUTION_X,
	OPT_RESOLUTION_Y,
	OPT_FULLSCREEN,
	OPT_WIDE_SHRINK,
	OPT_SCREEN_NUM,

	// Sound
	OPT_SOUND_GAIN_FX,
	OPT_SOUND_GAIN_MUSIC,
	//OPT_SOUND_GAIN_VOICE,

	// Network
	OPT_PROXY_ADDRESS,
	OPT_PROXY_TYPE,
	OPT_PROXY_PORT,

	// Map parameters
	OPT_MAP_NAME,
	OPT_MAP_Z,
	OPT_MAP_XY,
	OPT_MAP_NB_BOIDS,
	OPT_MAP_NB_OBSTACLES,
	OPT_MAP_NB_CLOUDS,

	// Other
	OPT_STRESS_TEST,
	//OPT_,

	MAX_OPTIONS
};


extern const char * const OPTION_NAME[/*MAX_OPTIONS*/];


#endif	// OPTIONS_CONSTANTS_H
