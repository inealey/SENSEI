//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_VecTraits_h
#define svtk_m_VecTraits_h

#include <svtkm/Types.h>

namespace svtkm
{

/// A tag for vectors that are "true" vectors (i.e. have more than one
/// component).
///
struct VecTraitsTagMultipleComponents
{
};

/// A tag for vectors that are really just scalars (i.e. have only one
/// component)
///
struct VecTraitsTagSingleComponent
{
};

/// A tag for vectors where the number of components are known at compile time.
///
struct VecTraitsTagSizeStatic
{
};

/// A tag for vectors where the number of components are not determined until
/// run time.
///
struct VecTraitsTagSizeVariable
{
};

namespace internal
{

template <svtkm::IdComponent numComponents>
struct VecTraitsMultipleComponentChooser
{
  using Type = svtkm::VecTraitsTagMultipleComponents;
};

template <>
struct VecTraitsMultipleComponentChooser<1>
{
  using Type = svtkm::VecTraitsTagSingleComponent;
};

} // namespace internal

/// The VecTraits class gives several static members that define how
/// to use a given type as a vector.
///
template <class VecType>
struct SVTKM_NEVER_EXPORT VecTraits
{
#ifdef SVTKM_DOXYGEN_ONLY
  /// \brief Type of the components in the vector.
  ///
  /// If the type is really a scalar, then the component type is the same as the scalar type.
  ///
  using ComponentType = typename VecType::ComponentType;

  /// \brief Base component type in the vector.
  ///
  /// Similar to ComponentType except that for nested vectors (e.g. Vec<Vec<T, M>, N>), it
  /// returns the base scalar type at the end of the composition (T in this example).
  ///
  using BaseComponentType = typename svtkm::VecTraits<ComponentType>::BaseComponentType;

  /// \brief Number of components in the vector.
  ///
  /// This is only defined for vectors of a static size.
  ///
  static constexpr svtkm::IdComponent NUM_COMPONENTS = VecType::NUM_COMPONENTS;

  /// Number of components in the given vector.
  ///
  static svtkm::IdComponent GetNumberOfComponents(const VecType& vec);

  /// \brief A tag specifying whether this vector has multiple components (i.e. is a "real" vector).
  ///
  /// This tag can be useful for creating specialized functions when a vector
  /// is really just a scalar.
  ///
  using HasMultipleComponents =
    typename internal::VecTraitsMultipleComponentChooser<NUM_COMPONENTS>::Type;

  /// \brief A tag specifying whether the size of this vector is known at compile time.
  ///
  /// If set to \c VecTraitsTagSizeStatic, then \c NUM_COMPONENTS is set. If
  /// set to \c VecTraitsTagSizeVariable, then the number of components is not
  /// known at compile time and must be queried with \c GetNumberOfComponents.
  ///
  using IsSizeStatic = svtkm::VecTraitsTagSizeStatic;

  /// Returns the value in a given component of the vector.
  ///
  SVTKM_EXEC_CONT static const ComponentType& GetComponent(
    const typename std::remove_const<VecType>::type& vector,
    svtkm::IdComponent component);
  SVTKM_EXEC_CONT static ComponentType& GetComponent(
    typename std::remove_const<VecType>::type& vector,
    svtkm::IdComponent component);

  /// Changes the value in a given component of the vector.
  ///
  SVTKM_EXEC_CONT static void SetComponent(VecType& vector,
                                          svtkm::IdComponent component,
                                          ComponentType value);

  /// \brief Get a vector of the same type but with a different component.
  ///
  /// This type resolves to another vector with a different component type. For example,
  /// svtkm::VecTraits<svtkm::Vec<T, N>>::ReplaceComponentType<T2> is svtkm::Vec<T2, N>.
  /// This replacement is not recursive. So VecTraits<Vec<Vec<T, M>, N>::ReplaceComponentType<T2>
  /// is svtkm::Vec<T2, N>.
  ///
  template <typename NewComponentType>
  using ReplaceComponentType = VecTemplate<NewComponentType, N>;

  /// \brief Get a vector of the same type but with a different base component.
  ///
  /// This type resolves to another vector with a different base component type. The replacement
  /// is recursive for nested types. For example,
  /// VecTraits<Vec<Vec<T, M>, N>::ReplaceComponentType<T2> is Vec<Vec<T2, M>, N>.
  ///
  template <typename NewComponentType>
  using ReplaceBaseComponentType = VecTemplate<
    typename VecTraits<ComponentType>::template ReplaceBaseComponentType<NewComponentType>,
    N>;

