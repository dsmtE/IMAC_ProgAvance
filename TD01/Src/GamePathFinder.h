//-----------------------------------------------------------------------------------
//
// Boids Tower Defense
// Copyright (C) 2007-2009  Sukender
// For more information, contact us : sukender@free.fr
//
// All rights reserved
//
//-----------------------------------------------------------------------------------

#ifndef APP_GAME_PATH_FINDER_H
#define APP_GAME_PATH_FINDER_H

#include "PathFinder.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/lock_guard.hpp>
#include <vector>
#include <osg/Timer>
#include <atomic>

class GamePathFinder;

/// Command given to a threaded PathFinder (GamePathFinder actually), to be inserted in a queue and then processed.
class PathFinderCommand {
public:
	enum EPriority {
		PRIORITY_MAKE_OBSTACLE_PERMANENT = 1,	///< Priority used when updating an obstable from temporary to permanent
		PRIORITY_UPDATE_PATH = 10,				///< Priority used when updating a path
		PRIORITY_NEW_PATH = 100,				///< Priority used when asking for a new path
		PRIORITY_OBSTACLE = 1000				///< Priority used when modifying the graph, especially with an obstacle
	};

	PathFinderCommand(int priority) : priority(priority), insertionDate(0) {}

	/// Method called back by GamePathFinder when the PathFinder is ready to accept a call to a function reading or modifying the underlying graph (such as compute()).
	///\warning The call is in the GamePathFinder thread.
	virtual void onPathFinderReady(PathFinder & pf) =0;

	/// Method called back by GamePathFinder when the PathFinder was modified by an external command (ex: added an obstacle) BEFORE the execution (onPathFinderReady()).
	///\return true if the path finder should delete this command (= NOT execute it), false if the command should still be processed.
	///\warning The call is in the GamePathFinder thread.
	virtual bool onPathFinderModified(GamePathFinder * gpf) =0;

	/// Allows sorting (used in a priority queue).
	bool operator<(const PathFinderCommand & v) const { return getCurrentPriority() < v.getCurrentPriority(); }

	/// Gets the current priority, depending on the base priority and the age of the command.
	double getCurrentPriority() const {
		static const unsigned int AGE_MULTIPLIER = 1000;
		return priority+age*age*AGE_MULTIPLIER;
	}

protected:
	int priority;				///< Base priority
	double age;					///< Value used for the queue to make older commands to have a higher priority
	osg::Timer_t insertionDate;	///< Value used to update age

	friend class GamePathFinder;		// To access insertionDate, age, and done
};


/// Path finder command that asks for a path and copies the result into a local vector.
/// Usage: Simply check if isDone()==true and then obtain the path using getResult().
class PathFinderCommandSimplePath : public PathFinderCommand {
public:
	PathFinderCommandSimplePath() : PathFinderCommand(PRIORITY_UPDATE_PATH), done(false) {}
	PathFinderCommandSimplePath(int priority, const osg::Vec3f & start, const osg::Vec3f & end) : PathFinderCommand(priority), start(start), end(end), done(false) {}

	virtual void onPathFinderReady(PathFinder & pf);
	virtual bool onPathFinderModified(GamePathFinder *);

	/// Checks if the path finder has ended the computation
	bool isDone() const {
		boost::lock_guard<boost::mutex> lock(resultMutex);
		return done;
	}

	/// Gives the result, only if isDone()==true.
	std::vector<osg::Vec3> & getResult() {
		boost::lock_guard<boost::mutex> lock(resultMutex);
		ASSERT(done);
		return result;
	}

	/// Resets the command before sending it once again to the path finder thread.
	void reset(int priority, const osg::Vec3f & start, const osg::Vec3f & end);

protected:
	osg::Vec3 start, end;		///< Path point

	mutable boost::mutex resultMutex;			///< Lock for reading/writing result and done
	std::vector<osg::Vec3> result;				///< Resulting path
	bool done;
};

// TODO TD2


/// Adds immediately a temporary sized obstacle to the pathfinder, and then adds asynchroneously a permanent one.
/// Makes acceptable the use of permanent obstacles in real-time.
osg::Vec3 addSizedObstacle(GamePathFinder & gpf, const osg::Vec3 & center, float size);


