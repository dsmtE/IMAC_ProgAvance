//-----------------------------------------------------------------------------------
//
// Boids Tower Defense
// Copyright (C) 2007-2009  Sukender
// For more information, contact us : sukender@free.fr
//
// All rights reserved
//
//-----------------------------------------------------------------------------------

#ifndef APP_BOID_H
#define APP_BOID_H


#include "Units.h"
#include "../Plugins/Boids/Boids.h"

/// Annoying autonomous mob, acting as an ennemy for the Constructor.
class Boid : public UnpilotedUnit
{
public:
	Boid(Physics::World * phyWorld, const HeightFieldData & hf, BoidsUtil & boidsUtil);

	virtual const char * className() const { return "Boid"; }
	virtual void isHitByCinetic  (double energy, const osg::Vec3 & point) {}
	virtual void step(dReal elapsedTime);

	virtual ~Boid();

protected:
	float size;
	BoidSim * sim;
	//const osg::HeightField & hf;
	const HeightFieldData & hf;
	Physics::Body * mainBody;
	BoidsUtil & boidsUtil;

	void createModel(float size);
};


#include <osgDB/DynamicLibrary>

/// Singleton used to load the Boids plugin.
class BTDWrapperLoader : public Util::Singleton<BTDWrapperLoader> {
public:
	osgDB::DynamicLibrary * openLibrary(const std::string& str) {
		auto & lib = getPointerForName(str);
		if (!lib.valid()) lib = osgDB::DynamicLibrary::loadLibrary(str);
		if (!lib.valid()) lib = osgDB::DynamicLibrary::loadLibrary(createLibraryNameForWrapper(str));
		return lib.get();
	}
	void closeLibrary(const std::string& str) {
		getPointerForName(str) = nullptr;
	}

	static std::string createLibraryNameForWrapper(const std::string& ext);
protected:
	osg::ref_ptr<osgDB::DynamicLibrary> boids;
	//osg::ref_ptr<osgDB::DynamicLibrary> dummy;

	osg::ref_ptr<osgDB::DynamicLibrary> & getPointerForName(const std::string& str) {
		//if (str == "boids") return boids;
		//THROW_STR("Unknown library (" + str + ")");
		return boids;
	}
};


#endif	// APP_BOID_H
