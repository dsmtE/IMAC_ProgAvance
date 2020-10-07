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

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <PVLE/Util/Version.h>
#include <string>


Version getVersion();		///< Returns version of the game.
std::string getRepositoryRevisionString();		///< Returns the repository revision for the "Constants.cpp" file (which is supposed to be modified and commited on each release).

extern char const * const APP_NAME;		///< Game name.
inline std::string getAppName() { return std::string(APP_NAME); }			///< Convienience function that returns APP_NAME in a std::string.

const unsigned int ReceivesShadowTraversalMask = 0x1;
const unsigned int CastsShadowTraversalMask = 0x2;


#endif	// CONSTANTS_H
