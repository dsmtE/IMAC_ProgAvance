//-----------------------------------------------------------------------------------
//
// Boids Tower Defense (based on Paratrooper code)
// Copyright (C) 2007-2008  Sukender
// For more information, contact us : sukender@free.fr
//
// All rights reserved
//
//-----------------------------------------------------------------------------------


#include "App.h"
#include "OptionsConstants.h"
#include "UserSwitches.h"
#include "Path.h"
#include <osgGA/GUIEventAdapter>
#include <boost/filesystem/convenience.hpp>
#include <fstream>

inline std::string getOpt(EOptionsConstants opt) { return std::string(OPTION_NAME[opt]); }

// TODO move this? (was in App::)
void setDefaultBindingsBTD(Bindings & k, AxisBindings & a) {
	//setDefaultBindings(k, a);
	k[osgGA::GUIEventAdapter::KEY_Control_L] = ControlState::FIRE1;
	k[osgGA::GUIEventAdapter::KEY_Control_R] = ControlState::FIRE1;
	//k[osgGA::GUIEventAdapter::KEY_Alt_L] = UserSwitches::BONUS;
	//k[osgGA::GUIEventAdapter::KEY_Alt_R] = UserSwitches::BONUS;
	//k[osgGA::GUIEventAdapter::KEY_Space] = UserSwitches::SPOTLIGHT;

	k[osgGA::GUIEventAdapter::KEY_KP_Left]  = ControlState::LEFT;
	k[osgGA::GUIEventAdapter::KEY_KP_Right] = ControlState::RIGHT;
	k[osgGA::GUIEventAdapter::KEY_KP_Up]    = ControlState::UP;
	k[osgGA::GUIEventAdapter::KEY_KP_Down]  = ControlState::DOWN;
	k[osgGA::GUIEventAdapter::KEY_Left]  = ControlState::LEFT;
	k[osgGA::GUIEventAdapter::KEY_Right] = ControlState::RIGHT;
	k[osgGA::GUIEventAdapter::KEY_Up]    = ControlState::UP;
	k[osgGA::GUIEventAdapter::KEY_Down]  = ControlState::DOWN;

	k['o'] = ControlState::ROLL_LEFT;
	k['p'] = ControlState::ROLL_RIGHT;
	k[osgGA::GUIEventAdapter::KEY_Page_Up] = ControlState::ROLL_LEFT;
	k[osgGA::GUIEventAdapter::KEY_Page_Down] = ControlState::ROLL_RIGHT;
	k[osgGA::GUIEventAdapter::KEY_KP_Page_Up] = ControlState::ROLL_LEFT;
	k[osgGA::GUIEventAdapter::KEY_KP_Page_Down] = ControlState::ROLL_RIGHT;

	k[osgGA::GUIEventAdapter::KEY_KP_Add]      = ControlState::ZOOM_IN;
	k[osgGA::GUIEventAdapter::KEY_KP_Subtract] = ControlState::ZOOM_OUT;
	k['+'] = ControlState::ZOOM_IN;
	k['-'] = ControlState::ZOOM_OUT;
	//k['+'] = ControlState::FASTER;
	//k['-'] = ControlState::SLOWER;

	// Unlock keyboard targetting
	k[osgGA::GUIEventAdapter::KEY_Shift_L] = ControlState::FAST_TOGGLE;
	k[osgGA::GUIEventAdapter::KEY_Shift_R] = ControlState::FAST_TOGGLE;

	// Difficulty settings
	//k[osgGA::GUIEventAdapter::KEY_KP_Add] = ControlState::FASTER;
	//k[osgGA::GUIEventAdapter::KEY_KP_Subtract] = ControlState::SLOWER;
	//k[osgGA::GUIEventAdapter::KEY_Page_Up] = ControlState::FASTER;
	//k[osgGA::GUIEventAdapter::KEY_Page_Down] = ControlState::SLOWER;

	k[osgGA::GUIEventAdapter::KEY_Pause] = ControlState::PAUSE;
	k[osgGA::GUIEventAdapter::KEY_BackSpace] = ControlState::RESET;
	//k[osgGA::GUIEventAdapter::KEY_Escape] = ControlState::MENU;
	k[osgGA::GUIEventAdapter::KEY_F1] = ControlState::MENU;
	k[osgGA::GUIEventAdapter::KEY_F2] = ControlState::SCORE_MENU;
	k[osgGA::GUIEventAdapter::KEY_Tab] = ControlState::SCORE_MENU;
	k[osgGA::GUIEventAdapter::KEY_Delete] = ControlState::CANCEL;
	k[osgGA::GUIEventAdapter::KEY_F12] = ControlState::TOGGLE_FULLSCREEN;
	k['f'] = ControlState::TOGGLE_FULLSCREEN;

	k[BUTTON_MOUSE_MODIFIER + osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON] = UserSwitches::ORDER_MOVE;
	k['s'] = UserSwitches::ORDER_STOP;
	k['b'] = UserSwitches::ORDER_BUILD_BASIC_TURRET;
	k[BUTTON_MOUSE_MODIFIER + osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON] = ControlState::VIEW1;
	//k[BUTTON_MOUSE_MODIFIER + osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON]  = ;

	//k[AXIS_JOY_MODIFIER + 0] = ControlState::;

	// --- Axis ---
	a[AXIS_MOUSE_MODIFIER + 0] = AxisControl(ControlState::AXIS_X, 1.3, 80, 1);
	a[AXIS_MOUSE_MODIFIER + 1] = AxisControl(ControlState::AXIS_Y, 1.3, -80, 1);
	a[AXIS_MOUSE_MODIFIER + 2] = AxisControl(ControlState::AXIS_SCROLL_1, 1, 1, 0);		// Mouse wheel 1
	a[AXIS_MOUSE_MODIFIER + 3] = AxisControl(ControlState::AXIS_SCROLL_2, 1, 1, 0);		// Mouse wheel 2

	//a[AXIS_JOY_MODIFIER + 0] = AxisControl(ControlState::, 1, 1, 0);
}


