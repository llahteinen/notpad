set(TARGET_NAME ${PROJECT_NAME})

set(PROJECT_SOURCES
        main.cpp
        forms/notpad.ui
        notpad.hpp
        notpad.cpp
)

set(RESOURCE_FILES
    forms/input.txt
    forms/styles.css
)


if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${TARGET_NAME}
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(${TARGET_NAME}
        ${PROJECT_SOURCES}
    )
endif()

qt6_add_resources(${TARGET_NAME} "resources"
    FILES
        ${RESOURCE_FILES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}  # Adds src/ to the include paths
)

target_link_libraries(${TARGET_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

target_compile_definitions(${TARGET_NAME} PRIVATE PROJECT_NAME="${PROJECT_NAME}")
target_compile_definitions(${TARGET_NAME} PRIVATE PROJECT_VERSION="${PROJECT_VERSION}")

# When this property is set to true the executable when linked on Windows will be created with a WinMain() entry point
# instead of just main(). This makes it a GUI executable instead of a console application.
# qDebug etc will go to system debug log instead of console. Stdout will go nowhere.
# If DebugView is open, QtCreator can't print debug out
set_target_properties(${TARGET_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
)

install(TARGETS ${TARGET_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${TARGET_NAME})
endif()
