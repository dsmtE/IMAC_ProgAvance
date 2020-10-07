//-----------------------------------------------------------------------------------
//
// Boids Tower Defense
// Copyright (C) 2007-2008  Sukender
// For more information, contact us : sukender@free.fr
//
// All rights reserved
//
//-----------------------------------------------------------------------------------

#include "Turret.h"
#include <PVLE/Physics/Utility.h>
#include <osg/Material>
//#include <osg/PolygonMode>
#include <osg/BlendFunc>
#include <osg/LOD>
#include <PVLE/3D/Utility3D.h>

const float TURRET_ADD_RADIUS = 40;		///< Distance to add around the turret to avoid constructor going too close (=thru) the turret


Turret::Turret(const osg::Vec3 & pos_, float radius, float length, GamePathFinder & pathFinder, BoidsUtil & boidsUtil, const C3DPhy * pInheritTeamAndPlayer) :
	UnpilotedUnit(nullptr, true, pInheritTeamAndPlayer), pathFinder(pathFinder), boidsUtil(boidsUtil), radius(radius)
{
	setMaxLifeForce(100);
	setLifeForce(getMaxLife());

	std::pair<bool, osg::Vec3> outPosData;
	{
		GamePathFinder::Lock lock(pathFinder);		// Thread safety for gpf.get()
		// Modify pathfinder
		pos = addSizedObstacle(pathFinder, pos_, radius+TURRET_ADD_RADIUS);
		pos += osg::Z_AXIS * (length*.4f);

		// Compute out position
		outPosData = pathFinder.get().findClosestFreePoint(pos, (radius+TURRET_ADD_RADIUS)*1.5f);
	}
	if (!outPosData.first) outPos = osg::Vec3();
	else outPos = outPosData.second;

	// Set physics geom
	auto geom = Physics::createCanonicalCapsuleGeom(radius, length);
	//geom = Physics::createCanonicalBoxGeom(size, Physics::SurfaceParams(Physics::SurfaceParams::SOFT_ERP, 5000, 0, 0, .02f) );
	dGeomSetPosition(*geom, pos);
	dGeomSetCategoryBits(*geom, Collide::UNIT);
	dGeomSetCollideBits (*geom, Collide::BOID);
	addGeom(geom);

	// Setup target for boids simulation, with default params
	boidTarget = boidsUtil.newTarget(pos, -1, 500);
}


Turret::~Turret() {
	boidsUtil.deleteTarget(boidTarget);
	boidTarget = nullptr;

	GamePathFinder::Lock lock(pathFinder);		// Thread safety for gpf.get()
	pathFinder.get().removeSizedObstacle(getPos(), radius+TURRET_ADD_RADIUS);
	pathFinder.triggerGraphModified();
}


void Turret::step(dReal elapsed) {
	if (isBuilt()) rechargeTime -= elapsed;		// No need to clamp above 0
	else {
		ASSERT(_class);
		// Compute elapsed build time
		float buildElapsed(elapsed);
		if ((_curBuildTime+buildElapsed) > _class->_buildTime) {
			buildElapsed = _class->_buildTime - _curBuildTime;
		}

		// Update current build time
		_curBuildTime += elapsed;

		// Set life after the build time so that onLifeChanged() knows the building is finished
		setLife(osg::clampBetween(getLife() + getMaxLife() * (buildElapsed / _class->_buildTime), 0.f, getMaxLife()));

		// Update boid target (invert flee/attract multiplier when building)
		ASSERT(boidTarget);
		boidTarget->setMultiplier( fabs(boidTarget->getMultiplier()) * isBuilt() ? -1 : 1 );
		boidsUtil.targetUpdated(boidTarget);

		// Somewhat update model
		//dGeomSetPosition(*geom, pos - osg::Z_AXIS * (length*.4f) + ???);
	}

	UnpilotedUnit::step(elapsed);
}






#include <PVLE/3D/Commons.h>
#include <osg/Material>
#include <osg/CullFace>
#include <osg/Geode>
#include "Laser.h"


