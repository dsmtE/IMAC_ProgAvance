//-----------------------------------------------------------------------------------
//
// Boids Tower Defense (based on Paratrooper code)
// Copyright (C) 2007-2008  Sukender
// For more information, contact us : sukender@free.fr
//
// All rights reserved
//
//-----------------------------------------------------------------------------------

#ifndef COMMON_H
#define COMMON_H

//#include <PVLE/Util/Util.h>
#include <PVLE/Entity/Explosion.h>
#include <boost/filesystem/path.hpp>

/// Creates a standard (for the app!) explosion.
///\warning This is \b not thread safe (as it uses a static variable).
///\param pos Position of the explision
///\param soundSamplePath Path of the sound file
///\param maxRadius Radius the explosion will have at the end.
///\param growingSpeed Amount the radius will grow per second. If <=0, then the value will be "maxRadius * 2".
///\param maxForce Maximum force (at the beginning) the explosion applies to surrounding objects. If <=0, then the value will be "growingSpeed * 2".
///\param maxDamage Maximum thermal (WeaponEnergy::THERMAL) damage (at the beginning) the explosion applies to surrounding objects. If <=0, then the value will be "maxForce / 10000".
std::shared_ptr<Explosion> createStdExplosion(const osg::Vec3f & pos, const boost::filesystem::path & soundSamplePath, float maxRadius = 15, float growingSpeed = -1, float maxForce = -1, float maxDamage = -1);


#endif	// APP_PATH_H
