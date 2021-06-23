/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLinearExtrusionFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLinearExtrusionFilter
 * @brief   sweep polygonal data creating a "skirt" from free edges and lines, and lines from
 * vertices
 *
 * svtkLinearExtrusionFilter is a modeling filter. It takes polygonal data as
 * input and generates polygonal data on output. The input dataset is swept
 * according to some extrusion function and creates new polygonal primitives.
 * These primitives form a "skirt" or swept surface. For example, sweeping a
 * line results in a quadrilateral, and sweeping a triangle creates a "wedge".
 *
 * There are a number of control parameters for this filter. You can
 * control whether the sweep of a 2D object (i.e., polygon or triangle strip)
 * is capped with the generating geometry via the "Capping" ivar. Also, you
 * can extrude in the direction of a user specified vector, towards a point,
 * or in the direction of vertex normals (normals must be provided - use
 * svtkPolyDataNormals if necessary). The amount of extrusion is controlled by
 * the "ScaleFactor" instance variable.
 *
 * The skirt is generated by locating certain topological features. Free
 * edges (edges of polygons or triangle strips only used by one polygon or
 * triangle strips) generate surfaces. This is true also of lines or
 * polylines. Vertices generate lines.
 *
 * This filter can be used to create 3D fonts, 3D irregular bar charts,
 * or to model 2 1/2D objects like punched plates. It also can be used to
 * create solid objects from 2D polygonal meshes.
 *
 * @warning
 * Some polygonal objects have no free edges (e.g., sphere). When swept,
 * this will result in two separate surfaces if capping is on, or no surface
 * if capping is off.
 *
 * @sa
 * svtkRotationalExtrusionFilter
 */

#ifndef svtkLinearExtrusionFilter_h
#define svtkLinearExtrusionFilter_h

#include "svtkFiltersModelingModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkDataArray;

#define SVTK_VECTOR_EXTRUSION 1
#define SVTK_NORMAL_EXTRUSION 2
#define SVTK_POINT_EXTRUSION 3

class SVTKFILTERSMODELING_EXPORT svtkLinearExtrusionFilter : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkLinearExtrusionFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Create object with normal extrusion type, capping on, scale factor=1.0,
   * vector (0,0,1), and extrusion point (0,0,0).
   */
  static svtkLinearExtrusionFilter* New();

  //@{
  /**
   * Set/Get the type of extrusion.
   */
  svtkSetClampMacro(ExtrusionType, int, SVTK_VECTOR_EXTRUSION, SVTK_POINT_EXTRUSION);
  svtkGetMacro(ExtrusionType, int);
  void SetExtrusionTypeToVectorExtrusion() { this->SetExtrusionType(SVTK_VECTOR_EXTRUSION); }
  void SetExtrusionTypeToNormalExtrusion() { this->SetExtrusionType(SVTK_NORMAL_EXTRUSION); }
  void SetExtrusionTypeToPointExtrusion() { this->SetExtrusionType(SVTK_POINT_EXTRUSION); }
  //@}

  //@{
  /**
   * Turn on/off the capping of the skirt.
   */
  svtkSetMacro(Capping, svtkTypeBool);
  svtkGetMacro(Capping, svtkTypeBool);
  svtkBooleanMacro(Capping, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get extrusion scale factor,
   */
  svtkSetMacro(ScaleFactor, double);
  svtkGetMacro(ScaleFactor, double);
  //@}

  //@{
  /**
   * Set/Get extrusion vector. Only needs to be set if VectorExtrusion is
   * turned on.
   */
  svtkSetVector3Macro(Vector, double);
  svtkGetVectorMacro(Vector, double, 3);
  //@}

  //@{
  /**
   * Set/Get extrusion point. Only needs to be set if PointExtrusion is
   * turned on. This is the point towards which extrusion occurs.
   */
  svtkSetVector3Macro(ExtrusionPoint, double);
  svtkGetVectorMacro(ExtrusionPoint, double, 3);
  //@}

protected:
  svtkLinearExtrusionFilter();
  ~svtkLinearExtrusionFilter() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int ExtrusionType;
  svtkTypeBool Capping;
  double ScaleFactor;
  double Vector[3];
  double ExtrusionPoint[3];

  void (svtkLinearExtrusionFilter::*ExtrudePoint)(double x[3], svtkIdType id, svtkDataArray* normals);
  void ViaNormal(double x[3], svtkIdType id, svtkDataArray* normals);
  void ViaVector(double x[3], svtkIdType id, svtkDataArray* normals = nullptr);
  void ViaPoint(double x[3], svtkIdType id, svtkDataArray* normals = nullptr);

private:
  svtkLinearExtrusionFilter(const svtkLinearExtrusionFilter&) = delete;
  void operator=(const svtkLinearExtrusionFilter&) = delete;
};

#endif
