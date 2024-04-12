cmake_minimum_required(VERSION 3.22)

project(
  "pyroc"
  VERSION 0.1.0
  LANGUAGES CXX
)



#
# Add libs
#

find_package(Vulkan REQUIRED)

add_subdirectory(libs/glfw)

#
# Set project options
#

include(cmake/StandardSettings.cmake)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(${PROJECT_NAME} ${sources})

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

#
# Set the project standard and warnings
#
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

target_compile_options(${PROJECT_NAME} PUBLIC -fno-exceptions)
target_compile_options(${PROJECT_NAME} PUBLIC -fno-unwind-tables)
target_compile_options(${PROJECT_NAME} PUBLIC -fno-rtti)

include(cmake/CompilerWarnings.cmake)
set_project_warnings(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} glfw ${GLFW_LIBRARIES} Vulkan::Vulkan)
