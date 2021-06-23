//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/testing/Testing.h>

#include <svtkm/cont/tbb/DeviceAdapterTBB.h>
#include <svtkm/exec/internal/testing/TestingTaskTiling.h>

int UnitTestTaskTilingTBB(int argc, char* argv[])
{
  return svtkm::testing::Testing::Run(
    svtkm::exec::internal::testing::TestTaskTiling<svtkm::cont::DeviceAdapterTagTBB>, argc, argv);
}
