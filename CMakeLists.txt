cmake_minimum_required (VERSION 2.8.3)
project (crhash)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# OpenCL
find_package(OpenCL)

if(OpenCL_FOUND)
  include_directories(${OpenCL_INCLUDE_DIRS})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_OPENCL=1")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_OPENCL=0")
endif()

# Include CL kernels as C header files
set(CL_COMPILED_SOURCES_DIR ${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CL_COMPILED_SOURCES_DIR})

if(OpenCL_FOUND)
  include_directories(${OPENCL_INCLUDE_DIRS})
endif()

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

# Threading
find_package(Threads)
if(CMAKE_USE_PTHREADS_INIT)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++0x -march=native -mtune=native")

file(GLOB SOURCES *.cpp)
add_executable(crhash
  ${SOURCES}
  ${CL_COMPILED_SOURCES}
)

if(OpenCL_FOUND)
  target_link_libraries(crhash ${OPENCL_LIBRARIES})
endif()
target_link_libraries(crhash ${CMAKE_THREAD_LIBS_INIT})
