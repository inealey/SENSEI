//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/internal/FunctionInterface.h>
#include <svtkm/testing/Testing.h>

#include <sstream>
#include <string>

namespace
{

using Type1 = svtkm::Id;
const Type1 Arg1 = 1234;

using Type2 = svtkm::Float64;
const Type2 Arg2 = 5678.125;

using Type3 = std::string;
const Type3 Arg3("Third argument");

using Type4 = svtkm::Vec3f_32;
const Type4 Arg4(1.2f, 3.4f, 5.6f);

using Type5 = svtkm::Id3;
const Type5 Arg5(4, 5, 6);

struct PointerTransform
{
  template <typename T, svtkm::IdComponent Index>
  struct ReturnType
  {
    using type = const T*;
  };

  template <typename T, typename IndexTag>
  const T* operator()(const T& x, IndexTag) const
  {
    return &x;
  }
};

void TestBasicFunctionInterface()
{
  using svtkm::internal::ParameterGet;
  std::cout << "Creating basic function interface." << std::endl;
  svtkm::internal::FunctionInterface<void(Type1, Type2, Type3)> funcInterface =
    svtkm::internal::make_FunctionInterface<void>(Arg1, Arg2, Arg3);

  std::cout << "Checking parameters." << std::endl;
  SVTKM_TEST_ASSERT(funcInterface.GetArity() == 3, "Got wrong number of parameters.");
  SVTKM_TEST_ASSERT(ParameterGet<1>(funcInterface) == Arg1, "Arg 1 incorrect.");
  SVTKM_TEST_ASSERT(ParameterGet<2>(funcInterface) == Arg2, "Arg 2 incorrect.");
  SVTKM_TEST_ASSERT(ParameterGet<3>(funcInterface) == Arg3, "Arg 3 incorrect.");

  svtkm::internal::FunctionInterface<void(Type1, Type2, Type3)> funcInterfaceEmpty;
  SVTKM_TEST_ASSERT(funcInterfaceEmpty.GetArity() == 3, "Got wrong number of parameters.");
  SVTKM_TEST_ASSERT(ParameterGet<1>(funcInterfaceEmpty) != Arg1, "Arg 1 incorrect.");
  SVTKM_TEST_ASSERT(ParameterGet<2>(funcInterfaceEmpty) != Arg2, "Arg 2 incorrect.");
  SVTKM_TEST_ASSERT(ParameterGet<3>(funcInterfaceEmpty) != Arg3, "Arg 3 incorrect.");

  auto funcInterface5 = svtkm::internal::make_FunctionInterface<void>(Arg1, Arg2, Arg3, Arg4, Arg5);
  std::cout << "Checking 5 parameter function interface." << std::endl;
  SVTKM_TEST_ASSERT(funcInterface5.GetArity() == 5, "Got wrong number of parameters.");
  SVTKM_TEST_ASSERT(ParameterGet<1>(funcInterface5) == Arg1, "Arg 1 incorrect.");
  SVTKM_TEST_ASSERT(ParameterGet<2>(funcInterface5) == Arg2, "Arg 2 incorrect.");
  SVTKM_TEST_ASSERT(ParameterGet<3>(funcInterface5) == Arg3, "Arg 3 incorrect.");
  SVTKM_TEST_ASSERT(ParameterGet<4>(funcInterface5) == Arg4, "Arg 4 incorrect.");
  SVTKM_TEST_ASSERT(ParameterGet<5>(funcInterface5) == Arg5, "Arg 5 incorrect.");
}

void TestStaticTransform()
{
  using svtkm::internal::ParameterGet;
  std::cout << "Trying static transform." << std::endl;
  using OriginalType = svtkm::internal::FunctionInterface<void(Type1, Type2, Type3)>;
  OriginalType funcInterface = svtkm::internal::make_FunctionInterface<void>(Arg1, Arg2, Arg3);

  std::cout << "Transform to pointer type." << std::endl;
  auto funcInterfaceTransform1 = funcInterface.StaticTransformCont(PointerTransform());

  using P1 = typename std::decay<decltype(ParameterGet<1>(funcInterfaceTransform1))>::type;
  using P2 = typename std::decay<decltype(ParameterGet<2>(funcInterfaceTransform1))>::type;
  using P3 = typename std::decay<decltype(ParameterGet<3>(funcInterfaceTransform1))>::type;

  SVTKM_STATIC_ASSERT((std::is_same<const Type1*, P1>::value));
  SVTKM_STATIC_ASSERT((std::is_same<const Type2*, P2>::value));
  SVTKM_STATIC_ASSERT((std::is_same<const Type3*, P3>::value));
}

void TestFunctionInterface()
{
  TestBasicFunctionInterface();
  TestStaticTransform();
}

} // anonymous namespace

int UnitTestFunctionInterface(int argc, char* argv[])
{
  return svtkm::testing::Testing::Run(TestFunctionInterface, argc, argv);
}
