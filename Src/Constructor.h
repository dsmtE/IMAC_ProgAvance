//-----------------------------------------------------------------------------------
//
// Boids Tower Defense
// Copyright (C) 2007-2009  Sukender
// For more information, contact us : sukender@free.fr
//
// All rights reserved
//
//-----------------------------------------------------------------------------------

#ifndef APP_CONSTRUCTOR_H
#define APP_CONSTRUCTOR_H

#include "Units.h"
#include "../Plugins/Boids/Boids.h"
//#include "CollisionCategories.h"
#include "GamePathFinder.h"
#include <vector>
#include <boost/thread/mutex.hpp>
#include <osg/observer_ptr>

namespace osg {
	class Material;
}

/// Constructor is a player's unit, that can move and build turrets.
/// It also can pick up bonuses and reach objectives.
class Constructor : public Unit
{
public:
	Constructor(Physics::World * phyWorld, GamePathFinder & pathFinder, WeightGetter * wg, BoidsUtil & boidsUtil, const C3DPhy * pInheritTeamAndPlayer = nullptr);
	virtual ~Constructor();

	virtual const char * className() const { return "Constructor"; }
	bool handleEvent(NetControlEvent * pEvent, TNL::EventConnection * pConnection);
	virtual void handleFrame(double elapsed);
	virtual void step(dReal stepSize);

	/// Unit does not react to cinetic hits (does not loose lif points on collision)
	virtual void isHitByCinetic  (double energy, const osg::Vec3 & point) {}

	boost::mutex & getPFMutex() { return pfMutex; }

	/// Issue a move order
	void moveTo(const osg::Vec3 & newPos);

	/// Cancel a move order (and only a move order)
	void cancelMove();
	/// Cancel a build order (and only a build order)
	void cancelBuild();

	enum EBuildOrder {
		BUILD_TURRET_BASIC,
		MAX_BUILD_ORDER
	};

	/// Builds at current position.
	void build(EBuildOrder buildOrder);
	bool isBuilding() const { return state >= BUILDING_FIRST && state <= BUILDING_LAST; }

	bool isStopped() const { return state == STOPPED; }

	virtual void onLifeChanged();

protected:
	void updateColor(float lifePercent);

	/// Callback given to the pathfinder that updates the constructor's state when a path has been computed.
	class LocalPathFinderModifyCB : public PathFinderModifyCB {
	public:
		LocalPathFinderModifyCB(Constructor & c) : c(c) {}
		virtual void operator()(GamePathFinder & gpf);
	protected:
		Constructor & c;
	};
	friend class LocalPathFinderModifyCB;

	GamePathFinder & pathFinder;
	std::shared_ptr<PathFinderCommandSimplePath> pfCommand;		///< A path finder command to be givent to pathFinder
	std::shared_ptr<WeightGetter> wg;							///< Computes the weight of graph edges
	std::shared_ptr<Physics::Geom> geom;						///< Main physical geometry of the constructor

	BoidsUtil & boidsUtil;
	Target * boidTarget;				///< Target used for boids simulation

	std::vector<osg::Vec3> pathPoints;	///< Result given by the path finder
	unsigned int curPointInPath;		///< Point counter (starts from 0) indicating the next point to move to
	float pointProgress;				///< Value in [0;1] indicating the progress between previous point and current one

	enum EState {
		STOPPED,			///< Unit doesn't move
		MOVING,				///< Unit moves
		STOPPING,			///< Unit moves but goes to the next graph point and will then stop
		BUILDING_FIRST,		///< Unit is currently busy ; state also tells the current building order (BUILDING_FIRST + BuildingOrder)
		BUILDING_LAST = BUILDING_FIRST+MAX_BUILD_ORDER-1,

		MAX_STATE
	};
	EState state;
	bool pfCommandLaunched;				///< Says if a command has been asynchroneously been given to the path finder
	osg::Vec3 zeroPoint;				///< Point before the first point in path

	void createModel(const osg::Vec3 & size);
	osg::MatrixTransform * drawnPath;		///< Node that contains the drawable path
	void removeDrawnPath();					///< Clears the drawn path (does nothing on orders/pathfinder)

	std::shared_ptr<LocalPathFinderModifyCB> pfModifyCB;
	boost::mutex pfMutex;				///< Mutex for path finder callbacks

	void doStop();
	void doBuildTurret(EBuildOrder buildOrder);
	std::weak_ptr<UnpilotedUnit> currentlyBuilding;
};


#endif	// APP_CONSTRUCTOR_H
