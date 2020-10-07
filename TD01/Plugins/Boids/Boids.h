///\file
/// Contains abstract classes for the Boid plugin to work.
/// Plugin author must derive from these classes.
/// Please note that all calls in the plugin are synchronous, so no thread-safety has to be acheived.


#ifndef BOIDS_PLUGIN
#define BOIDS_PLUGIN

#include <osg/Vec3>

/// Representation of a height field (terrain with a regular grid of points), for data exchange between executable and plugin.
/// This class is to be used as-is for both executable and plugin.
struct HeightFieldData {
	HeightFieldData() {}

	const float * heights = nullptr;	///< Array of heights (2D regular grid)
	unsigned int columns =0, rows =0;	///< Number of columns/rows in heights array
	float dx=0, dy=0;					///< Interval between each height point
	osg::Vec3 origin;					///< Offset of the whole heightfield

	/// Returns the position in space of a vertex of given line and column.
	/// Calling program is asserted to put the corresponding heightfield in global coordinates (that is to say coordinates given here are not transformed elsewhere).
	inline osg::Vec3 getVertex(unsigned int c,unsigned int r) const {
		return osg::Vec3(
			origin.x()+dx*static_cast<float>(c),
			origin.y()+dy*static_cast<float>(r),
			origin.z()+heights[c+r*columns]);
	}
};



/// Abstract class for a boid element, to be derived in the plugin.
/// A boid is an autonomous entity that may move according to Targets or other BoidSims.
/// The plugin may have two simulation modes (see EMode).
class BoidSim {
public:
	BoidSim(float mass) : mass(mass) {}
	virtual ~BoidSim() {}

	/// Computation modes for the boid plugin
	enum EMode {
		BOIDS_SET_POS,		///< Simulation generates the position of the boid on each frame
		BOIDS_SET_FORCE		///< Simulation generates a force to apply to a 'physics' boid each frame
	};

	virtual EMode getMode() const =0;						///< Gets the computation mode used for this implementation
	virtual void setPos(const osg::Vec3 & pos) =0;			///< Used when creating the boid, or when the boid has been updated by the calling program (used by BOIDS_SET_FORCE, but may also called by BOIDS_SET_POS).
	virtual osg::Vec3 getPos() const =0;					///< Gets the current position of the boid. For BOIDS_SET_FORCE, this should be the same as the last call to setPos().

	/// Returns an updated position or a force, depending on getMode().
	/// The elapsed time is guaranteed to be always the same during a given simulation.
	virtual osg::Vec3 update(const HeightFieldData & hf, float elapsedTime) =0;

protected:
	float mass;		///< Mass of the boid, potentially affecting the simulation.
};



/// Defines a target the boids should follow (or flee), consisting of a position and a multiplier that acts as the 'importance' of the target.
/// This class is to be used as-is for both executable and plugin.
class Target {
public:
	/// Builds a target with its position, its "importance" (multiplier) which may be negative, and the radius after which the target \b should not be taken into account.
	Target(const osg::Vec3 pos, float multiplier, float radius) : pos(pos), multiplier(multiplier), radius(radius) {}

	const osg::Vec3 & getPos() const { return pos; }
	void setPos(const osg::Vec3 & pos_) { pos = pos_; }

	float getMultiplier() const { return multiplier; }
	void setMultiplier(float m) { multiplier = m; }

	float getRadius() const { return radius; }
	void setRadius(float r) { radius = r; }

protected:
	osg::Vec3 pos;			///< Position of the target
	float multiplier;		///< Importance of the target
	float radius;			///< Radius after which the target \b should not be taken into account (this is only an indicator, as the boid algorithm can choose)
};


/// Abstract class for an utility singleton, to be derived in the plugin.
/// This allows the executable to create/delete boids and targets.
class BoidsUtil {
public:
	/// Method for the executable to tell the plugin the size of the world.
	/// This may be slightly different from the size of the terrain (to ensure boids stay on it, for instance).
	virtual void setBounds(const osg::Vec3 & min, const osg::Vec3 & max) =0;
	virtual void getBounds(osg::Vec3 & min, osg::Vec3 & max) =0;

	/// Creates and adds a new boid
	virtual BoidSim * newBoidSim(float mass) =0;
	/// Deletes a given boid
	virtual void deleteBoidSim(BoidSim *) =0;

	/// Creates and adds a target
	virtual Target * newTarget(const osg::Vec3 pos, float multiplier, float radius) =0;
	/// Deletes a given target
	virtual void deleteTarget(Target *) =0;
	/// Tells the system a given target has been updated
	virtual void targetUpdated(Target * target) {}

protected:
	BoidsUtil() {}
	virtual ~BoidsUtil() {}
};

extern "C" {
	/// Type used by the calling app to call getBoidsUtil() at runtime.
	/// The executable expects a "getBoidsUtil()" function with "C" signature, which returns the singleton instance of the derived BoidsUtil.
	typedef BoidsUtil & (*BoidsUtilInstanceFunc)();
	BoidsUtil & getBoidsUtil();	
}

#endif	// BOIDS_PLUGIN
