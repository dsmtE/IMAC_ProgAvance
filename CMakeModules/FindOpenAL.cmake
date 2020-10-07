# Locate OpenAL
# This module defines XXX_FOUND, XXX_INCLUDE_DIRS and XXX_LIBRARIES standard variables
#
# $OPENALDIR is an environment variable that would
# correspond to the ./configure --prefix=$OPENALDIR
# used in building OpenAL.
#
# Created by Eric Wing. This was influenced by the FindSDL.cmake module.
# Modified by Sukender (Benoit Neil) to find debug version of the library (for OpenAL-Soft)

# This makes the presumption that you are include al.h like
# #include "al.h"
# and not 
# #include <AL/al.h>
# The reason for this is that the latter is not entirely portable.
# Windows/Creative Labs does not by default put their headers in AL/ and 
# OS X uses the convention <OpenAL/al.h>.
# If not, you can SET(OPENAL_USE_AL_SUBDIR ON) for "AL/al.h"
# 
# For Windows, Creative Labs seems to have added a registry key for their 
# OpenAL 1.1 installer. I have added that key to the list of search paths,
# however, the key looks like it could be a little fragile depending on 
# if they decide to change the 1.00.0000 number for bug fix releases.
# Also, they seem to have laid down groundwork for multiple library platforms
# which puts the library in an extra subdirectory. Currently there is only
# Win32 and I have hardcoded that here. This may need to be adjusted as 
# platforms are introduced.
# The OpenAL 1.0 installer doesn't seem to have a useful key I can use.
# I do not know if the Nvidia OpenAL SDK has a registry key.
# 
# For OS X, remember that OpenAL was added by Apple in 10.4 (Tiger). 
# To support the framework, I originally wrote special framework detection 
# code in this module which I have now removed with CMake's introduction
# of native support for frameworks.
# In addition, OpenAL is open source, and it is possible to compile on Panther. 
# Furthermore, due to bugs in the initial OpenAL release, and the 
# transition to OpenAL 1.1, it is common to need to override the built-in
# framework. 
# Per my request, CMake should search for frameworks first in
# the following order:
# ~/Library/Frameworks/OpenAL.framework/Headers
# /Library/Frameworks/OpenAL.framework/Headers
# /System/Library/Frameworks/OpenAL.framework/Headers
#
# On OS X, this will prefer the Framework version (if found) over others.
# People will have to manually change the cache values of 
# OPENAL_LIBRARY to override this selection or set the CMake environment
# CMAKE_INCLUDE_PATH to modify the search paths.

IF(OPENAL_USE_AL_SUBDIR)
	SET(OPENAL_HEADER_NAMES "AL/al.h")
	SET(OPENAL_HEADER_SUFFIXES include include/OpenAL)
ELSE()
	SET(OPENAL_HEADER_NAMES "al.h")
	SET(OPENAL_HEADER_SUFFIXES include/AL include/OpenAL include)
ENDIF()

SET(OPENAL_SEARCH_PATHS
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local
	/usr
	/sw # Fink
	/opt/local # DarwinPorts
	/opt/csw # Blastwave
	/opt
	[HKEY_LOCAL_MACHINE\\SOFTWARE\\Creative\ Labs\\OpenAL\ 1.1\ Software\ Development\ Kit\\1.00.0000;InstallDir]
)

FIND_PATH(OPENAL_INCLUDE_DIR
	NAMES ${OPENAL_HEADER_NAMES}
	HINTS
	$ENV{OPENALDIR}
	$ENV{OPENAL_PATH}
	PATH_SUFFIXES ${OPENAL_HEADER_SUFFIXES}
	PATHS ${OPENAL_SEARCH_PATHS}
)

FIND_LIBRARY(OPENAL_LIBRARY 
	NAMES OpenAL al openal OpenAL32 openal32 libopenal
	HINTS
	$ENV{OPENALDIR}
	$ENV{OPENAL_PATH}
	PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64 lib/release Release
	PATHS ${OPENAL_SEARCH_PATHS}
)

# First search for d-suffixed libs
FIND_LIBRARY(OPENAL_LIBRARY_DEBUG 
	NAMES OpenALd ald openald OpenAL32d openal32d libopenald
	HINTS
	$ENV{OPENALDIR}
	$ENV{OPENAL_PATH}
	PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64 lib/debug Debug
	PATHS ${OPENAL_SEARCH_PATHS}
)

IF(NOT OPENAL_LIBRARY_DEBUG)
	# Then search for non suffixed libs if necessary, but only in debug dirs
	FIND_LIBRARY(OPENAL_LIBRARY_DEBUG 
		NAMES OpenAL al openal OpenAL32 openal32 libopenal
		HINTS
		$ENV{OPENALDIR}
		$ENV{OPENAL_PATH}
		PATH_SUFFIXES libs/Win32 libs/Win64 lib/debug Debug
		PATHS ${OPENAL_SEARCH_PATHS}
	)
ENDIF()

IF(OPENAL_LIBRARY)
	IF(OPENAL_LIBRARY_DEBUG)
		SET(OPENAL_LIBRARIES optimized "${OPENAL_LIBRARY}" debug "${OPENAL_LIBRARY_DEBUG}")
	ELSE()
		SET(OPENAL_LIBRARIES "${OPENAL_LIBRARY}")		# Could add "general" keyword, but it is optional
	ENDIF()
ENDIF()

# handle the QUIETLY and REQUIRED arguments and set XXX_FOUND to TRUE if all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OPENAL DEFAULT_MSG OPENAL_LIBRARIES OPENAL_INCLUDE_DIR)
