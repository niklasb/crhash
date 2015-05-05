cmake_minimum_required (VERSION 2.8.3)
project (crhash)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(OpenCL REQUIRED)

set(CL_COMPILED_SOURCES_DIR ${CMAKE_CURRENT_BINARY_DIR})

include_directories(${OPENCL_INCLUDE_DIRS})
include_directories(${CL_COMPILED_SOURCES_DIR})

file(GLOB CL_SOURCES "*.cl")
foreach(INPUT_FILE ${CL_SOURCES})
  get_filename_component(BASENAME ${INPUT_FILE} NAME)
  set(OUTPUT_FILE ${CL_COMPILED_SOURCES_DIR}/${BASENAME}.h)
  add_custom_command(
    OUTPUT ${OUTPUT_FILE}
    COMMAND ${CMAKE_COMMAND} -E copy ${INPUT_FILE} ${BASENAME}
    COMMAND xxd -i ${BASENAME} ${OUTPUT_FILE}
    DEPENDS ${INPUT_FILE}
    COMMENT "Compiling ${BASENAME} to C header file"
  )
  list(APPEND CL_COMPILED_SOURCES ${OUTPUT_FILE})
endforeach()

set(CMAKE_CXX_FLAGS "-O3 -std=c++0x -march=native -mtune=native")

file(GLOB SOURCES *.cpp)
add_executable(crhash
  ${SOURCES}
  ${CL_COMPILED_SOURCES}
)

target_link_libraries(crhash ${OPENCL_LIBRARIES})
