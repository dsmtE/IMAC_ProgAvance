///\file
/// Sukender: This is my implementation of the boids simulation for BTD. The code isn't clean nor optimized as it should be, but it works.

#include "Boids.h"
#include <vector>
#include <list>
#include <algorithm>
#include <memory>
#include <boost/foreach.hpp>

#include <assert.h>
#include <float.h>
#define ASSERT assert		///< To stick with main program syntax

#include <stdlib.h>		// For random functions
inline float rand(float max) { return rand() * max / static_cast<float>(RAND_MAX); }
inline float rand(float min, float max) { return rand() * (max-min) / static_cast<float>(RAND_MAX) + min; }

#if defined(_MSC_VER) || defined(__CYGWIN__) || defined(__MINGW32__) || defined( __BCPLUSPLUS__)  || defined( __MWERKS__)
	#include "../Export.h"		// Could be always included, but eases compilation of the plugin under Linux
#else
	#define BTD_PLUGIN_EXPORT
#endif

using TargetList = std::list<std::shared_ptr<Target>>;
TargetList listTargets;		///< Ugly global list of targets the boids may go to (or flee).

/// Scene bounds
osg::Vec3 boundMin, boundMax;

//float BOIDS_DEFAULT_TARGET_RADIUS = 0;

/// A very simple algorithm for for close neighbours.
/// Runs a functor for each neighbour.
class ProximityDatabase {
public:
	ProximityDatabase() {}
	void add(BoidSim * b) { ASSERT(b); list.push_back(b); }
	void remove(BoidSim * b) {
		ASSERT(b); 
		TList::iterator it = std::find(list.begin(), list.end(), b);
		if (it != list.end()) list.erase(it);
	}

	template<typename Functor>
	void for_each_neighbor(const BoidSim * baseBoid, Functor && f, float maxLen = FLT_MAX, float minLen = 1e-6f) {
		for(auto boid : list) {
			if (boid == baseBoid) continue;
			float distance = (boid->getPos() - baseBoid->getPos()).length();
			if (distance >= minLen && distance <= maxLen) f(boid, distance);
		}
	}

	// Non copyable
	ProximityDatabase(const ProximityDatabase &) =delete;
	ProximityDatabase & operator =(const ProximityDatabase &) =delete;

protected:
	using TList = std::vector< BoidSim* >;
	TList list;
};

ProximityDatabase proxymityDB;


enum Attenuation {
	ATTENUATION_NONE,			///< No attenuation
	ATTENUATION_LINEAR,			///< Force is linearily lower when further
	ATTENUATION_QUADRATIC,		///< Force is quadratically lower when further
	ATTENUATION_INV_LINEAR,		///< Force is linearily higher when further
	ATTENUATION_INV_QUADRATIC	///< Force is quadratically higher when further
};

inline float getAttenuation(float distance, Attenuation att) {
	ASSERT(distance > 0);
	if (att==ATTENUATION_NONE) return 1;
	if (att==ATTENUATION_LINEAR) return 1/distance;
	if (att==ATTENUATION_INV_LINEAR) return distance;
	if (att==ATTENUATION_QUADRATIC) return 1/(distance*distance);
	ASSERT(att==ATTENUATION_INV_QUADRATIC);
	return distance * distance;
}


/// Functor doing steering for cohesion.
class CohesionCB {
public:
	CohesionCB() : nbNeighbors(0) {}
	void operator()(BoidSim * boid, float distance) {
		neighborsPosSum += boid->getPos();
		++nbNeighbors;
	}
	unsigned int getNbNeighbors() const { return nbNeighbors; }
	osg::Vec3f getAveragePos() const { return nbNeighbors<=0 ? osg::Vec3f() : neighborsPosSum / static_cast<float>(nbNeighbors); }

	// Non copyable
	CohesionCB(const CohesionCB &) =delete;
	CohesionCB & operator =(const CohesionCB &) =delete;
protected:
	//const osg::Vec3f & _pos;
	osg::Vec3f neighborsPosSum;
	unsigned int nbNeighbors;
};


/// Functor making boids prefer having the same speed.
class CommonSpeedCB {
public:
	CommonSpeedCB(const osg::Vec3f & pos, Attenuation attenuation) : _pos(pos), attenuation(attenuation) {}
	void operator()(BoidSim * boid, float distance);

	//osg::Vec3f getSteer() /*const*/ { steer.normalize(); return steer; }
	osg::Vec3f getSteer() const {
		return weight>0 ? steer/weight : steer;
	}

