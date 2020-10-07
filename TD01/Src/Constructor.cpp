//-----------------------------------------------------------------------------------
//
// Boids Tower Defense
// Copyright (C) 2007-2008  Sukender
// For more information, contact us : sukender@free.fr
//
// All rights reserved
//
//-----------------------------------------------------------------------------------

#include "Constructor.h"
#include "UserSwitches.h"
#include "Turret.h"

// For explosions
#include "Common.h"
#include "Game.h"
#include "Path.h"

#include <PVLE/Physics/Utility.h>
#include <PVLE/3D/Commons.h>

#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <osg/Material>
#include <osg/CullFace>
#include <osg/Geode>

#include <boost/scope_exit.hpp>

static const UnpilotedUnitClass constructorClass(0);		// Build time
static const osg::Vec3 size(30, 30, 60);

void Constructor::LocalPathFinderModifyCB::operator()(GamePathFinder & gpf) {
	boost::lock_guard<boost::mutex> lock(c.getPFMutex());
	c.cancelMove();
	c.pfCommandLaunched = false;
}


Constructor::Constructor(Physics::World * phyWorld, GamePathFinder & pathFinder, WeightGetter * wg, BoidsUtil & boidsUtil, const C3DPhy * pInheritTeamAndPlayer) :
	Unit(phyWorld, pInheritTeamAndPlayer),
	pathFinder(pathFinder), wg(wg),
	geom(nullptr),
	boidsUtil(boidsUtil), boidTarget(nullptr),
	curPointInPath(0), pointProgress(0),
	state(STOPPED), pfCommandLaunched(false),
	drawnPath(nullptr)
{
	_class = &constructorClass;

	const osg::Vec3 pos(0,0,0);
	createModel(size);

	setMaxLifeForce(100);
	setLifeForce(getMaxLife());

	geom = Physics::createCanonicalBoxGeom(size);

	//geom = Physics::createCanonicalBoxGeom(size, Physics::SurfaceParams(Physics::SurfaceParams::SOFT_ERP, 5000, 0, 0, .02f) );
	dGeomSetPosition(*geom, pos);
	dGeomSetCategoryBits(*geom, Collide::UNIT);
	dGeomSetCollideBits (*geom, Collide::BOID);
	addGeom(geom);
	bind();

	// Setup target for boids simulation
	boidTarget = boidsUtil.newTarget(pos, 1, 500);

	pfCommand = std::make_shared<PathFinderCommandSimplePath>();
	pfModifyCB = std::make_shared<LocalPathFinderModifyCB>(*this);

	boost::lock_guard<boost::mutex> lock(pfMutex);
	pathFinder.addModifiedCallback(pfModifyCB);
}

Constructor::~Constructor() {
	boidsUtil.deleteTarget(boidTarget);
	boidTarget = nullptr;

	boost::lock_guard<boost::mutex> lock(pfMutex);
	pathFinder.removeModifiedCallback(pfModifyCB.get());

	Game * game = PVLEGameHolder::instance().get<Game>();
	// CACHE PRELOAD
	if (game->getState() != Game::STATE_STOP) {
		game->setGameOver();
		game->addInSceneDelayed(createStdExplosion(dGeomGetPositionV(*vGeoms[0]) + osg::Z_AXIS*(size.z() / 4), AppPaths::sounds() / "Explosions" / "Bomb.wav", 500, 1000, 7000, 2));
	}
}


void Constructor::moveTo(const osg::Vec3 & newPos) {
	if (isBuilding()) return;		// TODO: make the constructor issue a new order which it will follow after the construction
	cancelMove();
	pfCommand->reset(PathFinderCommand::PRIORITY_NEW_PATH, getPos(), newPos);
	pathFinder.push(pfCommand);
	pfCommandLaunched = true;
}

void Constructor::cancelMove() {
	if (pfCommandLaunched && !pfCommand->isDone()) {
		pathFinder.pop(pfCommand.get());
	}
	if (state == MOVING) state = STOPPING;
	pfCommandLaunched = false;
}

void Constructor::doStop() {
	state = STOPPED;
	removeDrawnPath();
}

void Constructor::cancelBuild() {
	if (isBuilding()) {
		state = STOPPED;
		std::shared_ptr<UnpilotedUnit> currentlyBuildingRef = currentlyBuilding.lock();
		if (currentlyBuildingRef) {
			p3DPhyOwner->markAsRemoved(currentlyBuildingRef);
			currentlyBuilding.reset();
		}
	}
}

