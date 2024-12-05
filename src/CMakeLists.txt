set(PROJECT_SOURCES
    # HEADERS
    clickablelabel.h
    datacontainer.h
    datamaintainer.h
    dialogabout.h
    dialogdbcreation.h
    dialogcontentslist.h
    dialogdbstatus.h
    dialogexistingdbs.h
    dialogfileprocresult.h
    dialogsettings.h
    files.h
    filterrule.h
    iconprovider.h
    itemfiletype.h
    lineedit.h
    mainwindow.h
    manager.h
    menuactions.h
    modeselector.h
    numbers.h
    nums.h
    procstate.h
    progressbar.h
    proxymodel.h
    settings.h
    shacalculator.h
    statusbar.h
    tools.h
    treeitem.h
    treemodel.h
    treemodeliterator.h
    verdatetime.h
    verjson.h
    view.h
    widgetfiletypes.h

    # SOURCES
    clickablelabel.cpp
    datacontainer.cpp
    datamaintainer.cpp
    dialogabout.cpp
    dialogdbcreation.cpp
    dialogcontentslist.cpp
    dialogdbstatus.cpp
    dialogexistingdbs.cpp
    dialogfileprocresult.cpp
    dialogsettings.cpp
    files.cpp
    filterrule.cpp
    iconprovider.cpp
    itemfiletype.cpp
    lineedit.cpp
    main.cpp
    mainwindow.cpp
    manager.cpp
    menuactions.cpp
    modeselector.cpp
    numbers.cpp
    procstate.cpp
    progressbar.cpp
    proxymodel.cpp
    settings.cpp
    shacalculator.cpp
    statusbar.cpp
    tools.cpp
    treeitem.cpp
    treemodel.cpp
    treemodeliterator.cpp
    verjson.cpp
    view.cpp
    widgetfiletypes.cpp

    # FORMS
    dialogabout.ui
    dialogdbcreation.ui
    dialogcontentslist.ui
    dialogdbstatus.ui
    dialogexistingdbs.ui
    dialogfileprocresult.ui
    dialogsettings.ui
    mainwindow.ui

    # RESOURCES
    ../res/resources.qrc
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

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(veretino)
endif()