  /// Copies the components in the given vector into a given Vec object.
  ///
  template <vktm::IdComponent destSize>
  SVTKM_EXEC_CONT static void CopyInto(const VecType& src, svtkm::Vec<ComponentType, destSize>& dest);
#endif // SVTKM_DOXYGEN_ONLY
};

namespace detail
{

template <typename T>
struct HasVecTraitsImpl
{
  template <typename A, typename S = typename svtkm::VecTraits<A>::ComponentType>
  static std::true_type Test(A*);

  static std::false_type Test(...);

  using Type = decltype(Test(std::declval<T*>()));
};

} // namespace detail

/// \brief Determines whether the given type has VecTraits defined.
///
/// If the given type T has a valid VecTraits class, then HasVecTraits<T> will be set to
/// std::true_type. Otherwise it will be set to std::false_type. For example,
/// HasVecTraits<svtkm::Id> is the same as std::true_type whereas HasVecTraits<void *> is the same
/// as std::false_type. This is useful to block the definition of methods using VecTraits when
/// VecTraits are not defined.
///
template <typename T>
using HasVecTraits = typename detail::HasVecTraitsImpl<T>::Type;

// This partial specialization allows you to define a non-const version of
// VecTraits and have it still work for const version.
//
template <typename T>
struct SVTKM_NEVER_EXPORT VecTraits<const T> : VecTraits<T>
{
};

#if defined(SVTKM_GCC) && (__GNUC__ <= 5)
namespace detail
{

template <typename NewT, svtkm::IdComponent Size>
struct VecReplaceComponentTypeGCC4or5
{
  using type = svtkm::Vec<NewT, Size>;
};

template <typename T, svtkm::IdComponent Size, typename NewT>
struct VecReplaceBaseComponentTypeGCC4or5
{
  using type =
    svtkm::Vec<typename svtkm::VecTraits<T>::template ReplaceBaseComponentType<NewT>, Size>;
};

} // namespace detail
#endif // GCC Version 4.8

template <typename T, svtkm::IdComponent Size>
struct SVTKM_NEVER_EXPORT VecTraits<svtkm::Vec<T, Size>>
{
  using VecType = svtkm::Vec<T, Size>;

  /// \brief Type of the components in the vector.
  ///
  /// If the type is really a scalar, then the component type is the same as the scalar type.
  ///
  using ComponentType = typename VecType::ComponentType;

  /// \brief Base component type in the vector.
  ///
  /// Similar to ComponentType except that for nested vectors (e.g. Vec<Vec<T, M>, N>), it
  /// returns the base scalar type at the end of the composition (T in this example).
  ///
  using BaseComponentType = typename svtkm::VecTraits<ComponentType>::BaseComponentType;

  /// Number of components in the vector.
  ///
  static constexpr svtkm::IdComponent NUM_COMPONENTS = VecType::NUM_COMPONENTS;

  /// Number of components in the given vector.
  ///
  SVTKM_EXEC_CONT
  static svtkm::IdComponent GetNumberOfComponents(const VecType&) { return NUM_COMPONENTS; }

  /// A tag specifying whether this vector has multiple components (i.e. is a
  /// "real" vector). This tag can be useful for creating specialized functions
  /// when a vector is really just a scalar.
  ///
  using HasMultipleComponents =
    typename internal::VecTraitsMultipleComponentChooser<NUM_COMPONENTS>::Type;

  /// A tag specifying whether the size of this vector is known at compile
  /// time. If set to \c VecTraitsTagSizeStatic, then \c NUM_COMPONENTS is set.
  /// If set to \c VecTraitsTagSizeVariable, then the number of components is
  /// not known at compile time and must be queried with \c
  /// GetNumberOfComponents.
  ///
  using IsSizeStatic = svtkm::VecTraitsTagSizeStatic;

  /// Returns the value in a given component of the vector.
  ///
  SVTKM_EXEC_CONT
  static const ComponentType& GetComponent(const VecType& vector, svtkm::IdComponent component)
  {
    return vector[component];
  }
  SVTKM_EXEC_CONT
  static ComponentType& GetComponent(VecType& vector, svtkm::IdComponent component)
  {
    return vector[component];
  }