	// Non copyable
	CommonSpeedCB(const CommonSpeedCB &) =delete;
	CommonSpeedCB & operator =(const CommonSpeedCB &) =delete;
protected:
	const osg::Vec3f & _pos;
	Attenuation attenuation;
	osg::Vec3f steer;
	float weight;
};


/// The concrete class for the boid simulation.
class BoidSimSKD : public BoidSim {
public:
	BoidSimSKD(float mass) : BoidSim(mass), randomTargetSteering(false) {
		proxymityDB.add(this);
	}
	virtual ~BoidSimSKD() {
		proxymityDB.remove(this);
	}

	virtual EMode getMode() const { return BOIDS_SET_FORCE; }
	virtual void setPos(const osg::Vec3 & pos_) {
		deltaPos = pos - pos_;
		pos = pos_;
	}
	virtual osg::Vec3 getPos() const { return pos; }

	osg::Vec3 getDeltaPos() const { return deltaPos; }

	osg::Vec3 getDir() const {
		osg::Vec3 dir(deltaPos);
		dir.normalize();
		return dir;
	}

	osg::Vec3 steerForCohesion(float maxLen = FLT_MAX, float minLen = 1e-6f/*, Attenuation attenuation = ATTENUATION_NONE*/) {
		CohesionCB cb;
		proxymityDB.for_each_neighbor(this, cb, maxLen, minLen);
		if (cb.getNbNeighbors() <= 0) return osg::Vec3();
		osg::Vec3 steer(cb.getAveragePos() - pos);
		steer.normalize();
		return steer;
	}

	osg::Vec3 steerForCommonSpeed(float elapsed, float maxLen = FLT_MAX, float minLen = 1e-6f, Attenuation attenuation = ATTENUATION_NONE) {
		ASSERT(elapsed>0);
		CommonSpeedCB cb(pos, attenuation);
		proxymityDB.for_each_neighbor(this, cb, maxLen, minLen);
		return cb.getSteer() /* / elapsed */;		// getSteer() is based on deltaPos (should be /elapsed, but then *elapsed to apply)
	}

	osg::Vec3 steerForSeparation(float maxLen = FLT_MAX, float minLen = 1e-6f, Attenuation attenuation = ATTENUATION_LINEAR) {
		osg::Vec3f steer;
		const auto localPos = pos;		// For lambda capture. Could have captured "this"
		proxymityDB.for_each_neighbor(this, [&localPos, &steer, attenuation](BoidSim * boid, float distance) {
			steer += (localPos - boid->getPos()) * getAttenuation(distance, attenuation);
		}, maxLen, minLen);
		return steer;
	}

