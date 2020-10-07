//-----------------------------------------------------------------------------------
//
// Boids Tower Defense
// Copyright (C) 2007-2008  Sukender
// For more information, contact us : sukender@free.fr
//
// All rights reserved
//
//-----------------------------------------------------------------------------------

#include "PathFinder.h"
#include <PVLE/Util/Math.h>
#include <float.h>
#include <boost/graph/dijkstra_shortest_paths_no_color_map.hpp>
#include <boost/graph/astar_search.hpp>
#include <boost/mem_fn.hpp>
#include <boost/bind.hpp>



/// Algorithm that calls a functor for all points in a projected circle (all points within a given radius).
template <class T>
void doSized(const osg::HeightField & hf, unsigned int col, unsigned int row, float radiusInCells, T & functor) {
	ASSERT(radiusInCells>0);
	ASSERT(hf.getRotation() == osg::Quat());			// Algorithm is simple and can't handle rotation yet
	ASSERT(hf.getXInterval() == hf.getYInterval());		// Algorithm is simple and can't handle non-squares yet
	float radius = radiusInCells*hf.getXInterval();		// In units

	// Determine min/max values of the for() loops
	unsigned int startC = osg::clampAbove( static_cast<int>( floor(col - radiusInCells) ), 0 );
	unsigned int startR = osg::clampAbove( static_cast<int>( floor(row - radiusInCells) ), 0 );
	unsigned int endC = osg::clampBelow  ( static_cast<int>( ceil (col + radiusInCells) ), static_cast<int>(hf.getNumColumns()-1) );
	unsigned int endR = osg::clampBelow  ( static_cast<int>( ceil (row + radiusInCells) ), static_cast<int>(hf.getNumRows()   -1) );

	// Apply functor
	osg::Vec3 center = hf.getVertex(col, row);
	for(unsigned int c=startC; c<=endC; ++c) {
		for(unsigned int r=startR; r<=endR; ++r) {
			if ( (hf.getVertex(c, r) - center).length() <= radius ) functor(c, r);
		}
	}
}

/// Algorithm that calls a functor for all points in a projected square (only points at the edge).
///\return true if at least one edge has been processed, false if all edges are outside the heightfield.
template <class T>
bool doSquare(const osg::HeightField & hf, unsigned int col, unsigned int row, int radiusInCells, T & functor) {
	ASSERT(radiusInCells>0);
	ASSERT(hf.getRotation() == osg::Quat());			// Algorithm is simple and can't handle rotation yet
	ASSERT(hf.getXInterval() == hf.getYInterval());		// Algorithm is simple and can't handle non-squares yet
	//float radius = radiusInCells*hf.getXInterval();		// In units

	int theoricalLeft  = static_cast<int>(col) - radiusInCells;
	int theoricalUp    = static_cast<int>(row) - radiusInCells;
	int theoricalRight = static_cast<int>(col) + radiusInCells;
	int theoricalDown  = static_cast<int>(row) + radiusInCells;

	bool okLeft  = theoricalLeft >= 0;
	bool okUp    = theoricalUp   >= 0;
	bool okRight = theoricalRight<= static_cast<int>(hf.getNumColumns()-1);
	bool okDown  = theoricalDown <= static_cast<int>(hf.getNumRows()   -1);

	if (!(okLeft || okUp || okRight || okDown)) return false;

	// Determine min/max values of the for() loops
	unsigned int startC = okLeft  ? theoricalLeft : 0;
	unsigned int startR = okUp    ? theoricalUp   : 0;
	unsigned int endC   = okRight ? theoricalRight: static_cast<int>(hf.getNumColumns()-1);
	unsigned int endR   = okDown  ? theoricalDown : static_cast<int>(hf.getNumRows()   -1);

	// Apply functor
	if (okLeft) {
		for(unsigned int r=startR; r<=endR; ++r) functor(startC, r);
	}
	if (okRight) {
		for(unsigned int r=startR; r<=endR; ++r) functor(endC, r);
	}
	if (okUp) {
		for(unsigned int c=startC+1; c<=endC-1; ++c) functor(c, startR);
	}
	if (okDown) {
		for(unsigned int c=startC+1; c<=endC-1; ++c) functor(c, endR);
	}
	return true;
}