//void Constructor::buildAt(const osg::Vec3 & buildingPos) {
//	// TODO
//}

void Constructor::build(EBuildOrder buildOrder) {
	if (isBuilding()) return;

	// TODO Buildings must not overlap (=must test in pathfinder if there is enough room for placing it)
	cancelMove();
	doStop();		// Force stop, even if not on a graph edge (we'll move the unit after)

	// Launch an order for building (construction is not instant)
	ASSERT(buildOrder == BUILD_TURRET_BASIC);
	doBuildTurret(buildOrder);
	ASSERT(isBuilding());
}

void Constructor::doBuildTurret(EBuildOrder buildOrder) {
	ASSERT(buildOrder == BUILD_TURRET_BASIC);

	// Set state
	state = static_cast<EState>(BUILDING_FIRST + buildOrder);

	osg::Vec3 curPos(getPos());

	auto turret = std::make_shared<BasicTurret>(curPos, pathFinder, boidsUtil, this);
	p3DPhyOwner->addInScene(turret);
	currentlyBuilding = turret;

	// Move the constructor to the closest graph point that is connected to others
	osg::Vec3 newPos( turret->getOutPos() );
	setPos(newPos);

	// Update boid target
	boidTarget->setPos(newPos);
	boidsUtil.targetUpdated(boidTarget);
}

void Constructor::removeDrawnPath() {
	if (!drawnPath) return;
	ASSERT(drawnPath->getNumParents() == 1 && drawnPath->getParent(0) == pModel);
	pModel->removeChild(drawnPath);
	drawnPath = nullptr;
}

bool Constructor::handleEvent(NetControlEvent * pEvent, TNL::EventConnection * pConnection) {
	// No need to test if pControler == nullptr because this method isn't called in that case.

	if (pEvent->getType() == ControlEvent::SWITCH_DOWN) {
		if (pEvent->getSwitchId() == UserSwitches::ORDER_MOVE) {
			const ControlState & cs = pControler->getControlState();
			moveTo(cs.getPickPosition());
			return true;
		}
		if (pEvent->getSwitchId() == UserSwitches::ORDER_STOP) {
			cancelMove();
			cancelBuild();
			return true;
		}
	}
	return false;
}



void Constructor::handleFrame(double elapsed) {
}

