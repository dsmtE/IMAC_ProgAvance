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
/// Abstract init classes. A unit is always composed of a 3D node and a geom, but most of them also have a body that binds their geom.

#ifndef ENT_UNITS_H
#define ENT_UNITS_H

//#include <osg/ref_ptr>
#include <osg/Matrix>

#include <PVLE/Entity/3DPhy.h>
#include <PVLE/Physics/Geom.h>
#include <PVLE/Physics/Body.h>
#include <PVLE/Input/MatrixGetter.h>
#include <PVLE/Util/Math.h>



// Forward declarations
namespace Physics {
	class Space;
	class World;
}
namespace osg {
	class Group;
	class MatrixTransform;
}


/// Flyweight for UnpilotedUnit class.
struct UnpilotedUnitClass {
	UnpilotedUnitClass(float buildTime) : _buildTime(buildTime) {}
	float _buildTime;
};




/// Abstract class for unpiloted units, that is to say units with life points but without a control mapper (= not controlled directly by switches and axes, which map to bouttons and mouse/joysick axes).
/// Unpiloted units have the ability to move and could thus be any unit without a control mapper, such as something that follows a predifined path.
/// However, unpiloted units should \b not be drones-like units, since they are piloted by an AI.
/// Unpiloted units can be with or without a physical body (only a geom/geom and body).
///\warning An unit, unpiloted or not, \b MUST be put in a C3DPhyOwner, because it assumes it can add things into the world, remove the created things, or remove itself.
///\author Sukender
///\version 0.5.1 - Still under dev
class UnpilotedUnit : public C3DPhy {
public:
	/// Creates a unit and add it to the given parent space (Physics), and to the given parent group (3D). The given parents must \b own their children since the unit doesn't.
	/// Derivate classes are responsible for geom and model creation upon construction. They typically call <tt>createGeom(); createModel(); bind();</tt>.
	///\param phyWorld Physical world, used on body creation.
	///\param phyBound Information that says if the unit's 3D representation should be continuously bound the unit's physics representation (<tt>true</tt>), or just once on construction (<tt>false</tt>).
	///\param pInheritTeamAndPlayer Pointer to a 3DPhy from which to copy (=inherit) team and player.
	UnpilotedUnit(Physics::World * phyWorld, bool phyBound = true, const C3DPhy * pInheritTeamAndPlayer = nullptr);

	virtual ContainerType getType() const { return ContainerType::UNIT; }
	virtual UnpilotedUnit * asUnit() { return this; }

	/// Returns the type name of the unit ("tank", "turret", etc.)
	virtual const char * className() const =0;
	/// Steps the unit by applying all necessaty forces, moving parts, firing, etc.
	///\note A good practice is to call <code>if (testHasDied()) return;</code> just after <code>UnpilotedUnit::step();</code> if it's not the last instruction.
	virtual void step(dReal stepSize);

	bool isBuilt() const { ASSERT(_class); return _curBuildTime >= _class->_buildTime; }

	bool isPhyBound() const { return _phyBound; }
	bool testHasDied() const { return _destroyed; }

	virtual void isHitByRadiant  (double energy, const osg::Vec3 & point) { addLife(-energy); }
	virtual void isHitByPerforant(double energy, const osg::Vec3 & point) { addLife(-energy); }
	virtual void isHitByThermal  (double energy, const osg::Vec3 & point) { addLife(-energy); }
	virtual void isHitByChemical (double energy, const osg::Vec3 & point) { addLife(-energy); }
	virtual void isHitByCinetic  (double energy, const osg::Vec3 & point) { addLife(-energy); }
	virtual void isHitBy(WeaponEnergy type, double energy, const osg::Vec3 & point);

	inline void  setMaxLife(float maxLife_) { maxLife = maxLife_; onLifeChanged(); }
	inline float getMaxLife() const { return maxLife; }

	inline void  setLife(float life_) { ASSERT(maxLife>0); life  = life_; onLifeChanged(); }
	inline void  addLife(float life_) { ASSERT(maxLife>0); life += life_; onLifeChanged(); }
	inline float getLife() const { return life; }

	virtual void onLifeChanged();		///< Default is to set _destroyed flag to true when life is <=0.

protected:
	Physics::World * phyWorld;		///< Physical world, used on body creation.
	bool _phyBound;					///< Continuously bound to physics ?
	bool _destroyed;

	/// Sets life points without any check.
	inline void setLifeForce(float life_) { life = life_; }
	/// Sets maximum life points without any check.
	inline void setMaxLifeForce(float maxLife_) { maxLife = maxLife_; }

	const UnpilotedUnitClass * _class;		// Flyweight pattern
	float _curBuildTime;

private:
	float life;			///< Current life points (units have no localized damage).
	float maxLife;		///< Maximum life points (units have no localized damage). Not in _class because maxlife may be affected by some bonuses.
};



#include <PVLE/Util/Assert.h>
#include <PVLE/Input/ControlMapper.h>
class Player;

/// Abstract base class of piloted unit : aeroplanes, drones, tanks, humans, fixed turrets, non-offensive units, etc.
/// Doesn't apply to unpiloted units (Destroyable buildings for instance).
/// This class is a raw ControlHandler, which has a raw pointer a ControlMapper to get inputs, so the caller (typically the app) is responsible for mapper's allocation and destruction.
///\author Sukender
///\version 0.5.4 - Not really complete... :)
class Unit : public UnpilotedUnit, public ControlHandler {
public:
	Unit(Physics::World * phyWorld, const C3DPhy * pInheritTeamAndPlayer = nullptr);

	virtual void step(dReal stepSize);
	virtual const char * className() const { return "Unit"; }

	///\name Weapons
	///@{
	//SWeaponMount pWeapons[MAX_WEAPONS];				///< Trivial array is faster than vector.
	///@}

	///\name Utility
	///@{
	///@}

//protected:
};



#endif    // ENT_UNITS_H
