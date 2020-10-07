# Locate TNL
# This module defines XXX_FOUND, XXX_INCLUDE_DIRS and XXX_LIBRARIES standard variables

FIND_PATH(TNL_INCLUDE_DIR
	NAMES tnl/tnl.h
	HINTS
	$ENV{TNLDIR}
	$ENV{TNL_PATH}
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

FIND_LIBRARY(TNL_LIBRARY 
	NAMES tnl libtnl
	HINTS
	$ENV{TNLDIR}
	$ENV{TNL_PATH}
	PATH_SUFFIXES lib lib64
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

FIND_LIBRARY(TNL_LIBRARY_DEBUG 
	NAMES tnld libtnld
	HINTS
	$ENV{TNLDIR}
	$ENV{TNL_PATH}
	PATH_SUFFIXES lib lib64
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



FIND_PATH(TNL_CRYPT_INCLUDE_DIR
	NAMES mycrypt.h
	HINTS
	$ENV{TNLDIR}
	$ENV{TNL_PATH}
	PATH_SUFFIXES libtomcrypt
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

FIND_LIBRARY(TNL_CRYPT_LIBRARY 
	NAMES libtomcrypt
	HINTS
	$ENV{TNLDIR}
	$ENV{TNL_PATH}
	PATH_SUFFIXES lib lib64
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

FIND_LIBRARY(TNL_CRYPT_LIBRARY_DEBUG 
	NAMES libtomcryptd
	HINTS
	$ENV{TNLDIR}
	$ENV{TNL_PATH}
	PATH_SUFFIXES lib lib64
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


IF(TNL_CRYPT_LIBRARY)
	IF(TNL_CRYPT_LIBRARY_DEBUG)
		SET(TNL_CRYPT_LIBRARIES optimized "${TNL_CRYPT_LIBRARY}" debug "${TNL_CRYPT_LIBRARY_DEBUG}")
	ELSE()
		SET(TNL_CRYPT_LIBRARIES "${TNL_CRYPT_LIBRARY}")		# Could add "general" keyword, but it is optional
	ENDIF()
ENDIF()

# handle the QUIETLY and REQUIRED arguments and set XXX_FOUND to TRUE if all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(TNL_CRYPT DEFAULT_MSG TNL_CRYPT_LIBRARIES TNL_CRYPT_INCLUDE_DIR)
