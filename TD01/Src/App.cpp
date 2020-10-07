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
/// Main code for the game (main loop, main loader, etc.). Also used as a lab for experiments and quite ugly :) ...

//#define USE_SHADOWS
#include "App.h"

// Adding missing std::make_unique for GCC < 4.9
//#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 9)
#if defined(__GNUC__) && __GNUC__ <= 4
namespace std {
	template<typename T, typename... Args>
	std::unique_ptr<T> make_unique(Args&&... args) { return std::unique_ptr<T>(new T(std::forward<Args>(args)...)); }
}
#endif

#include <osgViewer/Viewer>
#ifdef WIN32
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <osgViewer/api/Win32/GraphicsWindowWin32>
#endif

#include <osg/Group>
#include <osg/CullFace>
#include <osg/LightModel>
#include <osg/MatrixTransform>
#include <osg/ShapeDrawable>
#include <osg/Material>
#include <osg/PolygonMode>
#include <osg/TexEnvCombine>
#include <osg/TexGen>

#include <osgDB/ReadFile>

#include <osgUtil/LineSegmentIntersector>

#include <boost/cast.hpp>
#include <boost/format.hpp>
#include <boost/filesystem/convenience.hpp>

#include <PVLE/Config.h>
#include <PVLE/Util/Util.h>
#include <PVLE/Util/Pool.h>
#include <PVLE/Util/Rand.h>

#include <PVLE/3D/Utility3D.h>

#include <PVLE/Physics/World.h>
#include <PVLE/Physics/Body.h>
#include <PVLE/Physics/Geom.h>
#include <PVLE/Physics/Space.h>
#include <PVLE/Physics/Joint.h>
#include <PVLE/Physics/CommonSurfaceParams.h>
#include <PVLE/Physics/Utility.h>

#include <PVLE/Entity/PhysicsUpdateCB.h>
#include <PVLE/Entity/3DPhy.h>

#include <PVLE/Game/PVLEGame.h>
#include <PVLE/Game/PVLEPlayer.h>

#include <PVLE/Network/HTTP/Utility.h>
#include "Constants.h"

#include <PVLE/Input/ControlMapper.h>
#include <PVLE/Input/SplitManipulator.h>

#include <PVLE/Util/Math.h>
#include <PVLE/Util/FPSLimiter.h>
#include <PVLE/Util/AppOptions.h>


#ifdef PVLE_AUDIO
#	include <osgAudio/SoundRoot.h>
#	include <osgAudio/SoundManager.h>
#	include <osgAudio/SoundNode.h>
#endif

#include "Path.h"
#include "Game.h"
#include "OptionsConstants.h"
#include "CameraController.h"
#include "Constructor.h"
#include "DevHud.h"
#include "Turret.h"
#include "Obstacle.h"
#include "Boid.h"
#include "PathFinder.h"
#include "GamePathFinder.h"
#include "Fonts.h"

#include <boost/thread/thread.hpp>

// These is the map default size parameters WITHOUT SCALING or OFFSET.
// Please use App::mapBounds to get real coordinates.
const float SCENE_DEFAULT_XMIN = -2000;
const float SCENE_DEFAULT_XMAX = -SCENE_DEFAULT_XMIN;
const float SCENE_DEFAULT_YMIN = SCENE_DEFAULT_XMIN;
const float SCENE_DEFAULT_YMAX = SCENE_DEFAULT_XMAX;
const float SCENE_DEFAULT_ZMIN = 0;
const float SCENE_DEFAULT_ZMAX = 500;


// Declaration that should be in AppOptions.h
void setDefaultBindingsBTD(Bindings & k, AxisBindings & a);

// -------------------------------------------------------------------------------------------------

/// Computes weight of a move for the constructor
float btdPathWeight(const osg::Vec3 & offset) {
	float len = offset.length();
	float penalty;			// Penalty applied to climbing/going down
	if (offset.z()>0) {
		penalty = offset.z() * sqrtf(offset.z()) * 5;
	} else {
		const float threshold = -len*.5f;
		if (offset.z() > threshold) penalty = offset.z();
		else penalty = threshold;
	}
	float res = len + penalty;
	//osg::Vec3 flat(offset.x(), offset.y(), 0); flat.normalize();
	//float cosAngle = (offset/len) * flat;
	//TODO tan()? threshold ?	float penalty = (offset.z()>0 ? cosAngle : -cosAngle)*.5f + .5f;
	//if (penalty<.5f) penalty = .5f;		// Clamp
	//float res = len * penalty;

	ASSERT(res>0);		// Else you should double check the penalty
	return res;
}

/// Encapsulates btdPathWeight and gives the ability to be reference counted.
class BTDWeightGetter : public WeightGetter {
public:
	virtual float operator()(const osg::Vec3 & offset) { return btdPathWeight(offset);  }
};

// ---------------------------------------------------------------------------------

#include <PVLE/3D/Commons.h>

/// Creates a random cloud at a random position.
osg::ref_ptr<osg::MatrixTransform> makeCloud(const osg::BoundingBox & mapBounds) {
	// Create a MatrixTransform to position the cloud
	osg::ref_ptr<osg::MatrixTransform> matCloud = new osg::MatrixTransform;
	static const float OVER = 1.05f;		// 1 = XY limits are as the scene, 2 = XY limits are twice as large as the scene, etc...
	matCloud->setMatrix(
		osg::Matrix::rotate(rand(-osg::PI, osg::PI), osg::Vec3(rand(-.1f, .1f), rand(-.1f, .1f), 1)) *
		osg::Matrix::translate(rand(SCENE_DEFAULT_XMIN*OVER, SCENE_DEFAULT_XMAX*OVER), rand(SCENE_DEFAULT_YMIN*OVER, SCENE_DEFAULT_YMAX*OVER), rand(SCENE_DEFAULT_ZMAX*.5f, SCENE_DEFAULT_ZMAX*1.5f))
	);

	// Create and add a cloud geode under the matrix
	auto cloudGeode = createCloud(AppPaths::textures() / "smoke.rgb", rand(75, 300));
	matCloud->addChild(cloudGeode);

	// TODO TD3

	return matCloud;
}


// ---------------------------------------------------------------------------------

#include <osg/GraphicsContext>
#include <osgViewer/Viewer>
//#include <osgText/String>
#include <PVLE/Entity/Hud.h>
#include <PVLE/Input/ControlMapper.h>

class App::PIMPL {
public:
	std::vector<osg::observer_ptr<Hud> > hudList;		///< List of HUD's that may be changed when window is resized.

	osg::GraphicsContext::ScreenSettings screenResolutionInit;		///< Screen resolution when application starts.
	osg::GraphicsContext::ScreenSettings screenResolutionApp;		///< Screen resolution applied when the app goes fullscreen.

	osgViewer::Viewer viewer;

	std::shared_ptr<ControlMapper> controlMapper = std::make_shared<ControlMapper>();

	BoidsUtil * boidsUtil = nullptr;
	HeightFieldData heightFieldData;		// TODO Move this elsewhere!
};

// ---------------------------------------------------------------------------------

App::App() : pimpl(std::make_unique<PIMPL>()) {
	//screenNum= 10;		// TEST/DEBUG X11 forwarding
	AppPaths::ensurePaths();		// Ensure that minimal paths exist

	// Options: The initialization is done early in order to give the main() access to these options.
	initOptions();
}

