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
/// Physics global constants for categories of collisions.
/// See Physics::Geom::setCategoryBits() and Physics::Geom::setCollideBits().

#ifndef COLLISION_CATEGORIES_H
#define COLLISION_CATEGORIES_H

#include <PVLE/Util/Util.h>

namespace Collide {

enum ECollisionCategories {
	// Define collision flags here
	TERRAIN			= 0x0001,
	BOID			= 0x0002,
	UNIT			= 0x0004,
	OBSTACLE		= 0x0008,
	//OTHER			= 0x0004,

	ALL = TERRAIN | BOID | UNIT | OBSTACLE			///< Default flag for catergories and collide bits.
};

}	// namespace Collide


#endif	// COLLISION_CATEGORIES_H
