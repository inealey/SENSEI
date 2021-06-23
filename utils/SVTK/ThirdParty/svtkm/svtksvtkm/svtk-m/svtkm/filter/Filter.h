//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_filter_Filter_h
#define svtk_m_filter_Filter_h

#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/Field.h>
#include <svtkm/cont/Invoker.h>
#include <svtkm/cont/PartitionedDataSet.h>

#include <svtkm/filter/CreateResult.h>
#include <svtkm/filter/FieldSelection.h>
#include <svtkm/filter/FilterTraits.h>
#include <svtkm/filter/PolicyBase.h>


namespace svtkm
{
namespace filter
{
/// \brief base class for all filters.
///
/// This is the base class for all filters. To add a new filter, one can
/// subclass this (or any of the existing subclasses e.g. FilterField,
/// FilterDataSet, FilterDataSetWithField, etc. and implement relevant methods.
///
/// \section FilterUsage Usage
///
/// To execute a filter, one typically calls the `auto result =
/// filter.Execute(input)`. Typical usage is as follows:
///
/// \code{cpp}
///
/// // create the concrete subclass (e.g. MarchingCubes).
/// svtkm::filter::MarchingCubes marchingCubes;
///
/// // select fieds to map to the output, if different from default which is to
/// // map all input fields.
/// marchingCubes.SetFieldToPass({"var1", "var2"});
///
/// // execute the filter on svtkm::cont::DataSet.
/// svtkm::cont::DataSet dsInput = ...
/// auto outputDS = filter.Execute(dsInput);
///
/// // or, execute on a svtkm::cont::PartitionedDataSet
/// svtkm::cont::PartitionedDataSet mbInput = ...
/// auto outputMB = filter.Execute(mbInput);
/// \endcode
///
/// `Execute` methods take in the input DataSet or PartitionedDataSet to
/// process and return the result. The type of the result is same as the input
/// type, thus `Execute(DataSet&)` returns a DataSet while
/// `Execute(PartitionedDataSet&)` returns a PartitionedDataSet.
///
/// The implementation for `Execute(DataSet&)` is merely provided for
/// convenience. Internally, it creates a PartitionedDataSet with a single
/// partition for the input and then forwards the call to
/// `Execute(PartitionedDataSet&)`. The method returns the first partition, if
/// any, from the PartitionedDataSet returned by the forwarded call. If the
/// PartitionedDataSet returned has more than 1 partition, then
/// `svtkm::cont::ErrorFilterExecution` will be thrown.
///
/// \section FilterSubclassing Subclassing
///
/// Typically, one subclasses one of the immediate subclasses of this class such as
/// FilterField, FilterDataSet, FilterDataSetWithField, etc. Those may impose
/// additional constraints on the methods to implement in the subclasses.
/// Here, we describes the things to consider when directly subclassing
/// svtkm::filter::Filter.
///
/// \subsection FilterPreExecutePostExecute PreExecute and PostExecute
///
/// Subclasses may provide implementations for either or both of the following
/// methods.
///
/// \code{cpp}
///
/// template <typename DerivedPolicy>
/// void PreExecute(const svtkm::cont::PartitionedDataSet& input,
///           const svtkm::filter::PolicyBase<DerivedPolicy>& policy);
///
/// template <typename DerivedPolicy>
/// void PostExecute(const svtkm::cont::PartitionedDataSet& input, svtkm::cont::PartitionedDataSet& output
///           const svtkm::filter::PolicyBase<DerivedPolicy>& policy);
///
/// \endcode
///
/// As the name suggests, these are called and the beginning and before the end
/// of an `Filter::Execute` call. Most filters that don't need to handle
/// PartitionedDataSet specially, e.g. clip, cut, iso-contour, need not worry
/// about these methods or provide any implementation. If, however, your filter
/// needs do to some initialization e.g. allocation buffers to accumulate
/// results, or finalization e.g. reduce results across all partitions, then
/// these methods provide convenient hooks for the same.
///
/// \subsection FilterPrepareForExecution PrepareForExecution
///
/// A concrete subclass of Filter must provide `PrepareForExecution`
/// implementation that provides the meat for the filter i.e. the implementation
/// for the filter's data processing logic. There are two signatures
/// available; which one to implement depends on the nature of the filter.
///
/// Let's consider simple filters that do not need to do anything special to
/// handle PartitionedDataSet e.g. clip, contour, etc. These are the filters
/// where executing the filter on a PartitionedDataSet simply means executing
/// the filter on one partition at a time and packing the output for each
/// iteration info the result PartitionedDataSet. For such filters, one must
/// implement the following signature.
///
/// \code{cpp}
///
/// template <typename DerivedPolicy>
/// svtkm::cont::DataSet PrepareForExecution(
///         const svtkm::cont::DataSet& input,
///         const svtkm::filter::PolicyBase<DerivedPolicy>& policy);
///
/// \endcode
///
/// The role of this method is to execute on the input dataset and generate the
/// result and return it.  If there are any errors, the subclass must throw an
/// exception (e.g. `svtkm::cont::ErrorFilterExecution`).
///
/// In this case, the Filter superclass handles iterating over multiple
/// partitions in the input PartitionedDataSet and calling
/// `PrepareForExecution` iteratively.
///
/// The aforementioned approach is also suitable for filters that need special
/// handling for PartitionedDataSets which can be modelled as PreExecute and
/// PostExecute steps (e.g. `svtkm::filter::Histogram`).
///
/// For more complex filters, like streamlines, particle tracking, where the
/// processing of PartitionedDataSets cannot be modelled as a reduction of the
/// results, one can implement the following signature.
///
/// \code{cpp}
/// template <typename DerivedPolicy>
/// svtkm::cont::PartitionedDataSet PrepareForExecution(
///         const svtkm::cont::PartitionedDataSet& input,
///         const svtkm::filter::PolicyBase<DerivedPolicy>& policy);
/// \endcode
///
/// The responsibility of this method is the same, except now the subclass is
/// given full control over the execution, including any mapping of fields to
/// output (described in next sub-section).
///
/// \subsection FilterMapFieldOntoOutput MapFieldOntoOutput
///
/// Subclasses may provide `MapFieldOntoOutput` method with the following
/// signature:
///
/// \code{cpp}
///
/// template <typename DerivedPolicy>
/// SVTKM_CONT bool MapFieldOntoOutput(svtkm::cont::DataSet& result,
///                                   const svtkm::cont::Field& field,
///                                   const svtkm::filter::PolicyBase<DerivedPolicy>& policy);
///
/// \endcode
///
/// When present, this method will be called after each partition execution to
/// map an input field from the corresponding input partition to the output
/// partition.
///
template <typename Derived>
class Filter
{
public:
  SVTKM_CONT
  Filter();

