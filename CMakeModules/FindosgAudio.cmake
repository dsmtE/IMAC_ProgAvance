# Locate osgAudio
# This module defines XXX_FOUND, XXX_INCLUDE_DIRS and XXX_LIBRARIES standard variables
#
# Created by Sukender (Benoit Neil). Based on FindOpenAL.cmake module.

SET(OSGAUDIO_SEARCH_PATHS
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local
	/usr
	/sw # Fink
	/opt/local # DarwinPorts
	/opt/csw # Blastwave
	/opt
)

FIND_PATH(OSGAUDIO_INCLUDE_DIR NAMES osgAudio/SoundUpdateCB.h
	HINTS
	$ENV{OSGAUDIO_DIR}
	$ENV{OSGAUDIO_PATH}
	PATH_SUFFIXES include
	PATHS ${OSGAUDIO_SEARCH_PATHS}
)

FIND_LIBRARY(OSGAUDIO_LIBRARY
	NAMES osgAudio libosgAudio
	HINTS
	$ENV{OSGAUDIO_DIR}
	$ENV{OSGAUDIO_PATH}
	PATH_SUFFIXES lib64 lib
	PATHS ${OSGAUDIO_SEARCH_PATHS}
)

FIND_LIBRARY(OSGAUDIO_LIBRARY_DEBUG 
	NAMES osgAudiod libosgAudiod
	HINTS
	$ENV{OSGAUDIO_DIR}
	$ENV{OSGAUDIO_PATH}
	PATH_SUFFIXES lib64 lib
	PATHS ${OSGAUDIO_SEARCH_PATHS}
)


IF(OSGAUDIO_LIBRARY)
	IF(OSGAUDIO_LIBRARY_DEBUG)
		SET(OSGAUDIO_LIBRARIES optimized "${OSGAUDIO_LIBRARY}" debug "${OSGAUDIO_LIBRARY_DEBUG}")
	ELSE()
		SET(OSGAUDIO_LIBRARIES "${OSGAUDIO_LIBRARY}")		# Could add "general" keyword, but it is optional
	ENDIF()
ENDIF()

# handle the QUIETLY and REQUIRED arguments and set XXX_FOUND to TRUE if all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OSGAUDIO DEFAULT_MSG OSGAUDIO_LIBRARIES OSGAUDIO_INCLUDE_DIR)
