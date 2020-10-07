//-----------------------------------------------------------------------------------
//
// Boids Tower Defense (based on Paratrooper code)
// Copyright (C) 2007-2008  Sukender
// For more information, contact us : sukender@free.fr
//
// All rights reserved
//
//-----------------------------------------------------------------------------------

#ifndef APP_PATH_H
#define APP_PATH_H

#include <boost/filesystem/path.hpp>

/// Utility class for paths used in the app. May contain several useful methods in future versions.
class AppPaths {
public:
	static const boost::filesystem::path data();			///< Root for data
	static const boost::filesystem::path maps();			///< Level maps
	static const boost::filesystem::path models();
	static const boost::filesystem::path textures();
	static const boost::filesystem::path profiles();		///< Data for the player (key bindings, config, etc.)
	static const boost::filesystem::path sounds();
	static const boost::filesystem::path locales();		///< Translation base directory

	static void ensurePaths();
};


#endif	// APP_PATH_H
