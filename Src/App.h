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
/// Application header. Defines all application globals.

#ifndef APP_H
#define APP_H

#include <string>
#include <vector>
#include <memory>

#include <osg/ref_ptr>
#include <osg/Vec3>
#include <osg/Node>
#include <osg/BoundingBox>

#include <PVLE/Util/AppOptions.h>
//#include <PVLE/Util/Util.h>
#include <PVLE/Game/Simulation.h>

class Game;
class GamePathFinder;
class C3DPhy;
class Hud;

namespace Util {
	class FPSLimiter;
}

namespace Physics {
	class World;
}

class BoidsUtil;
class Boid;
//#include "../Plugins/Boids/Boids.h"

namespace boost {
	class thread;
}

// ---------------------------------------------------------------------------------


/// App is a "Keep all globals" class that holds everything for the app.
///\author Sukender
class App : public SimulationCallback, public std::enable_shared_from_this<App> {
public:
	App();			///< Minimal initialization of the app.
	void run();		///< Calls load() and then run the app.

	Simulation simulation;		///< Simulation parameters.

	/// Reads command line and "options file" options.
	///\return \c true if everything is okay, false if help message should be printed before exiting the app.
	bool readOptions(int argc, char **argv);
	boost::program_options::options_description cmdOptions;			///< Options descriptions for the command line.
	boost::program_options::options_description fileOptions;		///< Options descriptions for the options file.
	boost::program_options::variables_map options;					///< Options data.

	unsigned int getMaxFPS() const { return maxFPS; }

	/// Sets the camera when the window is resized or initialized.
	void resized(unsigned int x, unsigned int y, unsigned int width, unsigned int height);
	void toggleFullscreen();

	//const osg::Vec3f getCameraPos() const { return osg::Vec3f(0, 100, 80); }
	//const osg::Vec3f getCameraDir() const { return osg::Vec3f(0, -1, .05f); }

	const osg::BoundingBox & getMapBounds() const { return mapBounds; }
	void setMapBounds(const osg::BoundingBox & bb) { mapBounds = bb; }
	//osg::BoundingBox getBoidsBounds() const { return mapBounds; }

	virtual ~App();

private:
	void loadGameInit();						///< Minimal initialization for game object.
	void load();
	void unLoad();
	void viewerInit();

	osg::Vec3f pickPos;							///< Position of the mouse cursor (=what it points) in world coordinates.
	//void computePickPos(float x, float y);		///< Computes the position of the mouse cursor (=what it points) in world coordinates.
	osg::NodePath pickNodePath;					///< Node path from the root node to the pick plane.

	osg::ref_ptr<osg::Group> rootNode;			///< Root node of the current scene. Do not change this after realize(), unless you call setSceneData() with the new pointer.

	std::shared_ptr<Physics::World> phyWorld;	///< Physical world.
	std::unique_ptr<Game> game;					///< Current game data, and inserter for 3DPhy's.
	std::unique_ptr<GamePathFinder> pathFinder;
	std::unique_ptr<boost::thread>  pathFinderThread;
	//PVLEPlayer * pAppPlayer;

	std::unique_ptr<Util::FPSLimiter> fpsLimiter;
	unsigned int maxFPS = 0;
	unsigned int maxFPS_DEFAULT = 0;

	unsigned int shadowMapSize = 2048;

	osg::BoundingBox mapBounds;

	// Options
	void initOptions();
	void writeOptions(bool update = true);

	unsigned int screenNum = 0;					///< Number of the screen to display the app on.
	bool fullscreen = true;
	unsigned int windowX = 0, windowY = 0;		///< Size of the client area of the window. The game has (for now) only one viewer (and only one camera)...

	/// Frame types for FPSLimiter.
	struct Thread1FramesTypes {
		enum Values {
			GFX,
			PHYSICS,
			MAX
		};
	};

	virtual void simulationChanged(Simulation * pSimulation) override;

	class PIMPL;
	std::unique_ptr<PIMPL> pimpl;
};

#endif	// APP_H
