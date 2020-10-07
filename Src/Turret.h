//-----------------------------------------------------------------------------------
//
// Boids Tower Defense
// Copyright (C) 2007-2009  Sukender
// For more information, contact us : sukender@free.fr
//
// All rights reserved
//
//-----------------------------------------------------------------------------------

#ifndef APP_TURRET_H
#define APP_TURRET_H

#include "Units.h"
#include "../Plugins/Boids/Boids.h"
#include "CollisionCategories.h"
#include "GamePathFinder.h"

class Laser;

namespace osg {
	class Material;
}

/// Base class for all turrets in the game.
///\warning Creation of a turrets affects the path finder in a way that costs a lot of CPU ; you may thus create turrets asynchroneously.
class Turret : public UnpilotedUnit
{
public:
	/// Buils a turret by affecting the path finder and the boids simulation.
	Turret(const osg::Vec3 & pos, float radius, float length, GamePathFinder & pathFinder, BoidsUtil & boidsUtil, const C3DPhy * pInheritTeamAndPlayer = nullptr);
	virtual ~Turret();

	/// Updates rechargeTime
	virtual void step(dReal stepSize) override;

	/// Default is to not react to collisions
	virtual void isHitByCinetic(double energy, const osg::Vec3 & point) override {}

	/// Position of the turret, adjusted from  by the PathFinder gave 
	const osg::Vec3 & getTurretPos() const { return pos; }
	const osg::Vec3 & getOutPos()    const { return outPos; }
	float getRadius() const { return radius; }

protected:
	GamePathFinder & pathFinder;
	BoidsUtil & boidsUtil;
	Target * boidTarget = nullptr;		///< Target used for boids simulation
	osg::Vec3 pos;
	osg::Vec3 outPos;			///< Position a constructor can take once the building has been built
	float radius;
	float rechargeTime = 0;		///< Time before the weapon can fire
};


/// A cheap and basic turret that shoots at boids within a radius.
class BasicTurret : public Turret {
public:
	BasicTurret(const osg::Vec3 & pos, GamePathFinder & pathFinder, BoidsUtil & boidsUtil, const C3DPhy * pInheritTeamAndPlayer = nullptr);

	virtual const char * className() const { return "BasicTurret"; }
	virtual void step(dReal stepSize);

protected:
	void createModel(float radius, float length);
	void updateColor(float lifePercent);
	virtual void onLifeChanged();

	/// Tries to fire at a target, returns \c true if succeeded
	bool createOrUpdateLaser(const osg::Vec3 & weaponStart, std::shared_ptr<C3DPhy> p3DPhy);

	std::weak_ptr<C3DPhy> lastTarget;		///< Last target the turret shot at
	std::weak_ptr<Laser> lastLaser;
};

#endif	// APP_TURRET_H
