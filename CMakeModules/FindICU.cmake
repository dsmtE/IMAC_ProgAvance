# Locate ICU
# This module defines XXX_FOUND, XXX_INCLUDE_DIRS and XXX_LIBRARIES standard variables
#
# $ICUDIR is an environment variable that would
# correspond to the ./configure --prefix=$ICUDIR
# used in building ICU.
#
# By Sukender (Benoit NEIL), under the terms of the WTFPL (see cooresponding license file)

SET(ICU_SEARCH_PATHS
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local
	/usr
	/sw # Fink
	/opt/local # DarwinPorts
	/opt/csw # Blastwave
	/opt
)

FIND_PATH(ICU_INCLUDE_DIR
	NAMES unicode/unistr.h
	HINTS
	$ENV{ICUDIR}
	$ENV{ICU_PATH}
	PATH_SUFFIXES include
	PATHS ${ICU_SEARCH_PATHS}
)

INCLUDE(FindPackageHandleStandardArgs)
SET(ICU_FOUND ON)

MACRO(FIND_ICU_PART VARNAME LIBNAME)
	FIND_LIBRARY("ICU_LIBRARY_${VARNAME}"
		NAMES "${LIBNAME}"
		HINTS
		$ENV{ICUDIR}
		$ENV{ICU_PATH}
		PATH_SUFFIXES lib lib64
		PATHS ${ICU_SEARCH_PATHS}
	)
	FIND_LIBRARY("ICU_LIBRARY_${VARNAME}_DEBUG"
		NAMES "${LIBNAME}d"
		HINTS
		$ENV{ICUDIR}
		$ENV{ICU_PATH}
		PATH_SUFFIXES lib lib64
		PATHS ${ICU_SEARCH_PATHS}
	)

	IF("ICU_LIBRARY_${VARNAME}")
		IF("ICU_LIBRARY_${VARNAME}_DEBUG")
			SET("ICU_${VARNAME}_LIBRARIES" optimized "${ICU_LIBRARY_${VARNAME}}" debug "${ICU_LIBRARY_${VARNAME}_DEBUG}")
		ELSE()
			SET("ICU_${VARNAME}_LIBRARIES" "${ICU_LIBRARY_${VARNAME}}")		# Could add "general" keyword, but it is optional
		ENDIF()
	ENDIF()

	# handle the QUIETLY and REQUIRED arguments and set XXX_FOUND to TRUE if all listed variables are TRUE
	FIND_PACKAGE_HANDLE_STANDARD_ARGS("ICU_${VARNAME}" DEFAULT_MSG "ICU_${VARNAME}_LIBRARIES" ICU_INCLUDE_DIR)

	LIST(APPEND ICU_LIBRARIES "${ICU_${VARNAME}_LIBRARIES}")

	IF(NOT "ICU_${VARNAME}_FOUND")
		SET(ICU_FOUND OFF)
	ENDIF()
ENDMACRO()

SET(ICU_ALL_COMPONENTS I18N COMMON TEST IO LAYOUT LAYOUTEX STUBDATA UTIL)
#SET(ICU_ALL_NAMES icuin icuuc icutest icuio icule iculx icudt icutu)
IF(NOT ICU_FIND_COMPONENTS)
	SET(ICU_FIND_COMPONENTS ${ICU_ALL_COMPONENTS})
ENDIF()

FOREACH(COMPONENT ${ICU_FIND_COMPONENTS})
	# Code should be: if COMPONENT found in ICU_ALL_COMPONENTS, call FIND_ICU_PART with corresponding name in ICU_ALL_NAMES
	# Unfortunately I (Sukender) don't kno how to do it preperly with CMake

	IF(${COMPONENT} STREQUAL I18N)
		FIND_ICU_PART(I18N icuin)
	ELSEIF(${COMPONENT} STREQUAL COMMON)
		FIND_ICU_PART(COMMON icuuc)
	ELSEIF(${COMPONENT} STREQUAL TEST)
		FIND_ICU_PART(TEST icutest)
	ELSEIF(${COMPONENT} STREQUAL IO)
		FIND_ICU_PART(IO icuio)
	ELSEIF(${COMPONENT} STREQUAL LAYOUT)
		FIND_ICU_PART(LAYOUT icule)
	ELSEIF(${COMPONENT} STREQUAL LAYOUTEX)
		FIND_ICU_PART(LAYOUTEX iculx)
	ELSEIF(${COMPONENT} STREQUAL STUBDATA)
		FIND_ICU_PART(STUBDATA icudt)
	#ELSEIF(${COMPONENT} STREQUAL TESTPLUG)
		#FIND_ICU_PART(TESTPLUG testplug)
	ELSEIF(${COMPONENT} STREQUAL UTIL)
		FIND_ICU_PART(UTIL icutu)
	ENDIF()
ENDFOREACH()
