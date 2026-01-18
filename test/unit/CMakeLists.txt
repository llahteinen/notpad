set(TARGET_NAME unittest)

set(PROJECT_SOURCES

)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${TARGET_NAME}
        ${PROJECT_SOURCES}
    )
else()
    add_executable(${TARGET_NAME}
        ${PROJECT_SOURCES}
    )
endif()

target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

find_package(Qt6 REQUIRED COMPONENTS Test)
target_link_libraries(${TARGET_NAME} PRIVATE
    lib${PROJECT_NAME}
    Qt6::Test
)
