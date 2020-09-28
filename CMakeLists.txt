cmake_minimum_required(VERSION 3.1)
project(vulkanarray VERSION 0.1.0)

SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

##################################################################################
#                                 SDL2 SECTION
##################################################################################

# Find SDL2 and OpenGL
# Replace this with your vcpkg location
include(D:/code/vcpkg/scripts/buildsystems/vcpkg.cmake)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/modules")
find_package(SDL2 REQUIRED)
find_package(xtensor REQUIRED)
find_package(xtl REQUIRED)
find_package(xsimd REQUIRED)
if(NOT WIN32)
    find_package(OpenGL REQUIRED)
endif()

# Manually find pages in case your system is dumb
if (NOT ${SDL2_INCLUDE_DIR})
    set(SDL2_INCLUDE_DIR "D:/code/vcpkg/installed/x86-windows/include/SDL2")
    IF(NOT EXISTS ${SDL2_INCLUDE_DIR})
        message(FATAL_ERROR "Could not find SDL2_INCLUDE_DIR. It's either not installed, or you may need to manually provide the location (especially on windows).")
    endif()
endif()
if (NOT ${SDL2_LIBRARY})
    set(SDL2_LIBRARY "D:/code/vcpkg/installed/x86-windows/lib/SDL2.lib")
    IF(NOT EXISTS ${SDL2_LIBRARY})
        message(FATAL_ERROR "Could not find SDL2_LIBRARY. It's either not installed, or you may need to manually provide the location (especially on windows).")
    endif()
endif()
if (NOT DEFINED SDL2_DLL)
    set(SDL2_DLL "D:/code/vcpkg/installed/x86-windows/bin/SDL2.dll")
    IF(NOT EXISTS ${SDL2_DLL})
        message(FATAL_ERROR "Could not find SDL2_DLL. It's either not installed, or you may need to manually provide the location (especially on windows).")
    endif()
endif()

if (NOT DEFINED ESCAPI_DLL)
    set(ESCAPI_DLL "${CMAKE_CURRENT_LIST_DIR}/bin/Win32/escapi.dll")
    IF(NOT EXISTS ${ESCAPI_DLL})
        message(FATAL_ERROR "Could not find ESCAPI_DLL. It's either not installed, or you may need to manually provide the location (especially on windows).")
    endif()
endif()

# get_cmake_property(_variableNames VARIABLES)
# list (SORT _variableNames)
# foreach (_variableName ${_variableNames})
#     message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()



include_directories( vulkanarray ${xtensor_INCLUDE_DIRS} ${xsimd_INCLUDE_DIRS} ${xtl_INCLUDE_DIRS} ${SDL2_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${CMAKE_BINARY_DIR})
add_executable(vulkanarray src/main.cpp src/escapi.h src/escapi.cpp)
target_link_libraries(vulkanarray ${SDL2_LIBRARY} ${OPENGL_LIBRARIES})

include_directories( denoising_example ${xtensor_INCLUDE_DIRS} ${xsimd_INCLUDE_DIRS} ${xtl_INCLUDE_DIRS} ${SDL2_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${CMAKE_BINARY_DIR})
add_executable(denoising_example examples/denoinsing_autoencoder.cpp src/escapi.h src/escapi.cpp)
target_link_libraries(denoising_example ${SDL2_LIBRARY} ${OPENGL_LIBRARIES})

# Copy SDL2 DLL to output folder on Windows
# https://github.com/preshing/CMakeDemo/blob/master/CMakeLists.txt
if(WIN32)
    add_custom_command(TARGET vulkanarray POST_BUILD COMMAND
        ${CMAKE_COMMAND} -E copy_if_different ${SDL2_DLL} "$<TARGET_FILE_DIR:vulkanarray>")
    add_custom_command(TARGET vulkanarray POST_BUILD COMMAND
        ${CMAKE_COMMAND} -E copy_if_different ${ESCAPI_DLL} "$<TARGET_FILE_DIR:vulkanarray>")
endif()


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

find_package(Catch2 REQUIRED)
enable_testing()
add_subdirectory(lib/Catch2)
include_directories( catch_test ${xtensor_INCLUDE_DIRS} ${xsimd_INCLUDE_DIRS} ${xtl_INCLUDE_DIRS} ${SDL2_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${CMAKE_BINARY_DIR})
add_executable(catch_test test/test.cpp src/conv.hpp)
target_link_libraries(catch_test Catch2::Catch2)

include(CTest)
include(Catch)
catch_discover_tests(catch_test)