//-----------------------------------------------------------------------------------
//
// Boids Tower Defense
// Copyright (C) 2007-2009  Sukender
// For more information, contact us : sukender@free.fr
//
// All rights reserved
//
//-----------------------------------------------------------------------------------

#ifndef APP_CAMERA_CONTROLLER_H
#define APP_CAMERA_CONTROLLER_H

#include <PVLE/Input/Control.h>
#include <PVLE/Input/ControlMapper.h>
#include <PVLE/Input/MatrixGetter.h>

/// Class for RTS-like camera movement
class CameraController : public ControlHandler, public MatrixGetter {
public:
	CameraController(const osg::BoundingBox & bounds);

	virtual bool handleEvent(NetControlEvent * pEvent, TNL::EventConnection * pConnection);
	virtual void handleFrame(double elapsed);

	virtual void setByMatrix(const osg::Matrix & matrix);
	virtual void setByInverseMatrix(const osg::Matrix & matrix);

	virtual osg::Matrix getMatrix() const;
	virtual osg::Matrix getInverseMatrix() const;

protected:
	osg::BoundingBox bounds;

	osg::Vec3 eye, dir, up;
	osg::Vec3 launchTrans;
	osg::Vec3 prevMouseMove;
	float durationWithoutMouseEvents;
	double launchRot;
	double launchZoom;

	enum ERoll {
		ROLL_NONE,
		ROLL_LEFT,
		ROLL_RIGHT
	};
	enum EZoom {
		ZOOM_NONE,
		ZOOM_IN,
		ZOOM_OUT
	};
	void update(float elapsed, osg::Vec3 move, float scrollMove, bool viewMovedWithMouse, ERoll roll, EZoom zoom);
};


#endif	// APP_CAMERA_CONTROLLER_H
