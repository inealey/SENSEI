//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/DynamicCellSet.h>

#include <svtkm/cont/ArrayHandleConstant.h>

#include <svtkm/cont/testing/Testing.h>

namespace
{

using NonDefaultCellSetList =
  svtkm::List<svtkm::cont::CellSetStructured<1>,
             svtkm::cont::CellSetExplicit<svtkm::cont::ArrayHandleConstant<svtkm::UInt8>::StorageTag>>;

template <typename ExpectedCellType>
struct CheckFunctor
{
  void operator()(const ExpectedCellType&, bool& called) const { called = true; }

  template <typename UnexpectedType>
  void operator()(const UnexpectedType&, bool& called) const
  {
    SVTKM_TEST_FAIL("CastAndCall functor called with wrong type.");
    called = false;
  }
};

class DummyCellSet : public svtkm::cont::CellSet
{
};

void CheckEmptyDynamicCellSet()
{
  svtkm::cont::DynamicCellSet empty;

  SVTKM_TEST_ASSERT(empty.GetNumberOfCells() == 0, "DynamicCellSet should have no cells");
  SVTKM_TEST_ASSERT(empty.GetNumberOfFaces() == 0, "DynamicCellSet should have no faces");
  SVTKM_TEST_ASSERT(empty.GetNumberOfEdges() == 0, "DynamicCellSet should have no edges");
  SVTKM_TEST_ASSERT(empty.GetNumberOfPoints() == 0, "DynamicCellSet should have no points");

  empty.PrintSummary(std::cout);

  using CellSet2D = svtkm::cont::CellSetStructured<2>;
  using CellSet3D = svtkm::cont::CellSetStructured<3>;
  SVTKM_TEST_ASSERT(!empty.template IsType<CellSet2D>(), "DynamicCellSet reports wrong type.");
  SVTKM_TEST_ASSERT(!empty.template IsType<CellSet3D>(), "DynamicCellSet reports wrong type.");
  SVTKM_TEST_ASSERT(!empty.template IsType<DummyCellSet>(), "DynamicCellSet reports wrong type.");

  CellSet2D instance;
  SVTKM_TEST_ASSERT(!empty.IsSameType(instance), "DynamicCellSet reports wrong type.");

  bool gotException = false;
  try
  {
    instance = empty.Cast<CellSet2D>();
  }
  catch (svtkm::cont::ErrorBadType&)
  {
    gotException = true;
  }
  SVTKM_TEST_ASSERT(gotException, "Empty DynamicCellSet should have thrown on casting");

  auto empty2 = empty.NewInstance();
  SVTKM_TEST_ASSERT(empty.GetCellSetBase() == nullptr, "DynamicCellSet should contain a nullptr");
  SVTKM_TEST_ASSERT(empty2.GetCellSetBase() == nullptr, "DynamicCellSet should contain a nullptr");
}

template <typename CellSetType, typename CellSetList>
void CheckDynamicCellSet(const CellSetType& cellSet,
                         svtkm::cont::DynamicCellSetBase<CellSetList> dynamicCellSet)
{
  SVTKM_TEST_ASSERT(dynamicCellSet.template IsType<CellSetType>(),
                   "DynamicCellSet reports wrong type.");
  SVTKM_TEST_ASSERT(dynamicCellSet.IsSameType(cellSet), "DynamicCellSet reports wrong type.");
  SVTKM_TEST_ASSERT(!dynamicCellSet.template IsType<DummyCellSet>(),
                   "DynamicCellSet reports wrong type.");

  dynamicCellSet.template Cast<CellSetType>();

  bool called = false;
  dynamicCellSet.CastAndCall(CheckFunctor<CellSetType>(), called);

  SVTKM_TEST_ASSERT(
    called, "The functor was never called (and apparently a bad value exception not thrown).");

  called = false;
  CastAndCall(dynamicCellSet, CheckFunctor<CellSetType>(), called);

  SVTKM_TEST_ASSERT(
    called, "The functor was never called (and apparently a bad value exception not thrown).");
}

template <typename CellSetType, typename CellSetList>
void TryNewInstance(CellSetType, svtkm::cont::DynamicCellSetBase<CellSetList>& originalCellSet)
{
  svtkm::cont::DynamicCellSetBase<CellSetList> newCellSet = originalCellSet.NewInstance();

  SVTKM_TEST_ASSERT(newCellSet.template IsType<CellSetType>(), "New cell set wrong type.");

  SVTKM_TEST_ASSERT(originalCellSet.GetCellSetBase() != newCellSet.GetCellSetBase(),
                   "NewInstance did not make a copy.");
}

template <typename CellSetType, typename CellSetList>
void TryCellSet(CellSetType cellSet, svtkm::cont::DynamicCellSetBase<CellSetList>& dynamicCellSet)
{
  CheckDynamicCellSet(cellSet, dynamicCellSet);

  CheckDynamicCellSet(cellSet, dynamicCellSet.ResetCellSetList(svtkm::List<CellSetType>()));

  TryNewInstance(cellSet, dynamicCellSet);
}

template <typename CellSetType>
void TryDefaultCellSet(CellSetType cellSet)
{
  svtkm::cont::DynamicCellSet dynamicCellSet(cellSet);

  TryCellSet(cellSet, dynamicCellSet);
}

template <typename CellSetType>
void TryNonDefaultCellSet(CellSetType cellSet)
{
  svtkm::cont::DynamicCellSetBase<NonDefaultCellSetList> dynamicCellSet(cellSet);

  TryCellSet(cellSet, dynamicCellSet);
}

void TestDynamicCellSet()
{
  std::cout << "Try default types with default type lists." << std::endl;
  std::cout << "*** 2D Structured Grid ******************" << std::endl;
  TryDefaultCellSet(svtkm::cont::CellSetStructured<2>());
  std::cout << "*** 3D Structured Grid ******************" << std::endl;
  TryDefaultCellSet(svtkm::cont::CellSetStructured<3>());
  std::cout << "*** Explicit Grid ***********************" << std::endl;
  TryDefaultCellSet(svtkm::cont::CellSetExplicit<>());

  std::cout << std::endl << "Try non-default types." << std::endl;
  std::cout << "*** 1D Structured Grid ******************" << std::endl;
  TryNonDefaultCellSet(svtkm::cont::CellSetStructured<1>());
  std::cout << "*** Explicit Grid Constant Shape ********" << std::endl;
  TryNonDefaultCellSet(
    svtkm::cont::CellSetExplicit<svtkm::cont::ArrayHandleConstant<svtkm::UInt8>::StorageTag>());

  std::cout << std::endl << "Try empty DynamicCellSet." << std::endl;
  CheckEmptyDynamicCellSet();
}

} // anonymous namespace

int UnitTestDynamicCellSet(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestDynamicCellSet, argc, argv);
}
