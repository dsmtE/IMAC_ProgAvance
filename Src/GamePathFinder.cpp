//-----------------------------------------------------------------------------------
//
// Boids Tower Defense
// Copyright (C) 2007-2008  Sukender
// For more information, contact us : sukender@free.fr
//
// All rights reserved
//
//-----------------------------------------------------------------------------------

#include "GamePathFinder.h"
#include "Game.h"
#include <algorithm>
#include <boost/foreach.hpp>
#include <boost/scope_exit.hpp>

bool PathFinderCommandSimplePath::onPathFinderModified(GamePathFinder *) { return true; }

void PathFinderCommandSimplePath::onPathFinderReady(PathFinder & pf) {
	pf.compute(start, end);

	boost::lock_guard<boost::mutex> lock(resultMutex);
	done = true;

	// Copy result
	const auto & p = pf.getPredecessors();
	auto iEnd = pf.getEndIndex();
	if (p.empty() || p[iEnd] == iEnd) {
		// No path found
		result.clear();
		return;
	}

	// Count
	auto iStart = pf.getStartIndex();
	unsigned int nbPoints = 0;
	for (auto iCur = iEnd; iCur != iStart; iCur = p[iCur], ++nbPoints) {
		ASSERT(iCur != p[iCur] || iCur == iStart);
	}
	++nbPoints;		// Adding the "start" point

	// Allocate and fill points backwards
	result.resize(nbPoints);
	auto iCurPoint = nbPoints-1;
	for (auto iCur = iEnd; iCur != iStart; iCur = p[iCur], --iCurPoint) {
		ASSERT(iCurPoint>0 || p[iCur] == iStart);
		result[iCurPoint] = pf.getCoordFromDescriptor(iCur);
	}
	result[0] = pf.getCoordFromDescriptor(iStart);
}


void PathFinderCommandSimplePath::reset(int priority_, const osg::Vec3f & start_, const osg::Vec3f & end_) {
	boost::lock_guard<boost::mutex> lock(resultMutex);
	priority = priority_;
	start = start_;
	end = end_;
	done = false;
}



#include <OpenThreads/Thread>

GamePathFinder::GamePathFinder(const osg::HeightField & hf, std::shared_ptr<WeightGetter> wg) : pf(hf, wg), stop(false), triggeringCallbacksRemaining(0) {}

void GamePathFinder::operator()() {
	// Note about thread priority:
	// Initially the GamePathFinder's thread was meant to be a low priority thread. Unfortunately, there is no portable way to do it with C++2011 thread (nor boost.threads).
	// OpenThreads can. But design of this app is to be closer to the C++ "standards" (STL, boost) as possible.
	// Moreover, there are plenty of articles out there that say that tweaking threads priority may have bad effects.
	// Anyway, to acheive this properly, there is a need for 3rd-party lib or #if's.
	// For reference: http://en.cppreference.com/w/cpp/thread/thread/native_handle
	//
	//auto thisThread = OpenThreads::Thread::CurrentThread();
	//thisThread->setSchedulePriority(OpenThreads::Thread::THREAD_PRIORITY_LOW);		// No portable way to do it with boost...
	//int basePriority = thisThread->getSchedulePriority();

	//osg::Timer_t timerStart = osg::Timer::instance()->tick();

	for(; !stop; ) {
		std::shared_ptr<PathFinderCommand> command;
		{ // Block for mutex lock
			boost::unique_lock<boost::recursive_mutex> lock(queueMutex);
			for (; queue.empty(); ) {
				//LOG_DEBUG_INFO << "GamePathFinder goes to sleep" << std::endl;
				emptyQueueBlock.wait(lock);
				if (stop) return;
			}

			// Process command queue
			ASSERT(!queue.empty());
			LOG_DEBUG_INFO << "GamePathFinder processes a command (queue size=" << queue.size() << ")" << std::endl;

			// Update age, and find the command to be processed
			auto timerCur = osg::Timer::instance()->tick();
			std::vector<std::shared_ptr<PathFinderCommand>>::iterator highestPriorityCommand;
			double curPriority, highestPriority = -1;
			double maxAge = 0;
			for (auto it=queue.begin(), itEnd=queue.end(); it!=itEnd; ++it) {
				// Update age
				auto curCommand = it->get();
				curCommand->age = osg::Timer::instance()->delta_s((*it)->insertionDate, timerCur);
				osg::clampAbove(maxAge, curCommand->age);
				// Get the command to be executed
				curPriority = curCommand->getCurrentPriority();
				if (curPriority > highestPriority) {
					highestPriority = curPriority;
					highestPriorityCommand = it;
				}
			}

			//// If oldest command not is too aged, then raise the priority of the thread, else restore the priority of the thread
			//if (basePriority >= 0) {
			//	static const double BOOST_THREAD_AGE = .1;		// 1/10th sec
			//	if (maxAge >= BOOST_THREAD_AGE) {
			//		if (basePriority > OpenThreads::Thread::THREAD_PRIORITY_MAX && basePriority <= OpenThreads::Thread::THREAD_PRIORITY_MIN)
			//			thisThread->setSchedulePriority(static_cast<OpenThreads::Thread::ThreadPriority>(basePriority-1));		// -1 is LOWER priority (see ThreadPriority enum)
			//	}
			//	else thisThread->setSchedulePriority(static_cast<OpenThreads::Thread::ThreadPriority>(basePriority));
			//}

			// Process command
			command = *highestPriorityCommand;
			ASSERT(command);
			queue.erase(highestPriorityCommand);		// Remove from queue so that we can unlock the queue during computation
		} // Unlock mutex
		if (stop) return;							// Test before launching a (potentially) long command
		command->onPathFinderReady(pf);
	}
}

