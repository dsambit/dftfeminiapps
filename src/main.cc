// ---------------------------------------------------------------------
//
// Copyright (c) 2017-2022 The Regents of the University of Michigan and DFT-FE
// authors.
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


//
// C++ headers
//
#include <fstream>
#include <iostream>
#include <list>
#include <sstream>
#include <sys/stat.h>
#include <benchmarks.h>
#include <mpi.h>

int
main(int argc, char *argv[])
{
  //
  MPI_Init(&argc, &argv);

  const double start = MPI_Wtime();
  int          world_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  dftfe::benchmarkDeviceDirectMPIAllreduce(MPI_COMM_WORLD);
  //dftfe::benchmarkXtXDouble(MPI_COMM_WORLD);
  //dftfe::benchmarkXtXComplexDouble(MPI_COMM_WORLD);
  MPI_Finalize();
  return 0;
}
