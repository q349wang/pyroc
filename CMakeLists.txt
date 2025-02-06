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
find_package(Vulkan COMPONENTS glslc)
find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)

add_subdirectory(libs/glfw)

#
# Compile shaders
#

function(compile_shader target)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "ENV;FORMAT" "SOURCES")
    foreach(source ${arg_SOURCES})
        string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} output ${source}.${arg_FORMAT})
        string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} deps ${source}.d)

        get_filename_component(source_directory ${source} DIRECTORY)

        add_custom_command(
            OUTPUT ${output}
            DEPENDS ${source}
            DEPFILE ${deps}
            COMMENT "Compiling Shaders [${source}]"
            COMMAND
                ${glslc_executable}
                $<$<BOOL:${arg_ENV}>:--target-env=${arg_ENV}>
                $<$<BOOL:${arg_FORMAT}>:-mfmt=${arg_FORMAT}>
                -MD -MF ${deps}
                -I ${source_directory}
                -o ${output}
                ${source}
        )
        target_sources(${target} PRIVATE ${output})
    endforeach()
endfunction()

#
# Set project options
#

include(cmake/StandardSettings.cmake)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(${PROJECT_NAME} ${sources})

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

#
# Set the project standard and warnings
#
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

target_compile_options(${PROJECT_NAME} PUBLIC -fno-exceptions)
target_compile_options(${PROJECT_NAME} PUBLIC -fno-unwind-tables)
target_compile_options(${PROJECT_NAME} PUBLIC -fno-rtti)
target_compile_options(${PROJECT_NAME} PUBLIC -fms-extensions)

include(cmake/CompilerWarnings.cmake)
set_project_warnings(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PUBLIC glfw ${GLFW_LIBRARIES} Vulkan::Vulkan)


#
# demos
#

function(add_demo target)
  file(GLOB_RECURSE shader_srcs CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/demos/${target}/shaders/*.vert" "${CMAKE_CURRENT_SOURCE_DIR}/demos/${target}/shaders/*.frag")

  add_custom_target(shaders)

  set_target_properties(shaders PROPERTIES LINKER_LANGUAGE CXX)
  compile_shader(shaders ENV vulkan1.3 FORMAT bin SOURCES ${shader_srcs})
  
  file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/demos/${target}/*.cpp")

  add_executable(${target} ${sources})
  
  add_dependencies(${target} shaders)
  target_link_libraries(${target} ${PROJECT_NAME})
  target_link_options(${target} PRIVATE -mwindows)

endfunction()

add_demo(basic)