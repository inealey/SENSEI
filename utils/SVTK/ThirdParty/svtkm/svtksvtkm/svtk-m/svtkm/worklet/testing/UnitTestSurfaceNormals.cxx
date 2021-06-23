//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/worklet/SurfaceNormals.h>

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>
#include <svtkm/worklet/DispatcherMapTopology.h>

namespace
{

using NormalsArrayHandle = svtkm::cont::ArrayHandle<svtkm::Vec3f>;

void TestFacetedSurfaceNormals(const svtkm::cont::DataSet& dataset, NormalsArrayHandle& normals)
{
  std::cout << "Testing FacetedSurfaceNormals:\n";

  svtkm::worklet::FacetedSurfaceNormals faceted;
  faceted.Run(dataset.GetCellSet(), dataset.GetCoordinateSystem().GetData(), normals);

  svtkm::Vec3f expected[8] = { { -0.707f, -0.500f, 0.500f }, { -0.707f, -0.500f, 0.500f },
                              { 0.707f, 0.500f, -0.500f },  { 0.000f, -0.707f, -0.707f },
                              { 0.000f, -0.707f, -0.707f }, { 0.000f, 0.707f, 0.707f },
                              { -0.707f, 0.500f, -0.500f }, { 0.707f, -0.500f, 0.500f } };
  auto portal = normals.GetPortalConstControl();
  SVTKM_TEST_ASSERT(portal.GetNumberOfValues() == 8, "incorrect faceNormals array length");
  for (svtkm::Id i = 0; i < 8; ++i)
  {
    SVTKM_TEST_ASSERT(test_equal(portal.Get(i), expected[i], 0.001),
                     "result does not match expected value");
  }
}

void TestSmoothSurfaceNormals(const svtkm::cont::DataSet& dataset,
                              const NormalsArrayHandle& faceNormals)
{
  std::cout << "Testing SmoothSurfaceNormals:\n";

  NormalsArrayHandle pointNormals;
  svtkm::worklet::SmoothSurfaceNormals smooth;
  smooth.Run(dataset.GetCellSet(), faceNormals, pointNormals);

  svtkm::Vec3f expected[8] = { { -0.8165f, -0.4082f, -0.4082f }, { -0.2357f, -0.9714f, 0.0286f },
                              { 0.0000f, -0.1691f, 0.9856f },   { -0.8660f, 0.0846f, 0.4928f },
                              { 0.0000f, -0.1691f, -0.9856f },  { 0.0000f, 0.9856f, -0.1691f },
                              { 0.8165f, 0.4082f, 0.4082f },    { 0.8165f, -0.4082f, -0.4082f } };
  auto portal = pointNormals.GetPortalConstControl();
  SVTKM_TEST_ASSERT(portal.GetNumberOfValues() == 8, "incorrect pointNormals array length");
  for (svtkm::Id i = 0; i < 8; ++i)
  {
    SVTKM_TEST_ASSERT(test_equal(portal.Get(i), expected[i], 0.001),
                     "result does not match expected value");
  }
}

void TestSurfaceNormals()
{

  svtkm::cont::DataSet dataset =
    svtkm::cont::testing::MakeTestDataSet().Make3DExplicitDataSetPolygonal();
  NormalsArrayHandle faceNormals;

  TestFacetedSurfaceNormals(dataset, faceNormals);
  TestSmoothSurfaceNormals(dataset, faceNormals);
}

} // anonymous namespace

int UnitTestSurfaceNormals(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestSurfaceNormals, argc, argv);
}
