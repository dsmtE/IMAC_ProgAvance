# Boids Tower Defense CMake script
# by Sukender (Benoit Neil)


CMAKE_MINIMUM_REQUIRED(VERSION 3.2.0 FATAL_ERROR)

set(PROJECT_NAME BTD)
project( ${PROJECT_NAME} )
SET(${PROJECT_NAME}_MAJOR_VERSION 0)
SET(${PROJECT_NAME}_MINOR_VERSION 0)
SET(${PROJECT_NAME}_PATCH_VERSION 3)

SET(${PROJECT_NAME}_VERSION ${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION}.${${PROJECT_NAME}_PATCH_VERSION})


# We have some custom .cmake scripts not in the official distribution.
SET(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")

if(NOT "$ENV{BOOST_ROOT}" STREQUAL "")
	set(BOOST_ROOT "$ENV{BOOST_ROOT}")
	set(Boost_NO_SYSTEM_PATHS ON)
endif()

FIND_PACKAGE(PVLE)

# Use CMake compile features, or C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

IF(UNIX)
	# Not sure what this will do on Cygwin and Msys
	# Also, remember OS X X11 is a user installed option so it may not exist.
	FIND_PACKAGE(X11)
	# Some Unicies need explicit linkage to the Math library or the build fails.
	FIND_LIBRARY(MATH_LIBRARY m)
ENDIF()

#FIND_PACKAGE(ODE)


# Make the headers visible to everything
# IF(NOT ${PROJECT_BINARY_DIR} EQUAL ${PROJECT_SOURCE_DIR})
   # INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR}/Plugins)
# ENDIF()

# INCLUDE_DIRECTORIES(
    # ${PROJECT_SOURCE_DIR}/Plugins
# )


################################################################################
# Compiler warnings handling
# Copied from OSG's CMakeLists.txt

IF(WIN32)
    IF(MSVC)
        # This option is to enable the /MP switch for Visual Studio 2005 and above compilers
        OPTION(WIN32_USE_MP "Set to ON to build with the /MP option (Visual Studio 2005 and above)." OFF)
        MARK_AS_ADVANCED(WIN32_USE_MP)
        IF(WIN32_USE_MP)
            SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
        ENDIF()

        # # turn off various warnings
        # foreach(warning 4244 4251 4267 4275 4290 4786 4305 4996)
            # SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd${warning}")
        # ENDFOREACH()

        # # More MSVC specific compilation flags
        # ADD_DEFINITIONS(-D_SCL_SECURE_NO_WARNINGS)
        # ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE)
    ENDIF()
ENDIF()


# This is for an advanced option to give aggressive warnings
# under different compilers. If yours is not implemented, this option
# will not be made available.
IF(CMAKE_COMPILER_IS_GNUCXX)
    # To be complete, we might also do GNUCC flags,
    # but everything here is C++ code.
    # -Wshadow and -Woverloaded-virtual are also interesting flags, but project
    # returns too many hits.
    # FYI, if we do implement GNUCC, then -Wmissing-prototypes in another
    # interesting C-specific flag.
    # Also, there is a bug in gcc 4.0. Under C++, -pedantic will create
    # errors instead of warnings for certain issues, including superfluous
    # semicolons and commas, and the use of long long. -fpermissive seems
    # to be the workaround.
    SET(${PROJECT_NAME}_AGGRESSIVE_WARNING_FLAGS -Wall -Wparentheses -Wno-long-long -Wno-import -pedantic -Wreturn-type -Wmissing-braces -Wunknown-pragmas -Wunused -fpermissive)

    # Previous included -Wformat=2 in ${PROJECT_NAME}_AGGRESSIVE_WARNING_FLAGS but had to remove it due to standard library errors

	# For UPEM (to move in PVLE?)
	set(THREADS_PREFER_PTHREAD_FLAG ON)
	find_package(Threads REQUIRED)

ELSE()
    IF(MSVC)
        # FIXME: What are good aggressive warning flags for Visual Studio?
        # And do we need to further subcase this for different versions of VS?
        # CMake variables: MSVC60, MSVC70, MSVC71, MSVC80, CMAKE_COMPILER_2005
        SET(${PROJECT_NAME}_AGGRESSIVE_WARNING_FLAGS /W4 /wd4706 /wd4127 /wd4100)


    ELSE()
        # CMake lacks an elseif, so other non-gcc, non-VS compilers need
        # to be listed below. If unhandled, ${PROJECT_NAME}_AGGRESSIVE_WARNING_FLAGS should
        # remain unset.
    ENDIF()
ENDIF()

# This part is for the CMake menu option to toggle the warnings on/off.
# This will only be made available if we set values for ${PROJECT_NAME}_AGGRESSIVE_WARNING_FLAGS.
IF(${PROJECT_NAME}_AGGRESSIVE_WARNING_FLAGS)

    IF (APPLE)
        SET(DEFAULT_USE_AGGRESSIVE_WARNINGS OFF)
    ELSE()
        SET(DEFAULT_USE_AGGRESSIVE_WARNINGS ON)
    ENDIF()

    OPTION(${PROJECT_NAME}_USE_AGGRESSIVE_WARNINGS "Enable to activate aggressive warnings" ${DEFAULT_USE_AGGRESSIVE_WARNINGS})
    MARK_AS_ADVANCED(${PROJECT_NAME}_USE_AGGRESSIVE_WARNINGS)

    IF(${PROJECT_NAME}_USE_AGGRESSIVE_WARNINGS)
        # Add flags defined by ${PROJECT_NAME}_AGGRESSIVE_WARNING_FLAGS if they aren't already there
        FOREACH(flag ${${PROJECT_NAME}_AGGRESSIVE_WARNING_FLAGS})
            IF(NOT CMAKE_CXX_FLAGS MATCHES "${flag}")
                SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
            ENDIF()
        ENDFOREACH()
    ELSE()
        # Remove all flags considered aggresive
        FOREACH(flag ${${PROJECT_NAME}_AGGRESSIVE_WARNING_FLAGS})
            STRING(REGEX REPLACE "${flag}" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        ENDFOREACH()
    ENDIF()
ENDIF()

################################################################################
# Create bin and lib directories if required

SET(OUTPUT_BINDIR "${PROJECT_BINARY_DIR}/Bin")
MAKE_DIRECTORY("${OUTPUT_BINDIR}")
#SET(OUTPUT_LIBDIR "${PROJECT_BINARY_DIR}/Lib")
#MAKE_DIRECTORY("${OUTPUT_LIBDIR}")

# Set output dirs
#SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_LIBDIR})
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_BINDIR})
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_BINDIR})
# IF(WIN32)
	# SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_BINDIR})
