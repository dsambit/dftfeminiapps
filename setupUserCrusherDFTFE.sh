#!/bin/bash
# script to setup and build DFT-FE.

set -e
set -o pipefail

if [ -s CMakeLists.txt ]; then
    echo "This script must be run from the build directory!"
    exit 1
fi

# Path to project source
SRC=`dirname $0` # location of source directory

######################################################################
DCCL_PATH="/ccs/proj/mat187/dsambit/softwareDFTFEGcc/rcclnew2/rccl/build"
#DCCL_PATH="/ccs/proj/mat187/dsambit/softwareDFTFEGcc/rcclnew/rccl-rocm-5.3/build"
#DCCL_PATH="/ccs/proj/mat187/dsambit/softwareDFTFEGcc/rccl/rccl-rocm-5.1.3/build"

#Toggle GPU compilation
withGPU=ON
gpuLang="hip"     # Use "cuda"/"hip"
gpuVendor="amd" # Use "nvidia/amd"
withGPUAwareMPI=ON #Please use this option with care
                   #Only use if the machine supports 
                   #device aware MPI and is profiled
                   #to be fast

#Option to link to NCCL library (Only for GPU compilation)
withDCCL=ON

#Compiler options and flags
cxx_compiler=CC  #sets DCMAKE_CXX_COMPILER
cxx_flags="-march=znver3 -fPIC -I${MPICH_DIR}/include -I${ROCM_PATH}/include" #sets DCMAKE_CXX_FLAGS
cxx_flagsRelease="-O2" #sets DCMAKE_CXX_FLAGS_RELEASE
device_flags="-march=znver3 -O2 -munsafe-fp-atomics -I${MPICH_DIR}/include -I${ROCM_PATH}/include"
                           #setDCMAKE_CXX_CUDA/HIP_FLAGS 
                           #(only applicable for withGPU=ON)
device_architectures="gfx90a" # set DCMAKE_CXX_CUDA/HIP_ARCHITECTURES 
                           #(only applicable for withGPU=ON)


# build type: "Release" or "Debug"
build_type=Release

###########################################################################
#Usually, no changes are needed below this line
#

#if [[ x"$build_type" == x"Release" ]]; then
#  c_flags="$c_flagsRelease"
#  cxx_flags="$c_flagsRelease"
#else
#fi
out=`echo "$build_type" | tr '[:upper:]' '[:lower:]'`

function cmake_() {
  if [ "$gpuLang" = "cuda" ]; then
    cmake -DCMAKE_CXX_STANDARD=14 -DCMAKE_CXX_COMPILER=$cxx_compiler\
    -DCMAKE_CXX_FLAGS="$cxx_flags"\
    -DCMAKE_CXX_FLAGS_RELEASE="$cxx_flagsRelease" \
    -DCMAKE_BUILD_TYPE=$build_type \
    -DWITH_DCCL=$withDCCL -DCMAKE_PREFIX_PATH="$DCCL_PATH"\
    -DWITH_GPU=$withGPU -DGPU_LANG=$gpuLang -DGPU_VENDOR=$gpuVendor -DWITH_GPU_AWARE_MPI=$withGPUAwareMPI -DCMAKE_CUDA_FLAGS="$device_flags" -DCMAKE_CUDA_ARCHITECTURES="$device_architectures"\
    $1
  elif [ "$gpuLang" = "hip" ]; then
    cmake -DCMAKE_CXX_STANDARD=14 -DCMAKE_CXX_COMPILER=$cxx_compiler\
    -DCMAKE_CXX_FLAGS="$cxx_flags"\
    -DCMAKE_CXX_FLAGS_RELEASE="$cxx_flagsRelease" \
    -DCMAKE_BUILD_TYPE=$build_type \
    -DWITH_DCCL=$withDCCL -DCMAKE_PREFIX_PATH="$DCCL_PATH"\
    -DWITH_GPU=$withGPU -DGPU_LANG=$gpuLang -DGPU_VENDOR=$gpuVendor -DWITH_GPU_AWARE_MPI=$withGPUAwareMPI -DCMAKE_HIP_FLAGS="$device_flags" -DCMAKE_HIP_ARCHITECTURES="$device_architectures"\
    -DCMAKE_SHARED_LINKER_FLAGS="-L${ROCM_PATH}/lib -lamdhip64 -L${MPICH_DIR}/lib -lmpi -L${CRAY_MPICH_ROOTDIR}/gtl/lib -lmpi_gtl_hsa"\
     $1  
  fi
}

cmake_ "$SRC" && make -j8