App::~App()
{
	pimpl->hudList.clear();		// Avoids calling them when the resolution is restored.

	// Back to previous resolution
	//if (screenResolutionInit.width && screenResolutionInit.height)		// Not needed
	{
		osg::GraphicsContext::ScreenIdentifier si(screenNum);
		osg::GraphicsContext::WindowingSystemInterface *wsi = osg::GraphicsContext::getWindowingSystemInterface();
		ASSERT(wsi);
		//if (!wsi) THROW_STR("Error, no WindowSystemInterface available, cannot toggle window fullscreen.");
		wsi->setScreenSettings(si, pimpl->screenResolutionInit);
	}

	//delete boidsUtil;		// DON'T! The boids plugin handles it.
	unLoad();
}

void App::unLoad() {
	LOG_NOTICE << "Unloading" << std::endl;

	// Removes all 3DPhy's contained in the game
	//pimpl->controlMapper->clearHandlers();		// Avoid controls to trigger some actions. Also ensures that if a handler has a shared_ptr to a 3DPhy, the clearing of the 3DPhyOwner will not trigger a warning about a 3DPhy still referenced.
	game = nullptr;			// Do not delete when on client side (the networking system will handle the job)
	rootNode = nullptr;
	pimpl->hudList.clear();
}

#ifdef USE_SHADOWS
#include <osgShadow/ShadowedScene>
#include <osgShadow/ShadowMap>
//#include <osgShadow/ShadowVolume>
//#include <osgShadow/ShadowTexture>
//#include <osgShadow/ParallelSplitShadowMap>

osgShadow::ShadowTechnique * DEBUG_pShadowTechnique = nullptr;
#endif

void App::loadGameInit() {
	LOG_NOTICE << "Loading" << std::endl;

#ifdef PVLE_AUDIO
	// Allows auto-update of the audio listener
	rootNode->addChild(new osgAudio::SoundRoot());
#endif

	// Base
	osg::ref_ptr<osg::Group> pInnerRoot;
	osg::ref_ptr<osg::Group> pGameRoot  = nullptr;
	osg::ref_ptr<osg::LightSource> pLight0 = nullptr;
	osg::ref_ptr<osg::LightSource> pLight1 = nullptr;
#ifdef USE_SHADOWS
	if (shadowMapSize) {
		// --- WARNING ---
		// Shadowing seems to need to be done BEFORE veiwer.realize() in osg 2.2 and 2.4... What to do against that ???
		osg::ref_ptr<osgShadow::ShadowedScene> pShadowedScene = new osgShadow::ShadowedScene();
		pInnerRoot = pShadowedScene.get();
		//pShadowedScene->setReceivesShadowTraversalMask(~0);
		//pShadowedScene->setCastsShadowTraversalMask(~0);
		pShadowedScene->setReceivesShadowTraversalMask(ReceivesShadowTraversalMask);
		pShadowedScene->setCastsShadowTraversalMask(CastsShadowTraversalMask);

		// Light
		pLight0 = new osg::LightSource;
		pShadowedScene->addChild(pLight0);
		//osg::Light * pLightParams0 = new osg::Light();
		osg::Light * pLightParams0 = pLight0->getLight();
		ASSERT(pLightParams0);
		pLightParams0->setAmbient(osg::Vec4f(0.05f,0.05f,0.05f,1));
		pLightParams0->setDiffuse(osg::Vec4f(1,1,1,1));
		pLightParams0->setPosition(osg::Vec4(50,0,200,1));		// Point light needed for shadow map to work ?
		pLightParams0->setSpecular(osg::Vec4());
		//pLightParams0->setPosition(osg::Vec4(-1,-1,-1,0));
		//pLightParams0->setPosition(osg::Vec4(1,1,1,0));
		//pLightParams0->setDirection(osg::Vec3(-1,-1,-1));
		//pLightParams0->setPosition(osg::Vec4f(0.5f,0.25f,0.8f,0.0f));		// Test
		pLightParams0->setLightNum(0);
		//pLight0->setLocalStateSetModes(osg::StateAttribute::OFF);
		//pLight0->setLight(pLightParams0);

		osg::ref_ptr<osgShadow::ShadowMap> pST = new osgShadow::ShadowMap();
			pST->setTextureSize(osg::Vec2s(shadowMapSize, shadowMapSize));
			//pST->setTextureUnit(2);
			pST->setLight(pLight0);
		//osgShadow::ShadowVolume * pST = new osgShadow::ShadowVolume;
		//	pST->setDynamicShadowVolumes(true);
		//	//pST->setDrawMode(osgShadow::ShadowVolumeGeometry::STENCIL_TWO_SIDED);
		//	//pST->setDrawMode(osgShadow::ShadowVolumeGeometry::STENCIL_TWO_PASS);
		//osgShadow::ShadowTexture * pST = new osgShadow::ShadowTexture;
		//	//pST->setTextureUnit(1);
		//osgShadow::ParallelSplitShadowMap * pST = new osgShadow::ParallelSplitShadowMap(nullptr, 3);
		//	//pST->setPolygonOffset(osg::Vec2(-0.02,1.0)); //ATI Radeon
		//	//pSTsetPolygonOffset(osg::Vec2(10.0f,20.0f)); //NVidia

		pShadowedScene->setShadowTechnique(pST.get());
DEBUG_pShadowTechnique = pST.get();

		pGameRoot = new osg::Group;
		pShadowedScene->addChild(pGameRoot);
		//pLight0->addChild(pGameRoot);
		pGameRoot->setNodeMask(CastsShadowTraversalMask | ReceivesShadowTraversalMask);
	} else
#endif	// USE_SHADOWS
	{
		pInnerRoot = new osg::Group;

		// Light
		pLight0 = new osg::LightSource;
		pLight1 = new osg::LightSource;
		pGameRoot = pLight0;
		//pInnerRoot->addChild(pLight0);
		pInnerRoot->addChild(pLight1);
		pLight1->addChild(pLight0);
		osg::ref_ptr<osg::Light> pLightParams0 = new osg::Light();
		pLightParams0->setAmbient(osg::Vec4f(.5f,.5f,.5f,1));
		pLightParams0->setDiffuse(osg::Vec4f(1,1,1,1));
		pLightParams0->setSpecular(osg::Vec4(.7f,.7f,.7f,1));
		pLightParams0->setPosition(osg::Vec4(-1,-.5f,1,0));
		//pLightParams0->setDirection(osg::Vec3(-1,-.5,-.33f));
		pLightParams0->setLightNum(0);
		pLight0->setLocalStateSetModes(osg::StateAttribute::ON);
		pLight0->setLight(pLightParams0.get());

		// General ambient (=not an ambient lighting but a constant added evrywhere)
		//osg::LightModel* lightModel = new osg::LightModel;
		//lightModel->setAmbientIntensity(osg::Vec4(1,1,1,1));
		//pLight0->getOrCreateStateSet()->setAttribute(lightModel);
	}
	rootNode->addChild(pInnerRoot);
	pGameRoot->getOrCreateStateSet()->setAttributeAndModes(new osg::CullFace(osg::CullFace::BACK));		// For children nodes

	// Physical world
	phyWorld = std::make_shared<Physics::World>(osg::Vec3(mapBounds.xMax()-mapBounds.xMin(), mapBounds.yMax()-mapBounds.yMin(), mapBounds.zMax()-mapBounds.zMin()));		// Scene is roughly centered on (0,0,0)

	//Physics::SpaceTypeInfo spaceInfo(Physics::SpaceTypeInfo::SWEEP_AND_PRUNE);
	//spaceInfo.getSweepAndPruneData().axisOrder = Physics::SpaceTypeInfo::SAP_AXES_XYZ;
	//phyWorld = std::make_shared<Physics::World>( std::make_shared<Physics::Space>(spaceInfo) );

	//Physics::SpaceTypeInfo spaceInfo(Physics::SpaceTypeInfo::HASH);
	//phyWorld = std::make_shared<Physics::World>( std::make_shared<Physics::Space>(spaceInfo) );

	//phyWorld = std::make_shared<Physics::World>(osg::Vec3());

	// Create our 3D-phy owner, that will (among others) allow units to fire / create objects.
	// This object holds 3D-phys of the scene.
	// There can be multiple owners, of course: one can insert 3DPhys under a light node, another can insert in root node using another physic world, etc.
	game = std::make_unique<Game>(pGameRoot.get(), phyWorld->getGlobalSpace(), *phyWorld);
	ASSERT(Game::LIGHT_DAY == 0);			game->setLight(Game::LIGHT_DAY, pLight0.get());
	game->addCamera(pimpl->viewer.getCamera());

	//dWorldSetGravity(*phyWorld, 0,0,-9.81f);
	dWorldSetGravity(*phyWorld, 0,0,-50);
}


