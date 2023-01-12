// ---------------------------------------------------------------------
//
// Copyright (c) 2017-2021 The Regents of the University of Michigan and DFT-FE authors.
//
// This file is part of the DFT-FE code.
//
// The DFT-FE code is free software; you can use it, redistribute
// it, and/or modify it under the terms of the GNU Lesser General
// Public License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// The full text of the license can be found in the file LICENSE at
// the top level of the DFT-FE distribution.
//
// ---------------------------------------------------------------------
//
// @author Sambit Das
//


//Include generic C++ headers
#include <list>
#include <iostream>
#include <fstream>
#include <complex>
#include <vector>
#include <iomanip>
#include <mpi.h>
#include <memory>
#include <cstring>

#include <DeviceAPICalls.h>
#include <DeviceDataTypeOverloads.h>
#include <DeviceBlasWrapper.h>
#include <DeviceKernelLauncherConstants.h>
#include <MemoryStorage.h>
#include <deviceDirectCCLWrapper.h>
#include <benchmarks.h>
#include <dftfeDataTypes.h>
#include <deviceKernelsGeneric.h>

namespace dftfe
{

    // SConj=X^{T}*XConj in double precision, X is a M x N matrix stored as X^{T} with the
    // fastest index goint from 1 to N

    void
    fillParallelOverlapMatAsyncComputeCommun(
      const double *                        X,
      const unsigned int                               M,
      const unsigned int                               N,
      const unsigned int vectorsBlockSize,
      dftfe::utils::deviceBlasHandle_t &               handle,
      const MPI_Comm &                                 mpiCommDomain,
      utils::DeviceCCLWrapper &                        devicecclMpiCommDomain)
    {
#if defined(DFTFE_WITH_CUDA_NCCL) || defined(DFTFE_WITH_HIP_RCCL) || defined(DFTFE_WITH_DEVICE_AWARE_MPI)    
      const bool useDeviceDirectAllReduce=true;
#else
      const bool useDeviceDirectAllReduce=false;
#endif
      const unsigned int numberBlocks     = N / vectorsBlockSize;

      // create separate Device streams for data movement and computation
      dftfe::utils::deviceStream_t streamCompute, streamDataMove;
      dftfe::utils::deviceStreamCreate(&streamCompute);
      dftfe::utils::deviceStreamCreate(&streamDataMove);

      // attach deviceblas handle to compute stream
      dftfe::utils::deviceBlasWrapper::setStream(handle, streamCompute);

      // create array of compute and copy events on Devices
      // for all the blocks. These are required for synchronization
      // between compute, copy and communication as discussed above in the
      // pseudo code
      dftfe::utils::deviceEvent_t computeEvents[numberBlocks];
      dftfe::utils::deviceEvent_t copyEvents[numberBlocks];

      for (int i = 0; i < numberBlocks; ++i)
        {
          dftfe::utils::deviceEventCreate(&computeEvents[i]);
          dftfe::utils::deviceEventCreate(&copyEvents[i]);
        }

      // create pinned memory used later to copy from Device->CPU
      dftfe::utils::MemoryStorage<double,
                                  dftfe::utils::MemorySpace::HOST_PINNED>
        overlapMatrixBlockHost;
      overlapMatrixBlockHost.resize(N * vectorsBlockSize, 0);
      std::memset(overlapMatrixBlockHost.begin(),
                  0,
                  vectorsBlockSize * N * sizeof(double));

      // allocate device vectors to be used later
      dftfe::utils::MemoryStorage<double,
                                  dftfe::utils::MemorySpace::DEVICE>
        overlapMatrixBlock(N * vectorsBlockSize, double(0));
      dftfe::utils::MemoryStorage<double,
                                  dftfe::utils::MemorySpace::DEVICE>
        overlapMatrixBlockNext(N * vectorsBlockSize, double(0));

      const double scalarCoeffAlpha = double(1.0);
      const double scalarCoeffBeta  = double(0);


      unsigned int blockCount = 0;
      for (unsigned int ivec = 0; ivec < N; ivec += vectorsBlockSize)
        {
          // Correct block dimensions if block "goes off edge of" the matrix
          const unsigned int B = std::min(vectorsBlockSize, N - ivec);
          const unsigned int D = N - ivec;

              // Compute local XTrunc^{T}*XcBlock.
              if (ivec == 0)
                {
                  dftfe::utils::deviceBlasWrapper::gemm(
                    handle,
                    dftfe::utils::DEVICEBLAS_OP_N,
                      dftfe::utils::DEVICEBLAS_OP_T,
                    D,
                    B,
                    M,
                    &scalarCoeffAlpha,
                    X + ivec,
                    N,
                    X + ivec,
                    N,
                    &scalarCoeffBeta,
                    overlapMatrixBlock.begin(),
                    D);
                  // record completion of compute for first block
                  dftfe::utils::deviceEventRecord(computeEvents[blockCount],
                                                  streamCompute);
                }

              // Before swap host thread needs to wait till compute on
              // currentblock is over. Since swap occurs on the null stream, any
              // future operations in the streamDataMove will only occur after
              // both the compute on currentblock and swap is over. Note that at
              // this point there is nothing queued in the streamDataMove as all
              // previous operations in that stream are over.
              if ((dftfe::utils::deviceEventSynchronize(
                     computeEvents[blockCount]) ==
                   dftfe::utils::deviceSuccess) &&
                  (ivec > 0))
                overlapMatrixBlock.swap(overlapMatrixBlockNext);

              const unsigned int ivecNew = ivec + vectorsBlockSize;
              const unsigned int DNew    = N - ivecNew;
              const unsigned int BNew    = min(vectorsBlockSize, N - ivecNew);


              // start computations on the next block
              if (ivecNew <N)
                {
                  // evaluate X^{T} times XBlock
                  dftfe::utils::deviceBlasWrapper::gemm(
                    handle,
                    dftfe::utils::DEVICEBLAS_OP_N,
                      dftfe::utils::DEVICEBLAS_OP_T,
                    DNew,
                    BNew,
                    M,
                    &scalarCoeffAlpha,
                    X + ivecNew,
                    N,
                    X + ivecNew,
                    N,
                    &scalarCoeffBeta,
                    overlapMatrixBlockNext.begin(),
                    DNew);
                  // record completion of compute for next block
                  dftfe::utils::deviceEventRecord(computeEvents[blockCount + 1],
                                                  streamCompute);
                }

              if (useDeviceDirectAllReduce)
                {
                  // Sum local XTrunc^{T}*XcBlock across domain decomposition
                  // processors
                    devicecclMpiCommDomain.deviceDirectAllReduceWrapper(
                      overlapMatrixBlock.begin(),
                      overlapMatrixBlock.begin(),
                      D * B,
                      streamDataMove);
               }
              
              dftfe::utils::deviceMemcpyAsyncD2H(
                dftfe::utils::makeDataTypeDeviceCompatible(
                  overlapMatrixBlockHost.begin()),
                dftfe::utils::makeDataTypeDeviceCompatible(
                  overlapMatrixBlock.begin()),
                D * B * sizeof(double),
                streamDataMove);

              // record completion of Device->CPU copy for current block
              dftfe::utils::deviceEventRecord(copyEvents[blockCount],
                                              streamDataMove);

              // Check that Device->CPU on the current block has been completed.
              // If completed, perform blocking MPI commmunication on the
              // current block and copy to ScaLAPACK matri
              if (dftfe::utils::deviceEventSynchronize(
                    copyEvents[blockCount]) == dftfe::utils::deviceSuccess)
                {
                  // Sum local XTrunc^{T}*XcBlock across domain decomposition
                  // processors
                  if (!useDeviceDirectAllReduce)
                    MPI_Allreduce(MPI_IN_PLACE,
                                  overlapMatrixBlockHost.begin(),
                                  D * B,
                                  dataTypes::mpi_type_id(
                                    overlapMatrixBlockHost.begin()),
                                  MPI_SUM,
                                  mpiCommDomain);


                  // Copying only the lower triangular part to the ScaLAPACK
                  // overlap matrix
                }
          blockCount += 1;
        } // end block loop


      // return deviceblas handle to default stream
      dftfe::utils::deviceBlasWrapper::setStream(handle, NULL);

      for (int i = 0; i < numberBlocks; ++i)
        {
          dftfe::utils::deviceEventDestroy(computeEvents[i]);
          dftfe::utils::deviceEventDestroy(copyEvents[i]);
        }

      dftfe::utils::deviceStreamDestroy(streamCompute);
      dftfe::utils::deviceStreamDestroy(streamDataMove);
    }



void benchmarkXtX (const MPI_Comm & mpi_communicator)
{
  // set stdout precision
  std::cout << std::scientific << std::setprecision(18);

  /////INPUTS/////
  const unsigned int globalNumDofs=70000;//such that we have roughly 100k to 50k dofs per gpu when scaling from 2 GPU to 4 GPUs
  const unsigned int numberVectors=36600;//must be multiple of vectorsBlockSize
  const unsigned int vectorsBlockSize=732;
  ////////////////
  // Get the number of processes
  int nprocs;
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

  int this_process;
  MPI_Comm_rank(MPI_COMM_WORLD, &this_process);

  dftfe::utils::deviceKernelsGeneric::setupDevice();

  const unsigned int localNumDofs=globalNumDofs/nprocs;
 
 if (this_process==0)
  std::cout << "nprocs: "
            << nprocs
            << std::endl;

  if (this_process==0)
  std::cout << "localNumDofs: "
	    << localNumDofs
	    << std::endl;
 
  if (this_process==0)
  std::cout << "numberVectors: "
	    << numberVectors
	    << std::endl;

  dftfe::utils::deviceBlasHandle_t deviceblasHandle;
  dftfe::utils::deviceBlasWrapper::create(&deviceblasHandle);

  dftfe::utils::DeviceCCLWrapper gpucclMpiCommDomain;
  gpucclMpiCommDomain.init(MPI_COMM_WORLD);


  dftfe::utils::MemoryStorage<double,dftfe::utils::MemorySpace::DEVICE> dA(numberVectors*localNumDofs,1.0/numberVectors);

  double t1, t2;
  dftfe::utils::deviceSynchronize();
  MPI_Barrier(MPI_COMM_WORLD);
  t1 = MPI_Wtime();

  fillParallelOverlapMatAsyncComputeCommun(
                    dA.begin(),
                    localNumDofs,
                    numberVectors,
                    vectorsBlockSize,
                    deviceblasHandle,
                    MPI_COMM_WORLD,
                     gpucclMpiCommDomain);
  dftfe::utils::deviceSynchronize();
  MPI_Barrier(MPI_COMM_WORLD);
  t2 = MPI_Wtime();
  if (this_process==0)
  std::cout<<"Time in seconds: "<<t2-t1<<std::endl;


  dftfe::utils::deviceBlasWrapper::destroy(deviceblasHandle); 
}
}
