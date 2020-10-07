//-----------------------------------------------------------------------------------
//
// Boids Tower Defense (based on Paratrooper code)
// Copyright (C) 2007-2008  Sukender
// For more information, contact us : sukender@free.fr
//
// All rights reserved
//
//-----------------------------------------------------------------------------------

#include "Common.h"
#include "Path.h"
#include "Game.h"
//#include "CollisionCategories.h"
//#include <PVLE/3D/Utility3D.h>
#include <PVLE/3D/CameraShake.h>
#include <PVLE/3D/AutoDeleteChildCallback.h>
#include <osg/ImageSequence>

#ifdef PVLE_AUDIO
#	include <osgAudio/SoundNode.h>
#	include <osgAudio/SoundManager.h>
#	include <osgAudio/SoundState.h>
#	include <PVLE/Sound/AutoSoundUpdateCB.h>
#	include <PVLE/Sound/Common.h>
#endif

// Can be put outside the .cpp if necessary.
class StdExplosionParams : public Util::Singleton<StdExplosionParams>, public ExplosionParams {
public:
	StdExplosionParams() {
		maxRadius = 15;
		growingSpeed = 50;
		maxForce = 100;
	}
};


std::shared_ptr<Explosion> createStdExplosion(const osg::Vec3f & pos, const boost::filesystem::path & soundSamplePath, float maxRadius, float growingSpeed, float maxForce, float maxDamage) {
	if (growingSpeed<=0) growingSpeed = maxRadius * 2;
	if (maxForce    < 0) maxForce     = growingSpeed * 5;
	if (maxDamage   < 0) maxDamage    = maxForce / 10000;
	Game * game = PVLEGameHolder::instance().get<Game>();

	ExplosionParams params(StdExplosionParams::instance());
	params.maxRadius = maxRadius;
	params.growingSpeed = growingSpeed;
	params.maxForce = maxForce;
	float invDayLight = 1-game->computeDayLightIntensity();			// 0 = day, 1 = night
	params.lightDiffuse     *= invDayLight*4.f + .6f;		// Lower the light during day to do as if it could not illuminate much more than the sun
	params.lightDiffuse.w() = 1;
	params.diffuseColor.w() *= invDayLight*.2f + .8f;		// Lower the sphere alpha during day (less visible)
	//params.emissionColor    *= invDayLight*.3f + .7f;		// Lower the emissive during day (less "shiny") - Note, scaling the alpha component has no effect
	params.lightLinearAttenuation = 0.005f;
	params.spriteEmission = osg::Vec4f(1, 1, 1, .4f);
	//params.colorAttenuationPow = 1.1f;

	ASSERT(game);
	params.pLightStateSet = game->getParentGroup()->getOrCreateStateSet();

	auto pExplosion = std::make_shared<Explosion>(params, game->getLightSourceManager().request(.5, nullptr, true));
	pExplosion->setPos(pos);
	pExplosion->setQuaternion(osg::Quat(osg::PI_2, osg::X_AXIS));

	// CACHE PRELOAD
	//ASSERT(osgDB::Registry::instance()->getFromObjectCache((AppPaths::textures() / "Explosions/1/Explosion_1.png").string()) != nullptr);		// Object not in cache ?
	osg::ImageSequence * pIS = pExplosion->getAnimation();
	pIS->addImageFile((AppPaths::textures() / "Explosions/1/Explosion_1.png").string());
	pIS->addImageFile((AppPaths::textures() / "Explosions/1/Explosion_2.png").string());
	pIS->addImageFile((AppPaths::textures() / "Explosions/1/Explosion_3.png").string());
	pIS->addImageFile((AppPaths::textures() / "Explosions/1/Explosion_4.png").string());
	pIS->addImageFile((AppPaths::textures() / "Explosions/1/Explosion_5.png").string());
	pIS->addImageFile((AppPaths::textures() / "Explosions/1/Explosion_6.png").string());
	pIS->addImageFile((AppPaths::textures() / "Explosions/1/Explosion_7.png").string());
	pIS->addImageFile((AppPaths::textures() / "Explosions/1/Explosion_8.png").string());

	//for(Physics::Geom * pGeom : pExplosion->getGeoms()) {
	//	dGeomSetCategoryBits(*pGeom, Collide::CANNON_AMMO);		// By default, the explosion is considered to be the "property" of the player
	//	dGeomSetCollideBits(*pGeom, Collide::ALL);
	//}
	//pExplosion->setParam(WeaponEnergy::CINETIC, 10);
	pExplosion->setParam(WeaponEnergy::THERMAL, maxDamage);

#ifdef PVLE_AUDIO
	// Sound
	// CACHE_PRELOAD
	osg::ref_ptr<osgAudio::SoundUpdateCB> pSoundCB = Audio::createStdSoundCB(soundSamplePath, 10, game->getSoundGainFX()*1.33f, true, true);		// Gain for explosions because they're not loud
	if (pSoundCB.valid()) {
		pExplosion->getModel()->addUpdateCallback(pSoundCB.get());
		// Here we also add the sound callback to the Game object because we want the sound to survive the explosion deletion.
		game->addSoundUpdateCB(pSoundCB.get());
	}
#endif


	// Camera shake
	CameraShakeParams csParams;
	csParams.totalTime = maxRadius/growingSpeed;
	csParams.timeFullAmplitude = csParams.totalTime * .2f;
	csParams.amplitude = maxRadius * 40;
	csParams.setAnglesRandom();
	csParams.frequency = 7;
	csParams.dispersionTheta = osg::PI/8;
	csParams.dispersionPhi = csParams.dispersionTheta;
	csParams.distanceModel = CameraShakeParams::DistanceModel::LINEAR;

	osg::ref_ptr<CameraShake> pShake = new CameraShake(csParams);
	game->applyCameraShake(pShake.get());
	pExplosion->getModel()->addChild(pShake);
	pExplosion->getModel()->addUpdateCallback(new AutoDeleteChildCallback<CameraShake>());		// No need to call hasUpdateCallbackType()

	return pExplosion;
}

