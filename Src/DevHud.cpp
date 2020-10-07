//-----------------------------------------------------------------------------------
//
// Boids Tower Defense (based on Paratrooper code)
// Copyright (C) 2007-2008  Sukender
// For more information, contact us : sukender@free.fr
//
// All rights reserved
//
//-----------------------------------------------------------------------------------

#include "DevHud.h"
#include "Constants.h"
#include "Game.h"
#include "Fonts.h"
//#include <PVLE/3D/Utility3D.h>
#include <PVLE/Util/I18N.h>
//#include <osg/MatrixTransform>
#include <osgText/Text>
#include <boost/format.hpp>

DevHud::DevHud(unsigned int left_, unsigned int right_, unsigned int bottom_, unsigned int top_, App & app_)
	: Hud(left_, right_, bottom_, top_), app(app_)
{
	pGeode = new osg::Geode();
	pRoot2D->addChild(pGeode);
	auto localText = createText(Fonts::instance().pStandard.get(), 20, fromCornerLT(10, 10), osg::Vec4f(1,0.1f,0.1f,1));
	pText = localText.get();
	pText->setAlignment(osgText::Text::LEFT_TOP);
	pText->setText(APP_NAME);
	pGeode->addDrawable(pText);
}

void DevHud::onProjectionChanged(unsigned int left_, unsigned int right_, unsigned int bottom_, unsigned int top_) {
	Hud::onProjectionChanged(left_, right_, bottom_, top_);

	ASSERT(pGeode->getNumDrawables() == 1);
	auto localText = createText(Fonts::instance().pStandard.get(), 20, fromCornerLT(10, 10), osg::Vec4f(1,0.1f,0.1f,1));
	pText = localText.get();
	pText->setAlignment(osgText::Text::LEFT_TOP);
	pText->setText(APP_NAME);
	pGeode->setDrawable(0, pText);		// This would deallocate the old text
}

void DevHud::updateCB(osg::Node *node, osg::NodeVisitor *nv) {
	// Here we use getReferenceTime() (kind of "real time clock" for stats) and not getSimulationTime() (time for all nodes, animations, etc.).
	if (lastTime <0) {
		lastTime = nv->getFrameStamp()->getReferenceTime();
		return;
	}
	++FPS;
	elapsed = nv->getFrameStamp()->getReferenceTime() - lastTime;

	if (elapsed >= .5) {
		// FPS and mode display
		std::string strGameMode;
		unsigned int curFPS = static_cast<unsigned int>(FPS / elapsed);
		if (curFPS >= app.getMaxFPS() * .95f && app.getMaxFPS()!=0) {
#ifdef _DEBUG
			pText->setText(getAppName() + strGameMode + " - " + format_str(_("%1% FPS"), curFPS) + " (Max)" + subText, I18N::getEncoding());
#else
			pText->setText(getAppName() + strGameMode, I18N::getEncoding());
#endif
		} else
			pText->setText(getAppName() + strGameMode + " - " + format_str(_("%1% FPS"), curFPS) + subText, I18N::getEncoding());
		FPS = 0;
		lastTime = nv->getFrameStamp()->getReferenceTime();
	}
}

//void DevHud::setSubText(const std::string str) {
//	if (str.empty()) subText.clear();
//	else subText = "\n" + str;
//}

bool DevHudCEV::handleEvent(NetControlEvent * pEvent, TNL::EventConnection * pConnection) {
	ControlEvent::EEventType evType = pEvent->getType();			// Not used everywhere, but the code is easier to read like that.
	//Game * game = PVLEGameHolder::instance().get<Game>();
	if (evType == ControlEvent::SWITCH_DOWN) {
		ControlState::SwitchId switchId = pEvent->getSwitchId();
		if (switchId == ControlState::TOGGLE_FULLSCREEN) {
			app.toggleFullscreen();
			return true;
		}
	}
	return false;
}
