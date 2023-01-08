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
#include <MemoryStorage.h>
#include <deviceDirectCCLWrapper.h>
#include <benchmarks.h>
#include <dftfeDataTypes.h>
#include <deviceKernelsGeneric.h>

namespace dftfe
{

void benchmarkDeviceDirectMPIAllreduce (const MPI_Comm & mpi_communicator)
{
  // set stdout precision
  std::cout << std::scientific << std::setprecision(18);

  int nprocs;
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

  int this_process;
  MPI_Comm_rank(MPI_COMM_WORLD, &this_process);

  dftfe::utils::deviceKernelsGeneric::setupDevice();


  dftfe::utils::MemoryStorage<double,dftfe::utils::MemorySpace::DEVICE> dMat(10000000,0.1);
  double t1, t2;

#  if defined(DFTFE_WITH_CUDA_NCCL) || defined(DFTFE_WITH_HIP_RCCL)

  dftfe::utils::DeviceCCLWrapper gpucclMpiCommDomain;
  gpucclMpiCommDomain.init(MPI_COMM_WORLD);

  dftfe::utils::deviceSynchronize();
  MPI_Barrier(MPI_COMM_WORLD);
  t1 = MPI_Wtime();

  dftfe::utils::deviceStream_t streamDataMove;
  dftfe::utils::deviceStreamCreate(&streamDataMove);

  for (unsigned int i=0;i<50; i++)
      gpucclMpiCommDomain.deviceDirectAllReduceWrapper(
                      dMat.begin(),
                      dMat.begin(),
                      dMat.size(),
                      streamDataMove);
  
  dftfe::utils::deviceStreamSynchronize(streamDataMove);
  dftfe::utils::deviceStreamDestroy(streamDataMove);

  dftfe::utils::deviceSynchronize();
  MPI_Barrier(MPI_COMM_WORLD);
  t2 = MPI_Wtime();
  if (this_process==0)
  std::cout<<"Time in seconds for GPU Direct MPI_Allreduce using NCCL/RCCL: "<<t2-t1<<std::endl;

#endif

#if DFTFE_WITH_DEVICE_AWARE_MPI
  dftfe::utils::deviceSynchronize();
  MPI_Barrier(MPI_COMM_WORLD);
  t1 = MPI_Wtime();

  for (unsigned int i=0;i<50; i++)
     MPI_Allreduce(MPI_IN_PLACE,
                                  dMat.begin(),
                                  dMat.size(),
                                  dataTypes::mpi_type_id(
                                    dMat.begin()),
                                  MPI_SUM,
                                  MPI_COMM_WORLD);

  dftfe::utils::deviceSynchronize();
  MPI_Barrier(MPI_COMM_WORLD);
  t2 = MPI_Wtime();
  if (this_process==0)
  std::cout<<"Time in seconds for GPU Direct MPI_Allreduce using standard MPI library: "<<t2-t1<<std::endl;
#endif

}
}
