# Locate osgAL
# This module defines
# OSGAL_LIBRARY, OSGAL_LIBRARY_DEBUG
#OSGAL_FOUND, if false, do not try to link to osgAL
# OSGAL_INCLUDE_DIR, where to find the headers

# Try the user's environment request before anything else.
FIND_PATH(OSGAL_INCLUDE_DIR NAMES osgAL/SoundNode
  HINTS
  $ENV{OSGAL_DIR}
  $ENV{OSGAL_PATH}
  PATH_SUFFIXES include
  PATHS
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local
    /usr
    /sw # Fink
    /opt/local # DarwinPorts
    /opt/csw # Blastwave
    /opt
)

FIND_LIBRARY(OSGAL_LIBRARY
  NAMES osgal osgAL
  HINTS
  $ENV{OSGAL_DIR}
  $ENV{OSGAL_PATH}
  PATH_SUFFIXES lib64 lib
  PATHS
    ~/Library/Frameworks
    /Library/Frameworks
	/usr/local
	/usr
	/sw
	/opt/local
	/opt/csw
	/opt
)

FIND_LIBRARY(OSGAL_LIBRARY_DEBUG 
  NAMES osgald osgALd
  HINTS
  $ENV{OSGAL_DIR}
  $ENV{OSGAL_PATH}
  PATH_SUFFIXES lib64 lib
  PATHS
    ~/Library/Frameworks
    /Library/Frameworks
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
)


IF(OSGAL_LIBRARY)
	IF(OSGAL_LIBRARY_DEBUG)
		SET(OSGAL_LIBRARIES optimized "${OSGAL_LIBRARY}" debug "${OSGAL_LIBRARY_DEBUG}")
	ELSE()
		SET(OSGAL_LIBRARIES "${OSGAL_LIBRARY}")		# Could add "general" keyword, but it is optional
	ENDIF()
ENDIF()

# handle the QUIETLY and REQUIRED arguments and set XXX_FOUND to TRUE if all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OSGAL DEFAULT_MSG OSGAL_LIBRARIES OSGAL_INCLUDE_DIR)