  /// Changes the value in a given component of the vector.
  ///
  SVTKM_EXEC_CONT static void SetComponent(VecType& vector,
                                          svtkm::IdComponent component,
                                          ComponentType value)
  {
    vector[component] = value;
  }

/// \brief Get a vector of the same type but with a different component.
///
/// This type resolves to another vector with a different component type. For example,
/// svtkm::VecTraits<svtkm::Vec<T, N>>::ReplaceComponentType<T2> is svtkm::Vec<T2, N>.
/// This replacement is not recursive. So VecTraits<Vec<Vec<T, M>, N>::ReplaceComponentType<T2>
/// is svtkm::Vec<T2, N>.
///@{
#if defined(SVTKM_GCC) && (__GNUC__ <= 5)
  // Silly workaround for bug in GCC <= 5
  template <typename NewComponentType>
  using ReplaceComponentType =
    typename detail::VecReplaceComponentTypeGCC4or5<NewComponentType, Size>::type;
#else // !GCC <= 5
  template <typename NewComponentType>
  using ReplaceComponentType = svtkm::Vec<NewComponentType, Size>;
#endif
///@}

/// \brief Get a vector of the same type but with a different base component.
///
/// This type resolves to another vector with a different base component type. The replacement
/// is recursive for nested types. For example,
/// VecTraits<Vec<Vec<T, M>, N>::ReplaceComponentType<T2> is Vec<Vec<T2, M>, N>.
///@{
#if defined(SVTKM_GCC) && (__GNUC__ <= 5)
  // Silly workaround for bug in GCC <= 5
  template <typename NewComponentType>
  using ReplaceBaseComponentType =
    typename detail::VecReplaceBaseComponentTypeGCC4or5<T, Size, NewComponentType>::type;
#else // !GCC <= 5
  template <typename NewComponentType>
  using ReplaceBaseComponentType = svtkm::Vec<
    typename svtkm::VecTraits<ComponentType>::template ReplaceBaseComponentType<NewComponentType>,
    Size>;
#endif
  ///@}

  /// Converts whatever type this vector is into the standard SVTKm Tuple.
  ///
  template <svtkm::IdComponent destSize>
  SVTKM_EXEC_CONT static void CopyInto(const VecType& src, svtkm::Vec<ComponentType, destSize>& dest)
  {
    src.CopyInto(dest);
  }
};

template <typename T>
struct SVTKM_NEVER_EXPORT VecTraits<svtkm::VecC<T>>
{
  using VecType = svtkm::VecC<T>;

  /// \brief Type of the components in the vector.
  ///
  /// If the type is really a scalar, then the component type is the same as the scalar type.
  ///
  using ComponentType = typename VecType::ComponentType;

  /// \brief Base component type in the vector.
  ///
  /// Similar to ComponentType except that for nested vectors (e.g. Vec<Vec<T, M>, N>), it
  /// returns the base scalar type at the end of the composition (T in this example).
  ///
  using BaseComponentType = typename svtkm::VecTraits<ComponentType>::BaseComponentType;

  /// Number of components in the given vector.
  ///
  SVTKM_EXEC_CONT
  static svtkm::IdComponent GetNumberOfComponents(const VecType& vector)
  {
    return vector.GetNumberOfComponents();
  }

  /// A tag specifying whether this vector has multiple components (i.e. is a
  /// "real" vector). This tag can be useful for creating specialized functions
  /// when a vector is really just a scalar.
  ///
  /// The size of a \c VecC is not known until runtime and can always
  /// potentially have multiple components, this is always set to \c
  /// HasMultipleComponents.
  ///
  using HasMultipleComponents = svtkm::VecTraitsTagMultipleComponents;

  /// A tag specifying whether the size of this vector is known at compile
  /// time. If set to \c VecTraitsTagSizeStatic, then \c NUM_COMPONENTS is set.
  /// If set to \c VecTraitsTagSizeVariable, then the number of components is
  /// not known at compile time and must be queried with \c
  /// GetNumberOfComponents.
  ///
  using IsSizeStatic = svtkm::VecTraitsTagSizeVariable;

  /// Returns the value in a given component of the vector.
  ///
  SVTKM_EXEC_CONT
  static const ComponentType& GetComponent(const VecType& vector, svtkm::IdComponent component)
  {
    return vector[component];
  }
  SVTKM_EXEC_CONT
  static ComponentType& GetComponent(VecType& vector, svtkm::IdComponent component)
  {
    return vector[component];
  }