inline void populateCacheImage(const boost::filesystem::path & path) {
	auto image = osgDB::readRefImageFile(path);
	osgDB::Registry::instance()->addEntryToObjectCache(path.string(), image.get());
}
inline void populateCacheNode(const boost::filesystem::path & path) {
	auto image = osgDB::readRefNodeFile(path);
	osgDB::Registry::instance()->addEntryToObjectCache(path.string(), image.get());
}


#include <osg/TexMat>

// Quick-n-dirty (Used for testing purposes)
void App::load() {
	// First, enable the OSG cache (principally for images/textures)
	osg::ref_ptr<osgDB::ReaderWriter::Options> rwoptions = new osgDB::ReaderWriter::Options;
	rwoptions->setObjectCacheHint(osgDB::ReaderWriter::Options::CACHE_ALL);
	osgDB::Registry::instance()->setOptions(rwoptions.get());

	// Root node initialization
	{
		osg::ref_ptr<osg::LightModel> lightModel = new osg::LightModel;
		lightModel->setTwoSided(false);
		lightModel->setAmbientIntensity(osg::Vec4f(0,0,0,0));

		rootNode->getOrCreateStateSet()->setAttributeAndModes(lightModel.get(), osg::StateAttribute::ON);
		rootNode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::ON);
		rootNode->setCullingActive(true);
	}

	// Sky
	{
		auto pSkySphere = createSkySphere((AppPaths::textures() / "Sky/DemoSky01.png"));
		rootNode->addChild(pSkySphere);

		auto pImage = osgDB::readRefImageFile((AppPaths::textures() / "Sky/Night.png"));
		osg::ref_ptr<osg::Texture2D> pTexture = new osg::Texture2D;
		pTexture->setImage(pImage);
		osg::StateSet * pStateset = pSkySphere->getOrCreateStateSet();
		pStateset->setTextureAttributeAndModes(1, pTexture.get(), osg::StateAttribute::ON);

		osg::TexEnvCombine * pTC = new osg::TexEnvCombine;
		pTC->setCombine_RGB(osg::TexEnvCombine::INTERPOLATE);
		pTC->setSource0_RGB(osg::TexEnvCombine::PREVIOUS);
		pTC->setOperand0_RGB(osg::TexEnvCombine::SRC_COLOR);
		pTC->setSource1_RGB(osg::TexEnvCombine::TEXTURE);
		pTC->setOperand1_RGB(osg::TexEnvCombine::SRC_COLOR);
		pTC->setSource2_RGB(osg::TexEnvCombine::CONSTANT);
		pTC->setConstantColor(osg::Vec4(1,1,1,1));

		pStateset->setTextureAttributeAndModes(1, pTC);
		osg::ref_ptr<osg::TexGen> pTG = new osg::TexGen();
		pTG->setMode(osg::TexGen::SPHERE_MAP);
		pStateset->setTextureAttributeAndModes(1, pTG.get());
		//osg::Matrix::rotate(osg::PI, osg::Z_AXIS)
		//osg::Matrix::translate(.25f,0,0)
		//osg::Matrix::scale(1,-1,1)
		pStateset->setTextureAttributeAndModes(1, new osg::TexMat(osg::Matrix::translate(.25f,-.125f,0)));

		game->setSkyTexEnvCombine(pTC);
	}

#ifdef _DEBUG
	// Axis
	// In order to debug, axis are put under a transform. Just change the transform to move the axis frame.
	{
		osg::ref_ptr<osg::MatrixTransform> pAxisRef = new osg::MatrixTransform(osg::Matrix::translate(0,0,0));
		rootNode->addChild(pAxisRef);
		pAxisRef->addChild(createAxis(100));
	}
