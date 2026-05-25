
if (MSVC)
    add_compile_options(/wd4189) # Removes unused variable warning and error
    add_compile_options(/wd4702) # Removes unreachable code warning and error
else()
    add_compile_options(-Wno-error=unused-variable) # Turns unused-variable from error to warning
endif()

# Add each test cpp file here
set(PROJECT_SOURCES
    test_namefilterlist
    test_encodings
    test_textstream
    test_highlighter
    test_search
    test_utils
)

# Each of them will generate a separate executable
foreach(TEST ${PROJECT_SOURCES})
    set(TARGET_NAME unit${TEST})
    message("TEST ${TARGET_NAME}")

    if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
        qt_add_executable(${TARGET_NAME}
            ${TEST}.cpp
        )
    else()
        add_executable(${TARGET_NAME}
            ${TEST}.cpp
        )
    endif()

    #add_test(NAME test_${TARGET_NAME} COMMAND ${TARGET_NAME} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}) # Not sure what this is used for

    target_include_directories(${TARGET_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    find_package(Qt6 REQUIRED COMPONENTS Test)
    target_link_libraries(${TARGET_NAME} PRIVATE
        lib${PROJECT_NAME}
        Qt6::Test
        libtestutils
    )

    # Copy test data to build dir
    add_custom_command(
        TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/testdata" # From
            "${CMAKE_CURRENT_BINARY_DIR}/testdata" # To
    )
endforeach()
