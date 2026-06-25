# Build gxde-qdbusxml2cpp if not already present (needed for code generation)
set(TOOL_SRC "${PROJECT_SOURCE_DIR}/tools/qdbusxml2cpp")
set(TOOL_BIN "${PROJECT_SOURCE_DIR}/gxde-qdbusxml2cpp")
if(NOT EXISTS "${TOOL_BIN}")
    message(STATUS "gxde-qdbusxml2cpp not found, building from source...")
    execute_process(
        COMMAND ${CMAKE_COMMAND}
            -S "${TOOL_SRC}"
            -B "${CMAKE_BINARY_DIR}/tool-build"
            -DCMAKE_BUILD_TYPE=Release
        RESULT_VARIABLE _tool_config_res
        OUTPUT_VARIABLE _tool_config_out
        ERROR_VARIABLE _tool_config_err
    )
    if(NOT _tool_config_res EQUAL 0)
        message(FATAL_ERROR "Failed to configure gxde-qdbusxml2cpp:\n${_tool_config_err}")
    endif()

    execute_process(
        COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}/tool-build"
        RESULT_VARIABLE _tool_build_res
        OUTPUT_VARIABLE _tool_build_out
        ERROR_VARIABLE _tool_build_err
    )
    if(NOT _tool_build_res EQUAL 0)
        message(FATAL_ERROR "Failed to build gxde-qdbusxml2cpp:\n${_tool_build_err}")
    endif()

    file(COPY "${CMAKE_BINARY_DIR}/tool-build/gxde-qdbusxml2cpp"
         DESTINATION "${PROJECT_SOURCE_DIR}")
    message(STATUS "gxde-qdbusxml2cpp built successfully")
endif()

# generate code
execute_process(
    COMMAND python3 ${PROJECT_SOURCE_DIR}/generate_code.py
        --src-dir ${PROJECT_SOURCE_DIR}
        --build-dir ${CMAKE_CURRENT_BINARY_DIR}
        --xml-dir ${PROJECT_SOURCE_DIR}/xml
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    RESULT_VARIABLE RESULT_GEN
    OUTPUT_VARIABLE GEN_OUTPUT
    ERROR_VARIABLE ERROR_GEN
)

if(NOT RESULT_GEN EQUAL 0)
    message(FATAL_ERROR "generate_code.py failed (exit ${RESULT_GEN}):\n${ERROR_GEN}\n${GEN_OUTPUT}")
endif()

include(${CMAKE_CURRENT_BINARY_DIR}/generated.cmake)