#endif

	// Create "floor" object (plane under the height field so that objects cannot fall forever)
	{
		auto pGeomHandler = std::make_shared<Physics::PlaneHandler>(osg::Vec4d(0, 0, 1, -10000));
		auto pGeom = std::make_shared<Physics::Geom>(pGeomHandler, Physics::Materials::GRASS_SOFT);
		phyWorld->getGlobalSpace()->add(pGeom);
	}

	// Map H-Scale

	// Create clouds
	const unsigned nbClouds = options[OPTION_NAME[OPT_MAP_NB_CLOUDS]].as<unsigned int>();
	for(unsigned i=0; i<nbClouds; ++i) {
		game->getParentGroup()->addChild( makeCloud(mapBounds) );
	}

	// Create height-field object
	osg::ref_ptr<osg::Geode> pTerrainGeode;
	osg::ref_ptr<osg::HeightField> pHF;
	{
		// Heightfield data
		const float hScale = mapBounds.xMax()-mapBounds.xMin();
		const float vScale = (mapBounds.zMax()-mapBounds.zMin())/256.f;
		pHF = readHeightMap(AppPaths::maps() / options[OPTION_NAME[OPT_MAP_NAME]].as<std::string>() / "Height.png", vScale, hScale);
		// Move so that the ground is z=0 in its center (using the closest vertex with getVertex())
		auto offset = -pHF->getVertex(pHF->getNumColumns()/2, pHF->getNumRows()/2);
		pHF->setOrigin(pHF->getOrigin() + offset);
		// As offset may have non-zero XY values (if rows/colums number are even), mapBounds may need offsettig too (recalibration).
		mapBounds._min += offset;
		mapBounds._max += offset;

		LOG_INFO << "Initializing path finder" << std::endl;
		pathFinder = std::make_unique<GamePathFinder>(*pHF, std::make_shared<BTDWeightGetter>());

		LOG_INFO << "Starting GamePathFinder thread" << std::endl;
		pathFinderThread = std::make_unique<boost::thread>(std::ref(*pathFinder));

		{
			// ** 3D representation of the terrain **
			osg::ref_ptr<osg::MatrixTransform> pTerrain = new osg::MatrixTransform;

			pTerrainGeode = new osg::Geode();
			pTerrain->addChild(pTerrainGeode);
			pTerrainGeode->addDrawable(new osg::ShapeDrawable(pHF));
			auto pStateSet = create1DTextureHeightColorization(pHF.get());
			pTerrainGeode->setStateSet(pStateSet);

			// Add detail texture
			const float DETAIL_TEXTURE_SCALE = .005f;
			auto image = osgDB::readRefImageFile((AppPaths::maps() / "Demo01/Detail.png"));
			osg::ref_ptr<osg::Texture2D> pDetailTexture = new osg::Texture2D(image.get());
			pDetailTexture->setInternalFormat(GL_RGB);		// Force the image to have 3 or 4 channels so that combination will not be applied only on the red channel
			pDetailTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
			pDetailTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
			pDetailTexture->setFilter(osg::Texture::MIN_FILTER,osg::Texture::LINEAR_MIPMAP_LINEAR);
			pDetailTexture->setFilter(osg::Texture::MAG_FILTER,osg::Texture::LINEAR);
			pStateSet->setTextureAttributeAndModes(1, pDetailTexture.get());

			//osg::TexEnvCombine * pTC = new osg::TexEnvCombine;
			//pTC->setCombine_RGB(osg::TexEnvCombine::MODULATE);
			//pTC->setSource0_RGB(osg::TexEnvCombine::PREVIOUS);
			//pTC->setOperand0_RGB(osg::TexEnvCombine::SRC_COLOR);
			//pTC->setSource1_RGB(osg::TexEnvCombine::TEXTURE1);
			//pTC->setOperand1_RGB(osg::TexEnvCombine::SRC_COLOR);
			//pStateSet->setTextureAttributeAndModes(1, pTC);
			pStateSet->setTextureAttributeAndModes(1, new osg::TexEnv(osg::TexEnv::MODULATE));

			pStateSet->setTextureAttributeAndModes(1, new osg::TexGen);
			pStateSet->setTextureAttributeAndModes(1, new osg::TexMat(osg::Matrix::scale(DETAIL_TEXTURE_SCALE,DETAIL_TEXTURE_SCALE,1)));

			static const bool wireFrameTerrain = false;
			if (wireFrameTerrain) {
				// Set wireframe mode
				osg::ref_ptr<osg::PolygonMode> pPolyMode = new osg::PolygonMode;
				pPolyMode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
				pStateSet->setAttribute(pPolyMode.get());
				pStateSet->setAttributeAndModes(new osg::CullFace(osg::CullFace::FRONT_AND_BACK), osg::StateAttribute::OFF);
			}


			// ** Physics **
			// WARNING! An old (<=2009) ODE issue in heightfields causes long rays intersection very slow. Here we use a MeshHandler instead (which works as expected), as PVLE converts automatically OSG's heightfields to a mesh.
			auto pGeomHandler = std::make_shared<Physics::MeshHandler>(pHF.get());
			//auto pGeomHandler = std::make_shared<Physics::HeightFieldHandler>(pHF);

			//auto pGeom = std::make_shared<Physics::Geom>(pGeomHandler, nullptr, Physics::SurfaceParams(Physics::SurfaceParams::SOFT_ERP | Physics::SurfaceParams::SOFT_CFM, Physics::Materials::CONCRETE.mu, 0, 0, 0, 0.8, 1e-6));
			auto pGeom = std::make_shared<Physics::Geom>(pGeomHandler, Physics::Materials::GRASS_SOFT);
#ifdef USE_SHADOWS
			//if (shadowMapSize) pTerrain->setNodeMask(ReceivesShadowTraversalMask);
			if (shadowMapSize) pTerrain->setNodeMask(0);
#endif
			dGeomSetCategoryBits(*pGeom, Collide::TERRAIN);
			game->addInScene(std::make_shared<C3DPhy>(pTerrain.get(), pGeom));		// Note : no binding between 3D and Physics is needed
		}

		//// TEST Picker
		//pickNodePath.clear();		// Useless, but cleaner
		//pickNodePath.push_back(rootNode.get());
		//pickNodePath.push_back(pTerrain);
	}

	// Pick plane
	{
		//osg::InfinitePlane * pPlane = new osg::InfinitePlane();
		//pPlane->set(0,0,1,0);
		//osg::Node * pPickPlane = createShapeGeode(pPlane);

		osg::ref_ptr<osg::Geode> pPickPlane = new osg::Geode();
		pPickPlane->addDrawable(createSquare(
			osg::Vec3(mapBounds.xMin(), mapBounds.yMin(), 0),
			osg::Vec3(mapBounds.xMax()-mapBounds.xMin(), 0, 0),
			osg::Vec3(0, mapBounds.yMax()-mapBounds.yMin(), 0)
		));
		pPickPlane->getOrCreateStateSet()->setAttributeAndModes(new osg::CullFace(osg::CullFace::FRONT_AND_BACK));
		rootNode->addChild(pPickPlane);

		pickNodePath.clear();		// Useless, but cleaner
		pickNodePath.push_back(rootNode.get());
		pickNodePath.push_back(pPickPlane.get());
	}


	// Empty the cache to save memory. What is after will remain in the cache.
	osgDB::Registry::instance()->clearObjectCache();
	osgDB::Registry::instance()->setExpiryDelay(DBL_MAX);
	//osgDB::Registry::instance()->setExpiryFrames(INT_MAX);

	// CACHE PRELOAD
	// Preloading cached objects
	///\todo Ability to preload directories content (recursively) in a single instruction.
	{
		LOG_INFO << "Populating cache" << std::endl;
		populateCacheImage(AppPaths::textures() / "Explosions/1/Explosion_1.png");
		populateCacheImage(AppPaths::textures() / "Explosions/1/Explosion_2.png");
		populateCacheImage(AppPaths::textures() / "Explosions/1/Explosion_3.png");
		populateCacheImage(AppPaths::textures() / "Explosions/1/Explosion_4.png");
		populateCacheImage(AppPaths::textures() / "Explosions/1/Explosion_5.png");
		populateCacheImage(AppPaths::textures() / "Explosions/1/Explosion_6.png");
		populateCacheImage(AppPaths::textures() / "Explosions/1/Explosion_7.png");
		populateCacheImage(AppPaths::textures() / "Explosions/1/Explosion_8.png");

		populateCacheImage(AppPaths::textures() / "Laser.png");

#ifdef PVLE_AUDIO
		osgAudio::SoundManager::instance()->getSample((AppPaths::sounds() / "Explosions" / "Bomb.wav").string(), true);
		osgAudio::SoundManager::instance()->getSample((AppPaths::sounds() / "30935__aust_paul__possiblelazer.wav").string(), true);
#endif
	}

	auto libName  = BTDWrapperLoader::createLibraryNameForWrapper("boids");
	auto lib      = BTDWrapperLoader::instance().openLibrary(libName);
	if (!lib) lib = BTDWrapperLoader::instance().openLibrary("Bin/" + libName);
	if (!lib) lib = BTDWrapperLoader::instance().openLibrary("bin/" + libName);
	if (!lib) lib = BTDWrapperLoader::instance().openLibrary("Plugins/boids");
	if (!lib) THROW_STR("Could not locate the boids plugin '" + libName + "'. Please ensure the file is in the path.");
	BoidsUtilInstanceFunc f = reinterpret_cast<BoidsUtilInstanceFunc>( lib->getProcAddress("getBoidsUtil") );
	if (!f) THROW_STR("Could not locate getBoidsUtil() function in the boids plugin. Please double check your plugin.");
	pimpl->boidsUtil = &(*f)();
	pimpl->boidsUtil->setBounds(mapBounds._min, mapBounds._max);

	ASSERT(pHF->getFloatArray()->getType() == osg::Array::FloatArrayType);
	auto & heightFieldData = pimpl->heightFieldData;		// local alias
	heightFieldData.heights = reinterpret_cast<const float*>(pHF->getFloatArray()->getDataPointer());
	heightFieldData.columns = pHF->getNumColumns();
	heightFieldData.rows = pHF->getNumRows();
	heightFieldData.dx = pHF->getXInterval();
	heightFieldData.dy = pHF->getYInterval();
	heightFieldData.origin = pHF->getOrigin();

	const unsigned int nbBoids = options[OPTION_NAME[OPT_MAP_NB_BOIDS]].as<unsigned int>();
	for(unsigned int i=0; i<nbBoids; ++i) {
		auto boid = std::make_shared<Boid>(phyWorld.get(), heightFieldData, *pimpl->boidsUtil);
		game->addInScene(boid);
	}
	LOG_NOTICE << "Boids created" << std::endl;

	// Create a team for player's units
	auto playerTeam = std::make_shared<PVLETeam>();
	game->addTeam(playerTeam);

	// Create a test turret
	//auto turret = std::make_shared<BasicTurret>(osg::Vec3(500, 500, 0), *pathFinder, *pimpl->boidsUtil);
	//turret->setTeam(playerTeam);
	//game->addInScene(turret);

	// Create obstacles
	const unsigned int nbObstacles = options[OPTION_NAME[OPT_MAP_NB_OBSTACLES]].as<unsigned int>();
	for(unsigned int i=0; i<nbObstacles; ++i) {
		osg::Vec3 pos;
		// No position too close to the center, since we create the constructor there
		static const float MAX_SIZE = 50;
		for( ; (pos-osg::Vec3()).length2() < MAX_SIZE*MAX_SIZE*1.5f*1.5f; pos = osg::Vec3(rand(mapBounds.xMin(), mapBounds.xMax()), rand(mapBounds.yMin(), mapBounds.yMax()), 0) ) {}
		auto obstacle = std::make_shared<Obstacle>(pos, rand(MAX_SIZE / 2.f, MAX_SIZE), *pathFinder, *pimpl->boidsUtil);
		obstacle->setTeam(playerTeam.get());
		game->addInScene(obstacle);
	}
	LOG_NOTICE << "Obstacles created" << std::endl;

	auto constructor = std::make_shared<Constructor>(phyWorld.get(), *pathFinder, new BTDWeightGetter(), *pimpl->boidsUtil);
	game->addInScene(constructor);
	constructor->setTeam(playerTeam.get());
	auto controlMapper = pimpl->controlMapper;
	controlMapper->addHandlerBack(constructor, ControlMapper::HANDLER_ORDER_UNIT);

	// Remove the "C3DPhy + ControlEventHandler" from ControlMapper when 3DPhy is about to be destroyed.
	//constructor->setDeleteCallback(std::make_shared<ConstructodDeleteCB>(*pimpl->controlMapper));
	//constructor->setDeleteCallback(ConstructodDeleteCB(*pimpl->controlMapper));
	constructor->setDeleteCallback([controlMapper](C3DPhy * p3DPhy) {		// Note: closure captures (copy) a shared_ptr to controlMapper
		controlMapper->removeHandler(boost::polymorphic_cast<ControlEventHandler*>(p3DPhy));
	});


	LOG_NOTICE << "Constructor created" << std::endl;

	// Test for movement: immediately issue a move order
	constructor->moveTo(osg::Vec3(1900, 1600, 0));
}


