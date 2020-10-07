//-----------------------------------------------------------------------------------
//
// Boids Tower Defense
// Copyright (C) 2007-2009  Sukender
// For more information, contact us : sukender@free.fr
//
// All rights reserved
//
//-----------------------------------------------------------------------------------

#ifndef APP_PATH_FINDER_H
#define APP_PATH_FINDER_H

#include <osg/Shape>		// For the HeightField
#include <PVLE/Util/Assert.h>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>


/// Functor that computes an edge weight for PathFinder.
///\todo Replace with template / non virtual class.
class WeightGetter {
public:
	virtual float operator()(const osg::Vec3 & offset) =0;
};


/// Default weight computation for PathFinder.
class DefaultWeightGetter : public WeightGetter {
public:
	virtual float operator()(const osg::Vec3 & offset) {
		float penalty = offset.z()>0 ? offset.z() : 0;
		return offset.length() + penalty;
	}
};



/// HeightField-based path finder class, to find a way on a terrain and through obstacles.
class PathFinder {
public:
	/// Graph type
	// We shouldn't use boost::vecS here as a vertex removal would invalidate all iterators to vertices (same with edges). However this completely changes access-by-index strategy.
	//typedef boost::adjacency_list < boost::setS, boost::vecS, boost::directedS, boost::no_property, boost::property < boost::edge_weight_t, float > > graph_t;
	typedef boost::adjacency_list < boost::listS, boost::vecS, boost::directedS, boost::no_property, boost::property < boost::edge_weight_t, float > > graph_t;
	//typedef boost::adjacency_list < boost::listS, boost::listS, boost::directedS, boost::no_property, boost::property < boost::edge_weight_t, float > > graph_t;
	//typedef boost::adjacency_list < boost::hash_setS, boost::hash_setS, boost::directedS, boost::no_property, boost::property < boost::edge_weight_t, float > > graph_t;

	/// Edge type
	typedef boost::graph_traits < graph_t >::edge_descriptor edge_descriptor;
	/// Vertex type
	typedef boost::graph_traits < graph_t >::vertex_descriptor vertex_descriptor;

	// --------------------------------------------------------------------------------

	/// Builds a pathfinder from a heightfield.
	PathFinder(const osg::HeightField & hf, std::shared_ptr<WeightGetter> wg);

	/// Updates the object so that it stores results for a path between two points.
	/// Given points are rounded to closest graph nodes.
	void compute(const osg::Vec3f & start_, const osg::Vec3f & end_, bool useHeuristic = true) { compute(getVertexDescriptor(start_), getVertexDescriptor(end_), useHeuristic); }

	/// Updates the object so that it stores results for a path between two on the graph (column/row reference).
	void compute(unsigned int iStartCol, unsigned int iStartRow, unsigned int iEndCol, unsigned int iEndRow, bool useHeuristic = true) { compute(getVertexDescriptor(iStartCol, iStartRow), getVertexDescriptor(iEndCol, iEndRow), useHeuristic); }

	/// Updates the object so that it stores results for a path between two on the graph (index reference).
	void compute(vertex_descriptor iStart, vertex_descriptor iEnd, bool useHeuristic = true);

	osg::Vec3f getClosestPoint(const osg::Vec3f & point) {
		unsigned int col, row;
		getClosestPointCoords(point, col, row);
		return hf.getVertex(col, row);
	}

	/// Removes all graph connections from and to the given point.
	/// Temporary obstacles are \b much faster to add and remove, but they don't speed up the computation (they actually don't remove vertices but mark them with a huge distance).
	void addObstacle(unsigned int col, unsigned int row, bool temporary=true);

	/// Remove all graph connections from and to the point closest to the given one (projected on the floor).
	/// Temporary obstacles are \b much faster to add and remove, but they don't speed up the computation (they actually don't remove vertices but mark them with a huge distance).
	/// Given point is rounded to closest graph node.
	///\return The point used (corresponds to a graph point), which is the closest from the given one
	osg::Vec3f addObstacle(const osg::Vec3f & point, bool temporary=true) {
		unsigned int col, row;
		getClosestPointCoords(point, col, row);
		addObstacle(col, row, temporary);
		return hf.getVertex(col, row);
	}

	/// Rewires the graph to remove a one-point obstacle.
	void removeObstacle(unsigned int col, unsigned int row);

	/// Rewires the graph to remove a one-point obstacle.
	/// Given point is rounded to closest graph node.
	void removeObstacle(const osg::Vec3f & point) {
		unsigned int col, row;
		getClosestPointCoords(point, col, row);
		removeObstacle(col, row);
	}

	/// Remove all graph connections from and to points inside a circle (projected on the floor), in "cells" (elements of the heightfield, not real size).
	void addSizedObstacle(unsigned int col, unsigned int row, float radiusInCells, bool temporary=true);