  SVTKM_CONT
  ~Filter();

  /// \brief Specify which subset of types a filter supports.
  ///
  /// A filter is able to state what subset of types it supports
  /// by default. By default we use ListUniversal to represent that the
  /// filter accepts all types specified by the users provided policy
  using SupportedTypes = svtkm::ListUniversal;

  /// \brief Specify which additional field storage to support.
  ///
  /// When a filter gets a field value from a DataSet, it has to determine what type
  /// of storage the array has. Typically this is taken from the policy passed to
  /// the filter's execute. In some cases it is useful to support additional types.
  /// For example, the filter might make sense to support ArrayHandleIndex or
  /// ArrayHandleConstant. If so, the storage of those additional types should be
  /// listed here.
  using AdditionalFieldStorage = svtkm::ListEmpty;

  //@{
  /// \brief Specify which fields get passed from input to output.
  ///
  /// After a filter successfully executes and returns a new data set, fields are mapped from
  /// input to output. Depending on what operation the filter does, this could be a simple shallow
  /// copy of an array, or it could be a computed operation. You can control which fields are
  /// passed (and equivalently which are not) with this parameter.
  ///
  /// By default, all fields are passed during execution.
  ///
  SVTKM_CONT
  void SetFieldsToPass(const svtkm::filter::FieldSelection& fieldsToPass)
  {
    this->FieldsToPass = fieldsToPass;
  }

