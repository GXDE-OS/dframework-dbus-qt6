cmake_minimum_required(VERSION 3.16)
project(gxde-qdbusxml2cpp LANGUAGES CXX)

# Build gxde-qdbusxml2cpp with Qt6 (host tool for code generation)
# NOTE: Not currently integrated into the main build due to
# chicken-and-egg problem (tool must be built before codegen,
# but execute_process runs at configure time).
# To use: build manually with `cmake --build . --target gxde-qdbusxml2cpp`,
# then re-run cmake to generate headers with the fix tool.

find_package(Qt6 REQUIRED COMPONENTS Core DBus)

# Qt packaging either exports private targets with the public components or
# provides them as separate packages. Support both layouts.
if(NOT TARGET Qt6::CorePrivate)
    find_package(Qt6CorePrivate REQUIRED)
endif()
if(NOT TARGET Qt6::DBusPrivate)
    find_package(Qt6DBusPrivate REQUIRED)
endif()

add_executable(gxde-qdbusxml2cpp
    qdbusxml2cpp.cpp
)

target_link_libraries(gxde-qdbusxml2cpp PRIVATE
    Qt6::Core
    Qt6::DBus
)

# Access dbus and core private headers
target_link_libraries(gxde-qdbusxml2cpp PRIVATE
    Qt6::CorePrivate
    Qt6::DBusPrivate
)

set_target_properties(gxde-qdbusxml2cpp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
