cmake_minimum_required(VERSION 3.16)


set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 6)
set(PROJECT_VERSION_PATCH 0)
set(VERSION_SUFFIX "alpha")

set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

if(VERSION_SUFFIX)
    set(PROJECT_FULL_VERSION "${PROJECT_VERSION}-${VERSION_SUFFIX}")
else()
    set(PROJECT_FULL_VERSION "${PROJECT_VERSION}")
endif()
message("PROJECT_VERSION ${PROJECT_VERSION}")
message("PROJECT_FULL_VERSION ${PROJECT_FULL_VERSION}")

set(ORGANIZATION_NAME "llahteinen")
set(ORGANIZATION_DOMAIN "llahteinen.com") # NOTE: fake domain

project(NotPad VERSION ${PROJECT_VERSION} LANGUAGES CXX)


set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
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
    add_compile_options(-Weffc++ -Wno-error=effc++)
    add_compile_options(-Werror)
    # Selectively treat some warnings as warnings instead of errors
    add_compile_options(-Wno-error=deprecated-declarations) # The [[deprecated]] attribute
endif()
# Override the above if needed in unittest using target_compile_options()

include(GNUInstallDirs)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)


add_subdirectory(src)
add_subdirectory(test)

