cmake_minimum_required(VERSION 3.17.0)

# Define the project
file(STRINGS "std_execution.bs" STD_EXECUTION_BS_REVISION_LINE REGEX "Revision: [0-9]+")
string(REGEX REPLACE "Revision: ([0-9]+)" "\\1" STD_EXECUTION_BS_REVISION ${STD_EXECUTION_BS_REVISION_LINE})
project(P2300 LANGUAGES CXX VERSION "0.${STD_EXECUTION_BS_REVISION}.0")

option(P2300_ENABLE_CONAN "Enable Conan integration" ON)
option(P2300_ENABLE_TESTS "Enable tests" ON)
option(P2300_ENABLE_EXAMPLES "Enable examples" ON)

if (P2300_ENABLE_CONAN)
  if (EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
      include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
      conan_basic_setup(TARGETS)
  else ()
      message(FATAL_ERROR "The file conanbuildinfo.cmake doesn't exist, you have to run conan install first")
  endif ()
endif ()

# Print CMake configuration
message(STATUS "System           : ${CMAKE_SYSTEM}")
message(STATUS "System name      : ${CMAKE_SYSTEM_NAME}")
message(STATUS "System ver       : ${CMAKE_SYSTEM_VERSION}")
if (P2300_USE_CONAN)
  message(STATUS "Conan compiler   : ${CONAN_SETTINGS_COMPILER} ${CONAN_SETTINGS_COMPILER_VERSION}")
  message(STATUS "Conan libcxx     : ${CONAN_SETTINGS_COMPILER_LIBCXX}")
endif ()
message(STATUS)

# The main library
add_library(P2300 INTERFACE)

# Set the version and current build date
set(P2300_VERSION "${PROJECT_VERSION}")
set(P2300_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
string(TIMESTAMP P2300_BUILD_DATE "%Y-%m-%d")
string(TIMESTAMP P2300_BUILD_YEAR "%Y")

message(STATUS "Library ver      : ${P2300_VERSION}")
message(STATUS "Build date       : ${P2300_BUILD_DATE}")
message(STATUS "Build year       : ${P2300_BUILD_YEAR}")
message(STATUS)

# Set library version
set_target_properties(P2300 PROPERTIES
                      VERSION "${P2300_VERSION}"
                      SOVERSION "${P2300_VERSION_MAJOR}")

# Declare the public include directories
include(GNUInstallDirs)
target_include_directories(P2300 INTERFACE
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
                           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
                           )

# Ensure that we link with the threading library
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)
target_link_libraries(P2300 INTERFACE Threads::Threads)

# Use C++20 standard
target_compile_features(P2300 INTERFACE cxx_std_20)

# Proper use of libc++
if (CONAN_SETTINGS_COMPILER_LIBCXX STREQUAL "libc++")
    target_compile_options(P2300 INTERFACE -stdlib=libc++)
    target_link_options(P2300 INTERFACE -lc++)
endif ()

# Enable coroutines for GCC
target_compile_options(P2300 INTERFACE
                       $<$<BOOL:$<CXX_COMPILER_ID:GNU>>:
                       -fcoroutines>
                       )

# Install the library
include(CMakePackageConfigHelpers)

# Generate a config file
configure_package_config_file(P2300Config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/P2300Config.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/P2300")

# Generate a version file
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/P2300ConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion)

# Install the config and version files
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/P2300Config.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/P2300ConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/P2300")

# Export the P2300 target
install(
  TARGETS P2300
  EXPORT P2300Export
)

# Install headers
install(
  DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install configuration file for targets
install(
  EXPORT P2300Export
  NAMESPACE P2300::
  FILE P2300Targets.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/P2300
)

# Support target for examples and tests
add_library(P2300_executable_flags INTERFACE)

# Enable warnings
target_compile_options(P2300_executable_flags INTERFACE
                       $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
                       -Wall>
                       $<$<CXX_COMPILER_ID:MSVC>:
                       /W4>)

# Silence warnings with GCC
target_compile_options(P2300_executable_flags INTERFACE
                       $<$<BOOL:$<CXX_COMPILER_ID:GNU>>:
                       -Wno-non-template-friend>
                       )

# Template backtrace limit
target_compile_options(P2300_executable_flags INTERFACE
                       $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
                       -ftemplate-backtrace-limit=0>
                       )

# Always enable colored output
target_compile_options(P2300_executable_flags INTERFACE
                       $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:
                       -fcolor-diagnostics>
                       $<$<BOOL:$<CXX_COMPILER_ID:GNU>>:
                       -fdiagnostics-color=always>
                       )

# Don't require building everything when installing
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY ON)

# Set up test executable
if (P2300_ENABLE_TESTS)
  enable_testing()

  set(test_sourceFiles
      test/test_main.cpp
      test/cpos/test_cpo_bulk.cpp
      test/cpos/test_cpo_ensure_started.cpp
      test/cpos/test_cpo_receiver.cpp
      test/cpos/test_cpo_start.cpp
      test/cpos/test_cpo_connect.cpp
      test/cpos/test_cpo_schedule.cpp
      test/cpos/test_cpo_split.cpp
      test/cpos/test_cpo_upon_error.cpp
      test/cpos/test_cpo_upon_stopped.cpp
      test/concepts/test_concept_scheduler.cpp
      test/concepts/test_concepts_receiver.cpp
      test/concepts/test_concept_operation_state.cpp
      test/concepts/test_concepts_sender.cpp
      test/types/test_type_async_scope.cpp
      test/algos/factories/test_just.cpp
      test/algos/factories/test_transfer_just.cpp
      test/algos/factories/test_just_error.cpp
      test/algos/factories/test_just_stopped.cpp
      test/algos/adaptors/test_on.cpp
      test/algos/adaptors/test_transfer.cpp
      test/algos/adaptors/test_schedule_from.cpp
      test/algos/adaptors/test_then.cpp
      test/algos/adaptors/test_upon_error.cpp
      test/algos/adaptors/test_upon_stopped.cpp
      test/algos/adaptors/test_let_value.cpp
      test/algos/adaptors/test_let_error.cpp
      test/algos/adaptors/test_let_stopped.cpp
      test/algos/adaptors/test_bulk.cpp
      test/algos/adaptors/test_split.cpp
      test/algos/adaptors/test_when_all.cpp
      test/algos/adaptors/test_transfer_when_all.cpp
      test/algos/adaptors/test_into_variant.cpp
      test/algos/adaptors/test_stopped_as_optional.cpp
      test/algos/adaptors/test_stopped_as_error.cpp
      test/algos/adaptors/test_ensure_started.cpp
      test/algos/consumers/test_start_detached.cpp
      test/algos/consumers/test_sync_wait.cpp
      test/algos/other/test_execute.cpp
      test/detail/test_completion_signatures.cpp
      test/detail/test_create.cpp
      test/detail/test_utility.cpp
      test/queries/test_get_forward_progress_guarantee.cpp
      )

  add_executable(test.P2300 ${test_sourceFiles})

  target_include_directories(test.P2300 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/test ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(test.P2300 P2300)
  target_link_libraries(test.P2300 P2300_executable_flags)

  find_package(Catch2 REQUIRED)
  if (P2300_USE_CONAN)
    add_library(Catch2::Catch2 ALIAS CONAN_PKG::catch2)
  endif ()
  target_link_libraries(test.P2300 Catch2::Catch2)

  # Discover the Catch2 test built by the application
  include(CTest)
  include(Catch)
  catch_discover_tests(test.P2300)
endif ()

# Set up examples
if (P2300_ENABLE_EXAMPLES)
  function(def_example target sourceFile)
      add_executable(${target} ${sourceFile})
      target_link_libraries(${target} P2300)
      target_link_libraries(test.P2300 P2300_executable_flags)
  endfunction()

  def_example(example.hello_world "examples/hello_world.cpp")
  def_example(example.hello_coro "examples/hello_coro.cpp")
  def_example(example.scope "examples/scope.cpp")
  def_example(example.retry "examples/retry.cpp")
  def_example(example.then "examples/then.cpp")
  def_example(example.server_theme.let_value "examples/server_theme/let_value.cpp")
  def_example(example.server_theme.on_transfer "examples/server_theme/on_transfer.cpp")
endif ()