void Constructor::step(dReal elapsed) {
	UnpilotedUnit::step(elapsed);		// Should be called at the end, but there are plenty of returns, and BOOST_SCOPE_EXIT can't work with parent call.
	if (testHasDied()) return;

	// Handle user continuous inputs
	//ASSERT(pControler);
	//const ControlState & cs = pControler->getControlState();

	// First process buildings
	if (isBuilding()) {
		// currentlyBuilding may be nullptr if building has been destroyed
		std::shared_ptr<UnpilotedUnit> currentlyBuildingRef = currentlyBuilding.lock();
		if (currentlyBuildingRef) {
			if (!currentlyBuildingRef->isBuilt()) return;
		}
		// Done building
		currentlyBuilding.reset();
		state = STOPPED;
	}

	// Follow path
	if (state == STOPPED && !pfCommandLaunched) return;
	boost::lock_guard<boost::mutex> lock(pfMutex);

	static const float BASE_SPEED = 100;
	// Gets the result if needed
	if (pfCommandLaunched && pfCommand->isDone()) {
		if (pfCommand->getResult().empty()) {
			LOG_NOTICE << "Constructor could not find a path to destination" << std::endl;
			if (state == STOPPING) { state = STOPPED; return; }
			if (state == STOPPED) return;
		}
		else {
			// New path given
			pathPoints.swap( pfCommand->getResult() );
			state = MOVING;

			// TODO remove points at the begining of the path a point is closer (in terms of cost) than the 1st one.
			//	This may happen if the path finder thread took a long time to answer, or if "nearest" point is to the opposite direction.

			removeDrawnPath();
			drawnPath = new osg::MatrixTransform();
			//drawnPath->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
			drawnPath->addChild(drawPath(pathPoints));
			pModel->addChild(drawnPath);

			curPointInPath = 0;
			pointProgress = 0;
			pfCommandLaunched = false;
			zeroPoint = getPos();
		}
	}

	if (state == STOPPED) return;

	//ASSERT(!pathPoints.empty());
	if (pathPoints.empty() || curPointInPath >= pathPoints.size()) {
		cancelMove();
		return;
	}

	osg::Vec3 pos;
	for(; elapsed>0; ) {
		const osg::Vec3 & prevPoint = curPointInPath==0 ? zeroPoint : pathPoints[curPointInPath-1];
		const osg::Vec3 & curPoint  = pathPoints[curPointInPath];
		osg::Vec3 offset(curPoint-prevPoint);

		// Here we gould get the weight (=graph distance) from the PathFinderCommandSimplePath (if it were recording it), but the choice is to recompute it to keep more memory
		float weight = offset==osg::Vec3() ? 0 : (*wg)(offset);

		// Math notes:
		// speed = BASE_SPEED * offset.length() / weight;
		// moveDist = speed * elapsed;
		// movePointProgress = moveDist/offset.length();
		//
		// pointProgressSpeed = movePointProgress/elapsed;
		// pointProgressSpeed = moveDist/offset.length()/elapsed;
		// pointProgressSpeed = speed * elapsed/offset.length()/elapsed;
		// pointProgressSpeed = speed/offset.length();
		// pointProgressSpeed = BASE_SPEED / weight;

		const float prevPointProgress = pointProgress;
		if (weight <=0) pointProgress = 1;		// Set to next point
		else pointProgress += BASE_SPEED / weight * elapsed;

		// "Consume" elapsed time
		if (pointProgress <1) elapsed = 0;
		else {
			pointProgress -= 1;
			elapsed -= (1 - prevPointProgress) / BASE_SPEED * weight;
			ASSERT(elapsed >=0);
			++curPointInPath;
			if (state == STOPPING || curPointInPath >= pathPoints.size()) {
				pos = curPoint;		// Set to final position
				elapsed = 0;
				doStop();
				// Don't return, simply set the state == STOPPED
			}
		}

		if (state != STOPPED && elapsed<=0) pos = prevPoint + offset * pointProgress;		// Only needed when elapsed==0 but we put it in the loop in order to avoid copying prevPoint/curPoint :)
	}

	dGeomSetPosition(*geom, pos);
	//osg::Vec3 oldPos = dGeomGetPositionV(*geom);
	//static const float DELTA_POS_MIN2 = 5*5;
	//static const float DELTA_POS_MAX = 20;
	//osg::Vec3 offsetPos(pos-oldPos);
	//float offsetPosLen2 = offsetPos.length2();
	//if (offsetPosLen2 <= DELTA_POS_MIN2)
	//	dGeomSetPosition(*geom, pos);
	//else {
	//	float offsetPosLen = sqrtf(offsetPosLen2);
	//	//if (offsetPosLen > DELTA_POS_MAX)
	//	static const float MAX_SPEED = BASE_SPEED * 1.5f;
	//	dGeomSetPosition(*geom, oldPos + offsetPos * osg::clampBelow(elapsed * MAX_SPEED / offsetPosLen, 1.f) );
	//}

	// Update boid target
	boidTarget->setPos(pos);
	boidsUtil.targetUpdated(boidTarget);

	if (drawnPath) drawnPath->setMatrix(osg::Matrix::translate(-pos));		// Moved the path to the opposite so that it seems to be gloabal. A bit ugly but works :)
}


void Constructor::createModel(const osg::Vec3 & size_) {
	setModel( createShapeGeodeBox(size_) );

	osg::ref_ptr<osg::Material> material = new osg::Material();
	pModel->getOrCreateStateSet()->setAttributeAndModes(material.get());
	pModel->getOrCreateStateSet()->setAttributeAndModes(new osg::CullFace());
	updateColor(1);
}

void Constructor::updateColor(float lifePercent) {
	osg::StateSet * stateset = pModel->getOrCreateStateSet();
	osg::Material * material = boost::polymorphic_downcast<osg::Material *>( stateset->getAttribute(osg::StateAttribute::MATERIAL) );

	osg::Vec4 color(lifePercent, 0, 0, 1);
	material->setDiffuse(osg::Material::FRONT, color);
	osg::Vec4 ambient(color*.5f);
	ambient.w() = 1;
	material->setAmbient(osg::Material::FRONT, ambient);
	material->setSpecular(osg::Material::FRONT, osg::Vec4(.8f,.8f,.8f,1));
	material->setShininess(osg::Material::FRONT, 40);
}

void Constructor::onLifeChanged() {
	updateColor(getLife()/getMaxLife());
	Unit::onLifeChanged();
}
