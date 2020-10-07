//-----------------------------------------------------------------------------------
//
// Pro-Vocation Light Engine (PVLE)
// Copyright (C) 2007-2009  Sukender, KinoX & Buzib
// For more information, contact us : sukender@free.fr
//
// All rights reserved
//
//-----------------------------------------------------------------------------------

///\file
/// Definitions for exporting dynamic library definitions on some compilers.

#ifndef BTD_PLUGIN_EXPORT_H
#define BTD_PLUGIN_EXPORT_H

// Usage of these macros:
// Inside BTD_PLUGIN, define BTD_PLUGIN_LIB_STATIC or BTD_PLUGIN_LIB_DYNAMIC, depending on what you compile (lib or dll/so).
// Outside BTD_PLUGIN, debfine BTD_PLUGIN_LINK_STATIC or nothing, depending on what you link against (lib or dll/so).
#if defined(_MSC_VER) || defined(__CYGWIN__) || defined(__MINGW32__) || defined( __BCPLUSPLUS__)  || defined( __MWERKS__)
	#if defined( btd_boids_LIB_STATIC ) || defined( btd_boids_LINK_STATIC )
	#	define BTD_PLUGIN_EXPORT
	#	define BTD_PLUGIN_EXPORT_TEMPLATE
	#elif defined( btd_boids_EXPORTS )
	#	define BTD_PLUGIN_EXPORT   __declspec(dllexport)
	#	define BTD_PLUGIN_EXPORT_TEMPLATE
	#else
	#	define BTD_PLUGIN_EXPORT   __declspec(dllimport)
	//#	define BTD_PLUGIN_EXPORT
	#	define BTD_PLUGIN_EXPORT_TEMPLATE extern
	#endif
#else
	#define BTD_PLUGIN_EXPORT
	#define BTD_PLUGIN_EXPORT_TEMPLATE
#endif  


#endif	// BTD_PLUGIN_EXPORT_H
