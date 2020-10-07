//-----------------------------------------------------------------------------------
//
// Boids Tower Defense
// Copyright (C) 2007-2008  Sukender
// For more information, contact us : sukender@free.fr
//
// All rights reserved
//
//-----------------------------------------------------------------------------------

#include "Boid.h"

const float BOIDS_ZMIN = 100;
const float BOIDS_ZMAX = 150;

const float MAX_WEAPON_RANGE  = 30;
const float MAX_WEAPON_RANGE2 = MAX_WEAPON_RANGE*MAX_WEAPON_RANGE;


std::string BTDWrapperLoader::createLibraryNameForWrapper(const std::string& ext) {
	#if defined(WIN32)
		#ifdef _DEBUG
			return "btd_"+ext+"d.dll";
		#else
			return "btd_"+ext+".dll";
		#endif
	#elif macintosh
		return "btd_"+ext;
	#elif defined(__hpux__)
		// why don't we use PLUGIN_EXT from the makefiles here?
		return "btd_"+ext+".sl";
	#else
		return "btd_"+ext+".so";
	#endif
}



#include <PVLE/3D/Commons.h>
#include <PVLE/Util/Rand.h>
#include <PVLE/Util/Exception.h>
#include <PVLE/Physics/Utility.h>
#include <osg/LOD>
#include <osg/Material>
#include <osg/CullFace>
#include "Common.h"
#include "Game.h"
#include "Path.h"
#include "CollisionCategories.h"


Boid::Boid(Physics::World * phyWorld, const HeightFieldData & hf, BoidsUtil & boidsUtil) : UnpilotedUnit(phyWorld), hf(hf), boidsUtil(boidsUtil) {
	size = rand(5, 15);
	float mass = size/2;

	setMaxLifeForce(size);
	setLifeForce(getMaxLife());

	//boidList.push_back(this);
	sim = boidsUtil.newBoidSim(mass);
	if (!sim) THROW_STR("Boids plugin newBoidSim() did not return a BoidSim");

	//const osg::Vec3 boidSize(osg::Vec3(1,1,2) * 10);
	//setModel( createShapeGeodeBox(boidSize) );
	//Physics::Geom * geom = Physics::createCanonicalBox(*phyWorld, boidSize, 1, Physics::SurfaceParams(0, .1f) );

	osg::Vec3 boundsMin, boundsMax;
	boidsUtil.getBounds(boundsMin, boundsMax);
	osg::Vec3 pos(rand(boundsMin.x(), boundsMax.x()), rand(boundsMin.y(), boundsMax.y()), rand(boundsMin.z(), boundsMax.z()));
	sim->setPos(pos);

	createModel(size);

	if (sim->getMode() == BoidSim::BOIDS_SET_FORCE) {
		//Physics::Geom * geom = Physics::createCanonicalSphere(*phyWorld, boidSize, mass, Physics::SurfaceParams(Physics::SurfaceParams::SOFT_ERP, 5000, 0, 0, 0, .02f) );
		auto geom = Physics::createCanonicalSphere(*phyWorld, size, mass);
		mainBody = geom->getBody();
		dBodySetPosition(*mainBody, pos);
		dGeomSetCategoryBits(*geom, Collide::BOID);
		dGeomSetCollideBits (*geom, Collide::ALL);
		dBodySetAutoDisableFlag(*mainBody, 0);		// No auto-disable
		addGeom(geom);
		bind();
	} else {
		mainBody = nullptr;
		pModel->setMatrix( osg::Matrix::translate(pos) );
	}
}

Boid::~Boid() {
	boidsUtil.deleteBoidSim(sim);
	sim = nullptr;

	//std::vector<Boid*>::iterator it = std::find(boidList.begin(), boidList.end(), this);
	//ASSERT(it != boidList.end());
	//boidList.erase(it);

	auto game = PVLEGameHolder::instance().get<Game>();
	ASSERT(game);
	// CACHE PRELOAD
	if (game->getState() != Game::STATE_STOP) game->addInSceneDelayed(createStdExplosion(dGeomGetPositionV(*vGeoms[0]) + osg::Z_AXIS*5, AppPaths::sounds() / "Explosions" / "Bomb.wav", size*10, size*20, size*500, size*.05f));
}

void Boid::step(dReal elapsedTime) {
	ASSERT(hasReferenceFrameObject());
	sim->setPos(getPos());
	if (sim->getMode() == BoidSim::BOIDS_SET_FORCE) {
		// TODO Add collision test with the floor and apply forces only when on floor / OR / Apply momentum on boids instead of force
		// TODO Add collision test with the plane under the scene and delete the boid
		dBodyAddForce(*mainBody, sim->update(hf, elapsedTime));
		//dBodyAddTorque(*mainBody, sim->update(hf, elapsedTime));
	} else {
		pModel->setMatrix( osg::Matrix::translate(sim->update(hf, elapsedTime)) );
	}

	for(auto p3DPhy : get3DPhyOwner()->get3DPhys()) {
		// Fire at units not in the same team (pointer comparison)
		if (p3DPhy->getType() == ContainerType::UNIT && p3DPhy->getTeam() != getTeam()) {
			if ((getPos() - p3DPhy->getPos()).length2() <= MAX_WEAPON_RANGE2) {
				p3DPhy->isHitBy(WeaponEnergy::RADIANT, elapsedTime*1, p3DPhy->getPos());
				break;
			}
		}
	}

	UnpilotedUnit::step(elapsedTime);
}

void Boid::createModel(float boidSize) {
	setModel(new osg::MatrixTransform);
	osg::ref_ptr<osg::LOD> lod = new osg::LOD;
	pModel->addChild(lod);
	lod->setRangeMode(osg::LOD::PIXEL_SIZE_ON_SCREEN);
	unsigned int baseLodSize = 25;
	static const unsigned int DIST_MULTIPLIER = 4;
	lod->addChild( createShapeGeodeSphere(boidSize, .01f),           0, baseLodSize);
	lod->addChild( createShapeGeodeSphere(boidSize, .5f ), baseLodSize, baseLodSize*DIST_MULTIPLIER); baseLodSize*=DIST_MULTIPLIER;
	lod->addChild( createShapeGeodeSphere(boidSize, 1   ), baseLodSize, baseLodSize*DIST_MULTIPLIER); baseLodSize*=DIST_MULTIPLIER;
	lod->addChild( createShapeGeodeSphere(boidSize, 2   ), baseLodSize, FLT_MAX);

	osg::ref_ptr<osg::Material> material = new osg::Material;
	osg::Vec4 color(rand(.5f, 1), rand(.5f, 1), rand(.5f, 1), 1);
	material->setDiffuse( osg::Material::FRONT, color);
	osg::Vec4 ambient(color*.5f);
	ambient.w() = 1;
	material->setAmbient(osg::Material::FRONT, ambient);
	material->setSpecular(osg::Material::FRONT, osg::Vec4(.8f,.8f,.8f,1));
	material->setShininess(osg::Material::FRONT, 40);
	pModel->getOrCreateStateSet()->setAttributeAndModes(material.get());
	pModel->getOrCreateStateSet()->setAttributeAndModes(new osg::CullFace);
}
