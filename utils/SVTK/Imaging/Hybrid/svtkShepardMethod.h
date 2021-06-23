/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkShepardMethod.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkShepardMethod
 * @brief   interpolate points and associated scalars onto volume
 * using the method of Shepard
 *
 *
 * svtkShepardMethod is a filter used to interpolate point scalar values using
 * Shepard's method. The method works by resampling the scalars associated
 * with points defined on an arbitrary dataset onto a volume (i.e.,
 * structured points) dataset. The influence functions are described as
 * "inverse distance weighted". Once the interpolation is performed across
 * the volume, the usual volume visualization techniques (e.g.,
 * iso-contouring or volume rendering) can be used.
 *
 * Note that this implementation also provides the ability to specify the
 * power parameter p. Given the generalized Inverse Distance Weighting (IDW)
 * function with distance between points measured as d(x,xi), p is defined
 * as:
 * <pre>
 * u(x) = Sum(wi(x) * ui) / Sum(wi(x)) if d(x,xi) != 0
 * u(x) = ui                           if d(x,xi) == 0
 *
 * where wi(x) = 1 / (d(x,xi)^p
 * </pre>
 * Typically p=2, so the weights wi(x) are the inverse of the distance
 * squared. However, power parameters > 2 can be used which assign higher
 * weights for data closer to the interpolated point; or <2 which assigns
 * greater weight to points further away. (Note that if p!=2, performance may
 * be significantly impacted as the algorithm is tuned for p=2.)
 *
 * @warning
 * Strictly speaking, this is a modified Shepard's methodsince only points
 * within the MaxiumDistance are used for interpolation. By setting the
 * maximum distance to include the entire bounding box and therefore all
 * points, the class executes much slower but incorporates all points into
 * the interpolation process (i.e., a pure Shepard method).
 *
 * @warning
 * The input to this filter is any dataset type. This filter can be used to
 * resample the points of any type of dataset onto the output volume; i.e.,
 * the input data need not be unstructured with explicit point
 * representations.
 *
 * @warning
 * The bounds of the data (i.e., the sample space) is automatically computed
 * if not set by the user.
 *
 * @warning
 * If you use a maximum distance less than 1.0 (i.e., using a modified
 * Shephard's method), some output points may never receive a
 * contribution. The final value of these points can be specified with the
 * "NullValue" instance variable.
 *
 * @warning
 * This class has been threaded with svtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * SVTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * svtkGaussianSplatter svtkCheckerboardSplatter
 */

#ifndef svtkShepardMethod_h
#define svtkShepardMethod_h

#include "svtkImageAlgorithm.h"
#include "svtkImagingHybridModule.h" // For export macro

class SVTKIMAGINGHYBRID_EXPORT svtkShepardMethod : public svtkImageAlgorithm
{
public:
  //@{
  /**
   * Standard type and print methods.
   */
  svtkTypeMacro(svtkShepardMethod, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Construct with sample dimensions=(50,50,50) and so that model bounds are
   * automatically computed from the input. The null value for each unvisited
   * output point is 0.0. Maximum distance is 0.25. Power parameter p=2.
   */
  static svtkShepardMethod* New();

  /**
   * Set the i-j-k dimensions on which to interpolate the input points.
   */
  void SetSampleDimensions(int i, int j, int k);

  /**
   * Set the i-j-k dimensions on which to sample the input points.
   */
  void SetSampleDimensions(int dim[3]);

  //@{
  /**
   * Retrieve the i-j-k dimensions on which to interpolate the input points.
   */
  svtkGetVectorMacro(SampleDimensions, int, 3);
  //@}

  //@{
  /**
   * Specify the maximum influence distance of each input point. This
   * distance is a fraction of the length of the diagonal of the sample
   * space. Thus, values of 1.0 will cause each input point to influence all
   * points in the volume dataset. Values less than 1.0 can improve
   * performance significantly. By default the maximum distance is 0.25.
   */
  svtkSetClampMacro(MaximumDistance, double, 0.0, 1.0);
  svtkGetMacro(MaximumDistance, double);
  //@}

  //@{
  /**
   * Set the value for output points not receiving a contribution from any
   * input point(s). Output points may not receive a contribution when the
   * MaximumDistance < 1.
   */
  svtkSetMacro(NullValue, double);
  svtkGetMacro(NullValue, double);
  //@}

  //@{
  /**
   * Specify the position in space to perform the sampling. The ModelBounds
   * and SampleDimensions together define the output volume. (Note: if the
   * ModelBounds are set to an invalid state [zero or negative volume] then
   * the bounds are computed automatically.)
   */
  svtkSetVector6Macro(ModelBounds, double);
  svtkGetVectorMacro(ModelBounds, double, 6);
  //@}

  //@{
  /**
   * Set / Get the power parameter p. By default p=2. Values (which must be
   * a positive, real value) != 2 may affect performance significantly.
   */
  svtkSetClampMacro(PowerParameter, double, 0.001, 100);
  svtkGetMacro(PowerParameter, double);
  //@}

  /**
   * Compute ModelBounds from the input geometry.
   */
  double ComputeModelBounds(double origin[3], double ar[3]);

protected:
  svtkShepardMethod();
  ~svtkShepardMethod() override {}

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  // see svtkAlgorithm for details
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  // see algorithm for more info
  int FillInputPortInformation(int port, svtkInformation* info) override;

  int SampleDimensions[3];
  double MaximumDistance;
  double ModelBounds[6];
  double NullValue;
  double PowerParameter;

private:
  svtkShepardMethod(const svtkShepardMethod&) = delete;
  void operator=(const svtkShepardMethod&) = delete;
};

#endif