  SVTKM_CONT
  void SetFieldsToPass(const svtkm::filter::FieldSelection& fieldsToPass,
                       svtkm::filter::FieldSelection::ModeEnum mode)
  {
    this->FieldsToPass = fieldsToPass;
    this->FieldsToPass.SetMode(mode);
  }

  SVTKM_CONT
  void SetFieldsToPass(
    const std::string& fieldname,
    svtkm::cont::Field::Association association,
    svtkm::filter::FieldSelection::ModeEnum mode = svtkm::filter::FieldSelection::MODE_SELECT)
  {
    this->SetFieldsToPass({ fieldname, association }, mode);
  }

  SVTKM_CONT
  const svtkm::filter::FieldSelection& GetFieldsToPass() const { return this->FieldsToPass; }
  SVTKM_CONT
  svtkm::filter::FieldSelection& GetFieldsToPass() { return this->FieldsToPass; }
  //@}

  //@{
  /// Executes the filter on the input and produces a result dataset.
  ///
  /// On success, this the dataset produced. On error, svtkm::cont::ErrorExecution will be thrown.
  SVTKM_CONT svtkm::cont::DataSet Execute(const svtkm::cont::DataSet& input);

  template <typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet Execute(const svtkm::cont::DataSet& input,
                                        svtkm::filter::PolicyBase<DerivedPolicy> policy);
  //@}

  //@{
  /// Executes the filter on the input PartitionedDataSet and produces a result PartitionedDataSet.
  ///
  /// On success, this the dataset produced. On error, svtkm::cont::ErrorExecution will be thrown.
  SVTKM_CONT svtkm::cont::PartitionedDataSet Execute(const svtkm::cont::PartitionedDataSet& input);

  template <typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::PartitionedDataSet Execute(const svtkm::cont::PartitionedDataSet& input,
                                                   svtkm::filter::PolicyBase<DerivedPolicy> policy);
  //@}

  /// Map fields from input dataset to output.
  /// This is not intended for external use. Subclasses of Filter, however, may
  /// use this method to map fields.
  template <typename DerivedPolicy>
  SVTKM_CONT void MapFieldsToPass(const svtkm::cont::DataSet& input,
                                 svtkm::cont::DataSet& output,
                                 svtkm::filter::PolicyBase<DerivedPolicy> policy);

  /// Specify the svtkm::cont::Invoker to be used to execute worklets by
  /// this filter instance. Overriding the default allows callers to control
  /// which device adapters a filter uses.
  void SetInvoker(svtkm::cont::Invoker inv) { this->Invoke = inv; }

protected:
  svtkm::cont::Invoker Invoke;

private:
  svtkm::filter::FieldSelection FieldsToPass;
};
}
} // namespace svtkm::filter

#define SVTKM_FILTER_EXPORT_EXECUTE_METHOD_WITH_POLICY(Name, Policy)                                \
  extern template SVTKM_FILTER_TEMPLATE_EXPORT svtkm::cont::PartitionedDataSet                       \
  svtkm::filter::Filter<Name>::Execute(svtkm::cont::PartitionedDataSet const&,                       \
                                      svtkm::filter::PolicyBase<Policy>)
#define SVTKM_FILTER_INSTANTIATE_EXECUTE_METHOD_WITH_POLICY(Name, Policy)                           \
  template SVTKM_FILTER_EXPORT svtkm::cont::PartitionedDataSet Filter<Name>::Execute(                \
    svtkm::cont::PartitionedDataSet const&, svtkm::filter::PolicyBase<Policy>)

#define SVTKM_FILTER_EXPORT_EXECUTE_METHOD(Name)                                                    \
  SVTKM_FILTER_EXPORT_EXECUTE_METHOD_WITH_POLICY(Name, svtkm::filter::PolicyDefault)
#define SVTKM_FILTER_INSTANTIATE_EXECUTE_METHOD(Name)                                               \
  SVTKM_FILTER_INSTANTIATE_EXECUTE_METHOD_WITH_POLICY(Name, svtkm::filter::PolicyDefault)

#include <svtkm/filter/Filter.hxx>
#endif
