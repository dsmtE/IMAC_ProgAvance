# Markers are files containing data that reflect project configuration options, that may be read by FindXXX.cmake modules
# The script could be easily modified to generate scripts that could be interpreted by CMake with a simple INCLUDE(), but this is far too dangerous because it may be used for insertion of malicious code.

# Usage: SET_MARKER(projectDir versionNumber varName1 varName2...)
# TODO: handle values containing a ';' (need escaping the char?)
FUNCTION(SET_MARKER MARKER_DIR)
	FILE(TO_CMAKE_PATH "${MARKER_DIR}/CMakeMarker.txt" MARKER_PATH)
	SET(DATA_LIST "CMakeMarkers\n")
	FOREACH(VARNAME ${ARGN})
		SET(VALUE_STR "SET\;${VARNAME}")
		FOREACH(VALUE_ELEMENT ${${VARNAME}})
			# IF(NOT VALUE_STR OR VALUE_STR STREQUAL "")
				# SET(VALUE_STR "${VALUE_ELEMENT}")
			# ELSE()
				SET(VALUE_STR "${VALUE_STR}\;${VALUE_ELEMENT}")
			# ENDIF()
		ENDFOREACH()
		SET(DATA_LIST "${DATA_LIST}${VALUE_STR}\n")
	ENDFOREACH()
	FILE(WRITE ${MARKER_PATH} ${DATA_LIST})
ENDFUNCTION()

# Execute "SET" command in a marker file
FUNCTION(MARKER_COMMAND_SET REGEX_TO_MATCH VARLISTNAME)
	# Get variable name and variable value
	LIST(LENGTH ARGN LIST_SIZE)
	IF (LIST_SIZE EQUAL 0)
		MESSAGE(STATUS "Marker command ill-formatted. Syntax: SET;variable;value1[;value2[...]]")
	ELSE()
		LIST(GET ARGN 0 CUR_VARNAME)
		LIST(REMOVE_AT ARGN 0)
		IF(CUR_VARNAME AND NOT CUR_VARNAME STREQUAL "")
			LIST(LENGTH ARGN LIST_SIZE)
			STRING(REGEX MATCH "${REGEX_TO_MATCH}" REGEX_TEST ${CUR_VARNAME})
			IF(REGEX_TEST)
				# Push var list and values in parent scope
				SET(${VARLISTNAME} ${${VARLISTNAME}} ${CUR_VARNAME} PARENT_SCOPE)
				SET(${CUR_VARNAME} ${ARGN} PARENT_SCOPE)		# Works even if ARGN is empty
				#MESSAGE(STATUS "Variable set from markers file: ${CUR_VARNAME}=${ARGN}")

				#FOREACH(A ${ARGN})
				#MESSAGE("Debug - Value: ${A})")
				#ENDFOREACH()
			ELSE()
				#MESSAGE(STATUS "Variable REJECTED from markers file: ${CUR_VARNAME}=${ARGN}")
			ENDIF()
		ENDIF()
	ENDIF()
ENDFUNCTION()



# Reads a string and says if it is a command or not
FUNCTION(MARKER_IS_COMMAND CMD OUT_VARNAME)
	IF(CMD STREQUAL "SET")
		SET(${OUT_VARNAME} ON PARENT_SCOPE)
	ELSE()
		SET(${OUT_VARNAME} OFF PARENT_SCOPE)
	ENDIF()
ENDFUNCTION()

# Execute a marker command
FUNCTION(MARKER_PROCESS_COMMAND CMD VARLISTNAME REGEX_TO_MATCH)
	IF(${CMD} STREQUAL "SET")
		MARKER_COMMAND_SET(${REGEX_TO_MATCH} ${VARLISTNAME} ${ARGN})
		# Push var list and values in parent scope
		SET(${VARLISTNAME} ${${VARLISTNAME}} PARENT_SCOPE)
		FOREACH(VARNAME ${${VARLISTNAME}})
			SET(${VARNAME} ${${VARNAME}} PARENT_SCOPE)
		ENDFOREACH()
	ELSE()
		MESSAGE("Implementation error: cannot execute marker's \"${CMD}\" command")
	ENDIF()
ENDFUNCTION()


# Usage: GET_MARKER(projectDir regexOfAllowedVariablesNames)
FUNCTION(GET_MARKER MARKER_DIR REGEX_TO_MATCH)
	SET(VARLISTNAME VARLIST)		# May become an argument of GET_MARKER()

	IF (NOT REGEX_TO_MATCH OR REGEX_TO_MATCH STREQUAL "")
		SET(REGEX_TO_MATCH "^.*")		# Match all if not regex was passed
	ENDIF()
	FILE(TO_CMAKE_PATH "${MARKER_DIR}/CMakeMarker.txt" MARKER_PATH)

	FILE(STRINGS ${MARKER_PATH} FILE_DATA)

	# Extract identifier string
	LIST(LENGTH FILE_DATA LIST_SIZE)
	IF (LIST_SIZE GREATER 0)
		LIST(GET FILE_DATA 0 IDSTR)
		LIST(REMOVE_AT FILE_DATA 0)
		LIST(LENGTH FILE_DATA LIST_SIZE)
		IF (NOT IDSTR STREQUAL "CMakeMarkers")
			MESSAGE("File does not start with appropriate identifier. FindPVLE.cmake may be too old, or PVLE project was not compiled with up-to-date CMake scripts. Please consider updating PVLE.")
		ELSE()

			# Read commands
			LIST(APPEND FILE_DATA "SET")		# Add a dummy command at the end, to ensure the last one is not skipped
			SET(CUR_CMD )		# Current command
			SET(CUR_ARGS )		# Current arguments to command
			FOREACH(ELEMENT ${FILE_DATA})
				MARKER_IS_COMMAND(${ELEMENT} ISCMD)
				IF(ISCMD)
					# Reading a command. Need to process previous one.
					IF(CUR_CMD)
						SET(${VARLISTNAME} )
						MARKER_PROCESS_COMMAND(${CUR_CMD} ${VARLISTNAME} ${REGEX_TO_MATCH} ${CUR_ARGS})
						# Push var list and values in parent scope
						SET(${VARLISTNAME} ${${VARLISTNAME}} PARENT_SCOPE)
						FOREACH(VARNAME ${${VARLISTNAME}})
							#MESSAGE(STATUS "Debug: ${VARNAME} = ${${VARNAME}}")
							SET(${VARNAME} ${${VARNAME}} PARENT_SCOPE)
						ENDFOREACH()
					ENDIF()
					# Now start a new command.
					SET(CUR_CMD ${ELEMENT})
					SET(CUR_ARGS )
				ELSE()
					# Reading an argument
					IF(NOT CUR_CMD)		# No current command?
						MESSAGE(STATUS "Marker command \"${ELEMENT}\" isn't recognized or supported")
					ELSE()
						LIST(APPEND CUR_ARGS ${ELEMENT})
					ENDIF()
				ENDIF()
				LIST(REMOVE_AT ELEMENT 0)
			ENDFOREACH()

		ENDIF()
	ELSE()
		MESSAGE("File does not start with appropriate identifier. FindPVLE.cmake may be too old, or PVLE project was not compiled with up-to-date CMake scripts. Please consider updating PVLE.")
	ENDIF()
ENDFUNCTION()
