This repository is designed to compile with minimal dependencies:
CXX compilers, GPU aware MPI library and optionally NCCL/RCCL libraries.
Currently there are two bencharks that are called from src/main.cc:

* [1] benchmarkDeviceDirectMPIAllreduce (src/deviceDirectMPIAllreduce.cc): benchmarks GPU direct MPI\_Allreduce called 50 times 
 on a double array on the GPU with 1e+7 entries
* [2] benchmarkXtX (src/xtxDoubleDevice.cc): overlap matrix computation in double precision (cf. https://arxiv.org/abs/2203.07820), with blocked loop and asynchronous compute and communication implemented using CUDA/HIP streams 
  

Steps to compile
==========================================
* Modify setupFrontier.sh to set CXX compiler, CXX compiler flags, NCCL/RCCL library paths, and CUDA/HIP compiler paths.

* mkdir build and cd build (this directory can be created anywhere)

* bash `miniappSourceFullPath'/setupFrontier.sh  (Appropriate linking flags for GPU Direct MPICH is used here)
  
Summit results on benchmarkDeviceDirectMPIAllreduce
-------------
```
   module swap xl/16.1.1-10 gcc/9.1.0
   module load cmake
   module load essl/6.3.0
   module load cuda/11.0.3
   module load netlib-lapack/3.9.1
   module load netlib-scalapack/2.1.0-openblas
   module load openblas/0.3.15-omp
```

  1 node and 6 GPUs:
```
  Time in seconds for GPU Direct MPI_Allreduce using NCCL/RCCL: 2.367997640000001347e-01
  Time in seconds for GPU Direct MPI_Allreduce using standard MPI library: 3.096087765000000047e+00
```

  2 nodes and 12 GPUs:
```
Time in seconds for GPU Direct MPI_Allreduce using NCCL/RCCL: 2.978597040000003915e-01
Time in seconds for GPU Direct MPI_Allreduce using standard MPI library: 3.921587415999999493e+00  
```

 Crusher results on benchmarkDeviceDirectMPIAllreduce
 -------------
```
   module load PrgEnv-gnu
   module load craype-accel-amd-gfx90a
   module load rocm
   module load cmake
   module load openblas/0.3.17
   module unload cray-libsci
   export MPICH_GPU_SUPPORT_ENABLED=1
   export PE_MPICH_GTL_DIR_amd_gfx90a="-L${CRAY_MPICH_ROOTDIR}/gtl/lib"
   export PE_MPICH_GTL_LIBS_amd_gfx90a="-lmpi_gtl_hsa"
   
   and 5.1.3 RCCL version compatible with 5.1.0 ROCm version
```

  1 node and 8 GCDs:
```
Time in seconds for GPU Direct MPI_Allreduce using NCCL/RCCL: 8.236887570001272252e-01
Time in seconds for GPU Direct MPI_Allreduce using standard MPI library: 4.652598662999935186e+00  
```
  
  2 nodes and 16 GCDs:
```
Time in seconds for GPU Direct MPI_Allreduce using NCCL/RCCL: 2.978673024000272562e+00
Time in seconds for GPU Direct MPI_Allreduce using standard MPI library: 5.049497311999857629e+00
```

```
   module load PrgEnv-gnu
   module load craype-accel-amd-gfx90a
   module load rocm/5.4.0
   module load cray-mpich/8.1.23
   module load cmake
   module unload cray-libsci
   export MPICH_GPU_SUPPORT_ENABLED=1
   export PE_MPICH_GTL_DIR_amd_gfx90a="-L${CRAY_MPICH_ROOTDIR}/gtl/lib"
   export PE_MPICH_GTL_LIBS_amd_gfx90a="-lmpi_gtl_hsa"

and development branch of RCCL 
```

  1 node and 8 GCDs:
```
Time in seconds for GPU Direct MPI_Allreduce using NCCL/RCCL: 5.710980999992898433e-02
Time in seconds for GPU Direct MPI_Allreduce using standard MPI library: 4.663486804999593005e+00
```
  
  2 nodes and 16 GCDs:
```
Time in seconds for GPU Direct MPI_Allreduce using NCCL/RCCL: 2.238994716999968659e+00
Time in seconds for GPU Direct MPI_Allreduce using standard MPI library: 5.128894410000157222e+00
```  

Frontier results on benchmarkDeviceDirectMPIAllreduce
 -------------
```

   module load PrgEnv-gnu
   module load craype-accel-amd-gfx90a
   module load rocm
   module load cmake
   module load openblas
   module unload cray-libsci
   export MPICH_GPU_SUPPORT_ENABLED=1

$ module list
Currently Loaded Modules:
  1) craype-x86-trento                      10) cray-mpich/8.1.23
  2) libfabric/1.15.2.0                     11) PrgEnv-gnu/8.3.3
  3) craype-network-ofi                     12) darshan-runtime/3.4.0
  4) perftools-base/22.12.0                 13) hsi/default
  5) xpmem/2.5.2-2.4_3.45__gd0f7936.shasta  14) DefApps/default
  6) cray-pmi/6.1.8                         15) craype-accel-amd-gfx90a
  7) gcc/12.2.0                             16) rocm/5.3.0
  8) craype/2.7.19                          17) cmake/3.23.2
  9) cray-dsmml/0.2.2                       18) openblas/0.3.17

```
All RCCL results below on Frontier are without aws-rccl-ofi patch
  1 node and 8 GCDs (MPICH_SMP_SINGLE_COPY_MODE is default):
```
Time in seconds for GPU Direct MPI_Allreduce using NCCL/RCCL: 2.962938810001105594e-01
Time in seconds for GPU Direct MPI_Allreduce using standard MPI library: 1.767420677000018259e+00
```

  2 nodes and 16 GCDs (MPICH_SMP_SINGLE_COPY_MODE is default):
```
Time in seconds for GPU Direct MPI_Allreduce using NCCL/RCCL: 6.913603289999628032e-01
Time in seconds for GPU Direct MPI_Allreduce using standard MPI library: 1.886262456000167731e+00
```

  1 node and 8 GCDs:
```
export MPICH_SMP_SINGLE_COPY_MODE=NONE
Time in seconds for GPU Direct MPI_Allreduce using NCCL/RCCL: 2.945046769998498348e-01
Time in seconds for GPU Direct MPI_Allreduce using standard MPI library: 2.762089984999875014e+00
```

  2 nodes and 16 GCDs:
```
export MPICH_SMP_SINGLE_COPY_MODE=NONE
Time in seconds for GPU Direct MPI_Allreduce using NCCL/RCCL: 6.609293199999228818e-01
Time in seconds for GPU Direct MPI_Allreduce using standard MPI library: 2.946769545000051949e+00
```