/// Algorithm that calls a functor for all vertices around a point.
template <class T>
void doSurroundingVertices(const osg::HeightField & hf, unsigned int col, unsigned int row, T & functor) {
	unsigned int maxR = hf.getNumRows()-1;
	unsigned int maxC = hf.getNumColumns()-1;
	bool okBorderN = row>0;
	bool okBorderS = row<maxR;
	bool okBorderE = col<maxC;
	bool okBorderW = col>0;
	if (okBorderN)       functor(col, row, col  , row-1);		// North
	if (okBorderE) {
		if (okBorderN)   functor(col, row, col+1, row-1);		// N-E
		/*No condition*/ functor(col, row, col+1, row  );		// East
		if (okBorderS)   functor(col, row, col+1, row+1);		// S-E
	}
	if (okBorderS)       functor(col, row, col  , row+1);		// South
	if (okBorderW) {
		if (okBorderS)   functor(col, row, col-1, row+1);		// S-W
		/*No condition*/ functor(col, row, col-1, row  );		// West
		if (okBorderN)   functor(col, row, col-1, row-1);		// N-W
	}
}



PathFinder::PathFinder(const osg::HeightField & hf, std::shared_ptr<WeightGetter> wg) : g(hf.getNumColumns() * hf.getNumRows()), hf(hf), wg(wg) {
	using namespace boost;

	// Retreive the weight map
	weightmap = get(edge_weight, g);

	// For each vertice of the graph (corresponds to vertices of the heightfield),
	// add edges to all surrounding cells
	for(unsigned col = 0; col < hf.getNumColumns(); ++col) {
		for(unsigned row = 0; row < hf.getNumRows(); ++row) {
			// Call addEdgeAndWeight(x1, y1, x2, y2) on surrounding vertices
			EdgeAddFunctor temp(*this);		// Needed for gcc
			doSurroundingVertices(hf, col, row, temp);
		}
	}

	// Give the result structures the appropriate size
	unsigned int reserve = num_vertices(g);
	p.resize(reserve);
	d.resize(reserve);
	cleanResult();
}

// TODO TD2


#include <osg/Timer>
void PathFinder::compute(vertex_descriptor start_, vertex_descriptor end_, bool useHeuristic) {
	start = start_;
	end = end_;
	if (start == end) {
		// Path length is 0.
		cleanResult();
		return;
	}

	// Launch main algorithm
osg::Timer_t startTime = osg::Timer::instance()->tick();
useHeuristic = false;		// Force
	if (useHeuristic) {
		// TODO TD2
		// Use heuristic algorithm
	} else {
		// Use exact algorithm
		boost::dijkstra_shortest_paths_no_color_map(g, start, boost::predecessor_map(&p[0]).distance_map(&d[0]));
	}
LOG_ALWAYS << "PathFinder computed path in " << osg::Timer::instance()->delta_m(startTime, osg::Timer::instance()->tick()) << " ms" << std::endl;
}


void PathFinder::getClosestPointCoords(const osg::Vec3 & point, unsigned int & col, unsigned int & row) const {
	// Note: this method used to be more compact, but I (Sukender) added multiple intermediate named variables to allow debugging :)
	ASSERT(hf.getRotation() == osg::Quat());			// Algorithm is simple and can't handle rotation yet
	auto nbCols = hf.getNumColumns();
	auto nbRows = hf.getNumRows();
	auto corner1 = hf.getVertex(0,0);
	auto corner2 = hf.getVertex(nbCols-1,nbRows-1);
	auto pointHFRef = point-corner1;		// Point in HF coordinates, ie. using vertex(0,0) as origin.
	auto delta = corner2-corner1;
	ASSERT(delta.x() >0 && delta.y() >0);
	auto cellSizeX = delta.x() / (nbCols-1);
	auto cellSizeY = delta.y() / (nbRows-1);

	// HeightField algo makes x <=> col, y <=> row
	col = osg::clampBetween<unsigned int>( static_cast<unsigned int>(round(pointHFRef.x() / cellSizeX)), 0, nbCols-1 );
	row = osg::clampBetween<unsigned int>( static_cast<unsigned int>(round(pointHFRef.y() / cellSizeY)), 0, nbRows-1 );
}