/// Callback used when the graph is modified (Should be used to recompute paths).
/// All posted commands are lost when this callback is triggered; that is to say you may have to re-ask the graph to compute a path.
///\warning Take care to make this thread safe!
///\todo Replace with template / non-virtual class.
class PathFinderModifyCB {
public:
	virtual void operator()(GamePathFinder &) =0;
};



/// Runnable path finder (ie. functor to run in a thread), which accepts a queue of commands to be processed.
/// Commands are treated according to their priority and age (time they've been waiting in the queue).
///\todo Check if inheritance from PathFinder would be a good idea. Also check if merging both classes is a good idea (with virtual function calls, to allow locking/unlocking automatically).
class GamePathFinder {
public:
	/// Builds a path finder thread with same parameters as the "normal" path finder.
	GamePathFinder(const osg::HeightField & hf, std::shared_ptr<WeightGetter> wg);

	/// Gets the internal PathFinder object, to be used synchroneously \b BEFORE the thread has been started (Not thread safe), or using Lock class.
	///\warning Thread safety of read/write operations on the PathFinder are left to the caller.
	///\warning The caller \b must call triggerGraphModified() once the write operations on the PathFinder are done.
	PathFinder & get() {
		//boost::lock_guard<boost::recursive_mutex> lock(queueMutex);
		return pf;
	}

	/// Manually tells the thread to notify modification callbacks (this is not done automatically).
	///\sa get()
	///\note Thread-safe.
	void triggerGraphModified();

	/// Enqueues a command to be processed by the thread.
	/// Commands are not guaranteed to be processed in a given order since their base priority and age are taken into account (see PathFinderCommand::getCurrentPriority()).
	///\note Thread-safe.
	void push(std::shared_ptr<PathFinderCommand> command) {
		if (!command) return;
		{
			boost::lock_guard<boost::recursive_mutex> lock(queueMutex);
			queue.push_back(command);
		}
		emptyQueueBlock.notify_all();		// Does nothing if already unlocked
	}

	/// Removes a command from the queue if it exists.
	///\note Thread-safe.
	void pop(PathFinderCommand * command);

	/// Adds a callback to be triggered when the graph is modified.
	///\note Thread-safe.
	void addModifiedCallback(std::shared_ptr<PathFinderModifyCB> cb) {
		if (!cb) return;
		boost::lock_guard<boost::mutex> lock(cbMutex);
		callbacks.push_back(cb);
	}

	/// Removes a callback from the class (should be called in destructors).
	///\note Thread-safe.
	void removeModifiedCallback(PathFinderModifyCB * cb);

	//void synchDo()		// Will be implemented later (?)
	//void pause() {}		// Will be implemented later

	/// Tells the thread to stop its loop and softly exit.
	///\note Thread-safe.
	void softStop() {
		stop = true;
		emptyQueueBlock.notify_all();		// wakeup if waiting for a non empty queue - Does nothing if already unlocked
	}

	void operator()();

	/// Lock (using reentrant mutex) for path finder (get()) access.
	class Lock {
	public:
		explicit Lock(GamePathFinder & gpf) : gpf(gpf) { gpf.queueMutex.lock(); }
		~Lock() { gpf.queueMutex.unlock(); }

		// Non copyable
		Lock(const Lock &) =delete;
		Lock & operator=(const Lock &) =delete;
	private:
		GamePathFinder & gpf;
	};
	
protected:
	friend class Lock;

	typedef std::vector< std::shared_ptr<PathFinderCommand> > Queue;
	Queue queue;		///< Commands queue
	PathFinder pf;		///< Internal path finder object
	std::atomic<bool> stop /*= false*/;		///< Should we stop the thread?

	std::vector< std::shared_ptr<PathFinderModifyCB> > callbacks;
	std::atomic<unsigned int> triggeringCallbacksRemaining /*= 0*/;		// Avoids infinite looping when calling callbacks. Associated mutex: cbMutex.

	boost::recursive_mutex queueMutex;	///< Mutex for queue and pf concurrent access
	boost::mutex cbMutex;		///< Mutex for callbacks concurrent access
	boost::condition_variable_any emptyQueueBlock;		///< Used to make the thread sleep when the queue is empty
};


#endif	// APP_GAME_PATH_FINDER_H
