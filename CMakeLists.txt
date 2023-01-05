##
#  CMakeList file for DFT-FE
##
CMAKE_MINIMUM_REQUIRED(VERSION 3.17)
# Set the name of the project and target:
SET(TARGET "dftfe_exe")
SET(TARGETLIB "dftfe")
SET(TARGET_VERSION "1.0-pre") # used in docs/Doxyfile.in

#FIXME: find a better way to replace O3 with O2 in CMAKE_CXX_FLAGS_RELEASE
IF("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
  add_compile_options("-O2")
ENDIF()

PROJECT(${TARGET} LANGUAGES CXX)
IF (WITH_GPU)
    IF ("${GPU_LANG}" STREQUAL "cuda")
      enable_language(CUDA)
    ELSEIF ("${GPU_LANG}" STREQUAL "hip")
      enable_language(HIP)
    ELSE ()
      MESSAGE(FATAL_ERROR "\n"
    "*** Could not find valid GPU_LANG=cuda/hip ***\n\n"
    )
    ENDIF()
ENDIF()


ADD_EXECUTABLE(${TARGET}   src/main.cc)
set_property(TARGET "dftfe_exe" PROPERTY OUTPUT_NAME "dftfe")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

#set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(ENABLE_MPI)
  find_package(MPI REQUIRED)
  include_directories(${MPI_CXX_INCLUDE_PATH})
  set(DFTFE_MPI_LIBRARIES ${MPI_CXX_LIBRARIES})
endif()


# Declare all source files the target consists of:
SET(TARGET_SRC
  ./src/xtxDoubleDevice.cc
  ./utils/Exceptions.cc
  ./utils/MemoryManager.cc
  )

IF ("${GPU_LANG}" STREQUAL "cuda")

SET(DEVICE_SRC
  ./utils/MemoryTransferKernelsDevice.cc
  ./utils/DeviceDirectCCLWrapper.cc
  ./utils/DeviceAPICalls.cu.cc
  ./utils/DeviceBlasWrapper.cu.cc
  ./utils/DeviceKernelsGeneric.cc
  )

ELSEIF ("${GPU_LANG}" STREQUAL "hip")

SET(DEVICE_SRC
  ./utils/MemoryTransferKernelsDevice.cc
  ./utils/DeviceDirectCCLWrapper.cc
  ./utils/DeviceAPICalls.hip.cc
  ./utils/DeviceBlasWrapper.hip.cc
  ./utils/DeviceKernelsGeneric.cc
  )

ENDIF()


IF (WITH_GPU)
  LIST(APPEND TARGET_SRC ${DEVICE_SRC})
ENDIF()

IF (WITH_GPU)
    IF ("${GPU_LANG}" STREQUAL "cuda")
       set_source_files_properties(${TARGET_SRC} PROPERTIES LANGUAGE CUDA)
    ELSEIF ("${GPU_LANG}" STREQUAL "hip")
       set_source_files_properties(${TARGET_SRC} PROPERTIES LANGUAGE HIP)
    ENDIF()
ENDIF()

ADD_LIBRARY(${TARGETLIB} SHARED ${TARGET_SRC})

#IF("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
#
#target_compile_options(${TARGETLIB}
#    PUBLIC
#        -O2)
#ENDIF()

#
# RPATHS: https://cmake.org/Wiki/CMake_RPATH_handling#Mac_OS_X_and_the_RPATH
#
IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  ## cmake  --help-policy CMP0042
  set(CMAKE_MACOSX_RPATH ON)
ENDIF()

# use, i.e. don't skip the full RPATH for the build tree
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# the RPATH to be used when installing, but only if it's not a system directory
LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
IF("${isSystemDir}" STREQUAL "-1")
   SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
ENDIF("${isSystemDir}" STREQUAL "-1")

# make sure RPATHs are used on macOS
SET(CMAKE_INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib")


#Set GPU compilation flag
#
IF (WITH_GPU)
  ADD_DEFINITIONS(-DDFTFE_WITH_DEVICE)
  
  IF ("${GPU_VENDOR}" STREQUAL "nvidia")
    ADD_DEFINITIONS(-DDFTFE_WITH_DEVICE_NVIDIA)
  ELSEIF ("${GPU_VENDOR}" STREQUAL "amd")
    ADD_DEFINITIONS(-DDFTFE_WITH_DEVICE_AMD)
  ENDIF()

  IF ("${GPU_LANG}" STREQUAL "cuda")
    MESSAGE(STATUS "DFTFE with GPU LANG CUDA")
    ADD_DEFINITIONS(-DDFTFE_WITH_DEVICE_LANG_CUDA)
    find_package(CUDAToolkit REQUIRED)
    target_include_directories(${TARGETLIB} PUBLIC ${CUDAToolkit_INCLUDE_DIRS})
    target_link_libraries(${TARGETLIB} PUBLIC CUDA::cudart)
    target_link_libraries(${TARGETLIB} PUBLIC CUDA::cublas)
  ELSEIF ("${GPU_LANG}" STREQUAL "hip")
    MESSAGE(STATUS "DFTFE with GPU LANG HIP")
    ADD_DEFINITIONS(-DDFTFE_WITH_DEVICE_LANG_HIP)
    find_package(hipblas)
    target_link_libraries(${TARGETLIB} PUBLIC roc::hipblas)
  ENDIF()
  IF (WITH_GPU_AWARE_MPI)
    ADD_DEFINITIONS(-DDFTFE_WITH_DEVICE_AWARE_MPI)
    MESSAGE(STATUS "DFTFE with GPU Aware MPI")
  ENDIF()
ENDIF()

TARGET_LINK_LIBRARIES(${TARGETLIB} PUBLIC ${DFTFE_MPI_LIBRARIES})

#
#Dependencies
#

# NCCL. Pass path to -DCMAKE_PREFIX_PATH
#
IF (WITH_GPU AND "${GPU_LANG}" STREQUAL "cuda")
 IF (WITH_NCCL)
   find_package(NCCL REQUIRED)
   IF (NCCL_FOUND)
      MESSAGE(STATUS "Will Link With NCCL")
      ADD_DEFINITIONS(-DDFTFE_WITH_CUDA_NCCL)
      find_package(MPI REQUIRED)
      target_link_libraries(${TARGETLIB} PUBLIC NCCL MPI::MPI_CXX)
   ENDIF()
 ENDIF()
ENDIF()

TARGET_INCLUDE_DIRECTORIES(${TARGETLIB} PUBLIC ${CMAKE_SOURCE_DIR}/include)
TARGET_LINK_LIBRARIES(${TARGET} PUBLIC ${TARGETLIB})


#
# Custom "debug" and "release" make targets:
#
ADD_CUSTOM_TARGET(debug
COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Debug ${CMAKE_SOURCE_DIR}
COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target all
COMMENT "Switch CMAKE_BUILD_TYPE to Debug"
)

ADD_CUSTOM_TARGET(release
COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release ${CMAKE_SOURCE_DIR}
COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target all
COMMENT "Switch CMAKE_BUILD_TYPE to Release"
)


#
# FIXME: that will not play well with installation
#
ADD_DEFINITIONS(-DDFTFE_PATH="${CMAKE_CURRENT_SOURCE_DIR}")

#
# installation target.
# use with -DCMAKE_INSTALL_PREFIX=/path/to/
#
SET(TARGETLIBEXPORT "femdftTargets")
# 1. Install defined library target
install (TARGETS ${TARGETLIB} EXPORT ${TARGETLIBEXPORT}
         LIBRARY DESTINATION lib)
# 2. Install the binary
install (TARGETS ${TARGET}
         RUNTIME DESTINATION bin)
# 3. Install content of include dir
install (DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
         DESTINATION include)
