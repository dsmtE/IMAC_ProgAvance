//-----------------------------------------------------------------------------------
//
// Boids Tower Defense
// Copyright (C) 2007-2008  Sukender
// For more information, contact us : sukender@free.fr
//
// All rights reserved
//
//-----------------------------------------------------------------------------------

#include "Laser.h"
#include "Units.h"
//#include <osg/Geode>

//#include <osg/LineWidth>
//#include <osg/Depth>

#include <osg/CullFace>
#include <osg/BlendFunc>
#include <osg/Billboard>
#include <osgDB/ReadFile>
#include <PVLE/3D/Commons.h>
#include <PVLE/3D/Utility3D.h>
#include "Path.h"

#ifdef PVLE_AUDIO
#	include <osgAudio/SoundNode.h>
#	include <osgAudio/SoundManager.h>
#	include <osgAudio/SoundState.h>
#	include <PVLE/Sound/AutoSoundUpdateCB.h>
#	include <PVLE/Sound/Common.h>
#	include "Common.h"
#	include "Game.h"
#endif


//Laser::Laser(const osg::Vec3 & start, const osg::Vec3 & end, UnpilotedUnit * hitUnit, float energy, const osg::Vec4 & color, float ttl) {
Laser::Laser(const osg::Vec3 & start_, const osg::Vec3 & end_, UnpilotedUnit * hitUnit, float energy, float ttl) : start(start_), end(end_) {
	setTTL(ttl);

	// Affect target
	if (hitUnit) hitUnit->isHitByRadiant(energy, end);

	// Create model

	// Old code that creates a simple (and ugly) red line
	//pModel = new osg::MatrixTransform;
	//osg::Geode* geode = new osg::Geode;
	//pModel->addChild(geode);
	//osg::Geometry* geom = new osg::Geometry;
	//geode->addDrawable(geom);

	//const unsigned int nbLinePoints = 2;
	//osg::Vec3Array* coords = new osg::Vec3Array(nbLinePoints);
	//geom->setVertexArray(coords);

	////unsigned int iCurPoint = 0;
	////for (auto iCur = p[iEnd]; iCur != iStart; iCur = p[iCur], ++iCurPoint) {
	////	(*coords)[iCurPoint] = pf.getCoordFromDescriptor(iCur);
	////}
	//(*coords)[0] = start;
	//(*coords)[1] = end;

	//osg::Vec4Array* colorArray = new osg::Vec4Array(1);
	//(*colorArray)[0] = color;
	//geom->setColorArray(colorArray);
	//geom->setColorBinding(osg::Geometry::BIND_OVERALL);

	//geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP,0,nbLinePoints/*-1*/));

	//osg::StateSet* pStateset = geom->getOrCreateStateSet();
	//pStateset->setAttributeAndModes(new osg::LineWidth(3));
	//pStateset->setAttributeAndModes(new osg::Depth(osg::Depth::LEQUAL));
	//pStateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

	{
		pModel = new osg::MatrixTransform;

		auto laserGeode = new osg::Billboard;
		pModel->addChild(laserGeode);
		laserGeode->setMode(osg::Billboard::AXIAL_ROT);
		laserGeode->setAxis(-osg::Y_AXIS);
		laserGeode->setNormal(osg::Z_AXIS);
		{
			// CACHE PRELOAD
			// Create with dummy coordinates. set() will change it afterwards
			auto image = osgDB::readRefImageFile((AppPaths::textures() / "Laser.png"));
			auto geometry = createSquare(osg::Vec3(), osg::Vec3(), osg::Vec3(), image.get());
			osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array(4);
			(*colors)[2] = osg::Vec4(1,1,1,.2f);		// Transparent
			(*colors)[3] = osg::Vec4(1,1,1,.2f);		// Transparent
			(*colors)[0] = osg::Vec4(1,1,1,1);
			(*colors)[1] = osg::Vec4(1,1,1,1);
			geometry->setColorArray(colors.get());
			geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
			laserGeode->addDrawable(geometry.get());
			//setColor(laserGeode, osg::Vec4f(1, 1, 1, 1));
			auto stateset = laserGeode->getOrCreateStateSet();
			stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
			stateset->setAttributeAndModes(new osg::BlendFunc(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE));	// Additive
			stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
			//stateset->setAttribute(new osg::CullFace());
			stateset->setAttributeAndModes(new osg::CullFace(osg::CullFace::FRONT_AND_BACK), osg::StateAttribute::OFF);
		}
		set(start, end);
	}


#ifdef PVLE_AUDIO
	// Sound
	// CACHE_PRELOAD
	Game * game = PVLEGameHolder::instance().get<Game>();
	osg::ref_ptr<osgAudio::SoundUpdateCB> pSoundCB = Audio::createStdSoundCB(AppPaths::sounds() / "30935__aust_paul__possiblelazer.wav", 10, game->getSoundGainFX(), true, true);
	if (pSoundCB.valid()) {
		pModel->addUpdateCallback(pSoundCB.get());
		// Here we also add the sound callback to the Game object because we want the sound to survive the deletion.
		game->addSoundUpdateCB(pSoundCB.get());
	}
#endif
}

void Laser::set(const osg::Vec3 & start_, const osg::Vec3 & end_) {
	start = start_;
	end = end_;
	const auto diff = end_ - start_;
	const float WIDTH = 15.f;
	const auto len = diff.length();

	auto laserGeode = boost::polymorphic_downcast<osg::Billboard *>(pModel->getChild(0));
	auto geometry = boost::polymorphic_downcast<osg::Geometry *>(laserGeode->getDrawable(0));
	updateSquare(geometry, osg::Vec3f(WIDTH / 2, len / 2, 0), osg::Vec3f(-WIDTH, 0, 0), osg::Vec3f(0, -len, 0));

	pModel->setMatrix(osg::Matrix::translate(osg::Vec3f(0, -len / 2, 0)) * osg::Matrix::rotate(-osg::Y_AXIS, diff) * osg::Matrix::translate(start));
	//pModel->setMatrix(osg::Matrix::rotate( -osg::Y_AXIS, diff ) );
}