	/// Remove all graph connections from and to points inside a circle (projected on the floor).
	/// Given point is rounded to closest graph node.
	///\warning, this method may actually do nothing if radius is too small and point is not exact.
	osg::Vec3f addSizedObstacle(const osg::Vec3f & point, float radius, bool temporary=true) {
		ASSERT(radius>0);
		unsigned int col, row;
		getClosestPointCoords(point, col, row);
		addSizedObstacle(col, row, radius/hf.getXInterval(), temporary);
		return hf.getVertex(col, row);
	}

	/// Rewires the graph to remove an obstacle within a projected circle.
	void removeSizedObstacle(unsigned int col, unsigned int row, float radiusInCells);

	/// Rewires the graph to remove an obstacle within a projected circle.
	/// Given point is rounded to closest graph node.
	void removeSizedObstacle(const osg::Vec3f & point, float radius) {
		ASSERT(radius>0);
		unsigned int col, row;
		getClosestPointCoords(point, col, row);
		removeSizedObstacle(col, row, radius/hf.getXInterval());
	}

	/// Returns the heightfield used within the path finder.
	const osg::HeightField & getHeightField() const { return hf; }

	/// Returns the vertex of a height field point from the index used in the graph (converts from index to vertex).
	//osg::Vec3 getCoordFromDescriptor(unsigned int num) const {
	//	unsigned int col, row;
	//	getCoordFromDescriptor(num, col, row);
	//	return hf.getVertex(col, row);
	//}
	///\overload
	//osg::Vec3 getCoordFromDescriptor(vertex_descriptor vertexId) const { return getCoordFromDescriptor(static_cast<unsigned int>(vertexId)); }
	osg::Vec3 getCoordFromDescriptor(vertex_descriptor vertexId) const {
		unsigned int col, row;
		getCoordFromDescriptor(vertexId, col, row);
		return hf.getVertex(col, row);
	}

	/// \name Results
	///@{

	/// Gets the result (path found after call to compute()) in as a predecessor list.
	const std::vector<vertex_descriptor> & getPredecessors() const { return p; }

	/// Gets the index of the start point used in compute().
	vertex_descriptor getStartIndex() const { return start; }
	/// Gets the index of the end point used in compute().
	vertex_descriptor getEndIndex() const { return end; }

	///@}

	std::pair<bool, unsigned int> findClosestFreePoint(unsigned int col, unsigned int row, unsigned int startRadiusInCells = 1, unsigned int endRadiusInCells = INT_MAX);
	std::pair<bool, osg::Vec3>    findClosestFreePoint(osg::Vec3 pos, float startRadius = 0);

protected:
	/// Weight structure
	typedef boost::property_map<graph_t, boost::edge_weight_t>::type WeightMap;

	graph_t g;							///< Graph used for computations
	const osg::HeightField & hf;		///< Heightfield to work with
	WeightMap weightmap;				///< Edges weight (= distance)
	std::shared_ptr<WeightGetter> wg;	///< Functor that defines weight (used in graph construction, and when an obstacle is removed).

	// Results
	vertex_descriptor start, end;			///< Start/End point of the computation
	std::vector<vertex_descriptor> p;		///< Predecessor list (See graph documentation)
	std::vector<float> d;					///< Distances list

	/// Returns the weight to be applied in graph search, or <0 if the path is blocked.
	float computeDistance(unsigned int col1, unsigned int row1, unsigned int col2, unsigned int row2) const {
		osg::Vec3 offset = hf.getVertex(col2, row2) - hf.getVertex(col1, row1);
		return (*wg)(offset);
	}

	/// Sets two edges (back and forth between 2 points) to have an 'infinite' distance.
	void setMaxDistanceBidir(unsigned int v1, unsigned int v2);

	/// Sets two edges (back and forth between 2 points) to have an 'infinite' distance.
	void setMaxDistanceBidir(unsigned int col1, unsigned int row1, unsigned int col2, unsigned int row2) {
		setMaxDistanceBidir(getVertexDescriptor(col1, row1), getVertexDescriptor(col2, row2));
	}

	/// Restores computed weights of two edges (back and forth between 2 points).
	void restoreDistanceBidir(unsigned int col1, unsigned int row1, unsigned int col2, unsigned int row2) {
		restoreDistanceUnidir(col1, row1, col2, row2);
		restoreDistanceUnidir(col2, row2, col1, row1);
	}

	/// Restores computed weights of one edge (From point 1 to point 2 only).
	void restoreDistanceUnidir(unsigned int col1, unsigned int row1, unsigned int col2, unsigned int row2);

	/// Returns the index of a height field point, to be used in the graph
	vertex_descriptor getVertexDescriptor(unsigned int col, unsigned int row) const {
		ASSERT(col < hf.getNumColumns());
		ASSERT(row < hf.getNumRows());
		return static_cast<vertex_descriptor>(col+row*hf.getNumColumns());
	}

