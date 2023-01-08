This repository is designed to compile with minimal dependencies:
CXX compilers, GPU aware MPI library and optionally NCCL/RCCL libraries.
Currently there are two bencharks that can be called be src/main.cc:

* benchmarkDeviceDirectMPIAllreduce: benchmarks GPU direct MPI\_Allreduce call
 on a double array on the GPU with 1e+7 entries

* benchmarkXtX: overlap matrix computation in double precision (cf. https://arxiv.org/abs/2203.07820), with blocked loop and asynchronous compute and communication with streams   

Steps to compile
==========================================
* Install NCCL/RCCL library or load the appropriate module if available

* Modify setupUserCrusherDFTFE.sh  

* mkdir build and cd build (this directory can be created anywhere)

* bash `dftfeSourceFullPath'/setupUserCrusherDFTFE.sh
