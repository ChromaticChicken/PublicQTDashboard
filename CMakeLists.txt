cmake_minimum_required(VERSION 3.5)

project(goalsDashboard VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS WebEngineWidgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS WebChannel)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Multimedia)
find_package(Threads REQUIRED)
find_package(Python REQUIRED COMPONENTS Interpreter Development)
include_directories(${Python_INCLUDE_DIRS})

message(STATUS "Python include directories: ${Python_INCLUDE_DIRS}")

set(PROJECT_SOURCES
        main.cpp
        goalsdashboard.cpp
        goalsdashboard.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(goalsDashboard
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        areaframe.h areaframe.cpp
        fullframe.h fullframe.cpp
        ebayframe.h ebayframe.cpp
        ebay.config.json
        ebayordersframe.h ebayordersframe.cpp
        ebaymessagesframe.h ebaymessagesframe.cpp
        ebayinfoframe.h ebayinfoframe.cpp
        ebaygoalsframe.h ebaygoalsframe.cpp
        ebaycache.h ebaycache.cpp
        README.md
    )

set (CONFIG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/config.json)
set (EBAY_CONFIG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/ebay.config.json)
set (BYU_PNG ${CMAKE_CURRENT_SOURCE_DIR}/BYU.png)
set (MY_PYTHON_FILE ${CMAKE_CURRENT_SOURCE_DIR}/refreshRefreshToken.py)
set (KONAMICODE ${CMAKE_CURRENT_SOURCE_DIR}/konamiCode)
set (DEST_DIR ${CMAKE_CURRENT_BINARY_DIR})
set (KONAMICODE_DEST ${CMAKE_CURRENT_BINARY_DIR}/konamiCode)




add_custom_command(
    TARGET goalsDashboard POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CONFIG_FILE} ${DEST_DIR}
    COMMENT "Copying config.json to build directory"
)
add_custom_command (
    TARGET goalsDashboard POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${BYU_PNG} ${DEST_DIR}
    COMMENT "Copying BYU.png to build directory"
)
add_custom_command (
    TARGET goalsDashboard POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${EBAY_CONFIG_FILE} ${DEST_DIR}
    COMMENT "Copying ebay.config.json to build directory"
)

add_custom_command(
    TARGET goalsDashboard POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${KONAMICODE} ${KONAMICODE_DEST}
    COMMENT "Copying konamiCode folder to build directory"
)

add_custom_command(
    TARGET goalsDashboard POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${MY_PYTHON_FILE} ${DEST_DIR}
    COMMENT "Copying refreshRefreshToken.py to build directory"
)

# Define target properties for Android with Qt 6 as:
#    set_property(TARGET goalsDashboard APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(goalsDashboard SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(goalsDashboard
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(goalsDashboard PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::WebEngineWidgets
    Qt${QT_VERSION_MAJOR}::WebChannel
    Qt${QT_VERSION_MAJOR}::Multimedia
    Threads::Threads
    ${Python_LIBRARIES}
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.goalsDashboard)
endif()
set_target_properties(goalsDashboard PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS goalsDashboard
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(goalsDashboard)
endif()
