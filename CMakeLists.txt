cmake_minimum_required(VERSION 3.50)

project(libfixmath LANGUAGES CXX C DESCRIPTION "Cross Platform Fixed Point Maths Library")

include(GNUInstallDirs)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

if( NOT CMAKE_BUILD_TYPE )
        set( CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE )
endif()

if( CMAKE_BUILD_TYPE EQUAL Debug )
	add_compile_options(-g)
endif()
if( CMAKE_GENERATOR_PLATFORM STREQUAL "x64" OR CMAKE_GENERATOR_PLATFORM STREQUAL "x86_64" OR CMAKE_GENERATOR_PLATFORM STREQUAL "AMD64" )
	add_compile_options(-mfpmath=sse)
endif()
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

file(GLOB libfixmath-srcs src/*.c)
file(GLOB libfixmath-incs include/libfixmath/*.h)

option(BUILD_SHARED_LIBS "Build shared libraries!" ON)
option(BUILD_STATIC_LIBS "Build static libraries!" ON)
option(BUILD_TESTS       "Build tests." ON)

include_directories(PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/libfixmath>
	            PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

if(BUILD_SHARED_LIBS)
add_library(fixmath_shared SHARED ${libfixmath-srcs})
set_target_properties(fixmath_shared PROPERTIES OUTPUT_NAME fixmath)
install(TARGETS fixmath_shared)
endif()

if(BUILD_STATIC_LIBS)
add_library(fixmath_static STATIC ${libfixmath-srcs})
set_target_properties(fixmath_static PROPERTIES OUTPUT_NAME fixmath)
install(TARGETS fixmath_static)
endif()

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/libfixmath DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR} COMPONENT headers)

if(BUILD_TESTS)
    # We're in the root, define additional targets for developers.
    include(tests/tests.cmake)

    file(GLOB fixsingen-srcs utils/fixsingen/*.c)
    file(GLOB fixtest-srcs utils/fixtest/*.c utils/fixtest/*.h)

    add_executable(fixtest ${fixtest-srcs})
    target_link_libraries(fixtest PRIVATE fixmath_static m)
    target_include_directories(fixtest PRIVATE ${CMAKE_SOURCE_DIR})

    add_executable(fixsingen ${fixsingen-srcs})
    target_link_libraries(fixsingen PRIVATE fixmath_static m)
    target_include_directories(fixsingen PRIVATE ${CMAKE_SOURCE_DIR})
endif()

