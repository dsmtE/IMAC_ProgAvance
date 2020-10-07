//-----------------------------------------------------------------------------------
//
// Boids Tower Defense
// Copyright (C) 2007-2009  Sukender
// For more information, contact us : sukender@free.fr
//
// All rights reserved
//
//-----------------------------------------------------------------------------------

#ifndef APP_OBSTACLE_H
#define APP_OBSTACLE_H

#include "Units.h"
#include "CollisionCategories.h"
#include "GamePathFinder.h"
#include "../Plugins/Boids/Boids.h"

/// Dumb and ugly rock on the floor, with life points (so it can be destroyed).
///\warning Creation of an obstacle affects the path finder in a way that costs a lot of CPU ; you may thus create Obstacles asynchroneously.
class Obstacle : public UnpilotedUnit
{
public:
	/// Buils an obstacle by affecting the path finder and the boids simulation.
	Obstacle(const osg::Vec3 & pos, float radius, GamePathFinder & pathFinder, BoidsUtil & boidsUtil, const C3DPhy * pInheritTeamAndPlayer = nullptr);
	virtual ~Obstacle();

	virtual const char* className() const override;
	virtual void step(dReal stepSize) override;

	/// Default is to not react to collisions
	virtual void isHitByCinetic(double energy, const osg::Vec3& point) override;

protected:
	GamePathFinder & pathFinder;
	BoidsUtil & boidsUtil;
	Target * boidTarget;		///< Target used for boids simulation
	float radius;

	void createModel(float radius);
};

#endif	// APP_OBSTACLE_H
