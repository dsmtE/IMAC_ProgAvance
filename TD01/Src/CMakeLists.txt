SET(EXE_NAME btd)
SET(HEADER_PATH ${BTD_SOURCE_DIR}/Src)

# SET(HEADERS
	# #${BTD_CONFIG_HEADER}		# Usually located in the binary dir for out-of-source builds
# )
SET(SOURCE
	App.h
	App.cpp
	AppOptions.cpp
	Boid.h
	Boid.cpp
	CollisionCategories.h
	Common.h
	Common.cpp
	CameraController.h
	CameraController.cpp
	Constants.h
	Constants.cpp
	Constructor.h
	Constructor.cpp
	DevHud.h
	DevHud.cpp
	Fonts.h
	Game.h
	Game.cpp
	GamePathFinder.h
	GamePathFinder.cpp
	Laser.h
	Laser.cpp
	Main.cpp
	Obstacle.h
	Obstacle.cpp
	OptionsConstants.h
	OptionsConstants.cpp
	Path.h
	Path.cpp
	PathFinder.h
	PathFinder.cpp
	Turret.h
	Turret.cpp
	Units.h
	Units.cpp
	UserSwitches.h
	UserSwitches.cpp
)

SET(RESSOURCES )
IF(MSVC_ID)
#	SET(RESSOURCES "../Data/Icon.rc")
ENDIF()

ADD_EXECUTABLE(
	${EXE_NAME}
    #${HEADERS}
	${SOURCE}
	${RESSOURCES}
)


# Setup dependencies

#SET(DEPENDENCIES_NAMES PVLE ${PVLE_MANDATORY_DEPENDENCIES} ${PVLE_OPTIONAL_DEPENDENCIES})
#add_library(PVLE SHARED IMPORTED)
target_link_libraries(${EXE_NAME} PVLE)
IF(CMAKE_COMPILER_IS_GNUCXX)
	target_link_libraries(${EXE_NAME} Threads::Threads)
endif()
# INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIRS} )
# IF(UNIX)
	# LIST(APPEND DEPENDENCIES_NAMES MATH_LIBRARY X11)
	# ADD_DEFINITIONS( -DPVLE_USE_X11 )
# ENDIF()

#INCLUDE_DIRECTORIES( ${OPENSCENEGRAPH_INCLUDE_DIRS} )
# INCLUDE_DIRECTORIES( ${OPENGL_INCLUDE_DIR} )
#LINK_WITH_VARIABLES( ${EXE_NAME} ${DEPENDENCIES_NAMES} )
# TARGET_LINK_LIBRARIES( ${EXE_NAME} ${OPENGL_LIBRARIES} )
# TARGET_LINK_LIBRARIES( ${EXE_NAME} ${OPENSCENEGRAPH_LIBRARIES} )

#ADD_DEFINITIONS(
	#-DBOOST_ENABLE_ASSERT_HANDLER
#)

# ADD_DEFINITIONS(
	# -DBOOST_ALL_NO_LIB		# No boost auto link
	# -DBOOST_ALL_DYN_LINK
# )

# There is a problem on Windows/MSVC using BOOST_ALL_NO_LIB:
IF(MSVC)
	ADD_DEFINITIONS(
		-DBOOST_ALL_DYN_LINK		# Boost auto-link to dynamic libarries
		#-DBOOST_PROGRAM_OPTIONS_DYN_LINK
	)
ELSE()
	ADD_DEFINITIONS(
		-DBOOST_ALL_NO_LIB		# No boost auto link
	)
ENDIF()


# Add the postfix to the executable since it is not added automatically as for modules and shared libraries
SET_TARGET_PROPERTIES(${EXE_NAME} PROPERTIES DEBUG_POSTFIX "${CMAKE_DEBUG_POSTFIX}")

IF(MSVC_IDE)
	# Enables asynchronous exceptions
	SET_TARGET_PROPERTIES(${EXE_NAME} PROPERTIES COMPILE_FLAGS "/EHa")

	# Ugly workaround to remove the "/debug" or "/release" in each output
	SET_TARGET_PROPERTIES(${EXE_NAME} PROPERTIES PREFIX "../")
	#SET_TARGET_PROPERTIES(${EXE_NAME} PROPERTIES IMPORT_PREFIX "../")
ENDIF()
	
#SET_TARGET_PROPERTIES(${EXE_NAME} PROPERTIES VERSION ${BTD_VERSION})

#INCLUDE(ModuleInstall OPTIONAL)