  /// Changes the value in a given component of the vector.
  ///
  SVTKM_EXEC_CONT
  static void SetComponent(VecType& vector, svtkm::IdComponent component, ComponentType value)
  {
    vector[component] = value;
  }

  /// \brief Get a vector of the same type but with a different component.
  ///
  /// This type resolves to another vector with a different component type. For example,
  /// svtkm::VecTraits<svtkm::Vec<T, N>>::ReplaceComponentType<T2> is svtkm::Vec<T2, N>.
  /// This replacement is not recursive. So VecTraits<Vec<Vec<T, M>, N>::ReplaceComponentType<T2>
  /// is svtkm::Vec<T2, N>.
  ///
  template <typename NewComponentType>
  using ReplaceComponentType = svtkm::VecC<NewComponentType>;

  /// \brief Get a vector of the same type but with a different base component.
  ///
  /// This type resolves to another vector with a different base component type. The replacement
  /// is recursive for nested types. For example,
  /// VecTraits<Vec<Vec<T, M>, N>::ReplaceComponentType<T2> is Vec<Vec<T2, M>, N>.
  ///
  template <typename NewComponentType>
  using ReplaceBaseComponentType = svtkm::VecC<
    typename svtkm::VecTraits<ComponentType>::template ReplaceBaseComponentType<NewComponentType>>;

  /// Converts whatever type this vector is into the standard SVTKm Tuple.
  ///
  template <svtkm::IdComponent destSize>
  SVTKM_EXEC_CONT static void CopyInto(const VecType& src, svtkm::Vec<ComponentType, destSize>& dest)
  {
    src.CopyInto(dest);
  }
};

template <typename T>
struct SVTKM_NEVER_EXPORT VecTraits<svtkm::VecCConst<T>>
{
  using VecType = svtkm::VecCConst<T>;

  /// \brief Type of the components in the vector.
  ///
  /// If the type is really a scalar, then the component type is the same as the scalar type.
  ///
  using ComponentType = typename VecType::ComponentType;

  /// \brief Base component type in the vector.
  ///
  /// Similar to ComponentType except that for nested vectors (e.g. Vec<Vec<T, M>, N>), it
  /// returns the base scalar type at the end of the composition (T in this example).
  ///
  using BaseComponentType = typename svtkm::VecTraits<ComponentType>::BaseComponentType;

  /// Number of components in the given vector.
  ///
  SVTKM_EXEC_CONT
  static svtkm::IdComponent GetNumberOfComponents(const VecType& vector)
  {
    return vector.GetNumberOfComponents();
  }

  /// A tag specifying whether this vector has multiple components (i.e. is a
  /// "real" vector). This tag can be useful for creating specialized functions
  /// when a vector is really just a scalar.
  ///
  /// The size of a \c VecCConst is not known until runtime and can always
  /// potentially have multiple components, this is always set to \c
  /// HasMultipleComponents.
  ///
  using HasMultipleComponents = svtkm::VecTraitsTagMultipleComponents;

  /// A tag specifying whether the size of this vector is known at compile
  /// time. If set to \c VecTraitsTagSizeStatic, then \c NUM_COMPONENTS is set.
  /// If set to \c VecTraitsTagSizeVariable, then the number of components is
  /// not known at compile time and must be queried with \c
  /// GetNumberOfComponents.
  ///
  using IsSizeStatic = svtkm::VecTraitsTagSizeVariable;

  /// Returns the value in a given component of the vector.
  ///
  SVTKM_EXEC_CONT
  static const ComponentType& GetComponent(const VecType& vector, svtkm::IdComponent component)
  {
    return vector[component];
  }

  /// Changes the value in a given component of the vector.
  ///
  SVTKM_EXEC_CONT
  static void SetComponent(VecType& vector, svtkm::IdComponent component, ComponentType value)
  {
    vector[component] = value;
  }

  /// \brief Get a vector of the same type but with a different component.
  ///
  /// This type resolves to another vector with a different component type. For example,
  /// svtkm::VecTraits<svtkm::Vec<T, N>>::ReplaceComponentType<T2> is svtkm::Vec<T2, N>.
  /// This replacement is not recursive. So VecTraits<Vec<Vec<T, M>, N>::ReplaceComponentType<T2>
  /// is svtkm::Vec<T2, N>.
  ///
  template <typename NewComponentType>
  using ReplaceComponentType = svtkm::VecCConst<NewComponentType>;

