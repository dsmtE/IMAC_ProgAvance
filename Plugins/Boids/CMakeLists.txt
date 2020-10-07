SET(LIB_NAME btd_boids)

SET(SOURCE
	Boids.h
	Boids.cpp
	../Export.h
)

SET(RESSOURCES )
IF(MSVC_ID)
#	SET(RESSOURCES "../Data/Icon.rc")
ENDIF()

ADD_LIBRARY(
	${LIB_NAME}
	MODULE
    #${HEADERS}
	${SOURCE}
	${RESSOURCES}
)

target_link_libraries(${LIB_NAME} PVLE)

#ADD_DEFINITIONS( -DBTD_PLUGIN_LIB_DYNAMIC )

# Removes any prefix (such as "lib" under unices) so that the plugin always has the same name
SET_TARGET_PROPERTIES(${LIB_NAME} PROPERTIES PREFIX "")

IF(MSVC_IDE)
	# Enables asynchronous exceptions
	SET_TARGET_PROPERTIES(${LIB_NAME} PROPERTIES COMPILE_FLAGS "/EHa")

	# Ugly workaround to remove the "/debug" or "/release" in each output
	SET_TARGET_PROPERTIES(${LIB_NAME} PROPERTIES PREFIX "../")
	#SET_TARGET_PROPERTIES(${LIB_NAME} PROPERTIES IMPORT_PREFIX "../")
ENDIF()
	
#SET_TARGET_PROPERTIES(${LIB_NAME} PROPERTIES VERSION ${BTD_VERSION})

#INCLUDE(ModuleInstall OPTIONAL)