void App::initOptions() {
	static const char * const COMMENT_START   = "";
	static const char * const COMMENT_DISPLAY = "Display options";
	static const char * const COMMENT_SOUND   = "Sound options";
	static const char * const COMMENT_NETWORK = "Network options";
	static const char * const COMMENT_MAPS = "Map parameters";
	static const char * const COMMENT_OTHER = "Other options";
	namespace po = boost::program_options;

	// Info
	po::options_description info_cmdline("Information options");
	info_cmdline.add_options()
		((getOpt(OPT_VERSION)+",v").c_str(), "Print version")
		((getOpt(OPT_HELP)+",h").c_str(), "Display help message");

	// File version
	po::options_description fileID(COMMENT_START);
	fileID.add_options()
		(OPTION_NAME[OPT_FILE_VERSION], po::value<unsigned int>()->default_value(0), "Do not edit this entry manually!");

	// Display
	po::options_description display(COMMENT_DISPLAY);
	display.add_options()
		((getOpt(OPT_RESOLUTION_X)+",x").c_str(), po::value<unsigned int>(&windowX)->default_value(0), "If ResolutionX or Y is null, then default (=desktop resolution) is used")
		((getOpt(OPT_RESOLUTION_Y)+",y").c_str(), po::value<unsigned int>(&windowY)->default_value(0))
		((getOpt(OPT_FULLSCREEN)+",f").c_str(), po::value<bool>(&fullscreen)->default_value(true)->implicit_value(true), "Full screen mode. Specifying \"-f0\" on command line forces the windowed mode.")
		(OPTION_NAME[OPT_WIDE_SHRINK], po::value<bool>()->default_value(false)->implicit_value(true), "Vertically shrink the image for wide screens resolutions (ie. >4/3). If not set, you just see more of the scene on the left/right borders.")
		((getOpt(OPT_SCREEN_NUM)+",s").c_str(), po::value<unsigned int>(&screenNum)->default_value(0), "Number of the screen to display the app on. For single screen configs, this must be 0.");

	// Sound
	po::options_description sound(COMMENT_SOUND);
	sound.add_options()
		(OPTION_NAME[OPT_SOUND_GAIN_FX], po::value<float>()->default_value(.4f), "Sets the volume (=gain) for sound effects, in [0;+INF] (usually in [0;1]).")
		(OPTION_NAME[OPT_SOUND_GAIN_MUSIC], po::value<float>()->default_value(1), "Sets the volume (=gain) for musics, in [0;+INF] (usually in [0;1]).");

	// Network
	po::options_description net(COMMENT_NETWORK);
	net.add_options()
		(OPTION_NAME[OPT_PROXY_ADDRESS], po::value<std::string>()->default_value(""), "Proxy IP (ex: 192.168.0.1) or machine name (Ex: proxy1). Do not set for no proxy.")
		(OPTION_NAME[OPT_PROXY_TYPE], po::value<std::string>()->default_value("HTTP"), "Proxy type can be one of { HTTP, SOCKS4, SOCK5, SOCKS4A, SOCKS5_HOSTNAME }")
		(OPTION_NAME[OPT_PROXY_PORT], po::value<unsigned int>()->default_value(8080));

	// Map parameters
	po::options_description maps(COMMENT_MAPS);
	maps.add_options()
		(OPTION_NAME[OPT_MAP_NAME], po::value<std::string>()->default_value("Demo01"), "Map name" )
		(OPTION_NAME[OPT_MAP_Z], po::value<float>()->default_value(1), "Height (Z) scale multiplier for the map")
		(OPTION_NAME[OPT_MAP_XY], po::value<float>()->default_value(1), "Size (XY) scale multiplier for the map")
		((getOpt(OPT_MAP_NB_BOIDS)+",b").c_str(), po::value<unsigned int>()->default_value(100), "Number of boids")
		((getOpt(OPT_MAP_NB_OBSTACLES)+",o").c_str(), po::value<unsigned int>()->default_value(30), "Number of obstacles")
		((getOpt(OPT_MAP_NB_CLOUDS)+",c").c_str(), po::value<unsigned int>()->default_value(5), "Number of decorative clouds (may affect performance)");

	// Other
	po::options_description other(COMMENT_OTHER);
	other.add_options()
		((getOpt(OPT_STRESS_TEST)+",t").c_str(), po::value<bool>(/*&stressTest*/)->default_value(false)->implicit_value(true), "BoidSim memory stress test");

	// -- Groups --
	cmdOptions.add(info_cmdline).add(display).add(sound).add(net).add(maps).add(other);
	fileOptions.add(fileID).add(display).add(sound).add(net).add(maps);
}