const unsigned int BASIC_TURRET_RADIUS = 20;
const unsigned int BASIC_TURRET_LENGTH = 100;
const float MAX_LASER_LENGTH = 500;
const float MAX_LASER_LENGTH2 = MAX_LASER_LENGTH*MAX_LASER_LENGTH;
const float RECHARGE_TIME = 1;

static const float LASER_DURATION = .2f;
static const float EXTRAPOLATE_TIME = LASER_DURATION * .5f;

static const UnpilotedUnitClass basicTurretClass(5);		// Build time

BasicTurret::BasicTurret(const osg::Vec3 & pos, GamePathFinder & pathFinder, BoidsUtil & boidsUtil, const C3DPhy * pInheritTeamAndPlayer) :
	Turret(pos+osg::Z_AXIS*BASIC_TURRET_LENGTH/2.f, BASIC_TURRET_RADIUS, BASIC_TURRET_LENGTH, pathFinder, boidsUtil, pInheritTeamAndPlayer)
{
	_class = &basicTurretClass;
	setMaxLifeForce(100);
	setLifeForce(1e-5f);		// Non zero value: we start building
	createModel(BASIC_TURRET_RADIUS, BASIC_TURRET_LENGTH);
	//boidTarget->setRadius(MAX_LASER_LENGTH * .9f);
	boidTarget->setRadius(MAX_LASER_LENGTH);
	set3DtoPhy();
}


void BasicTurret::createModel(float radius_, float length) {
	setModel(new osg::MatrixTransform);
	osg::ref_ptr<osg::LOD> lod = new osg::LOD();
	pModel->addChild(lod);
	lod->setRangeMode(osg::LOD::PIXEL_SIZE_ON_SCREEN);
	unsigned int baseLodSize = 80;
	static const unsigned int DIST_MULTIPLIER = 4;
	lod->addChild( createShapeGeodeCapsule(radius_, length, .01f),           0, baseLodSize);
	lod->addChild( createShapeGeodeCapsule(radius_, length, .33f), baseLodSize, baseLodSize*DIST_MULTIPLIER); baseLodSize*=DIST_MULTIPLIER;
	lod->addChild( createShapeGeodeCapsule(radius_, length, 1   ), baseLodSize, baseLodSize*DIST_MULTIPLIER); baseLodSize*=DIST_MULTIPLIER;
	lod->addChild( createShapeGeodeCapsule(radius_, length, 2   ), baseLodSize, FLT_MAX);

	osg::ref_ptr<osg::Material> material = new osg::Material();
	pModel->getOrCreateStateSet()->setAttributeAndModes(material.get());
	pModel->getOrCreateStateSet()->setAttributeAndModes(new osg::CullFace());
	updateColor(0);
}

void BasicTurret::updateColor(float lifePercent) {
	osg::StateSet * stateset = pModel->getOrCreateStateSet();
	osg::Material * material = boost::polymorphic_downcast<osg::Material *>( stateset->getAttribute(osg::StateAttribute::MATERIAL) );

	osg::Vec4 color;		// Final color: 0, .8, .9
	if (!isBuilt()) {
		ASSERT(_class);
		float builtPercent = _curBuildTime / _class->_buildTime;
		color = osg::Vec4(1-lifePercent,  1 -lifePercent*.2f,  1 -lifePercent*.1f, .1f + builtPercent*.6f);
	} else {
		color = osg::Vec4(            0, .1f+lifePercent*.7f, .2f+lifePercent*.7f, 1);
	}
	premultipyAlpha(color);
	material->setDiffuse( osg::Material::FRONT, color);
	osg::Vec4 ambient(color*.5f);
	ambient.w() = 1;
	material->setAmbient(osg::Material::FRONT, ambient);
	material->setSpecular(osg::Material::FRONT, osg::Vec4(.8f,.8f,.8f,1));
	material->setShininess(osg::Material::FRONT, 40);

	// Mode when building
	if (!isBuilt()) {
		//osg::PolygonMode * pPolyMode = new osg::PolygonMode;
		//stateset->setAttribute(pPolyMode);
		//pPolyMode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);

		stateset->setAttributeAndModes(new osg::BlendFunc(osg::BlendFunc::ONE, osg::BlendFunc::ONE_MINUS_SRC_ALPHA));	// Premultiplied alpha

		//stateset->setAttributeAndModes(new osg::CullFace(osg::CullFace::FRONT_AND_BACK), osg::StateAttribute::OFF);
		//stateset->setAttributeAndModes(new osg::CullFace(osg::CullFace::BACK));
		stateset->setAttributeAndModes(new osg::CullFace());
	} else {
		//stateset->removeAttribute(osg::StateAttribute::POLYGONMODE);
		stateset->removeAttribute(osg::StateAttribute::BLENDFUNC);
		stateset->setAttributeAndModes(new osg::CullFace());
	}
	stateset->setRenderingHint(isBuilt() ? osg::StateSet::OPAQUE_BIN : osg::StateSet::TRANSPARENT_BIN);
}

