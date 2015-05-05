cmake_minimum_required (VERSION 2.8.3)
project (crhash)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(OpenCL REQUIRED)

include_directories(${OPENCL_INCLUDE_DIRS})

SET(RESOURCE_COMPILER xxd)
FILE(GLOB COMPILED_RESOURCES "*.cl")
FOREACH(INPUT_FILE ${COMPILED_RESOURCES})
  SET(OUTPUT_FILE ${INPUT_FILE}.h)
  ADD_CUSTOM_COMMAND(
    OUTPUT ${OUTPUT_FILE}
    COMMAND ${RESOURCE_COMPILER} -i ${INPUT_FILE} ${OUTPUT_FILE}
    COMMENT "Compiling ${INPUT_FILE} to C header file")
  LIST(APPEND COMPILED_RESOURCES ${OUTPUT_FILE})
ENDFOREACH()

SET(CMAKE_CXX_FLAGS "-std=c++0x -march=native -mtune=native")

FILE(GLOB Sources *.cpp)
FILE(GLOB Headers *.{h,hpp})
FILE(GLOB CLSources *.cl)
ADD_EXECUTABLE (crhash
  ${Sources}
  ${Headers}
  ${CLSources}
  )

target_link_libraries(crhash ${OPENCL_LIBRARIES})
