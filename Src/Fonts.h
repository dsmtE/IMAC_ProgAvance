//-----------------------------------------------------------------------------------
//
// Boids Tower Defense (based on Paratrooper code)
// Copyright (C) 2007-2008  Sukender
// For more information, contact us : sukender@free.fr
//
// All rights reserved
//
//-----------------------------------------------------------------------------------

#ifndef FONTS_H
#define FONTS_H

#include <PVLE/Util/Singleton.h>
#include <osgText/Font>

//namespace osgText {
//	class Font;
//}

/// Singleton containing fonts for the app.
/// The fonts must be loaded upon app initialization.
class Fonts : public Util::Singleton<Fonts> {
public:
	osg::ref_ptr<osgText::Font> pStandard;		///< Standard font
};


#endif	// FONTS_H
