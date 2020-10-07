//-----------------------------------------------------------------------------------
//
// Boids Tower Defense (based on Paratrooper code)
// Copyright (C) 2007-2009  Sukender
// For more information, contact us : sukender@free.fr
//
// All rights reserved
//
//-----------------------------------------------------------------------------------

#ifndef APP_DEV_HUD_H
#define APP_DEV_HUD_H

#include <PVLE/Entity/Hud.h>

//#include <PVLE/Input/Control.h>
#include <PVLE/Input/ControlMapper.h>
//#include <PVLE/Input/MatrixGetter.h>

#include "App.h"

/// A HUD for developpement (not clean nor documented!).
class DevHud : public Hud {
public:
	DevHud(unsigned int left, unsigned int right, unsigned int bottom, unsigned int top, App & app);
	virtual void onProjectionChanged(unsigned int left, unsigned int right, unsigned int bottom, unsigned int top);
	virtual void updateCB(osg::Node *node, osg::NodeVisitor *nv);

	void setSubText(const std::string & str) {
		if (str.empty()) subText.clear();
		else subText = "\n" + str;
	}

protected:
	unsigned int FPS = 0;
	App & app;
	double elapsed = 0, lastTime = -1;
	osg::Geode* pGeode = nullptr;				///< Avoids de-referencing on projection change.
	osgText::Text * pText = nullptr;			///< Avoids de-referencing on each frame.
	std::string subText;
};


class DevHudCEV : public ControlEventHandler {
public:
	DevHudCEV(App & app) : app(app) {}
	virtual bool handleEvent(NetControlEvent * pEvent, TNL::EventConnection * pConnection);
protected:
	App & app;
};

#endif	// APP_DEV_HUD_H