void App::writeOptions(bool update) {
	static const char * const COMMENT_START =
		"# Configuration file\n"
		"# ------------------\n"
		"#\n"
		"# You can edit the values, except the file version which allows the program to detect obsolete files.\n";

	///\todo Find a cleaner way to "update" options values in the variables map.
	if (update) {
		//options.find("Difficulty")->second.value() = initialDifficulty;
		options.find("ResolutionX")->second.value() = windowX;
		options.find("ResolutionY")->second.value() = windowY;
		options.find("FullScreen")->second.value() = fullscreen;
		//namespace po = boost::program_options;
		//po::notify(options);		// Doesn't seem to work...
	}

	const boost::filesystem::path optionsPath(AppPaths::profiles() / "Options.txt");
	AppOptions::write(options, fileOptions, optionsPath, COMMENT_START);
	//ASSERT(boost::filesystem::exists(optionsPath));
}


bool App::readOptions(int argc, char **argv) {
	namespace po = boost::program_options;
	const boost::filesystem::path optionsPath(AppPaths::profiles() / "Options.txt");
	if (!boost::filesystem::exists(optionsPath)) writeOptions(false);
	ASSERT(boost::filesystem::exists(optionsPath));

	po::store(po::parse_command_line(argc, argv, cmdOptions), options);
	std::ifstream ifs(optionsPath.string().c_str());
	po::store(po::parse_config_file(ifs, fileOptions), options);
	ifs.close();
	po::notify(options);

	return options.count("help")==0;
}