	/// Returns the index of the closest height field point, to be used in the graph
	vertex_descriptor getVertexDescriptor(const osg::Vec3 & point) const {
		unsigned int col, row;
		getClosestPointCoords(point, col, row);
		return getVertexDescriptor(col, row);
	}

	/// Returns the column and row of the closest height field point
	void getClosestPointCoords(const osg::Vec3 & point, unsigned int & col, unsigned int & row) const;

	/// Returns the col/row of a height field point from the index used in the graph
	void getCoordFromDescriptor(vertex_descriptor vertexId, unsigned int & col, unsigned int & row) const {
		unsigned int numCol = hf.getNumColumns();
		auto num = static_cast<unsigned int>(vertexId);
		row = num / numCol;
		col = num - row*numCol;
	}

	/// Add a weighted edge to the graph
	void addEdgeAndWeight(unsigned int col1, unsigned int row1, unsigned int col2, unsigned int row2) {
		float distance = computeDistance(col1, row1, col2, row2);
		if (distance >= 0) {
			edge_descriptor e; bool inserted;
			tie(e, inserted) = add_edge(getVertexDescriptor(col1, row1), getVertexDescriptor(col2, row2), g);
			ASSERT(inserted);		// We did not use a set or map (but a list or vector)
			weightmap[e] = distance;
		}
	}

	/// Removes an edge from the graph (much slower than changing the edge weight)
	void removeEdge(unsigned int col1, unsigned int row1, unsigned int col2, unsigned int row2) {
		boost::remove_edge(getVertexDescriptor(col1, row1), getVertexDescriptor(col2, row2), g);
	}

	void cleanResult();

	class EdgeAddFunctor {
	public:
		EdgeAddFunctor(PathFinder & pf) : pf(pf) {}
		void operator()(unsigned int col, unsigned int row, unsigned int col2, unsigned int row2) { pf.addEdgeAndWeight(col, row, col2, row2); }
	protected:
		PathFinder & pf;
	};

	class AddObstacleFunctor {
	public:
		AddObstacleFunctor(PathFinder & pf, bool temporary) : pf(pf), temporary(temporary) {}
		void operator()(unsigned int col, unsigned int row) { pf.addObstacle(col, row, temporary); }
	protected:
		PathFinder & pf;
		bool temporary;
	};

	class RemoveObstacleFunctor {
	public:
		RemoveObstacleFunctor(PathFinder & pf) : pf(pf) {}
		void operator()(unsigned int col, unsigned int row) { pf.removeObstacle(col, row); }
	protected:
		PathFinder & pf;
	};

	class SetMaxDistanceBidirFunctor {
	public:
		SetMaxDistanceBidirFunctor(PathFinder & pf) : pf(pf) {}
	//	void operator()(unsigned int col, unsigned int row) { pf.setMaxDistanceBidir(col, row); }
		void operator()(unsigned int col, unsigned int row, unsigned int col2, unsigned int row2) { pf.setMaxDistanceBidir(col, row, col2, row2); }
	protected:
		PathFinder & pf;
	};

	class RestoreDistanceBidirFunctor {
	public:
		RestoreDistanceBidirFunctor(PathFinder & pf) : pf(pf) {}
		void operator()(unsigned int col, unsigned int row, unsigned int col2, unsigned int row2) { pf.restoreDistanceBidir(col, row, col2, row2); }
	protected:
		PathFinder & pf;
	};

	class HasEdgeGoingFrom {
	public:
		HasEdgeGoingFrom(PathFinder & pf) : pf(pf), found(false) {}
		void operator()(unsigned int col1, unsigned int row1, unsigned int col2, unsigned int row2);
		bool result() const { return found; }
	protected:
		PathFinder & pf;
		bool found;
	};

	/// Functor that searches for an edge that has a finite-distance edge that goes away from it.
	class FindClosestFreePointFunctor {
	public:
		FindClosestFreePointFunctor(PathFinder & pf /*, unsigned int rowInit, unsigned int colInit*/) : pf(pf)/*, rowInit(rowInit), colInit(colInit)*/, found(false) {}
		void operator()(unsigned int col, unsigned int row);
		bool hasFound() const { return found; }
		unsigned int getResultCol() const { return resultCol; }
		unsigned int getResultRow() const { return resultRow; }

	protected:
		PathFinder & pf;
		//unsigned int rowInit, colInit;
		bool found;
		unsigned int resultCol, resultRow;
	};
};


namespace osg {
	class Geode;
}

/// Creates a line to show a path.
osg::Geode * drawPath(const std::vector<osg::Vec3> & path);

/// Convenience function that creates a line to show a computed path directly from the pathfinder's result.
///\warning The call isn't thread safe.
osg::Geode * drawPath(const PathFinder & pf);


#endif	// APP_PATH_FINDER_H
