set(TARGET_NAME libtestutils)

set(PROJECT_SOURCES
    testutils.cpp
)

qt_add_library(${TARGET_NAME} STATIC
    ${PROJECT_SOURCES}
)

target_include_directories(${TARGET_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}     # Adds test/utils/ to the include paths
    ${CMAKE_CURRENT_SOURCE_DIR}/../ # Adds test/ to the include paths
)

target_link_libraries(${TARGET_NAME} PUBLIC Qt6::Widgets)
