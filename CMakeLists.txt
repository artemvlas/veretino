cmake_minimum_required(VERSION 3.5)

project(veretino VERSION 0.5.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#set(CMAKE_BUILD_TYPE Release)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_definitions(-DAPP_NAME="Veretino")
add_definitions(-DAPP_VERSION="${CMAKE_PROJECT_VERSION}")
add_definitions(-DAPP_NAME_VERSION="Veretino ${CMAKE_PROJECT_VERSION}")

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Svg)
#find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Svg)

set(PROJECT_SOURCES
    # HEADERS
    src/clickablelabel.h
    src/datacontainer.h
    src/datamaintainer.h
    src/dialogabout.h
    src/dialogdbcreation.h
    src/dialogcontentslist.h
    src/dialogdbstatus.h
    src/dialogexistingdbs.h
    src/dialogfileprocresult.h
    src/dialogsettings.h
    src/files.h
    src/filterrule.h
    src/iconprovider.h
    src/itemfiletype.h
    src/jsondb.h
    src/lineedit.h
    src/mainwindow.h
    src/manager.h
    src/menuactions.h
    src/modeselector.h
    src/numbers.h
    src/procstate.h
    src/progressbar.h
    src/proxymodel.h
    src/settings.h
    src/shacalculator.h
    src/statusbar.h
    src/tools.h
    src/treeitem.h
    src/treemodel.h
    src/treemodeliterator.h
    src/view.h
    src/widgetfiletypes.h

    # SOURCES
    src/clickablelabel.cpp
    src/datacontainer.cpp
    src/datamaintainer.cpp
    src/dialogabout.cpp
    src/dialogdbcreation.cpp
    src/dialogcontentslist.cpp
    src/dialogdbstatus.cpp
    src/dialogexistingdbs.cpp
    src/dialogfileprocresult.cpp
    src/dialogsettings.cpp
    src/files.cpp
    src/filterrule.cpp
    src/iconprovider.cpp
    src/itemfiletype.cpp
    src/jsondb.cpp
    src/lineedit.cpp
    src/main.cpp
    src/mainwindow.cpp
    src/manager.cpp
    src/menuactions.cpp
    src/modeselector.cpp
    src/numbers.cpp
    src/procstate.cpp
    src/progressbar.cpp
    src/proxymodel.cpp
    src/settings.cpp
    src/shacalculator.cpp
    src/statusbar.cpp
    src/tools.cpp
    src/treeitem.cpp
    src/treemodel.cpp
    src/treemodeliterator.cpp
    src/view.cpp
    src/widgetfiletypes.cpp

    # FORMS
    src/dialogabout.ui
    src/dialogdbcreation.ui
    src/dialogcontentslist.ui
    src/dialogdbstatus.ui
    src/dialogexistingdbs.ui
    src/dialogfileprocresult.ui
    src/dialogsettings.ui
    src/mainwindow.ui

    # RESOURCES
    res/resources.qrc
)

# ${PROJECT_NAME} is veretino
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME}
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(${PROJECT_NAME}
        ${PROJECT_SOURCES}
    )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Svg
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
#if(${QT_VERSION} VERSION_LESS 6.1.0)
#  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.veretino)
#endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(veretino)
endif()

# Post target stuff
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

    # Install desktop entry
    install(FILES res/veretino.desktop DESTINATION /usr/share/applications)

    # Install icon
    install(FILES res/icons/generic/veretino.svg DESTINATION /usr/share/icons/hicolor/scalable/apps)

endif()
