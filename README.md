This repository is designed to compile with minimal dependencies:
CXX compilers, GPU aware MPI library and optionally NCCL/RCCL libraries.
Currently there are two bencharks that are called from src/main.cc:

* [1] benchmarkDeviceDirectMPIAllreduce (src/deviceDirectMPIAllreduce.cc): benchmarks GPU direct MPI\_Allreduce called 50 times 
 on a double array on the GPU with 1e+7 entries
* [2] benchmarkXtX (src/xtxDoubleDevice.cc): overlap matrix computation in double precision (cf. https://arxiv.org/abs/2203.07820), with blocked loop and asynchronous compute and communication implemented using CUDA/HIP streams 
  

Steps to compile
==========================================
* Install NCCL/RCCL library or load the appropriate module if available

* Modify setupCrusher.sh to set CXX compiler, CXX compiler flags, NCCL/RCCL library paths, and CUDA/HIP compiler paths.

* mkdir build and cd build (this directory can be created anywhere)

* bash `miniappSourceFullPath'/setupCrusher.sh  
  
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
  1) craype-x86-trento                      10) PrgEnv-gnu/8.3.3
  2) libfabric/1.15.0.0                     11) xalt/1.3.0
  3) craype-network-ofi                     12) DefApps/default
  4) perftools-base/22.06.0                 13) craype-accel-amd-gfx90a
  5) xpmem/2.4.4-2.3_11.2__gff0e1d9.shasta  14) rocm/5.4.0
  6) cray-pmi/6.1.3                         15) cray-mpich/8.1.23
  7) gcc/11.2.0                             16) cmake/3.23.2
  8) craype/2.7.16                          17) openblas/0.3.17
  9) cray-dsmml/0.2.2
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

