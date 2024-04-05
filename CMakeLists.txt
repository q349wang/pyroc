cmake_minimum_required(VERSION 3.22)

project(
  "pyroc"
  VERSION 0.1.0
  LANGUAGES CXX
)

#
# Set project options
#

include(cmake/StandardSettings.cmake)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_executable(${PROJECT_NAME} ${sources})

#
# Set the project standard and warnings
#
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)


include(cmake/CompilerWarnings.cmake)
set_project_warnings(${PROJECT_NAME})