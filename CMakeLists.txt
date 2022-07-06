# SPDX-FileCopyrightText: Copyright (c) 2009-2022 TRUMPF Laser GmbH, authors: Daniel Schnabel
#
# SPDX-License-Identifier: MPL-2.0

cmake_minimum_required(VERSION 3.14)
project(trumpf-qds-buffer-core VERSION 0.1)

option(INSTALL_PUBLIC_HEADER "INSTALL_PUBLIC_HEADER" ON)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

# Set CXX to 17
if(NOT DEFINED CMAKE_CXX_STANDARD OR CMAKE_CXX_STANDARD LESS 17)
    set(CMAKE_CXX_STANDARD 17)
    message(WARNING "Minimum C++17 required. Setting standard to 17.")
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS json REQUIRED)

add_library(${PROJECT_NAME}
  src/ring_buffer.cpp
  src/data_source_internal.cpp
  src/data_source_factory.cpp
  src/parsing/data_validator.cpp
)

# Generate export header to define "__declspec(dllexport)".
# If built as static lib, the generated header contains empty defines.
include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME} BASE_NAME qds_core)

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION 1)

if (INSTALL_PUBLIC_HEADER)
    set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "\
    include/data_source_factory.hpp;\
    include/i_data_source_in.hpp;\
    include/i_data_source_out.hpp;\
    include/i_data_source_in_out.hpp;\
    include/measurement.hpp;\
    include/types.hpp;\
    include/exception.hpp;\
    ${PROJECT_BINARY_DIR}/qds_core_export.h\
    ")
endif()

target_include_directories(${PROJECT_NAME} PUBLIC include ${PROJECT_BINARY_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${Boost_LIBRARIES})

target_compile_options(${PROJECT_NAME} PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)

install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)

################################
# Unit Tests
################################
option(TESTING "TESTING" ON)
option(USE_SYSTEM_GTEST "USE_SYSTEM_GTEST" OFF)

if (TESTING)
    if (USE_SYSTEM_GTEST)
        find_package(GTest)
    else()
        ### install GTest
        include(FetchContent)

        FetchContent_Declare(
          GoogleTest
          GIT_REPOSITORY https://github.com/google/googletest.git
          GIT_TAG        release-1.11.0
        )
        
        set(INSTALL_GTEST OFF)
        
        # For Windows: Prevent overriding the parent project's compiler/linker settings
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(GoogleTest)
    endif()
    
    include(GoogleTest)

    ### build tests
    add_executable(${PROJECT_NAME}-tests
      src/ring_buffer.test.cpp
      src/data_source_internal.test.cpp
      src/parsing/data_validator.test.cpp
    )
	
if(WIN32 AND BUILD_SHARED_LIBS)
    target_sources(${PROJECT_NAME}-tests PRIVATE $<TARGET_OBJECTS:${PROJECT_NAME}>)
    if(NOT USE_SYSTEM_GTEST)
        # copy gtest DLLs
        add_custom_command(TARGET ${PROJECT_NAME}-tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>" $<TARGET_FILE_DIR:${PROJECT_NAME}-tests>)
    endif()
endif()

    target_include_directories(${PROJECT_NAME}-tests PRIVATE include)
    target_link_libraries(${PROJECT_NAME}-tests PRIVATE GTest::gtest_main ${PROJECT_NAME} ${Boost_LIBRARIES})

    gtest_discover_tests(${PROJECT_NAME}-tests)

    enable_testing()
endif()
