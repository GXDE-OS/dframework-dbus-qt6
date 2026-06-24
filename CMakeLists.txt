cmake_minimum_required(VERSION 3.16)
project(DFrameworkDBusQt6 VERSION 6.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

add_compile_options(
    -fstack-protector-strong
    -D_FORTIFY_SOURCE=1
    -fPIC
)

add_link_options(
    -Wl,-z,noexecstack
    -Wl,-z,lazy
    -pie
)

find_package(Qt6 REQUIRED COMPONENTS DBus)

add_subdirectory(generated)

set(QTDBUSEXTENDED_SOURCES
    src/qtdbusextended/dbusextendedabstractinterface.cpp
    src/qtdbusextended/dbusextendedpendingcallwatcher.cpp
)

set(QTDBUSEXTENDED_HEADERS
    src/qtdbusextended/DBusExtended
    src/qtdbusextended/dbusextended.h
    src/qtdbusextended/DBusExtendedAbstractInterface
    src/qtdbusextended/dbusextendedabstractinterface.h
    src/qtdbusextended/dbusextendedpendingcallwatcher_p.h
)

set(TYPES_SOURCES
    src/types/arrayint.cpp
    src/types/zoneinfo.cpp
    src/types/screenrect.cpp
    src/types/inputdevicelist.cpp
    src/types/localelist.cpp
    src/types/mfainfolist.cpp
    src/types/audioport.cpp
    src/types/audioportlist.cpp
    src/types/keyboardlayoutlist.cpp
    src/types/arealist.cpp
    src/types/qvariantlist.cpp
    src/types/brightnessmap.cpp
    src/types/touchscreenmap.cpp
    src/types/touchscreeninfolist.cpp
    src/types/touchscreeninfolist_v2.cpp
    src/types/batterypresentinfo.cpp
    src/types/batterystateinfo.cpp
    src/types/batterypercentageinfo.cpp
    src/types/wacomdevicelist.cpp
    src/types/searchmap.cpp
    src/types/resolutionlist.cpp
    src/types/resolution.cpp
    src/types/rotationlist.cpp
    src/types/reflectlist.cpp
    src/types/appupdateinfolist.cpp
    src/types/mirrorinfolist.cpp
    src/types/windowlist.cpp
    src/types/networkerrors.cpp
    src/types/networkconnectiondata.cpp
    src/types/networkavailablekeys.cpp
    src/types/propertylist.cpp
    src/types/dbusobjectpathlist.cpp
    src/types/linkinfolist.cpp
    src/types/sinkinfolist.cpp
    src/types/qvariantmap.cpp
    src/types/appscgroupinfolist.cpp
    src/types/chrometablist.cpp
    src/types/windowinfomap.cpp
    src/types/dbusimagelist.cpp
    src/types/dbustooltip.cpp
    src/types/dockrect.cpp
    src/types/intstring.cpp
    src/types/controlcenterrect.cpp
    src/types/defenderversioninfo.cpp
    src/types/defenderfileinfo.cpp
    src/types/defenderupdatestate.cpp
    src/types/defenderdissection.cpp
    src/types/launcheriteminfo.cpp
    src/types/launcheriteminfolist.cpp
    src/types/hardwareinfo.cpp
    src/types/defenderprocesslist.cpp
    src/types/defenderprocessinfo.cpp
    src/types/defenderdisplay.cpp
    src/types/defenderappflow.cpp
    src/types/defenderprocinfo.cpp
    src/types/defenderprocinfolist.cpp
    src/types/policysubject.cpp
    src/types/policyresult.cpp
    src/types/policydetails.cpp
    src/types/lastoreupdatepackagesinfo.cpp
)

set(TYPES_HEADERS
    src/types/arrayint.h
    src/types/zoneinfo.h
    src/types/audioport.h
    src/types/audioportlist.h
    src/types/batterypercentageinfo.h
    src/types/batterypresentinfo.h
    src/types/batterystateinfo.h
    src/types/brightnessmap.h
    src/types/touchscreenmap.h
    src/types/touchscreeninfolist.h
    src/types/touchscreeninfolist_v2.h
    src/types/screenrect.h
    src/types/inputdevicelist.h
    src/types/keyboardlayoutlist.h
    src/types/localelist.h
    src/types/mfainfolist.h
    src/types/arealist.h
    src/types/qvariantlist.h
    src/types/wacomdevicelist.h
    src/types/searchmap.h
    src/types/resolutionlist.h
    src/types/resolution.h
    src/types/rotationlist.h
    src/types/reflectlist.h
    src/types/appupdateinfolist.h
    src/types/mirrorinfolist.h
    src/types/windowlist.h
    src/types/networkerrors.h
    src/types/networkconnectiondata.h
    src/types/networkavailablekeys.h
    src/types/propertylist.h
    src/types/dbusobjectpathlist.h
    src/types/linkinfolist.h
    src/types/sinkinfolist.h
    src/types/qvariantmap.h
    src/types/appscgroupinfo.h
    src/types/appscgroupinfolist.h
    src/types/chrometablist.h
    src/types/windowinfomap.h
    src/types/dbusimagelist.h
    src/types/dbustooltip.h
    src/types/dockrect.h
    src/types/intstring.h
    src/types/controlcenterrect.h
    src/types/defenderversioninfo.h
    src/types/defenderfileinfo.h
    src/types/defenderupdatestate.h
    src/types/defenderdissection.h
    src/types/launcheriteminfo.h
    src/types/launcheriteminfolist.h
    src/types/hardwareinfo.h
    src/types/defenderprocesslist.h
    src/types/defenderprocessinfo.h
    src/types/defenderdisplay.h
    src/types/defenderappflow.h
    src/types/defenderprocinfo.h
    src/types/defenderprocinfolist.h
    src/types/policysubject.h
    src/types/policyresult.h
    src/types/policydetails.h
    src/types/lastoreupdatepackagesinfo.h
)

add_library(dframeworkdbus_qt6 SHARED
    ${GENERATED_SOURCES}
    ${GENERATED_HEADERS}
    ${QTDBUSEXTENDED_SOURCES}
    ${QTDBUSEXTENDED_HEADERS}
    ${TYPES_SOURCES}
    ${TYPES_HEADERS}
)

target_include_directories(dframeworkdbus_qt6
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/qtdbusextended>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/qtdbusextended
        ${CMAKE_CURRENT_BINARY_DIR}/generated
)

target_compile_definitions(dframeworkdbus_qt6 PRIVATE
    QT_DBUS_EXTENDED_LIBRARY
    QT_NO_KEYWORDS
)

target_link_libraries(dframeworkdbus_qt6 PUBLIC
    Qt6::DBus
)

set_target_properties(dframeworkdbus_qt6 PROPERTIES
    OUTPUT_NAME dframeworkdbus-qt6
    VERSION ${PROJECT_VERSION}
    SOVERSION 6
    EXPORT_NAME DFrameworkdbusQt6
)

include(GNUInstallDirs)

set(INCLUDES_HEADERS
    ${QTDBUSEXTENDED_HEADERS}
    ${GENERATED_HEADERS}
)

install(TARGETS dframeworkdbus_qt6
    EXPORT DFrameworkdbusQt6Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libdframeworkdbus-qt6-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

install(FILES ${INCLUDES_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libdframeworkdbus-qt6-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

install(FILES ${TYPES_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libdframeworkdbus-qt6-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}/types
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    DFrameworkdbusQt6Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/DFrameworkdbusQt6Config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/DFrameworkdbusQt6
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/DFrameworkdbusQt6ConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/DFrameworkdbusQt6Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/DFrameworkdbusQt6ConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/DFrameworkdbusQt6
)

install(EXPORT DFrameworkdbusQt6Targets
    FILE DFrameworkdbusQt6Targets.cmake
    NAMESPACE DFrameworkdbusQt6::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/DFrameworkdbusQt6
)
