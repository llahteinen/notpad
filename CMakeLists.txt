cmake_minimum_required(VERSION 3.22)

string(TIMESTAMP BUILD_YYWW "%y%V" UTC)   # Year-WeekOfYear e.g. "2643", using ISO 8601 week numbers.

set(BUILD_N 0) # Running number for the build, between 0...9. Increment this if there are more than one release on the same week.

if(BUILD_N GREATER 9)
    message(FATAL_ERROR "BUILD_N has to be smaller than 10")
endif()

set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION_TWEAK ${BUILD_YYWW}${BUILD_N})
set(VERSION_SUFFIX "rc1")

set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}")
set(PROJECT_SHORT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

if(VERSION_SUFFIX)
    set(PROJECT_FULL_VERSION "${PROJECT_VERSION}-${VERSION_SUFFIX}")
else()
    set(PROJECT_FULL_VERSION "${PROJECT_VERSION}")
endif()
message("PROJECT_VERSION ${PROJECT_VERSION}")
message("PROJECT_FULL_VERSION ${PROJECT_FULL_VERSION}")
message("PROJECT_SHORT_VERSION ${PROJECT_SHORT_VERSION}")

set(ORGANIZATION_NAME "llahteinen")
set(ORGANIZATION_DOMAIN "llahteinen.com") # NOTE: fake domain
set(PROJECT_URL "https://github.com/llahteinen/qutepad")

project(QutePad VERSION ${PROJECT_VERSION} LANGUAGES CXX)


set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
#    add_compile_options($<$<CONFIG:Release>:/O1>) # O2 is MSVC default
    add_compile_options(/W4)     # Enable all normal warnings (Level 1–4)
#    add_compile_options(/Wall)   # /Wall gives warnings in moc generated files and other silly warnings
    add_compile_options(/WX)     # All compiler warnings as errors
    # Selectively enable some warnings or errors
    add_compile_options(/we5262) # switch case implicit fall-through
    # Selectively disable some warnings
    # /wd disables the warning. With MSVC there is no way to selectively treat something only as warning but not error :<
#    add_compile_options(/wd4996) # The [[deprecated]] attribute
else()
    add_compile_options(-Wall)
    add_compile_options(-Wextra)
    add_compile_options(-Wpedantic)
    add_compile_options(-Werror)
    # Selectively treat some warnings as warnings instead of errors
    add_compile_options(-Wno-error=deprecated-declarations) # The [[deprecated]] attribute

    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(-Weffc++ -Wno-error=effc++)
    endif()

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-Wno-gnu-zero-variadic-macro-arguments) # Clang gave errors with some Qt macros
        # Additional common clang + Qt options if getting errors later with Qt code
#        add_compile_options(-Wno-zero-as-null-pointer-constant)
#        add_compile_options(-Wno-deprecated-declarations)
    endif()
endif()
# Override the above if needed in unittest for more relaxed setup

include(GNUInstallDirs)

# Does not compile with Qt5
# Compiles with Qt 6.8.1 MSVC
find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
# SingleApplication uses Network


add_subdirectory(src)
add_subdirectory(test)

