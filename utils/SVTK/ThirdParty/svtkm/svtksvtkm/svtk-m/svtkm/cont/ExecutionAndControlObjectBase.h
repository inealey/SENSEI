//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ExecutionAndControlObjectBase_h
#define svtk_m_cont_ExecutionAndControlObjectBase_h

#include <svtkm/cont/ExecutionObjectBase.h>

namespace svtkm
{
namespace cont
{

/// Base \c ExecutionAndControlObjectBase class. These are objects that behave
/// as execution objects but can also be use din the control environment.
/// Any subclass of \c ExecutionAndControlObjectBase must implement a
/// \c PrepareForExecution method that takes a device adapter tag and returns
/// an object for that device as well as a \c PrepareForControl that simply
/// returns an object that works in the control environment.
///
struct ExecutionAndControlObjectBase : svtkm::cont::ExecutionObjectBase
{
};

namespace internal
{

namespace detail
{

struct CheckPrepareForControl
{
  template <typename T>
  static auto check(T* p) -> decltype(p->PrepareForControl(), std::true_type());

  template <typename T>
  static auto check(...) -> std::false_type;
};

} // namespace detail

template <typename T>
using IsExecutionAndControlObjectBase =
  std::is_base_of<svtkm::cont::ExecutionAndControlObjectBase, typename std::decay<T>::type>;

template <typename T>
struct HasPrepareForControl
  : decltype(detail::CheckPrepareForControl::check<typename std::decay<T>::type>(nullptr))
{
};

} // namespace internal
}
} // namespace svtkm::cont

/// Checks that the argument is a proper execution object.
///
#define SVTKM_IS_EXECUTION_AND_CONTROL_OBJECT(execObject)                                           \
  static_assert(::svtkm::cont::internal::IsExecutionAndControlObjectBase<execObject>::value,        \
                "Provided type is not a subclass of svtkm::cont::ExecutionAndControlObjectBase.");  \
  static_assert(::svtkm::cont::internal::HasPrepareForExecution<execObject>::value,                 \
                "Provided type does not have requisite PrepareForExecution method.");              \
  static_assert(::svtkm::cont::internal::HasPrepareForControl<execObject>::value,                   \
                "Provided type does not have requisite PrepareForControl method.")

#endif //svtk_m_cont_ExecutionAndControlObjectBase_h
