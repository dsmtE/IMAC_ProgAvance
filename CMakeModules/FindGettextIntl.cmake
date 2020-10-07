# Locate Gettext library (iconv)
# This module defines XXX_FOUND, XXX_INCLUDE_DIRS and XXX_LIBRARIES standard variables

# Try the user's environment request before anything else.
FIND_PATH(GETTEXT_INTL_INCLUDE_DIR NAMES libintl.h
  HINTS
  $ENV{INTL_DIR}
  $ENV{INTL_PATH}
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

FIND_LIBRARY(GETTEXT_INTL_LIBRARY
  NAMES intl libintl
  HINTS
  $ENV{INTL_DIR}
  $ENV{INTL_PATH}
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

FIND_LIBRARY(GETTEXT_INTL_LIBRARY_DEBUG 
  NAMES intld libintld
  HINTS
  $ENV{INTL_DIR}
  $ENV{INTL_PATH}
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

IF(GETTEXT_INTL_LIBRARY)
	IF(GETTEXT_INTL_LIBRARY_DEBUG)
		SET(GETTEXT_INTL_LIBRARIES optimized "${GETTEXT_INTL_LIBRARY}" debug "${GETTEXT_INTL_LIBRARY_DEBUG}")
	ELSE()
		SET(GETTEXT_INTL_LIBRARIES "${GETTEXT_INTL_LIBRARY}")		# Could add "general" keyword, but it is optional
	ENDIF()
ENDIF()

# handle the QUIETLY and REQUIRED arguments and set XXX_FOUND to TRUE if all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GETTEXT_INTL DEFAULT_MSG GETTEXT_INTL_LIBRARIES GETTEXT_INTL_INCLUDE_DIR)
