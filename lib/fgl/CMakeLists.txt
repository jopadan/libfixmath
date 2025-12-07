cmake_minimum_required( VERSION 3.10 )

project( fgl VERSION 0.1 LANGUAGES C )
set(PROJECT_DESCRIPTION "fgl fixedpoint math OpenGL library")

set( CMAKE_C_STANDARD 23 )
set( CMAKE_C_STANDARD_REQUIRED ON )
set( CMAKE_C_EXTENSIONS ON )

include(GNUInstallDirs)

if( NOT CMAKE_BUILD_TYPE )
        set( CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE )
endif()

if( CMAKE_BUILD_TYPE EQUAL Debug )
	add_compile_options(-g)
endif()
if( CMAKE_GENERATOR_PLATFORM STREQUAL "x64" OR CMAKE_GENERATOR_PLATFORM STREQUAL "x86_64" OR CMAKE_GENERATOR_PLATFORM STREQUAL "AMD64" )
	add_compile_options(-mfpmath=sse)
endif()

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_STATIC_LIBS "Build static libraries" ON)
option(BUILD_EXAMPLES "Build examples/samples" ON)

add_compile_options(-march=native -O3)
add_compile_options(-D_FILE_OFFSET_BITS=64)
add_compile_options(-fdata-sections)
add_compile_options(-fpermissive)
add_compile_options(-ffunction-sections)
add_compile_options(-Wno-array-bounds)
add_compile_options(-Wno-unused-parameter)
add_compile_options(-Wno-unused-result)
add_compile_options(-Wno-pedantic)
add_compile_options(-Wno-narrowing)
add_link_options(-Wl,--gc-sections)
add_link_options(-Wl,--print-gc-sections)
add_link_options(-Wl,-s)

FILE(GLOB FGL_SOURCES src/*.c)
include_directories(INTERFACE
        PUBLIC_HEADER $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        PUBLIC_HEADER $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/fgl>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(BUILD_SHARED_LIBS)
add_library( ${PROJECT_NAME}_shared SHARED ${FGL_SOURCES})
set_target_properties(${PROJECT_NAME}_shared PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
install(TARGETS ${PROJECT_NAME}_shared)
endif()

if(BUILD_STATIC_LIBS)
add_library( ${PROJECT_NAME}_static STATIC ${FGL_SOURCES})
set_target_properties(${PROJECT_NAME}_static PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
install(TARGETS ${PROJECT_NAME}_static)
endif()

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/fgl DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} COMPONENT headers)

add_library(${PROJECT_NAME} INTERFACE)

if(BUILD_EXAMPLES)
	FILE(GLOB FGL_SPINNING_CUBE_SOURCES samples/${PROJECT_NAME}_spinning_cube/*.c)
	find_package(SDL REQUIRED)
	add_executable(${PROJECT_NAME}_spinning_cube ${FGL_SPINNING_CUBE_SOURCES})
	target_link_libraries(fgl_static PRIVATE SDL::SDL fixmath)
endif()