/// Trivial update callback used to move a node to the pick position (element under cursor), mainly for debug.
class CursorCB : public osg::NodeCallback {
public:
	CursorCB(const ControlMapper & controlMapper) : controlMapper(controlMapper) {}
	virtual void operator()(osg::Node* node, osg::NodeVisitor* nv) {
		boost::polymorphic_downcast<osg::MatrixTransform *>(node)->setMatrix(osg::Matrix::translate(controlMapper.getControlState().getPickPosition()));
	}
protected:
	const ControlMapper & controlMapper;
};



class ResizedCallback : public osg::GraphicsContext::ResizedCallback {
public:
	ResizedCallback(App * pApp) : pApp(pApp) {}
	virtual void resizedImplementation(osg::GraphicsContext *gc, int x, int y, int width, int height) {
		pApp->resized(x, y, width, height);
		gc->resizedImplementation(x, y, width, height);
	}
protected:
	App * pApp;
};


void App::resized(unsigned int /*x*/, unsigned int /*y*/, unsigned int width, unsigned int height) {
	//posX = x;
	//posY = y;
	windowX = width;
	windowY = height;
	ASSERT(windowX>0 && windowY>0);

	float aspectRatio = windowX/static_cast<float>(windowY);
	const float refAspectRatio = 4/3.f;		// Game is intended to have a 4/3 aspect ratio by default
	osg::Camera * pCamera = pimpl->viewer.getCamera();
	// Disable auto ZNear/ZFar (Faster, and so skybox doesn't need to modify CullSettngs on cull traversal, and we do NOT need auto compute !)
	pCamera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
	double fovy=90., basePixelRatio=1., zNear=.05, zFar=10000.;
	double pixelRatio = basePixelRatio;
	if (aspectRatio < refAspectRatio || options[OPTION_NAME[OPT_WIDE_SHRINK]].as<bool>()) {
		// Aspect ratio is < 4/3 or there is an option to horizontally shrink the image for >4/3 resolutions.
		// Note: We also could move the camera backwards instead of horizontally shrinking the output.
		pixelRatio = basePixelRatio;
	} else {
		// Image is not horizontally shrinked. Resolutions >4/3 will then see more of the world on the left and right sides.
		pixelRatio = basePixelRatio * (aspectRatio / refAspectRatio);
	}
	pCamera->setProjectionMatrixAsPerspective(fovy, pixelRatio, zNear, zFar);
	pCamera->setProjectionResizePolicy(osg::Camera::FIXED);

	// Change all existing HUDs. They should be registered in the App class to track them.
	for(auto & pHud : pimpl->hudList) {
		if (pHud.valid()) pHud->onProjectionChanged(0, width, 0, height);
		//else remove the pointer from the list
	}
}


void App::viewerInit() {
	// Note : See WindowSizeHandler in ViewerEventHandlers.cpp for resolution changing/fullscreen toggle, etc.
	auto & screenResolutionInit = pimpl->screenResolutionInit;		// local alias
	auto & screenResolutionApp  = pimpl->screenResolutionApp;		// local alias
	auto & viewer = pimpl->viewer;			// local alias

	// Init the screen
	osg::GraphicsContext::WindowingSystemInterface *wsi = osg::GraphicsContext::getWindowingSystemInterface();
	if (!wsi) THROW_STR("Error, no WindowSystemInterface available.");
	screenNum = osg::clampBetween(screenNum, static_cast<unsigned int>(0), wsi->getNumScreens());
	osg::GraphicsContext::ScreenIdentifier si(screenNum);

	// Stores the initial resolution
	wsi->getScreenSettings(si, screenResolutionInit);

	if (windowX==0 || windowY==0) {
		// Fullscreen with the current desktop resolution
		fullscreen = true;
		wsi->getScreenSettings(si, screenResolutionApp);
		windowX = screenResolutionApp.width;
		windowY = screenResolutionApp.height;
	}
	ASSERT(windowX>0 && windowY>0);

	// Init the window
	viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
	viewer.setUpViewOnSingleScreen(screenNum);	// Avoids putting the world across screens.
	viewer.setSceneData(rootNode.get());		// Pass the loaded scene graph to the viewer.
	viewer.realize();							// Create the windows and run the threads. WARNING ! realize() auto-centers the camera on the whole scene (so be warned if the scene is empty !)
	// As realize() auto-centers the camera on the whole scene (and the loading happens after realize() ), it is needed to set up the manipulator (= the camera position)

	// Disable light attached to the viewer
	viewer.setLightingMode(osg::View::NO_LIGHT);
	osgViewer::Viewer::Windows windows;
	viewer.getWindows(windows);
	ASSERT(!windows.empty());

	unsigned int posX(0), posY(0);
	if (fullscreen) {
		screenResolutionApp = screenResolutionInit;
		screenResolutionApp.width = windowX;
		screenResolutionApp.height = windowY;
		screenResolutionApp = getClosestFullscreenResolution(si, screenResolutionApp);
		windowX = screenResolutionApp.width;
		windowY = screenResolutionApp.height;
		if (!wsi->setScreenSettings(si, screenResolutionApp)) {
			LOG_ERROR << boost::format(_("Could not set fullscreen resulotion to %1%x%2%. Defaulting to windowed mode.")) % windowX % windowY << std::endl;
			fullscreen = false;
		}
	}
	if (!fullscreen) {		// Re-test 'fullscreen' boolean because fullscreen mode may have failed to initialize.
		// Windowed mode
		posX = 100;
		posY = 100;
		wsi->getScreenSettings(si, screenResolutionApp);		// Set app's fullscreen resolution to the desktop
	}

	resized(posX, posY, windowX, windowY);

	std::string title(APP_NAME);
	//viewer.windowName = title;
	//pCamera->setWindowName(title);

	// Init windows
	ASSERT(windows.size() == 1);
	osgViewer::GraphicsWindow * pWin = windows[0];
	pWin->setResizedCallback(new ResizedCallback(this));
	{
		pWin->useCursor(true);
		pWin->setWindowRectangle(posX, posY, windowX, windowY);		// WARNING ! Window size INCLUDES the border ! This is NOT the size of the client area.
		pWin->setWindowDecoration(!fullscreen);			// The client area will be smaller if the decoration is set ! And windowX/windowY represent the CLIENT area.
		pWin->grabFocusIfPointerInWindow();
		pWin->setWindowName(title.c_str());
	}
}


