//-----------------------------------------------------------------------------------
//
// Boids Tower Defense
// Copyright (C) 2007-2008  Sukender
// For more information, contact us : sukender@free.fr
//
// All rights reserved
//
//-----------------------------------------------------------------------------------

#include "CameraController.h"
#include <PVLE/Util/Math.h>

const int BASE_HEIGHT = 500;		///< Reference Z position

//const float MIN_MOUSE_MOVEMENT = 0.1f;
const float MIN_MOUSE_MOVEMENT = 0;
const float MOUSE_FILTER_TIME = 0.1f;
const float MOUSE_MOVE_SENSITIVITY = 1;
const float MOUSE_ZOOM_SENSITIVITY = 2;

const double ACCELARATION = 30;			// Acceleration is linear with time
const double DECELERATION = 10;			// Deceleration is linear with time
const double SPEED_DECELERATION = .1;	// "Deceleration speed" is inverse linear with speed
const double MAX_SPEED = 10;
const double FAST_MOVE_COEF = 10;

const double ROT_ACCELARATION = 1;
const double ROT_DECELERATION = .4;
const double ROT_MAX_SPEED = .08;

const double ZOOM_ACCELARATION = 20;
const double ZOOM_DECELERATION = 20;
const double ZOOM_MAX_SPEED = 12;
const double MIN_ZOOM = 150;			// BASE_HEIGHT * .5
const double MAX_ZOOM = 1500;		// BASE_HEIGHT * 10



CameraController::CameraController(const osg::BoundingBox & bounds) : bounds(bounds), eye(0,0,BASE_HEIGHT), dir(0,1,-2), up(0,0,1), durationWithoutMouseEvents(FLT_MAX), launchRot(0), launchZoom(0) {
	dir.normalize();
	up.normalize();
	update(0, osg::Vec3(), 0, false, ROLL_NONE, ZOOM_NONE);
}


bool CameraController::handleEvent(NetControlEvent * pEvent, TNL::EventConnection * pConnection) {
	return false;
}


void CameraController::setByMatrix(const osg::Matrix & matrix) {
	eye = matrix.getTrans();			// TODO This is temporary. Write the appropriate method...

	//osg::Matrix mat = osg::Matrix::rotate(osg::PI_2, osg::Vec3(1,0,0)) * matrix;
	//eye = mat.getTrans();
	//dir = osg::Y_AXIS * mat;
	//up = osg::Z_AXIS * mat;
}

void CameraController::setByInverseMatrix(const osg::Matrix & matrix) {
	setByMatrix(osg::Matrix::inverse(matrix));
}

osg::Matrix CameraController::getMatrix() const {
	//dir.normalize();
	//up.normalize();
	ASSERT(fabs(dir.length()) -1 < 1e-6f);
	ASSERT(fabs(up.length()) -1 < 1e-6f);
	return osg::Matrix::rotate(-osg::PI_2, osg::Vec3(1,0,0)) * osg::Matrix::inverse(osg::Matrix::lookAt(eye, eye+dir, up));
}

osg::Matrix CameraController::getInverseMatrix() const {
	//dir.normalize();
	//up.normalize();
	ASSERT(fabs(dir.length()) -1 < 1e-6f);
	ASSERT(fabs(up.length()) -1 < 1e-6f);
	return osg::Matrix::lookAt(eye, eye+dir, up) * osg::Matrix::rotate(osg::PI_2, osg::Vec3(1,0,0));
}


///\todo Split the code into smaller methods
void CameraController::handleFrame(double elapsed) {
	ASSERT(pControler);
	ControlState & cs = pControler->getControlState();

	// Camera translation
	osg::Vec3 move;
	bool viewMovedWithMouse = cs[ControlState::VIEW1];
	if (viewMovedWithMouse) {
		move.x() = cs.getAxis(ControlState::AXIS_X);
		move.y() = cs.getAxis(ControlState::AXIS_Y);
		//if (move == osg::Vec3()) {
		if (move.length2() < MIN_MOUSE_MOVEMENT*MIN_MOUSE_MOVEMENT) {
			durationWithoutMouseEvents += elapsed;
			if (durationWithoutMouseEvents < MOUSE_FILTER_TIME) move = prevMouseMove;
			else move = osg::Vec3();
		} else durationWithoutMouseEvents = 0;
		if (move != osg::Vec3()) {
			move *= MOUSE_MOVE_SENSITIVITY;
			//const osg::Vec3 moveBeforeAdjust(move);
			//float len = move.length();
			//float prevLen = prevMouseMove.length();
			//if (len < prevLen*.5f) move = (move + prevMouseMove*.5f) / 1.5f;		// Smooth movement if there's a big drop
			//prevMouseMove = moveBeforeAdjust;
		}
		prevMouseMove = move;
	}
	if (!viewMovedWithMouse) {
		if (cs[ControlState::LEFT]) move.x() = -1;
		else if (cs[ControlState::RIGHT]) move.x() = 1;
		if (cs[ControlState::UP]) move.y() = 1;
		else if (cs[ControlState::DOWN]) move.y() = -1;
		move.normalize();
		if (cs[ControlState::FAST_TOGGLE]) move *= FAST_MOVE_COEF;
	}
	float scrollMove = cs.getAxis(ControlState::AXIS_SCROLL_1) * MOUSE_ZOOM_SENSITIVITY;

	ERoll roll = ROLL_NONE;
	EZoom zoom = ZOOM_NONE;
	if (cs[ControlState::ROLL_LEFT]) roll = ROLL_LEFT;
	else if (cs[ControlState::ROLL_RIGHT]) roll = ROLL_RIGHT;
	if (cs[ControlState::ZOOM_IN]) zoom = ZOOM_IN;
	else if (cs[ControlState::ZOOM_OUT]) zoom = ZOOM_OUT;

	update(elapsed, move, scrollMove, viewMovedWithMouse, roll, zoom);

	// Ask mouse pointer to be re-centered if necessary
	ControlMapper * cm = dynamic_cast<ControlMapper*>(pControler);
	if (cm) {
		if (viewMovedWithMouse) cm->disablePointerMouse();
		else cm->enablePointerMouse();
	}
}

