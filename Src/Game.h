//-----------------------------------------------------------------------------------
//
// Boids Tower Defense (based on Paratrooper code)
// Copyright (C) 2007-2008  Sukender
// For more information, contact us : sukender@free.fr
//
// All rights reserved
//
//-----------------------------------------------------------------------------------

#ifndef GAM_GAME_H
#define GAM_GAME_H

#include <PVLE/Game/PVLEGame.h>
#include <PVLE/Util/Assert.h>
#include <PVLE/Util/I18N.h>
#include <PVLE/Util/Callback.h>
#include <PVLE/Physics/World.h>
#include <PVLE/3D/LightSourceManager.h>

#include <osg/observer_ptr>
#include <osg/Vec3>
#include <osg/Camera>
#include <osgText/String>
#include <boost/array.hpp>
#include <vector>
#include <array>

class IGeomCollisionContainer;
class CameraShake;
class Game;

namespace osg {
	class LightSource;
	class TexEnvCombine;
}

#ifdef PVLE_AUDIO
#       include <osgAudio/SoundUpdateCB.h>
#endif

/// Base class for object that can be warned of game events.
class GameCallback : public Util::Callback<GameCallback> {
public:
	virtual void gameStart(Game *) =0;
	virtual void gameOver(Game *) =0;
	virtual void scoreChanged(Game *) =0;
};

#include <PVLE/3D/AutoParticleSystemUpdater.h>

/// Hanldes the game events, generation of bonuses, monsters, etc...
///\version 0 - Not ready yet
class Game : public PVLEGame {
public:
	Game(osg::Group * pParentGroup, Physics::Space * pParentSpace, Physics::World & phyWorld, bool cheatEnabled = false);
	virtual ~Game();

	///\name Lights
	///@{

	/// Different static (eg. assigned for the whole game) light sources the game is supposed to handle (= change).
	/// All other light sources are dynamically allocated by the LightSourceManager.
	enum ELightsIDs {
		LIGHT_DAY,
		//LIGHT_,
		NB_LIGHTS
	};
	BOOST_STATIC_ASSERT(NB_LIGHTS <= 8);		// Else we need a special manager for light sources.

	void setLight(unsigned int num, osg::LightSource * pLightSource) { gameLightSources[num] = pLightSource; updateLights(); }
	void setSkyTexEnvCombine(osg::TexEnvCombine * pSkyTexEnvCombine_) { pSkyTexEnvCombine = pSkyTexEnvCombine_; }

	/// Time of the day, that matches 0 = midnight, .5 = 12h.
	float getLightTime() const;
	/// Computes the intensity of the day light (1=full, 0=night).
	float computeDayLightIntensity() const;
	///@}

	virtual void step(dReal stepSize);

	/// Returns current score (cannot be negative).
	inline unsigned int getScore() const { return score; }

	void setGameCallback(std::shared_ptr<GameCallback> pGameCB_) { pGameCB = pGameCB_; }
	void startGame();		// Starts the game
	void resetGame();		// Call the "game over" callback if needed, and reset all by marking 3DPhy's as removed. You need to call startGame() after the physics and GFX steps have been finished.
	void setGameOver();

	enum EState {
		STATE_STOP,				///< Game is not running but is ready to start.
		STATE_RUN,				///< Game is running normally (can be paused).
		STATE_GAME_OVER			///< Game is not running because it is over (not yet ready to start).
	};
	inline EState getState() { return state; }

	osgParticle::AutoParticleSystemUpdater * getParticlesManager() { return pParticlesManager.get(); }
	const osgParticle::AutoParticleSystemUpdater * getParticlesManager() const { return pParticlesManager.get(); }
	void addNonLocalParticleSystem(osgParticle::ParticleSystem * pPS) { pParticlesManager->addParticleSystem(pPS); }

	LightSourceManager & getLightSourceManager() { return lightSourceManager; }
	const LightSourceManager & getLightSourceManager() const { return lightSourceManager; }

	float getSoundGainFX()    const { return soundGainFX; }
	float getSoundGainMusic() const { return soundGainMusic; }
	void setSoundGainFX(float gain)    { soundGainFX = gain>=0 ? gain : 0; }
	void setSoundGainMusic(float gain) { soundGainMusic = gain>=0 ? gain : 0; }

#ifdef PVLE_AUDIO
	/// Adds a sound callback that will be deleted once the sound callback has ended.
	/// Thus the sound node can survive its "native" parents deletion.
	/// For example, if you create an explosion node and you want its sound to "survive" to the node's deletion, then you create a dummy node with the appropriate sound callback and add it to the game via this method.
	void addSoundUpdateCB(osgAudio::SoundUpdateCB * pSoundCB);
#endif

	inline void addCamera(osg::Camera * pCamera) { vCameras.push_back(pCamera); }

	/// Sets all cameras to be affected by the given CameraShake effect.
	void applyCameraShake(CameraShake * pCameraShake);

protected:
	Physics::World & phyWorld;
	std::weak_ptr<GameCallback> pGameCB;

	// Lights and sky
	std::array<osg::LightSource *, NB_LIGHTS> gameLightSources;
	float lightTime;		///< Time of the "day" for lights (in [0; 1[).
	//float dayTimeMultiplier;
	void updateLights();
	osg::TexEnvCombine * pSkyTexEnvCombine = nullptr;

	EState state = STATE_STOP;

	osg::ref_ptr<osgParticle::AutoParticleSystemUpdater> pParticlesManager;

	inline void setScore(unsigned int _score) {
		score = _score;
	}
	/// Not for removing points !
	inline void addScore(unsigned int deltaScore) { setScore(score + deltaScore); }
	inline void removeScore(unsigned int deltaScore) { setScore(deltaScore>score ? 0 : score - deltaScore); }

	float soundGainFX = 1, soundGainMusic = 1;

#ifdef PVLE_AUDIO
	/// List of sounds that must survive after their "native" parent deletion.
	typedef std::vector<osg::ref_ptr<osgAudio::SoundUpdateCB> > TAbandonnedSoundGroup;
	TAbandonnedSoundGroup abandonnedSoundGroup;
	void cleanAbandonnedSoundGroup();
#endif

	std::vector<osg::observer_ptr<osg::Camera> > vCameras;

private:
	unsigned int score = 1;
	LightSourceManager lightSourceManager;
};


#endif	// GAM_GAME_H