void BasicTurret::onLifeChanged() {
	updateColor(getLife()/getMaxLife());
	Turret::onLifeChanged();
}


inline Physics::Body * getBody(C3DPhy * object) {
	if (object->getNumGeoms()>=1) {
		Physics::Body * body = object->getGeom(0)->getBody();
		if (body) return body;
	}
	if (object->getNumAdditionalBodies() >= 1) return object->getAdditionalBody(0);
	return nullptr;
}


bool BasicTurret::createOrUpdateLaser(const osg::Vec3 & weaponStart, std::shared_ptr<C3DPhy> p3DPhy) {
	ASSERT(!get3DPhyOwner()->isDeleting());
	ASSERT(p3DPhy);
	// Fire at units not in the same team (pointer comparison)
	if (p3DPhy->getType() == ContainerType::UNIT && p3DPhy->getTeam() != getTeam()) {
		/*&& !boost::polymorphic_downcast<UnpilotedUnit*>(target.get())->testHasDied()*/

		osg::Vec3 weaponEnd(p3DPhy->getPos());
		//// Tell the laser to fire a bit at extrapolated position, to be nicer
		//auto body = getBody(p3DPhy.get());
		//if (body) {
		//	osg::Vec3f vel( dBodyGetLinearVelV(*body) * EXTRAPOLATE_TIME);
		//	weaponEnd += vel;
		//}
		if ((weaponEnd-weaponStart).length2() <= MAX_LASER_LENGTH2) {
			auto laser = lastLaser.lock();
			if (laser) {
				// Update current laser
				laser->set(weaponStart, weaponEnd);
			}
			else {
				// New laser
				laser = std::make_shared<Laser>(weaponStart, weaponEnd, boost::polymorphic_downcast<UnpilotedUnit *>(p3DPhy.get()), 5, LASER_DURATION);
				get3DPhyOwner()->addInScene(laser);
				rechargeTime = RECHARGE_TIME;
				lastLaser = laser;
			}
			lastTarget = p3DPhy;
			return true;
		}
	}
	return false;
}

void BasicTurret::step(dReal elapsed) {
	//TODO;		// Do not move too fast (angular velocity for new shots and laser tracking)?

	//bool wasBuilt = isBuilt();

	Turret::step(elapsed);		// Updates current building time (/!\ Also may destroy unit if 0 life points)
	if (testHasDied() || !isBuilt()) return;
	auto weaponStart = getPos() + osg::Z_AXIS * (BASIC_TURRET_LENGTH * .5f);

	// New shot
	if (rechargeTime <= 0) {
		// Weapon is ready
		// Try last target
		auto target = lastTarget.lock();
		if (!target || !createOrUpdateLaser(weaponStart, target)) {
			// Search for another one if last target failed
			// Fire at first target in list
			for(auto p3DPhy : get3DPhyOwner()->get3DPhys()) {
				if (createOrUpdateLaser(weaponStart, p3DPhy)) break;
			}
		}
	}

	// Laser tracking
	else {
		auto target = lastTarget.lock();
		auto laser = lastLaser.lock();
		if (laser) {
			if (target) createOrUpdateLaser(weaponStart, target);		// Update laser
			//else;		// Leave laser
		}
	}

	//if (!isBuilt() || !wasBuilt) {
	//	onLifeChanged();		// Update model
	//}
}
