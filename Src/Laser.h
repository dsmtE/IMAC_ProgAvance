//-----------------------------------------------------------------------------------
//
// Boids Tower Defense
// Copyright (C) 2007-2009  Sukender
// For more information, contact us : sukender@free.fr
//
// All rights reserved
//
//-----------------------------------------------------------------------------------

#ifndef APP_LASER_H
#define APP_LASER_H

#include <PVLE/Entity/3DPhy.h>

/// Simple and basic "ammunition" ; can't miss its target, fires instantly.
///\version 2 - Replaced the ugly line with a sprite
///\todo make the laser do damage on each step rather than on the first frame.
class Laser : public C3DPhy {
public:
	//Laser(const osg::Vec3 & start, const osg::Vec3 & end, UnpilotedUnit * hitUnit, float energy=1, const osg::Vec4 & color = osg::Vec4(1,0,0,1), float ttl=.2f);
	Laser(const osg::Vec3 & start, const osg::Vec3 & end, UnpilotedUnit * hitUnit, float energy=1, float ttl=.2f);

	const osg::Vec3 & getStart() const { return start; }
	const osg::Vec3 & getEnd() const { return end; }

	/// Moves start and end point of the laser.
	void set(const osg::Vec3 & start_, const osg::Vec3 & end_);

protected:
	osg::Vec3 start, end;
};

#endif	// APP_LASER_H
