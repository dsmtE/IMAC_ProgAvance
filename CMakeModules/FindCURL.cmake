# - Find curl
# This module defines XXX_FOUND, XXX_INCLUDE_DIRS and XXX_LIBRARIES standard variables

# Look for the header file.
FIND_PATH(CURL_INCLUDE_DIR NAMES curl/curl.h
	HINTS
	$ENV{CURL_DIR}
	$ENV{CURL_PATH}
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

# Look for the library.
FIND_LIBRARY(CURL_LIBRARY
	NAMES libcurl_imp libcurl curl curllib
	HINTS
	$ENV{CURL_DIR}
	$ENV{CURL_PATH}
	PATH_SUFFIXES lib64 lib lib/Release
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

FIND_LIBRARY(CURL_LIBRARY_DEBUG
	NAMES libcurld_imp libcurld curld curllibd
	HINTS
	$ENV{CURL_DIR}
	$ENV{CURL_PATH}
	PATH_SUFFIXES lib64 lib lib/Debug
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

IF(CURL_LIBRARY)
	IF(CURL_LIBRARY_DEBUG)
		SET(CURL_LIBRARIES optimized "${CURL_LIBRARY}" debug "${CURL_LIBRARY_DEBUG}")
	ELSE()
		SET(CURL_LIBRARIES "${CURL_LIBRARY}")		# Could add "general" keyword, but it is optional
	ENDIF()
ENDIF()

# handle the QUIETLY and REQUIRED arguments and set XXX_FOUND to TRUE if all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CURL DEFAULT_MSG CURL_LIBRARIES CURL_INCLUDE_DIR)
