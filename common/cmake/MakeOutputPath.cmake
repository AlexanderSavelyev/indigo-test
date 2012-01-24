set(COMMON_OUTPUT_PATH dist/${SYSTEM_FOLDER_NAME}/${SUBSYSTEM_FOLDER_NAME})

set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/${COMMON_OUTPUT_PATH}/lib)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/${COMMON_OUTPUT_PATH}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${COMMON_OUTPUT_PATH}/shared)

MESSAGE(STATUS "Common output dir: ${CMAKE_BINARY_DIR}/${COMMON_OUTPUT_PATH}")

macro(PACK_STATIC proj)
	INSTALL(TARGETS ${proj} DESTINATION static/${SYSTEM_FOLDER_NAME}/${SUBSYSTEM_FOLDER_NAME})
endmacro()

macro(PACK_SHARED proj)
	INSTALL(TARGETS ${proj} DESTINATION shared/${SYSTEM_FOLDER_NAME}/${SUBSYSTEM_FOLDER_NAME})
endmacro()
