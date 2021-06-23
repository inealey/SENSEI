//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/worklet/DispatcherMapTopology.h>
#include <svtkm/worklet/WorkletMapTopology.h>

#include <svtkm/worklet/CellAverage.h>
#include <svtkm/worklet/PointAverage.h>

#include <svtkm/Math.h>

#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/DeviceAdapterTag.h>

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

namespace test_explicit
{

class MaxPointOrCellValue : public svtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(FieldInCell inCells,
                                FieldInPoint inPoints,
                                CellSetIn topology,
                                FieldOutCell outCells);
  using ExecutionSignature = void(_1, _4, _2, PointCount, CellShape, PointIndices);
  using InputDomain = _3;

  SVTKM_CONT
  MaxPointOrCellValue() {}

  template <typename InCellType,
            typename OutCellType,
            typename InPointVecType,
            typename CellShapeTag,
            typename PointIndexType>
  SVTKM_EXEC void operator()(const InCellType& cellValue,
                            OutCellType& maxValue,
                            const InPointVecType& pointValues,
                            const svtkm::IdComponent& numPoints,
                            const CellShapeTag& svtkmNotUsed(type),
                            const PointIndexType& svtkmNotUsed(pointIDs)) const
  {
    //simple functor that returns the max of cellValue and pointValue
    maxValue = static_cast<OutCellType>(cellValue);
    for (svtkm::IdComponent pointIndex = 0; pointIndex < numPoints; ++pointIndex)
    {
      maxValue = svtkm::Max(maxValue, static_cast<OutCellType>(pointValues[pointIndex]));
    }
  }
};
}

namespace
{

static void TestMaxPointOrCell();
static void TestAvgPointToCell();
static void TestAvgCellToPoint();

void TestWorkletMapTopologyExplicit(svtkm::cont::DeviceAdapterId id)
{
  std::cout << "Testing Topology Worklet ( Explicit ) on device adapter: " << id.GetName()
            << std::endl;

  TestMaxPointOrCell();
  TestAvgPointToCell();
  TestAvgCellToPoint();
}

static void TestMaxPointOrCell()
{
  std::cout << "Testing MaxPointOfCell worklet" << std::endl;
  svtkm::cont::testing::MakeTestDataSet testDataSet;
  svtkm::cont::DataSet dataSet = testDataSet.Make3DExplicitDataSet0();
  auto cellset = dataSet.GetCellSet().Cast<svtkm::cont::CellSetExplicit<>>();

  svtkm::cont::ArrayHandle<svtkm::Float32> result;

  svtkm::worklet::DispatcherMapTopology<::test_explicit::MaxPointOrCellValue> dispatcher;
  dispatcher.Invoke(dataSet.GetField("cellvar").GetData().ResetTypes(svtkm::TypeListFieldScalar()),
                    dataSet.GetField("pointvar").GetData().ResetTypes(svtkm::TypeListFieldScalar()),
                    &cellset,
                    result);

  std::cout << "Make sure we got the right answer." << std::endl;
  SVTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(0), 100.1f),
                   "Wrong result for PointToCellMax worklet");
  SVTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(1), 100.2f),
                   "Wrong result for PointToCellMax worklet");
}

static void TestAvgPointToCell()
{
  std::cout << "Testing AvgPointToCell worklet" << std::endl;

  svtkm::cont::testing::MakeTestDataSet testDataSet;
  svtkm::cont::DataSet dataSet = testDataSet.Make3DExplicitDataSet0();
  auto cellset = dataSet.GetCellSet();

  svtkm::cont::ArrayHandle<svtkm::Float32> result;

  svtkm::worklet::DispatcherMapTopology<svtkm::worklet::CellAverage> dispatcher;
  dispatcher.Invoke(&cellset, dataSet.GetField("pointvar"), &result);

  std::cout << "Make sure we got the right answer." << std::endl;
  SVTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(0), 20.1333f),
                   "Wrong result for PointToCellAverage worklet");
  SVTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(1), 35.2f),
                   "Wrong result for PointToCellAverage worklet");

  std::cout << "Try to invoke with an input array of the wrong size." << std::endl;
  bool exceptionThrown = false;
  try
  {
    dispatcher.Invoke(dataSet.GetCellSet(),
                      dataSet.GetField("cellvar"), // should be pointvar
                      result);
  }
  catch (svtkm::cont::ErrorBadValue& error)
  {
    std::cout << "  Caught expected error: " << error.GetMessage() << std::endl;
    exceptionThrown = true;
  }
  SVTKM_TEST_ASSERT(exceptionThrown, "Dispatcher did not throw expected exception.");
}

static void TestAvgCellToPoint()
{
  std::cout << "Testing AvgCellToPoint worklet" << std::endl;

  svtkm::cont::testing::MakeTestDataSet testDataSet;
  svtkm::cont::DataSet dataSet = testDataSet.Make3DExplicitDataSet1();
  auto field = dataSet.GetField("cellvar");

  svtkm::cont::ArrayHandle<svtkm::Float32> result;

  svtkm::worklet::DispatcherMapTopology<svtkm::worklet::PointAverage> dispatcher;
  dispatcher.Invoke(dataSet.GetCellSet(), &field, result);

  std::cout << "Make sure we got the right answer." << std::endl;
  SVTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(0), 100.1f),
                   "Wrong result for CellToPointAverage worklet");
  SVTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(1), 100.15f),
                   "Wrong result for CellToPointAverage worklet");

  std::cout << "Try to invoke with an input array of the wrong size." << std::endl;
  bool exceptionThrown = false;
  try
  {
    dispatcher.Invoke(dataSet.GetCellSet(),
                      dataSet.GetField("pointvar"), // should be cellvar
                      result);
  }
  catch (svtkm::cont::ErrorBadValue& error)
  {
    std::cout << "  Caught expected error: " << error.GetMessage() << std::endl;
    exceptionThrown = true;
  }
  SVTKM_TEST_ASSERT(exceptionThrown, "Dispatcher did not throw expected exception.");
}

} // anonymous namespace

int UnitTestWorkletMapTopologyExplicit(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::RunOnDevice(TestWorkletMapTopologyExplicit, argc, argv);
}
