//-----------------------------------------------------------------------------------
//
// Boids Tower Defense (based on Paratrooper code)
// Copyright (C) 2007-2008  Sukender
// For more information, contact us : sukender@free.fr
//
// All rights reserved
//
//-----------------------------------------------------------------------------------

#include "Units.h"

#include <PVLE/Physics/Space.h>
#include <PVLE/Physics/Contact.h>
#include <PVLE/Entity/PhysicsUpdateCB.h>
#include <osg/Group>

#include <PVLE/Util/Log.h>
#include <PVLE/Util/Assert.h>
//#include "../Game/Player.h"


UnpilotedUnit::UnpilotedUnit(Physics::World * phyWorld, bool phyBound, const C3DPhy * pInheritTeamAndPlayer)
	: C3DPhy(pInheritTeamAndPlayer), phyWorld(phyWorld), _phyBound(phyBound), _destroyed(false),
	_class(nullptr), _curBuildTime(0),
	life(-1), maxLife(-1)
{
}

//UnpilotedUnit::UnpilotedUnit() : phyWorld(nullptr), _canMove(false) {}


//void UnpilotedUnit::hitBefore(......) {
//	.......
//	inout_pOtherBody = nullptr;		// To create a "one way" collision
//}

void UnpilotedUnit::step(dReal stepSize) {
	// Destruction code is in step() instead of onLifeChanged to avoid the object to be marked as removed before modifications (See C3DPhyOwner documentation)
	if (testHasDied()) {
		//if (!deleteCB || deleteCB(this)) {
		markAsRemoved();	// Never modify geom or model after this call (See C3DPhyOwner documentation)
		return;
	}

	C3DPhy::step(stepSize);
	//if (osg::equivalent(ttl, 0)) return;	// Never modify geom or model if TTL == 0 (default is to destroy 3D-Phy on TTL == 0)
}

void UnpilotedUnit::onLifeChanged() {
	if (life<=0) _destroyed = true;
	else if (life>maxLife) life = maxLife;
}

void UnpilotedUnit::isHitBy(WeaponEnergy type, double energy, const osg::Vec3 & point) {
	switch(type) {
		case WeaponEnergy::RADIANT:
			isHitByRadiant(energy, point);
			return;
		case WeaponEnergy::PERFORANT:
			isHitByPerforant(energy, point);
			return;
		case WeaponEnergy::THERMAL:
			isHitByThermal(energy, point);
			return;
		case WeaponEnergy::CHEMICAL:
			isHitByChemical(energy, point);
			return;
		case WeaponEnergy::CINETIC:
			isHitByCinetic(energy, point);
			return;
		//case WeaponEnergy::MAX_WEAPON_ENERGY:
		default:
			ASSERT(false && "Implementation error");
	}
}

// --------------------------------------------------------------------------------------------------------


Unit::Unit(Physics::World * phyWorld, const C3DPhy * pInheritTeamAndPlayer)
	: UnpilotedUnit(phyWorld, true, pInheritTeamAndPlayer)
{
}

//Unit::Unit() {}


void Unit::step(dReal stepSize) {
	UnpilotedUnit::step(stepSize);
	//// -- Same as UnpilotedUnit --
	//if (testHasDied()) {
	//	if (!pDeleteCB.valid() || (*pDeleteCB)(this)) {
	//		ASSERT(p3DPhyOwner);				// Units must have an owner
	//		p3DPhyOwner->markAsRemoved(shared_from_this());	// Never modify geom or model after this call (See C3DPhyOwner documentation)
	//	}
	//	return;
	//}

	//C3DPhy::step(stepSize);
	//if (osg::equivalent(ttl, 0)) return;	// Never modify geom or model if TTL == 0 (default is to destroy 3D-Phy on TTL == 0)
	//// -- End --

	//handleFrame(stepSize);
}
