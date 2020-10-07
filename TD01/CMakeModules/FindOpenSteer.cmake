# Locate OpenSteer
# This module defines XXX_FOUND, XXX_INCLUDE_DIRS and XXX_LIBRARIES standard variables
#
# use SET(OPENSTEER_DOUBLE_PRECISION true) to link against double precision OPENSTEER

# Try the user's environment request before anything else.
FIND_PATH(OPENSTEER_INCLUDE_DIR OpenSteer/SteerLibrary.h
	HINTS
	$ENV{OPENSTEER_DIR}
	$ENV{OPENSTEER_PATH}
	${ADDITIONAL_SEARCH_PATHS}
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

FIND_LIBRARY(OPENSTEER_LIBRARY
	NAMES opensteer
	HINTS
	$ENV{OPENSTEER_DIR}
	$ENV{OPENSTEER_PATH}
	${ADDITIONAL_SEARCH_PATHS}
	PATH_SUFFIXES lib64 lib win32/lib64 win32/lib linux/lib64 linux/lib macosx/lib64 macosx/lib
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

FIND_LIBRARY(OPENSTEER_LIBRARY_DEBUG 
	NAMES opensteerd
	HINTS
	$ENV{OPENSTEER_DIR}
	$ENV{OPENSTEER_PATH}
	${ADDITIONAL_SEARCH_PATHS}
	PATH_SUFFIXES lib64 lib win32/lib64 win32/lib linux/lib64 linux/lib macosx/lib64 macosx/lib
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


# handle the QUIETLY and REQUIRED arguments and set CURL_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OPENSTEER DEFAULT_MSG OPENSTEER_LIBRARY OPENSTEER_INCLUDE_DIR)
INCLUDE(FindPackageTargetLibraries)
FIND_PACKAGE_SET_STD_INCLUDE_AND_LIBS(OPENSTEER)
