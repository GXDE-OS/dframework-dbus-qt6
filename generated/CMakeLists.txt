execute_process(
    COMMAND python3 ${PROJECT_SOURCE_DIR}/generate_code.py
        --src-dir ${PROJECT_SOURCE_DIR}
        --build-dir ${CMAKE_CURRENT_BINARY_DIR}
        --xml-dir ${PROJECT_SOURCE_DIR}/../xml
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    RESULT_VARIABLE RESULT_GEN
    OUTPUT_VARIABLE GEN_OUTPUT
    ERROR_VARIABLE ERROR_GEN
)

if(NOT RESULT_GEN EQUAL 0)
    message(WARNING "generate_code.py failed (exit ${RESULT_GEN}): ${ERROR_GEN}")
endif()

include(${CMAKE_CURRENT_BINARY_DIR}/generated.cmake)