  /// \brief Get a vector of the same type but with a different base component.
  ///
  /// This type resolves to another vector with a different base component type. The replacement
  /// is recursive for nested types. For example,
  /// VecTraits<Vec<Vec<T, M>, N>::ReplaceComponentType<T2> is Vec<Vec<T2, M>, N>.
  ///
  template <typename NewComponentType>
  using ReplaceBaseComponentType = svtkm::VecCConst<
    typename svtkm::VecTraits<ComponentType>::template ReplaceBaseComponentType<NewComponentType>>;

  /// Converts whatever type this vector is into the standard SVTKm Tuple.
  ///
  template <svtkm::IdComponent destSize>
  SVTKM_EXEC_CONT static void CopyInto(const VecType& src, svtkm::Vec<ComponentType, destSize>& dest)
  {
    src.CopyInto(dest);
  }
};

namespace internal
{
/// Used for overriding VecTraits for basic scalar types.
///
template <typename ScalarType>
struct SVTKM_NEVER_EXPORT VecTraitsBasic
{
  using ComponentType = ScalarType;
  using BaseComponentType = ScalarType;
  static constexpr svtkm::IdComponent NUM_COMPONENTS = 1;
  using HasMultipleComponents = svtkm::VecTraitsTagSingleComponent;
  using IsSizeStatic = svtkm::VecTraitsTagSizeStatic;

  SVTKM_EXEC_CONT
  static svtkm::IdComponent GetNumberOfComponents(const ScalarType&) { return 1; }

  SVTKM_EXEC_CONT
  static const ComponentType& GetComponent(const ScalarType& vector, svtkm::IdComponent)
  {
    return vector;
  }
  SVTKM_EXEC_CONT
  static ComponentType& GetComponent(ScalarType& vector, svtkm::IdComponent) { return vector; }

  SVTKM_EXEC_CONT static void SetComponent(ScalarType& vector,
                                          svtkm::IdComponent,
                                          ComponentType value)
  {
    vector = value;
  }

  template <typename NewComponentType>
  using ReplaceComponentType = NewComponentType;

  template <typename NewComponentType>
  using ReplaceBaseComponentType = NewComponentType;

  template <svtkm::IdComponent destSize>
  SVTKM_EXEC_CONT static void CopyInto(const ScalarType& src, svtkm::Vec<ScalarType, destSize>& dest)
  {
    dest[0] = src;
  }
};
} // namespace internal

/// \brief VecTraits for Pair types
///
/// Although a pair would seem better as a size-2 vector, we treat it as a
/// scalar. This is because a \c Vec is assumed to have the same type for
/// every component, and a pair in general has a different type for each
/// component. Thus we treat a pair as a "scalar" unit.
///
template <typename T, typename U>
struct SVTKM_NEVER_EXPORT VecTraits<svtkm::Pair<T, U>>
  : public svtkm::internal::VecTraitsBasic<svtkm::Pair<T, U>>
{
};

} // anonymous namespace

#define SVTKM_BASIC_TYPE_VECTOR(type)                                                               \
  namespace svtkm                                                                                   \
  {                                                                                                \
  template <>                                                                                      \
  struct SVTKM_NEVER_EXPORT VecTraits<type> : public svtkm::internal::VecTraitsBasic<type>           \
  {                                                                                                \
  };                                                                                               \
  }

/// Allows you to treat basic types as if they were vectors.

SVTKM_BASIC_TYPE_VECTOR(float)
SVTKM_BASIC_TYPE_VECTOR(double)

SVTKM_BASIC_TYPE_VECTOR(bool)
SVTKM_BASIC_TYPE_VECTOR(char)
SVTKM_BASIC_TYPE_VECTOR(signed char)
SVTKM_BASIC_TYPE_VECTOR(unsigned char)
SVTKM_BASIC_TYPE_VECTOR(short)
SVTKM_BASIC_TYPE_VECTOR(unsigned short)
SVTKM_BASIC_TYPE_VECTOR(int)
SVTKM_BASIC_TYPE_VECTOR(unsigned int)
SVTKM_BASIC_TYPE_VECTOR(long)
SVTKM_BASIC_TYPE_VECTOR(unsigned long)
SVTKM_BASIC_TYPE_VECTOR(long long)
SVTKM_BASIC_TYPE_VECTOR(unsigned long long)

//#undef SVTKM_BASIC_TYPE_VECTOR

#endif //svtk_m_VecTraits_h