void PathFinder::setMaxDistanceBidir(unsigned int v1, unsigned int v2) {
	edge_descriptor e; bool exists;
	tie(e, exists) = edge(v1, v2, g);
	if (exists) weightmap[e] = FLT_MAX;
	tie(e, exists) = edge(v2, v1, g);
	if (exists) weightmap[e] = FLT_MAX;
}


void PathFinder::restoreDistanceUnidir(unsigned int col1, unsigned int row1, unsigned int col2, unsigned int row2) {
	unsigned int v1 = getVertexDescriptor(col1, row1);
	unsigned int v2 = getVertexDescriptor(col2, row2);
	float distance = computeDistance(col1, row1, col2, row2);

	edge_descriptor e; bool exists;
	tie(e, exists) = edge(v1, v2, g);
	if (exists) {
		// Edge exists: put the appropriate weight
		if (distance >= 0) weightmap[e] = distance;
		else weightmap[e] = FLT_MAX;
	} else {
		// Edge doesn't exists: create and fill it
		if (distance >= 0) {
			bool inserted;
			tie(e, inserted) = add_edge(getVertexDescriptor(col1, row1), getVertexDescriptor(col2, row2), g);
			ASSERT(inserted);		// We did not use a set or map (but a list or vector)
			weightmap[e] = distance;
		}
	}
}


void PathFinder::addSizedObstacle(unsigned int col, unsigned int row, float radiusInCells, bool temporary) {
	// Call addObstacle(x, y, temporary) on cells within a radius
	AddObstacleFunctor temp(*this, temporary);
	doSized(hf, col, row, radiusInCells, temp);
}


void PathFinder::removeSizedObstacle(unsigned int col, unsigned int row, float radiusInCells) {
	// Call removeObstacle(x, y) on cells within a radius
	RemoveObstacleFunctor temp(*this);
	doSized(hf, col, row, radiusInCells, temp);
}


void PathFinder::addObstacle(unsigned int col, unsigned int row, bool temporary) {
	if (temporary) {
		// Temporary obstacle: set distances to FLT_MAX
		// Call setMaxDistanceBidir(x1, y1, x2, y2) on surrounding vertices
		SetMaxDistanceBidirFunctor temp(*this);
		doSurroundingVertices(hf, col, row, temp);
	}
	// "Permanent" (=long term) obstacle: clear the vertex' edges
	else {
		vertex_descriptor v = vertex(getVertexDescriptor(col, row), g);
		boost::clear_vertex(v, g);			// Invalidates iterators only for edges
		//boost::remove_vertex(v, g);		// Removal of the point is impossible here as we get them by index
	}
}

void PathFinder::removeObstacle(unsigned int col, unsigned int row) {
	// Temporary or not, the restoration of the edges call the same method
	// Call restoreDistanceBidir(x1, y1, x2, y2) on surrounding vertices
	RestoreDistanceBidirFunctor temp(*this);
	doSurroundingVertices(hf, col, row, temp);
}


/// Dumb incrementor
template<typename T>
class IncGenerator {
public:
	IncGenerator(T start) : i(start) {}
	T operator()() { return i++; }
protected:
	T i;
};

void PathFinder::cleanResult() {
	std::generate(p.begin(), p.end(), IncGenerator<int>(0));
	std::generate(d.begin(), d.end(), IncGenerator<int>(0));
}

void PathFinder::HasEdgeGoingFrom::operator()(unsigned int col1, unsigned int row1, unsigned int col2, unsigned int row2) {
	if (found) return;
	edge_descriptor e; bool exists;
	tie(e, exists) = edge(pf.getVertexDescriptor(col1, row1), pf.getVertexDescriptor(col2, row2), pf.g);
	if (exists) {
		// Test if length is not infinite
		if (pf.weightmap[e] != FLT_MAX) found = true;
	}
}


void PathFinder::FindClosestFreePointFunctor::operator()(unsigned int col, unsigned int row) {
	if (found) return;
	//PathFinder::HasEdgeGoingFrom temp(pf, col, row);
	PathFinder::HasEdgeGoingFrom temp(pf);
	doSurroundingVertices(pf.getHeightField(), col, row, temp);
	if (temp.result()) {
		found = true;
		resultCol = col;
		resultRow = row;
	}
}