void App::toggleFullscreen() {
	osg::GraphicsContext::WindowingSystemInterface *wsi = osg::GraphicsContext::getWindowingSystemInterface();
	ASSERT(wsi);
	ASSERT(windowX>0 && windowY>0);
	ASSERT(pimpl->screenResolutionInit.width>0 && pimpl->screenResolutionInit.height>0);
	ASSERT(pimpl->screenResolutionApp.width>0 && pimpl->screenResolutionApp.height>0);
	osg::GraphicsContext::ScreenIdentifier si(screenNum);
	osg::GraphicsContext::ScreenSettings newRes;

	unsigned int posX, posY;
	fullscreen = !fullscreen;
	if (fullscreen) {
		newRes = pimpl->screenResolutionApp;
		posX = 0;
		posY = 0;
		windowX = newRes.width;
		windowY = newRes.height;
	} else {
		newRes = pimpl->screenResolutionInit;
		posX = 100;
		posY = 100;
		windowX = newRes.width/2;
		windowY = newRes.height/2;
	}
	ASSERT(windowX>0 && windowY>0);

	osgViewer::Viewer::Windows windows;
	pimpl->viewer.getWindows(windows);
	ASSERT(windows.size()==1);
	osgViewer::GraphicsWindow * pWin = windows[0];
	{
		pWin->setWindowRectangle(posX, posY, windowX, windowY);		// WARNING ! Window size INCLUDES the border ! This is NOT the size of the client area.
		pWin->setWindowDecoration(!fullscreen);			// The client area will be smaller if the decoration is set ! And windowX/windowY represent the CLIENT area.
	}

	// Set screen AFTER the window or else it will do crap.
#ifdef _DEBUG
	bool resolutionChanged = 
#endif
	wsi->setScreenSettings(si, newRes);
	ASSERT(resolutionChanged);
}


#include "UserSwitches.h"

