//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/testing/TestingImplicitFunction.h>

namespace
{

void TestImplicitFunctions()
{
  svtkm::cont::testing::TestingImplicitFunction testing;
  testing.Run(svtkm::cont::DeviceAdapterTagSerial());
}

} // anonymous namespace

int UnitTestSerialImplicitFunction(int argc, char* argv[])
{
  auto& tracker = svtkm::cont::GetRuntimeDeviceTracker();
  tracker.ForceDevice(svtkm::cont::DeviceAdapterTagSerial{});
  return svtkm::cont::testing::Testing::Run(TestImplicitFunctions, argc, argv);
}