	virtual osg::Vec3 update(const HeightFieldData & /*hf*/, float elapsed) {
		static const float FORCE_ADJUST = .3f;
		static const float BOIDS_MULTIPLIER = 1e6f*FORCE_ADJUST;
		static const float TARGET_MULTIPLIER = BOIDS_MULTIPLIER*5;
		//static const float BORDER_MULTIPLIER = BOIDS_MULTIPLIER*20;

		osg::Vec3 dir = getDir();
		osg::Vec3 steer;

		// Boids steering
		steer += (steerForCohesion(500, 100) + steerForSeparation(50) * 2 /*+ steerForCommonSpeed(elapsed, 150)*/) * BOIDS_MULTIPLIER;

		// Target steering
		osg::Vec3 targetSteer;
		for(auto target : listTargets) {
			if (target->getMultiplier() != 0) {
				osg::Vec3 offset(target->getPos() - pos);
				float distance = offset.length();
				if (distance > 0 && distance < target->getRadius()) {
					targetSteer += offset * (target->getMultiplier() / distance * getAttenuation(distance, ATTENUATION_QUADRATIC));		// Normalized offset * multiplier
				}
			}
		}
		// Random target steering
		if (rand(20) < elapsed) {		// 20 sec random
			// Toggle random target steering
			randomTargetSteering = !randomTargetSteering;
			if (randomTargetSteering) randomTarget = osg::Vec3(rand(boundMin.x(), boundMax.x()), rand(boundMin.y(), boundMax.y()), 0);
		}
		if (randomTargetSteering) {
			osg::Vec3 offset(randomTarget - pos);
			float distance = offset.length();
			if (distance > 0) {
				targetSteer += offset * (1/distance * getAttenuation(distance, ATTENUATION_QUADRATIC));		// Normalized offset * multiplier
			}
		}
		targetSteer.normalize();
		steer += targetSteer * TARGET_MULTIPLIER;

		//// Borders steering
		//if (pos.x() < boundMin.x()) steer += osg::X_AXIS * BORDER_MULTIPLIER;
		//if (pos.y() < boundMin.y()) steer += osg::Y_AXIS * BORDER_MULTIPLIER;
		////if (pos.z() < boundMin.z()) steer += osg::Z_AXIS * BORDER_MULTIPLIER;
		//if (pos.x() > boundMax.x()) steer -= osg::X_AXIS * BORDER_MULTIPLIER;
		//if (pos.y() > boundMax.y()) steer -= osg::Y_AXIS * BORDER_MULTIPLIER;
		////if (pos.z() > boundMax.z()) steer -= osg::Z_AXIS * BORDER_MULTIPLIER;

		// Horizontal steer only
		steer.z() = 0;

		//// Constant speed steering (horizontal only)
		//static const float TARGET_SPEED = 1;
		//ASSERT(elapsed>0);
		//osg::Vec3 horizSpeed(deltaPos.x() / elapsed, deltaPos.y() / elapsed, 0);
		//horizSpeed += steer*elapsed;		// Take into account current steering
		//float curSpeed = horizSpeed.length();
		//if (curSpeed>0) steer += horizSpeed * ((TARGET_SPEED - curSpeed) / curSpeed);

		// Limit (1)
		float len = steer.length();
		static const float MAX_STEER = 300 * FORCE_ADJUST * mass;
		if (len > MAX_STEER) steer *= MAX_STEER / len;

		// Max speed steering
		static const float MAX_SPEED = 70;
		float speed = ((deltaPos / elapsed) + (steer * elapsed)).length();
		//if (speed >MAX_SPEED) steer += dir * (speed / elapsed);
		if (speed >MAX_SPEED) steer += dir * osg::clampBetween(speed / elapsed * .1f, -MAX_STEER, MAX_STEER);

		// Constant speed steering (horizontal only)
		//static const float TARGET_SPEED = 15;
		//ASSERT(elapsed>0);
		////osg::Vec3 speed(deltaPos / elapsed);
		////speed += steer*elapsed;		// Take into account current steering
		////float curSpeed = speed.length();
		//osg::Vec3 horizSpeed(deltaPos.x() / elapsed, deltaPos.y() / elapsed, 0);
		//horizSpeed += steer*elapsed;		// Take into account current steering
		//float curSpeed = horizSpeed.length();
		//if (curSpeed > TARGET_SPEED) steer += dir * osg::clampBetween((curSpeed-TARGET_SPEED) * .1f, -MAX_STEER, MAX_STEER);

		//// Limit (2)
		//len = steer.length();
		//if (len > MAX_STEER)
		//	steer *= MAX_STEER / len;

		return steer /* * elapsed*/;
	}

protected:
	osg::Vec3 pos, deltaPos;
	osg::Vec3 randomTarget;
	bool randomTargetSteering;
};


void CommonSpeedCB::operator()(BoidSim * boid, float distance) {
	float att = getAttenuation(distance, attenuation);
	if (att>1) att=1;
	steer += static_cast<BoidSimSKD*>(boid)->getDeltaPos() * att;
	weight += att;
}

// ------------------------------------------------------------------------------

/// The concrete utility class, to be used by the main program to act on the boid simulation.
class BoidsUtilSKD : public BoidsUtil {
public:
	static BoidsUtilSKD & instance() {
		// Meyer's singleton
		static BoidsUtilSKD b;
		return b;
	}

	virtual void setBounds(const osg::Vec3 & min, const osg::Vec3 & max) {
		boundMin = min;
		boundMax = max;
		//BOIDS_DEFAULT_TARGET_RADIUS = (boundMax.x()-boundMin.x()) + (boundMax.y()-boundMin.y());			///< Default radius of "detection" for a Target
	}
	virtual void getBounds(osg::Vec3 & min, osg::Vec3 & max) {
		min = boundMin;
		max = boundMax;
	}

	virtual BoidSim * newBoidSim(float mass) { return new BoidSimSKD(mass); }
	virtual void deleteBoidSim(BoidSim * b) { delete b; }

	virtual Target * newTarget(const osg::Vec3 pos, float multiplier, float radius) {
		// TODO TD1 - See listTargets global
	}
	virtual void deleteTarget(Target * t) {
		// TODO TD1 - See listTargets global
	}

	virtual void targetUpdated(Target * target) {
		// Nothing to do for now, as targets parameters are directly read on each simulation step
	}

	// Non copyable
	BoidsUtilSKD(const BoidsUtilSKD &) =delete;
	BoidsUtilSKD & operator=(const BoidsUtilSKD &) =delete;
protected:
	BoidsUtilSKD() {}
};

// TODO TD1 - Plugin entry point