std::pair<bool, unsigned int> PathFinder::findClosestFreePoint(unsigned int col, unsigned int row, unsigned int startRadiusInCells, unsigned int endRadiusInCells) {
	FindClosestFreePointFunctor finder(*this);
	for(bool squareOk = true; squareOk && startRadiusInCells <= endRadiusInCells; ++startRadiusInCells) {
		squareOk = doSquare(hf, col, row, startRadiusInCells, finder);
		if (finder.hasFound()) {
			return std::pair<bool, unsigned int>( true, getVertexDescriptor(finder.getResultCol(), finder.getResultRow()) );
		}
	}
	return std::pair<bool, unsigned int>(false, 0);
}


std::pair<bool, osg::Vec3> PathFinder::findClosestFreePoint(osg::Vec3 pos, float startRadius) {
	unsigned int col, row;
	getClosestPointCoords(pos, col, row);
	auto radiusInCells = std::max(1U, static_cast<unsigned int>( startRadius/hf.getXInterval() ));		// Get floor value here
	std::pair<bool, unsigned int> result = findClosestFreePoint(col, row, radiusInCells);

	if (!result.first) return std::pair<bool, osg::Vec3>( false, osg::Vec3() );
	return std::pair<bool, osg::Vec3>( true, getCoordFromDescriptor(result.second) );
}



#include <osg/Geometry>
#include <osg/LineWidth>
#include <osg/Depth>
#include <osg/Geode>
#include <boost/foreach.hpp>

osg::Geode * drawPath(const std::vector<osg::Vec3> & path) {
	if (path.size() <=1) return nullptr;

	osg::Geode * geode = new osg::Geode;
	osg::Geometry* geom = new osg::Geometry;
	geode->addDrawable(geom);

	// Fill vertices coordinates
	osg::Vec3Array* coords = new osg::Vec3Array(path.size());
	geom->setVertexArray(coords);

	const osg::Vec3 drawOffset(0,0,5);
	osg::Vec3Array::iterator it = coords->begin();
	for(const auto & point : path) {
		*it = point + drawOffset;
		++it;
	}

	// Setup color
	osg::Vec4Array* color = new osg::Vec4Array(1);
	(*color)[0] = osg::Vec4(1,1,0,1);
	geom->setColorArray(color);
	geom->setColorBinding(osg::Geometry::BIND_OVERALL);

	// Setup the primitive
	geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP,0,path.size()));

	// Setup stateset for rendering
	osg::StateSet* pStateset = geom->getOrCreateStateSet();
	pStateset->setAttributeAndModes(new osg::LineWidth(2));
	pStateset->setAttributeAndModes(new osg::Depth(osg::Depth::LEQUAL));
	pStateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

	return geode;
}

osg::Geode * drawPath(const PathFinder & pf) {
	const std::vector<PathFinder::vertex_descriptor> & p = pf.getPredecessors();
	//const osg::HeightField & hf = pf.getHeightField();
	unsigned int iStart = pf.getStartIndex();
	unsigned int iEnd = pf.getEndIndex();

	if (p.empty() || p[iEnd] == iEnd) {
		// No path found
		return nullptr;
	}

	// Counting lines
	unsigned int nbLinePoints = 0;
	for (unsigned int iCur = p[iEnd]; iCur != iStart; iCur = p[iCur], ++nbLinePoints) {
		ASSERT(iCur != p[iCur] || iCur == iStart);
	}
	if (nbLinePoints<=1) return nullptr;		// 1 point only?

	// TEST - Draw spheres:
	//osg::TessellationHints * th = new osg::TessellationHints;
	//th->setDetailRatio(.01f);
	//for (auto iCur = p[iEnd]; iCur != iStart; iCur = p[iCur]) {
	//	geode->addDrawable( new osg::ShapeDrawable(new osg::Sphere(getCoordFromDescriptor(iCur) , 5), th) );
	//	ASSERT(iCur != p[iCur] || iCur == iStart);
	//}

	std::vector<osg::Vec3> path(nbLinePoints);
	unsigned int iCurPoint = 0;
	for (auto iCur = p[iEnd]; iCur != iStart; iCur = p[iCur], ++iCurPoint) {
		path[iCurPoint] = pf.getCoordFromDescriptor(iCur);
		ASSERT(iCur != p[iCur] || iCur == iStart);
	}

	return drawPath(path);
}
