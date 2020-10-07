//-----------------------------------------------------------------------------------
//
// Boids Tower Defense
// Copyright (C) 2007-2008  Sukender
// For more information, contact us : sukender@free.fr
//
// All rights reserved
//
//-----------------------------------------------------------------------------------

#include "Obstacle.h"
#include <PVLE/Physics/Utility.h>
#include <PVLE/Physics/CommonSurfaceParams.h>

const float OBSTACLE_ADD_RADIUS = 30;		///< Distance to add around the Obstacle to avoid constructor going too close (=thru) the Obstacle


Obstacle::Obstacle(const osg::Vec3 & pos, float radius, GamePathFinder & pathFinder, BoidsUtil & boidsUtil, const C3DPhy * pInheritTeamAndPlayer) :
	UnpilotedUnit(nullptr, true, pInheritTeamAndPlayer), pathFinder(pathFinder), boidsUtil(boidsUtil), boidTarget(nullptr), radius(radius)
{
	setMaxLifeForce(100);
	setLifeForce(getMaxLife());

	// Modify pathfinder
	osg::Vec3 graphPoint;
	graphPoint = addSizedObstacle(pathFinder, pos, radius+OBSTACLE_ADD_RADIUS);
	//graphPoint += osg::Z_AXIS * (radius*.2f);

	// Set physics geom
	auto geom = Physics::createCanonicalSphereGeom(radius, Physics::Materials::CONCRETE);
	dGeomSetPosition(*geom, graphPoint);
	dGeomSetCategoryBits(*geom, Collide::OBSTACLE);
	dGeomSetCollideBits (*geom, Collide::BOID);
	addGeom(geom);

	// Setup 3D
	createModel(radius);
	set3DtoPhy();

	// Setup target for boids simulation, with default params
	boidTarget = boidsUtil.newTarget(graphPoint, -.5f, radius*2.5f);		// Quite strongly repuslive but at very close range
}


Obstacle::~Obstacle() {
	boidsUtil.deleteTarget(boidTarget);
	boidTarget = nullptr;

	GamePathFinder::Lock lock(pathFinder);		// Thread safety for gpf.get()
	pathFinder.get().removeSizedObstacle(getPos(), radius+OBSTACLE_ADD_RADIUS);
	pathFinder.triggerGraphModified();
}

const char* Obstacle::className() const { return "Obstacle"; }

void Obstacle::step(dReal elapsed) {
	UnpilotedUnit::step(elapsed);
}

void Obstacle::isHitByCinetic(double energy, const osg::Vec3& point) {}


#include <PVLE/3D/Commons.h>
#include <PVLE/Util/Rand.h>
#include <osg/Material>
#include <osg/CullFace>
#include <osg/Geode>


void Obstacle::createModel(float radius_) {
	setModel( createShapeGeodeSphere(radius_, .01f) );

	osg::ref_ptr<osg::Material> material = new osg::Material();
	osg::Vec4 color(rand(.5f, .7f), rand(.4f, .7f), rand(.3f, .7f), 1);
	material->setDiffuse( osg::Material::FRONT, color);
	osg::Vec4 ambient(color*.5f);
	ambient.w() = 1;
	material->setAmbient(osg::Material::FRONT, ambient);
	material->setSpecular(osg::Material::FRONT, osg::Vec4(.8f,.8f,.8f,1));
	material->setShininess(osg::Material::FRONT, 40);
	pModel->getOrCreateStateSet()->setAttributeAndModes(material.get());
	pModel->getOrCreateStateSet()->setAttributeAndModes(new osg::CullFace());
}
