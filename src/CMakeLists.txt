set(TARGET_NAME lib${PROJECT_NAME})
set(EXE_TARGET_NAME ${PROJECT_NAME})

set(PROJECT_SOURCES
        forms/notpad.ui
        notpad.hpp
        notpad.cpp
        settings.hpp
        settings.cpp
        tab.hpp
        tab.cpp
        editor.hpp
        editor.cpp
        file.hpp
        file.cpp
        utils/namefilterlist.hpp
        utils/namefilterlist.cpp
)

set(RESOURCE_FILES
    forms/todo.txt
    forms/styles.css
    res/icon.ico
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_library(${TARGET_NAME} STATIC
        ${PROJECT_SOURCES}
    )
else()
    add_library(${TARGET_NAME} STATIC
        ${PROJECT_SOURCES}
    )
endif()

qt6_add_resources(${TARGET_NAME} "resources"
    FILES
        ${RESOURCE_FILES}
)

target_include_directories(${TARGET_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}  # Adds src/ to the include paths
)

target_link_libraries(${TARGET_NAME} PUBLIC Qt${QT_VERSION_MAJOR}::Widgets)

target_compile_definitions(${TARGET_NAME} PRIVATE PROJECT_NAME="${PROJECT_NAME}")
target_compile_definitions(${TARGET_NAME} PRIVATE PROJECT_VERSION="${PROJECT_VERSION}")
target_compile_definitions(${TARGET_NAME} PRIVATE ORGANIZATION_NAME="${ORGANIZATION_NAME}")
target_compile_definitions(${TARGET_NAME} PRIVATE ORGANIZATION_DOMAIN="${ORGANIZATION_DOMAIN}")

target_compile_definitions(${TARGET_NAME} PRIVATE
#    $<$<CONFIG:Debug>:
#    >
    $<$<CONFIG:Release>:
        QT_NO_DEBUG_OUTPUT   # Disable qDebug output
    >
)
target_compile_options(${TARGET_NAME} PRIVATE
    $<$<CONFIG:Release>:
        $<$<CXX_COMPILER_ID:MSVC>:
            /O1
            /MD
        >
    >
)

set(EXE_SOURCES
        main.cpp
)
if(WIN32)
    list(APPEND EXE_SOURCES resource.rc)
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${EXE_TARGET_NAME}
        MANUAL_FINALIZATION
        ${EXE_SOURCES}
    )
else()
    add_executable(${EXE_TARGET_NAME}
        ${EXE_SOURCES}
    )
endif()

target_link_libraries(${EXE_TARGET_NAME} PRIVATE
    ${TARGET_NAME}
)

# When this property is set to true the executable when linked on Windows will be created with a WinMain() entry point
# instead of just main(). This makes it a GUI executable instead of a console application.
# qDebug etc will go to system debug log instead of console. Stdout will go nowhere.
# If DebugView is open, QtCreator can't print debug out
set_target_properties(${EXE_TARGET_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
)

install(TARGETS ${TARGET_NAME} ${EXE_TARGET_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${EXE_TARGET_NAME})
endif()