void App::run()
{
#if defined(_DEBUG) && defined(PVLE_UNIT_TESTS)
	Util::UnitTestSingleton::testPart1();
#endif

#ifdef USE_SHADOWS
	if (shadowMapSize) osg::DisplaySettings::instance()->setMinimumNumStencilBits(8);
#endif

#ifdef PVLE_AUDIO
	osgAudio::SoundManager * pSoundManager = osgAudio::SoundManager::instance();
#ifdef _DEBUG
	pSoundManager->init(16);		// Allow to detect "no more ressources" condition earlier
#else
	pSoundManager->init(64);
#endif
	// Distance model
	// InverseDistance: Gain = ReferenceDistance / (ReferenceDistance + RolloffFactor * (Distance – ReferenceDistance))
	// Or: Gain = 1 / (1 + RolloffFactor * (Distance/ReferenceDistance – 1))
	// InverseDistanceClamped: If distance is below the ReferenceDistance, gain is clamped
	//pSoundManager->getEnvironment()->setDistanceModel(osgAudio::InverseDistance);
	pSoundManager->getEnvironment()->setDistanceModel(osgAudio::InverseDistanceClamped);
	//pSoundManager->getEnvironment()->setDistanceModel(oshAudio::None);
	//pSoundManager->getEnvironment()->setDopplerFactor(1);
	//pSoundManager->getEnvironment()->setSoundVelocity(.340f);

	// Manual update of the audio listener. Unnecessary with a SoundRoot.
	//pSoundManager->getListener()->setPosition(getCameraPos().x(), getCameraPos().y(), getCameraPos().z());
	//pSoundManager->getListener()->setOrientation(getCameraDir().x(), getCameraDir().y(), getCameraDir().z(), 0, 0, 1);
#endif


	// ----- App's local constants & variables -----
	// Physics parameters
	static const unsigned int BASE_SIMULATION_FREQ = 60;		// 60 Hz physics simulation
	simulation.setBasePhyStep(1./BASE_SIMULATION_FREQ);
	const double MAX_DELTA_S_PHYSICS = 1.;

	fpsLimiter = std::make_unique<Util::FPSLimiter>(Thread1FramesTypes::MAX);
	fpsLimiter->setMaxFPS(maxFPS, Thread1FramesTypes::GFX);
	fpsLimiter->setMinDelay(simulation.getCurPhyStep(), Thread1FramesTypes::PHYSICS);
	simulation.addOrSetCallback(shared_from_this());

	// Network parameters
	//static const unsigned int netFreq = 50;										// Network maximum frequency (Hz)
	//static const unsigned int netFreqDedicated = 500;							// Network maximum frequency (Hz) for a dedicated server
	//static const double netStepSize = 1. / netFreq;						// == netPeriod
	//static const double netStepSizeDedicated = 1. / netFreqDedicated;	// == netPeriod for a dedicated server

	maxFPS_DEFAULT = std::min(static_cast<unsigned int>(60), BASE_SIMULATION_FREQ) * 1.5f;	// *1.5 because FPSLimiter is not yet complete.
	maxFPS = maxFPS_DEFAULT;

	// Root node for all app
	rootNode = new osg::Group();
	if (!rootNode) THROW_STR("rootNode : initial allocation failed");

	// Load all fonts
	{
		boost::filesystem::path fontPath = AppPaths::data() / "default.ttf";
		//if (!boost::filesystem::exists(fontPath)) THROW_STR("Missing data : Font file not found");
		Fonts::instance().pStandard = osgText::readFontFile(fontPath.string());
		if (!Fonts::instance().pStandard) THROW_STR("Could not load font file");
	}

	float hScale = options[OPTION_NAME[OPT_MAP_XY]].as<float>();
	float vScale = options[OPTION_NAME[OPT_MAP_Z]].as<float>();
	mapBounds.set(
		SCENE_DEFAULT_XMIN*hScale, SCENE_DEFAULT_YMIN*hScale, SCENE_DEFAULT_ZMIN*vScale,
		SCENE_DEFAULT_XMAX*hScale, SCENE_DEFAULT_YMAX*hScale, SCENE_DEFAULT_ZMAX*vScale);

	// ----- Init the view -----
	if (!shadowMapSize) viewerInit();


	// ----- Setup loading -----

	// Init game
	loadGameInit();
	ASSERT(game);
	PVLEGameHolder::instance().setCurrentGame(game.get());	// Sets the current game for PVLE, so that it can be reached if needed.
	SimulationHolder::instance().set(&simulation);			// Sets the current simulation parameters, so that it can be reached if needed.
	//game->setFriendlyFire(1.f);
	game->setSoundGainFX(options[OPTION_NAME[OPT_SOUND_GAIN_FX]].as<float>());
	game->setSoundGainMusic(options[OPTION_NAME[OPT_SOUND_GAIN_MUSIC]].as<float>());

#ifdef PVLE_AUDIO
/*	// Set intro/loading music
	if (game->getSoundGainMusic() > 0) {
		openalpp::FileStream * pFstream = new openalpp::FileStream((AppPaths::sounds() / "Music/Ambient01.ogg").string());		// TODO RAII
		//osg::ref_ptr<openalpp::Source> pSource = new openalpp::Source(pFstream.get());
		//openalpp::Sample *pSample = new openalpp::Sample((AppPaths::sounds() / "Music/OriginalIntro.wav").string());
		osgAudio::SoundState * pSoundState = new osgAudio::SoundState();
		//pSoundState->setSample(pSample);
		pSoundState->setStream(pFstream);
		pSoundState->setPlay(true);
		pSoundState->setLooping(true);
		pSoundState->setGain(game->getSoundGainMusic());
		pSoundState->setAmbient(true);
		pSoundState->allocateSource(20, true);
		pSoundState->apply();
		osgAudio::SoundManager::instance()->addSoundState(pSoundState);

		// Add the sound state to a node so that we can leave the code block without object deleted (or memory leak)
		osgAudio::SoundNode * pSound = new osgAudio::SoundNode;
		pSound->setSoundState(pSoundState);
		rootNode->addChild(pSound);
	}
*/
#endif

	// ----- Load scene -----
	load();

	// ----- Inputs -----
	SplitManipulator * pSplitManipulator = nullptr;	// Owned by the viewer
	Bindings bindings;								// Do not put in an inner block since references on this are kept
	AxisBindings axisBindings;						// Same as 'bindings'

	//ASSERT(pAppPlayer);			// The player must exist to init inputs

	// ----- Load parameters -----
	//osgIntrospection::Reflector<osgGA::GUIEventAdapter::Key_Symbol> keyReflector();

	// ** Key, buttons and axis bindings **
	// Load or set default
	{
		bool setDefault;
		boost::filesystem::path bindingsPath(AppPaths::profiles() / "KeyBindings.txt");
		if (boost::filesystem::exists(bindingsPath)) setDefault = !loadBindings(bindings, axisBindings, bindingsPath, UserSwitches::switchesNames, UserSwitches::MAX_USER_SWITCH-ControlState::USER_SWITCH_0, UserSwitches::BINDINGS_FILE_VERSION);
		else setDefault = true;
		if (setDefault) setDefaultBindingsBTD(bindings, axisBindings);

		// Ensure no system controls are overriden
		bindings[osgGA::GUIEventAdapter::KEY_Escape] = ControlState::MENU;

		// Save bindings
		//saveBindings(bindings, axisBindings, bindingsPath, UserSwitches::switchesNames, UserSwitches::MAX_USER_SWITCH-ControlState::USER_SWITCH_0, UserSwitches::BINDINGS_FILE_VERSION);
	}

	// ----- Initialization -----
	auto & controlMapper = *pimpl->controlMapper;		// local alias
	auto & viewer = pimpl->viewer;		// local alias

	// Basic HUD
	osg::ref_ptr<DevHud> pDevHud = new DevHud(0, windowX, 0, windowY, *this);
	rootNode->addChild(pDevHud);
	pimpl->hudList.push_back(pDevHud.get());

	// Menu (holds a control event handler)
	//osg::ref_ptr<MenuNode> pMenu = new MenuNode(this, 0, windowX, 0, windowY);
	//rootNode->addChild(pMenu);
	//addHud(pMenu.get());

	//game->setGameCallback(pMenu);


	// Initialize the player's control mapper
	controlMapper.setBindings(&bindings, &axisBindings);				// Init bindings

	// Add handler(s) to the player's control mapper if necessary (here, the menu)
	auto cameraController = std::make_shared<CameraController>(mapBounds);
	controlMapper.addHandlerBack(cameraController, ControlMapper::HANDLER_ORDER_UNIT);

// Cursor test
//osg::MatrixTransform * cursor = new osg::MatrixTransform;
//rootNode->addChild(cursor);
//osg::Geode * geode = new osg::Geode;
//cursor->addChild(geode);
//geode->addDrawable(new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(), 20)));
//cursor->addUpdateCallback(new CursorCB(controlMapper));

	controlMapper.addHandlerBack(std::make_shared<DevHudCEV>(*this), ControlMapper::HANDLER_ORDER_MENU);			// Handler "in front of" any other control handling (with a higher priority = a lower order index)
	//controlMapper.addHandlerBack(pMenu, ControlMapper::HANDLER_ORDER_MENU);
	//controlMapper.addHandlerBack(pCannon, ControlMapper::HANDLER_ORDER_UNIT);
	controlMapper.enablePointerMouse();		// Say that the mouse is not a game axis like a joystick

	// Create the manipulator
	{
		//pSplitManipulator = new SplitManipulator(new FixedMatrixGetter(getCameraPos(), getCameraDir(), osg::Z_AXIS), &controlMapper);
		pSplitManipulator = new SplitManipulator(cameraController.get(), &controlMapper);

		//pSplitManipulator = new SplitManipulator(new FixedMatrixGetter(osg::Vec3f(0,0,150), osg::Vec3f(0,0,-1), osg::Vec3f(0,-1,0)), &controlMapper);
		//pSplitManipulator = new SplitManipulator(new FixedMatrixGetter(osg::Vec3f(300,300,200), osg::Vec3f(-1,-1,-1), osg::Z_AXIS), &controlMapper);
		viewer.setCameraManipulator(pSplitManipulator);										// Main evant handler (split manipultaor)
	}

	if (shadowMapSize) viewerInit();


	// Memory stress test
	if (options[OPTION_NAME[OPT_STRESS_TEST]].as<bool>()) {
		LOG_ALWAYS << "Memory stress test for boids started" << std::endl;
		unsigned int STRESS_TEST_PASSES = 30000;
		for(unsigned int i=0; i<STRESS_TEST_PASSES; ++i) {
			auto stressBoid = std::make_shared<Boid>(phyWorld.get(), pimpl->heightFieldData, *pimpl->boidsUtil);
			game->addInScene(stressBoid);
			game->markAsRemoved(stressBoid);
			if (i%(STRESS_TEST_PASSES/10) == 0) {
				game->doDelayed3DPhy();
				game->doDeleteGfx();
				game->doDeletePhy();
				LOG_ALWAYS << "   " << i*100.f/STRESS_TEST_PASSES << "%..." << std::endl;
			}
		}
		game->doDelayed3DPhy();
		game->doDeleteGfx();
		game->doDeletePhy();
		LOG_ALWAYS << "Memory stress test for boids ended" << std::endl;
	}

	// ----- Run -----
	LOG_NOTICE << "Starting main loop" << std::endl;

	// Set the viewer to have the right scene (not the loading screen).
	viewer.setSceneData(rootNode.get());
	viewer.getCamera()->setClearMask(GL_DEPTH_BUFFER_BIT);		// Set the clear mask / clear color as appropriate (Deactivate the clearing of the color buffer because a skybox is present)

	double phyLastTime = 0, phyCurTime = 0;
	double phyLastTimeLocal = 0;			// Time in the physics simulation (slowed down when simulation is)
	//double gfxLastTime = viewer.getFrameStamp()->getReferenceTime(), gfxCurTime;		// Use getSimulationTime() ?
	//double netLastTime = 0, netCurTime = 0;
	const float reRandomizeMin = 5;
	const float reRandomizeMax = 120;
	float reRandomize = rand(reRandomizeMin, reRandomizeMax);

	int FrameCount = 0;
	osg::Timer_t TimerStart = osg::Timer::instance()->tick();

	unsigned int steps;			// Number of steps for the physics
	unsigned int curStep;		// Loop variable

	while( !viewer.done() ) {
		// --- GFX ---
		game->doDelayed3DPhy();		// Delete 3D-Phys that have been queued for deletion (does not delete geom or model, but enqueues them for deletion)
		game->doDeleteGfx();		// Delete 3D models that have been queued for deletion

		// If the piloted unit has been destroyed/changed during physics simulation, we need to update the matrix getter of the manipulator.
		// This is why the following instructions are inside the loop.
		//pSplitManipulator->setMatrixGetter(pAppPlayer->getMatrixGetter());													// Set the matrix getter

		(*fpsLimiter)(osg::Timer::instance()->delta_s(TimerStart, osg::Timer::instance()->tick()));

		if (fpsLimiter->frameNeeded(osg::Timer::instance()->delta_s(TimerStart, osg::Timer::instance()->tick()), Thread1FramesTypes::GFX)) {
			//game->getLightSourceManager().checkForAvailableLights();
			//viewer.advance(phyLastTimeLocal);
			viewer.frame(phyLastTimeLocal);
			++FrameCount;
		}

		controlMapper.computePickPos(viewer, &pickNodePath);		// Use the "pick plane" for an approximate 3D picking

		// --- Physics ---
		phyCurTime = osg::Timer::instance()->delta_s(TimerStart, osg::Timer::instance()->tick());	// Doesn't use viewer.getFrameStamp() because the code will move to an other thread

		if (reRandomize<0) {
			auto pick2d(controlMapper.getControlState().getPointer());
			unsigned int mouseRandomSource = static_cast<unsigned int>(pick2d.x()) % 17 + static_cast<unsigned int>(pick2d.y()) % 11;		// Use prime numbers on modulus
			reRandomize = rand(reRandomizeMin, reRandomizeMax) + mouseRandomSource / 97.f;						// Also a prime number... No need, but I like it that way !
			srand();
		} else reRandomize -= (phyCurTime - phyLastTime);

		if (simulation.isPaused()) {
			// Special case - Paused
			phyLastTime = phyCurTime;
			fpsLimiter->frame(osg::Timer::instance()->delta_s(TimerStart, osg::Timer::instance()->tick()), Thread1FramesTypes::PHYSICS);
		}
		else
		{
			// Not paused
			double curPhyStep = simulation.getCurPhyStep();		// Constant over all game ONLY IF phySpeedStep does not change

			if (phyCurTime - phyLastTime < MAX_DELTA_S_PHYSICS) {							// WARNING : The test is NOT "elapsed * phySpeedNum" but "elapsed" because it's a test based on real time.
				ASSERT(phyLastTime <= phyCurTime);

				if (fpsLimiter->testFrameNeeded(osg::Timer::instance()->delta_s(TimerStart, osg::Timer::instance()->tick()), Thread1FramesTypes::PHYSICS)) {
					double curPhyNbStepsPerSec = simulation.getCurPhyNbStepsPerSec();		// Steps number in non-simulation mode
					steps = static_cast<unsigned int>((phyCurTime-phyLastTime) * curPhyNbStepsPerSec);
					for(curStep=0; curStep<steps; ++curStep) {
						fpsLimiter->frame(osg::Timer::instance()->delta_s(TimerStart, osg::Timer::instance()->tick()), Thread1FramesTypes::PHYSICS);
						game->doDelayed3DPhy();		// Delete 3D-Phys that have been queued for deletion/addition (does not delete geom or model, but enqueues them for deletion)
						game->doDeletePhy();			// Delete geoms that have been queued for deletion

						// "Close contact check" cleanup : Remove contactsPos at the begining of the turn
						game->clearBodyContactPos();

						//if (pTriMeshTestHandler) pTriMeshTestHandler->meshStep();	// Tell ODE the way trimeshes have moved
						game->step(curPhyStep);		// Step all 3D-Phys (Must be done for every 3DPhyOwner the app has)
						phyWorld->step(curPhyStep);	// Step the world

						// Clear events and axis values, as 3D-Phys should have consumed them
						//pAppPlayer->getControlMapper().clear();
// TODO Extremely ugly! Don't remember where to put the call when the ControlHandler isn't in the game object :S
cameraController->handleFrame(curPhyStep);
						controlMapper.clear();
					}
					//if (steps==0) fpsLimiter->frame(osg::Timer::instance()->delta_s(TimerStart, osg::Timer::instance()->tick()), FPS_LIMITER_THREAD1_PHYSICS_FRAME);

					ASSERT(curPhyNbStepsPerSec>0);
					phyLastTime += steps / curPhyNbStepsPerSec;		// not "phyLastTime = phyCurTime" because steps is a rounded number and we want to keep track of real-time.
					phyLastTimeLocal += steps * curPhyStep;
					if (phyLastTime > phyCurTime) phyLastTime = phyCurTime;
				}
			} else {
				// Do not compute physics if there is a big slowdown on the computer (and not in simulation mode).
				phyLastTime = phyCurTime;
			}
		}

		// Clear events and axis values if physics did not.
		controlMapper.clear();

		if (game->getState() == Game::STATE_GAME_OVER) {
			pDevHud->setSubText("GAME OVER!");
		}

		if (game->getState() == Game::STATE_STOP) {
			game->doDelayed3DPhy();			// In order to compute all remaining points before starting a new game
			game->startGame();
			pDevHud->setSubText("");
		}
	}

	//game->stopGame();		// To avoid creation of things (such as explosions)
	//pathFinder->pause();

	writeOptions();

#ifdef PVLE_AUDIO
	osgAudio::SoundManager::instance()->shutdown();
#endif

	double TmpTime = osg::Timer::instance()->delta_s(TimerStart, osg::Timer::instance()->tick());
	LOG_NOTICE << boost::format("Time elapsed (sec) : %.1f") % TmpTime << std::endl;
	LOG_NOTICE << boost::format("Frames rendered    : %.1f") % FrameCount << std::endl;
	LOG_NOTICE << boost::format("Average FPS        : %.1f") % (TmpTime>0 ? FrameCount/TmpTime : 0) << std::endl;

	pathFinder->softStop();
	unLoad();
	pathFinderThread->join();
}


void App::simulationChanged(Simulation * pSimulation) {
	ASSERT(pSimulation == &simulation);
	ASSERT(fpsLimiter);

	if (pSimulation->isPaused()) {
		// Fewer FPS in pause
		maxFPS = maxFPS_DEFAULT / 5.f;
		fpsLimiter->setMinDelay(0, Thread1FramesTypes::PHYSICS);
	} else {
		// Normal mode
		maxFPS = maxFPS_DEFAULT;
		fpsLimiter->setMinDelay(pSimulation->getCurPhyStep(), Thread1FramesTypes::PHYSICS);
	}
	fpsLimiter->setMaxFPS(maxFPS, Thread1FramesTypes::GFX);

	if (_nestedCallback) _nestedCallback->simulationChanged(pSimulation);
}