void GamePathFinder::triggerGraphModified() {
	// A callback calling this (in operator()) may destroy itself and/or induce looping this function.
	// This is not the case now, but code should handle that (tricky) case.
	// TODO handle self-destruction of a command

	// If looping, exit (after incrementing the counter)
	if (++triggeringCallbacksRemaining >1) return;
	// Loop until there are no "triggers" remaining
	for(;;) {
		// Cancel orders
		{
			boost::lock_guard<boost::recursive_mutex> lock(queueMutex);
			queue.erase(std::remove_if(queue.begin(), queue.end(), [this](std::shared_ptr<PathFinderCommand> & c) {
				return c->onPathFinderModified(this);
			}), queue.end());
		}
		// Trigger callbacks
		if (PVLEGameHolder::instance().get<Game>()->getState() != Game::STATE_STOP) {		// Do not trigger callbacks when game is stopped
			boost::lock_guard<boost::mutex> lock(cbMutex);
			for(auto & cb : callbacks) cb->operator()(*this);
		}
		ASSERT(triggeringCallbacksRemaining>0);
		if (--triggeringCallbacksRemaining == 0) break;		// Condition not in the for() loop to avoid testing it at first iteration
		LOG_DEBUG_INFO << BOOST_CURRENT_FUNCTION << " looping" << std::endl;
	}
}


void GamePathFinder::pop(PathFinderCommand * command) {
	if (!command) return;
	boost::lock_guard<boost::recursive_mutex> lock(queueMutex);
	//auto it = std::find(queue.begin(), queue.end(), command);
	auto it = std::find_if(queue.begin(), queue.end(), [command](const Queue::value_type & v) {
		return v.get() == command;
	});
	if (it!=queue.end()) queue.erase(it);
}


void GamePathFinder::removeModifiedCallback(PathFinderModifyCB * cb) {
	if (!cb) return;
	boost::lock_guard<boost::recursive_mutex> lock(queueMutex);
	//for(auto it = std::find(callbacks.begin(), callbacks.end(), cb); it != callbacks.end(); it = std::find(callbacks.begin(), callbacks.end(), cb)) {
	//	callbacks.erase(it);
	//}
	callbacks.erase(std::remove_if(callbacks.begin(), callbacks.end(), [cb](std::shared_ptr<PathFinderModifyCB> & cur) {
		return cur.get() == cb;
	}), callbacks.end());
}

// TODO TD2

osg::Vec3 addSizedObstacle(GamePathFinder & gpf, const osg::Vec3 & center, float size) {
	GamePathFinder::Lock lock(gpf);		// Thread safety for gpf.get()
	osg::Vec3 graphPoint = gpf.get().addSizedObstacle(center, size, true);		// Temporary obstacle
	gpf.triggerGraphModified();

// TODO TD2

	return graphPoint;
}