void CameraController::update(float elapsed, osg::Vec3 move, float scrollMove, bool viewMovedWithMouse, ERoll roll, EZoom zoom) {
	float currentZoomLevel = eye.z() / BASE_HEIGHT;

	// Turn the translation according to dir (projected on Z=0)
	osg::Vec3 projectedDir(dir.x(), dir.y(), 0);
	projectedDir.normalize();
	osg::Matrix horizRotation = osg::Matrix::rotate(osg::Y_AXIS, projectedDir);
	move = move * horizRotation;

	if (viewMovedWithMouse) {
		double prevSpeed = launchTrans.length();
		double len = static_cast<double>(move.length());
		//double newSpeed = osg::clampBelow(len, MAX_SPEED*currentZoomLevel);
		double newSpeed = osg::clampBelow(len, MAX_SPEED*currentZoomLevel * FAST_MOVE_COEF);
		if (newSpeed > prevSpeed*.33) launchTrans = move * (newSpeed / len);
		else if (prevSpeed > 0) {
			newSpeed = osg::clampAbove(prevSpeed*pow(SPEED_DECELERATION, static_cast<double>(elapsed)) - elapsed*DECELERATION*currentZoomLevel, 0.);
			launchTrans *= newSpeed / prevSpeed;
		}
	} else {
		if (move != osg::Vec3()) {
			// User is moving the camera
			//move.normalize();
			double prevSpeed = launchTrans.length();
			double newSpeed = osg::clampBelow(prevSpeed + elapsed*ACCELARATION*currentZoomLevel, MAX_SPEED*currentZoomLevel);
			launchTrans = move * newSpeed;
			// TODO avoid accelerating when going the opposite direction of the current one
		} else {
			double prevSpeed = launchTrans.length();
			if (prevSpeed > 0) {
				double newSpeed = osg::clampAbove(prevSpeed*pow(SPEED_DECELERATION, static_cast<double>(elapsed)) - elapsed*DECELERATION*currentZoomLevel, 0.);
				launchTrans *= newSpeed / prevSpeed;
			}
		}
	}
	eye += launchTrans;

	eye.x() = osg::clampBetween(eye.x(), static_cast<osg::Vec3::value_type>(bounds.xMin()), static_cast<osg::Vec3::value_type>(bounds.xMax()));
	eye.y() = osg::clampBetween(eye.y(), static_cast<osg::Vec3::value_type>(bounds.yMin()), static_cast<osg::Vec3::value_type>(bounds.yMax()));

	// Rotation
	if (roll == ROLL_LEFT) {
		launchRot = osg::clampBelow(launchRot + elapsed * ROT_ACCELARATION, ROT_MAX_SPEED);
	}
	else if (roll == ROLL_RIGHT) {
		launchRot = osg::clampAbove(launchRot - elapsed * ROT_ACCELARATION, -ROT_MAX_SPEED);
	}
	else {
		if (launchRot >= 0) {
			launchRot = osg::clampAbove(launchRot - elapsed * ROT_DECELERATION, 0.);
		} else {
			launchRot = osg::clampBelow(launchRot + elapsed * ROT_DECELERATION, 0.);
		}
	}
	dir = dir * osg::Matrix::rotate(launchRot, osg::Z_AXIS);

	// Zoom
	if (zoom == ZOOM_IN) {
		launchZoom = osg::clampBelow(launchZoom - elapsed * ZOOM_ACCELARATION, -ZOOM_MAX_SPEED);
	}
	else if (zoom == ZOOM_OUT) {
		launchZoom = osg::clampAbove(launchZoom + elapsed * ZOOM_ACCELARATION, ZOOM_MAX_SPEED);
	}
	else if (scrollMove != 0) {
		launchZoom = osg::clampBetween(launchZoom + scrollMove * 20 * elapsed * ZOOM_ACCELARATION, -ZOOM_MAX_SPEED, ZOOM_MAX_SPEED);
	}
	else {
		if (launchZoom >= 0) {
			launchZoom = osg::clampAbove(launchZoom - elapsed * ZOOM_DECELERATION, 0.);
		} else {
			launchZoom = osg::clampBelow(launchZoom + elapsed * ZOOM_DECELERATION, 0.);
		}
	}
	eye.z() = osg::clampBetween(eye.z() + launchZoom, MIN_ZOOM, MAX_ZOOM);
	//currentZoomLevel = eye.z() / BASE_HEIGHT;

	// Tilt the camera according to currentZoomLevel
	//dir = dir * osg::Matrix::rotate(1/currentZoomLevel * elapsed * .01f, osg::X_AXIS * horizRotation);
	//float targetAngle = 1/currentZoomLevel;
	//dir = osg::Vec3(dir.x(), dir.y(), 0) * osg::Matrix::rotate(1/currentZoomLevel, osg::X_AXIS * horizRotation);

	dir.z() = -osg::clampBetween(sigmoid(currentZoomLevel*2.f-.2f), 0.2f, .98f);
	dir.normalize();

	currentZoomLevel = eye.z() / BASE_HEIGHT;
}
