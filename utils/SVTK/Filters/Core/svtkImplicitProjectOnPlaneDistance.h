/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImplicitProjectOnPlaneDistance.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImplicitProjectOnPlaneDistance
 *
 * This class recieve a plannar polygon as input. Given a point, it can
 * evaluate the L0 or L2 norm between the projection of this point on the plan
 * of the polygon and the polygon itself.
 *
 * An interesting use of this class is to enable the L0 norm and evaluate the
 * "projected distance" between every vertex of a mesh and the given plannar polygon.
 * As a reslut, all the vertices that project onto the polygon will corresond to the value 0
 * and ohter ones will recieve the value 1.
 * From there, we can use a clip to keep only the part of the mesh "below" the polygon.
 *
 * TLDR: This filter allows to clip using the extrusion of any plannar polygon.
 */

#ifndef svtkImplicitProjectOnPlaneDistance_h
#define svtkImplicitProjectOnPlaneDistance_h

#include "svtkImplicitFunction.h"

#include "svtkAbstractCellLocator.h" // User defined cellLocator
#include "svtkFiltersCoreModule.h"   // For export macro
#include "svtkSmartPointer.h"        // It has svtkSmartPointer fields

class svtkGenericCell;
class svtkPolyData;
class svtkPlane;

class SVTKFILTERSCORE_EXPORT svtkImplicitProjectOnPlaneDistance : public svtkImplicitFunction
{
public:
  enum class NormType
  {
    L0 = 0,
    L2 = 2
  };

  static svtkImplicitProjectOnPlaneDistance* New();
  svtkTypeMacro(svtkImplicitProjectOnPlaneDistance, svtkImplicitFunction);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Return the MTime also considering the Input dependency.
   */
  svtkMTimeType GetMTime() override;

  /**
   * Project x onto the plane defined by the Input polydata and evalute the
   * distance to the geometry defined by the Input polydata.
   */
  using svtkImplicitFunction::EvaluateFunction;
  double EvaluateFunction(double x[3]) override;

  /**
   * Evaluate function gradient of nearest triangle to point x[3].
   * WARNING: not implemented as it is of no use in this context.
   */
  void EvaluateGradient(double x[3], double g[3]) override;

  /**
   * Set the input svtkPolyData used for the implicit function
   * evaluation. This polydata needs to be planar.
   */
  void SetInput(svtkPolyData* input);

  //@{
  /**
   * Set/get the tolerance usued for the locator.
   * Default is 0.01.
   */
  svtkGetMacro(Tolerance, double);
  svtkSetMacro(Tolerance, double);
  //@}

  //@{
  /**
   * Set the norm to use:
   * L0: 0 when the projection is inside the input polygon, 1 otherwise
   * L2: Euclidean distance between the projection and the polygon (default)
   */
  NormType GetNorm() const { return Norm; }
  void SetNorm(NormType n)
  {
    Norm = n;
    Modified();
  }
#ifndef __SVTK_WRAP_JAVA__
  // The Java wrappers cannot resolve this signature from the one above,
  // see https://gitlab.kitware.com/svtk/svtk/issues/17744
  void SetNorm(int n)
  {
    Norm = static_cast<NormType>(n);
    Modified();
  }
#endif
  //@}

  //@{
  /**
   * Set/get the Locator used by to compute the distance.
   * A svtkStaticCellLocator is provided by default if
   * none is given by the user.
   */
  svtkGetSmartPointerMacro(Locator, svtkAbstractCellLocator);
  svtkSetSmartPointerMacro(Locator, svtkAbstractCellLocator);
  //@}

protected:
  svtkImplicitProjectOnPlaneDistance();
  ~svtkImplicitProjectOnPlaneDistance() override = default;

  /**
   * Create a default locator (svtkStaticCellLocator).
   * Used to create one when none is specified by the user.
   */
  void CreateDefaultLocator(void);

  double Tolerance;
  NormType Norm;

  svtkSmartPointer<svtkPolyData> Input;
  svtkSmartPointer<svtkAbstractCellLocator> Locator;
  svtkSmartPointer<svtkPlane> ProjectionPlane;

  // Stored here to avoid multiple allocation / dealloction
  svtkSmartPointer<svtkGenericCell> UnusedCell;
  double Bounds[6];

private:
  svtkImplicitProjectOnPlaneDistance(const svtkImplicitProjectOnPlaneDistance&) = delete;
  void operator=(const svtkImplicitProjectOnPlaneDistance&) = delete;
};

#endif
