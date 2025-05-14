##############
## Veretino ##
##############
cmake_minimum_required(VERSION 3.5)

project(veretino VERSION 0.6.2 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_BUILD_TYPE Release)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories("include")

if(WIN32)
    # folder containing libqmicroz.dll
    link_directories("libs")
endif()

add_definitions(-DAPP_NAME="Veretino")
add_definitions(-DAPP_VERSION="${CMAKE_PROJECT_VERSION}")
add_definitions(-DAPP_NAME_VERSION="Veretino ${CMAKE_PROJECT_VERSION}")

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Svg)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Svg)

add_subdirectory(src)

# Post target stuff
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(CMAKE_INSTALL_PREFIX "/usr")

    include(GNUInstallDirs)
    install(TARGETS ${PROJECT_NAME}
        BUNDLE DESTINATION .
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    # Install desktop entry
    install(FILES res/veretino.desktop DESTINATION /usr/share/applications)

    # Install icon
    install(FILES res/icons/generic/veretino.svg DESTINATION /usr/share/icons/hicolor/scalable/apps)

endif()
