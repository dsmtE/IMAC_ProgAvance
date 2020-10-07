//-----------------------------------------------------------------------------------
//
// Boids Tower Defense (based on Paratrooper code)
// Copyright (C) 2007-2008  Sukender
// For more information, contact us : sukender@free.fr
//
// All rights reserved
//
//-----------------------------------------------------------------------------------

#include "Path.h"
#include <PVLE/Util/Exception.h>
#include <boost/filesystem/convenience.hpp>


boost::filesystem::path cachedData;	// Ugly global

const boost::filesystem::path AppPaths::data() {
	if (!cachedData.empty()) return cachedData;		// Cached value
	cachedData = "Data";
	if (boost::filesystem::exists(cachedData)) return cachedData;
	cachedData = "../Data";
	if (boost::filesystem::exists(cachedData)) return cachedData;
	cachedData = "../../Data";
	if (boost::filesystem::exists(cachedData)) return cachedData;

	// Default
	cachedData = "../Data";
	return cachedData;
}

const boost::filesystem::path AppPaths::maps()     { return data() / "Maps"; }
const boost::filesystem::path AppPaths::models()   { return data() / "Models"; }
const boost::filesystem::path AppPaths::textures() { return data() / "Textures"; }
const boost::filesystem::path AppPaths::profiles() { return data() / "Profiles"; }
const boost::filesystem::path AppPaths::sounds()   { return data() / "Sounds"; }
const boost::filesystem::path AppPaths::locales()  { return data() / "Locale"; }

void AppPaths::ensurePaths() {
	// 1. Test critical paths (those that can't be created)
	if (!boost::filesystem::exists(data())) THROW_STR(data().string() + " directory doesn't exist");

	// 2. Create non-existing paths
	if (!boost::filesystem::exists(profiles())) boost::filesystem::create_directories(profiles());
}