# ELSE()
	# SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_LIBDIR})
# ENDIF()


OPTION(BUILD_BINARIES_USING_PER_CONFIGURATION_DIR "For per-configuration generators (VStudio, XCode), this will place binaries into a subdirectory (Debug, Release...)." OFF)
MARK_AS_ADVANCED(BUILD_BINARIES_USING_PER_CONFIGURATION_DIR)
IF(BUILD_BINARIES_USING_PER_CONFIGURATION_DIR)
	# Subdir output
	# Unset specific variables
	FOREACH(CONF ${CMAKE_CONFIGURATION_TYPES})
		STRING(TOUPPER "${CONF}" CONF)
		SET("CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONF}" )
		SET("CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONF}" )
		SET("CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONF}" )
	ENDFOREACH()
ELSE()
	# Normal output
	# Remove subdirs (/Debug, /Release) that VS/XCode create, by setting per-configuration directories instead of global setting
	FOREACH(CONF ${CMAKE_CONFIGURATION_TYPES})
		STRING(TOUPPER "${CONF}" CONF)
		SET("CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONF}" "${OUTPUT_LIBDIR}")
		SET("CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONF}" "${OUTPUT_BINDIR}")
		IF(WIN32)
			SET("CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONF}" "${OUTPUT_BINDIR}")
		ELSE()
			SET("CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONF}" "${OUTPUT_LIBDIR}")
		ENDIF()
	ENDFOREACH()
ENDIF()

################################################################################
# Installation stuff

SET(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
#ADD_DEFINITIONS(-DOSG_DEBUG_POSTFIX=${CMAKE_DEBUG_POSTFIX})

# Adds includes for a library found with FIND_PACKAGE
# SCOPE must be in INTERFACE|PUBLIC|PRIVATE, as for target_include_directories()
FUNCTION(INCLUDE_WITH_VARIABLES TRGTNAME SCOPE)
	FOREACH(CUR_DEPENDENCY ${ARGN})
		target_include_directories(${TRGTNAME} ${SCOPE} ${${CUR_DEPENDENCY}_INCLUDE_DIR} ${${CUR_DEPENDENCY}_INCLUDE_DIRS} ${${CUR_DEPENDENCY}_INCLUDES})
	ENDFOREACH()
ENDFUNCTION()

# Adds includes and link properties for a library found with FIND_PACKAGE
# SCOPE must be in INTERFACE|PUBLIC|PRIVATE, as for target_include_directories()
FUNCTION(LINK_WITH_VARIABLES TRGTNAME SCOPE)
	INCLUDE_WITH_VARIABLES(${TRGTNAME} ${SCOPE} ${ARGN})
	FOREACH(CUR_DEPENDENCY ${ARGN})
		target_link_libraries(${TRGTNAME} ${${CUR_DEPENDENCY}_LIBRARIES} )
	ENDFOREACH()
ENDFUNCTION()


################################################################################
# Set Config file

# SET(BTD_CONFIG_HEADER "${PROJECT_BINARY_DIR}/Include/BTD/Config.h")
# CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/Src/Config.in" "${BTD_CONFIG_HEADER}")

ADD_SUBDIRECTORY(Src)
ADD_SUBDIRECTORY(Plugins/Boids)

# This needs to be run very last so other parts of the scripts can take advantage of this.
IF(NOT ${PROJECT_NAME}_CONFIG_HAS_BEEN_RUN_BEFORE)
    SET(${PROJECT_NAME}_CONFIG_HAS_BEEN_RUN_BEFORE 1 CACHE INTERNAL "Flag to track whether this is the first time running CMake or if CMake has been configured before")
ENDIF()
