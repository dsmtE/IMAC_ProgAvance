# Locate OpenAL++
# This module defines XXX_FOUND, XXX_INCLUDE_DIRS and XXX_LIBRARIES standard variables
#
# Created by Sukender (Benoit Neil). Based on FindOpenAL.cmake module.

SET(OPENALPP_SEARCH_PATHS
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local
	/usr
	/sw # Fink
	/opt/local # DarwinPorts
	/opt/csw # Blastwave
	/opt
)

FIND_PATH(OPENALPP_INCLUDE_DIR NAMES openalpp/AudioBase openalpp/AudioBase.h
	HINTS
	$ENV{OSGAUDIO_DIR}
	$ENV{OSGAUDIO_PATH}
	$ENV{OSGAL_DIR}
	$ENV{OSGAL_PATH}
	PATH_SUFFIXES include
	PATHS ${OPENALPP_SEARCH_PATHS}
)

FIND_LIBRARY(OPENALPP_LIBRARY
	NAMES openalpp openALpp OpenALpp
	HINTS
	$ENV{OSGAUDIO_DIR}
	$ENV{OSGAUDIO_PATH}
	$ENV{OSGAL_DIR}
	$ENV{OSGAL_PATH}
	PATH_SUFFIXES lib64 lib
	PATHS ${OPENALPP_SEARCH_PATHS}
)

FIND_LIBRARY(OPENALPP_LIBRARY_DEBUG 
	NAMES openalppd openALppd OpenALppd
	HINTS
	$ENV{OSGAUDIO_DIR}
	$ENV{OSGAUDIO_PATH}
	$ENV{OSGAL_DIR}
	$ENV{OSGAL_PATH}
	PATH_SUFFIXES lib64 lib
	PATHS ${OPENALPP_SEARCH_PATHS}
)


IF(OPENALPP_LIBRARY)
	IF(OPENALPP_LIBRARY_DEBUG)
		SET(OPENALPP_LIBRARIES optimized "${OPENALPP_LIBRARY}" debug "${OPENALPP_LIBRARY_DEBUG}")
	ELSE()
		SET(OPENALPP_LIBRARIES "${OPENALPP_LIBRARY}")		# Could add "general" keyword, but it is optional
	ENDIF()
ENDIF()

# handle the QUIETLY and REQUIRED arguments and set XXX_FOUND to TRUE if all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OPENALPP DEFAULT_MSG OPENALPP_LIBRARIES OPENALPP_INCLUDE_DIR)
