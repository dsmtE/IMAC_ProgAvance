//-----------------------------------------------------------------------------------
//
// Boids Tower Defense (based on Paratrooper code)
// Copyright (C) 2007-2008  Sukender
// For more information, contact us : sukender@free.fr
//
// All rights reserved
//
//-----------------------------------------------------------------------------------

#include <PVLE/Config.h>

#include <PVLE/Util/Util.h>
#include <PVLE/Util/Log.h>
#include <PVLE/Util/I18N.h>
#include <PVLE/Util/Rand.h>
#include "App.h"
#include "Path.h"
#include "Constants.h"

#include <osg/ArgumentParser>
#include <iostream>
#include <string>

#include <PVLE/Util/AppOptions.h>

#include <boost/format.hpp>
#include <boost/exception/diagnostic_information.hpp>

using namespace std;

void displayHelp(const App & app) {
	std::cout
		<< APP_NAME << "\n"
		<< format_str(_("%1% (v%2%) is a mini tower defense game."), getAppName() % getVersion().toString()) << "\n"
		<< format_str(_("Please note that command line options overwirte those in %1%"), (AppPaths::profiles() / "Options.txt")) << "\n"
		<< app.cmdOptions
		<< std::endl;
}


int main(int argc, char **argv) {
#ifndef _DEBUG
	// Release mode : Set verbosity to a level controlled by the app, not the environment
	//osg::setNotifyLevel(osg::FATAL);		// Or ERROR ?

	// Redirect log to a file
// #include <iostream>
// #include <fstream>
// static std::ofstream    g_log("c:\\Message.log");
// main...
// {
	// std::cout.rdbuf(g_log.rdbuf());
	// std::cerr.rdbuf(g_log.rdbuf());
#endif

	srand();		// Initialize random seed (with current time)

	try {
		LOG_INFO << "App started" << std::endl;

		I18N::init(APP_NAME, AppPaths::locales(), "");

		// Create the game (with default options)
		auto pApp = std::make_shared<App>();		// Is it really necessary? It should be faster to let the OS do memory cleanup by using an 'App*' and not calling the destructor. In that case, we should do minimal cleanup (such as restoring the screen resolution) before exiting.
		try {
			if (!pApp->readOptions(argc, argv)) {
				displayHelp(*pApp);
				return 1;
			}
		}
		catch (std::exception &e) {
			LOG_FATAL << "Error on command line:" << e.what() << std::endl;
			displayHelp(*pApp);
			return 1;
		}
		catch (...) {
			LOG_FATAL << "Error on command line." << std::endl;
			displayHelp(*pApp);
			return 1;
		}

		// Run the game
		pApp->run();

		LOG_INFO << "App terminated without any uncaught exception" << std::endl;
	}
	catch (boost::exception &e) {
		LOG_FATAL << "Error : Exception raised\n  Description: " << boost::diagnostic_information(e) << std::endl;
		return 1;
	}
	catch (std::exception &e) {
		LOG_FATAL << "Error : Exception raised (std::exception)\n  Description: " << e.what() << std::endl;
		return 1;
	}
	catch (...) {
		LOG_FATAL << "Error : Exception raised (unknown)" << std::endl;
		return 1;
	}

	//if (osg::Referenced::getDeleteHandler()) {
	//	osg::Referenced::getDeleteHandler()->setNumFramesToRetainObjects(0);
	//	osg::Referenced::getDeleteHandler()->flushAll();
	//}
	return 0;
}
