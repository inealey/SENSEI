/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFitToHeightMapFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkFitToHeightMapFilter
 * @brief   adjust polydata to fit image height map
 *
 * svtkFitToHeightMapFilter "drapes" or "covers" a height map image by
 * determining new point coordinates of an input polydata by projecting (in
 * the z-direction) onto the height map. Different projection approaches can
 * be used including projecting points, or averaging / taking the minimum /
 * taking the maximum of the cell's points or sampled interior points.  The
 * filter passes the input to the output, however new new point coordinates
 * are generated, and point normals are not passed through. Note that the
 * draping supports verts, lines, polygons, and triangle strips.
 *
 * @warning
 * Since new point coordinates are generated, planar polygons may become
 * non-planar. To guarantee planarity, use a triangle mesh as input. Also
 * note that strategies based on averaging across a cell, or the points of a
 * cell, set the z-value for all of the cell's points to the same
 * value. However, as currently written, depending on the order in which
 * cells are processed, the last z-value set is the final value (since points
 * may be shared by multiple cells). Thus the filter works best with triangle
 * meshes, or if cells are topologically disconnected (i.e., points are only
 * used by one cell - use svtkShrinkFilter to topologically disconnect
 * the cells in a mesh).
 *
 * @warning
 * The point normals are not passed through to the output as the projection
 * process may distort the projected polydata surface.
 *
 * @warning
 * The interior cell sampling is currently carried out by triangulating the
 * cells, and then taking the centroid of each resulting triangle. Thus the
 * number of samples for each cell is (number of cell points - 2). Future
 * implementations may use a different sampling strategy.
 *
 * @warning
 * Points outside of the height map image are clamped to the boundary of the
 * height map. This may produce unexpected behavior in some cases.
 *
 * @warning
 * This class has been threaded with svtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * SVTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * svtkTrimmedExtrusionFilter svtkShrinkFilter svtkShrinkPolyData
 */

#ifndef svtkFitToHeightMapFilter_h
#define svtkFitToHeightMapFilter_h

#include "svtkFiltersModelingModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkImageData;

class SVTKFILTERSMODELING_EXPORT svtkFitToHeightMapFilter : public svtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods for construction, type and printing.
   */
  static svtkFitToHeightMapFilter* New();
  svtkTypeMacro(svtkFitToHeightMapFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Set the height map for the filter.  Note that this method does not
   * connect the pipeline. The algorithm will work on the input data as it is
   * without updating the producer of the data.  See SetHeightMapConnection()
   * for connecting the pipeline.
   */
  void SetHeightMapData(svtkImageData* idata);

  //@{
  /**
   * Specify the pipeline connection to the height map.
   */
  void SetHeightMapConnection(svtkAlgorithmOutput* algOutput);
  //@}

  //@{
  /**
   * Get a pointer to the height map.
   */
  svtkImageData* GetHeightMap();
  svtkImageData* GetHeightMap(svtkInformationVector* sourceInfo);
  //@}

  // Strategies to fit the polydata.
  enum FittingStrategy
  {
    POINT_PROJECTION = 0,
    POINT_MINIMUM_HEIGHT = 1,
    POINT_MAXIMUM_HEIGHT = 2,
    POINT_AVERAGE_HEIGHT = 3,
    CELL_MINIMUM_HEIGHT = 4,
    CELL_MAXIMUM_HEIGHT = 5,
    CELL_AVERAGE_HEIGHT = 6,
  };

  //@{
  /**
   * Specify a strategy for fitting, or projecting, the polydata to the
   * height field.  By default the points of the polydata are projected onto
   * the height field (POINT_PROJECTION).  However, to preserve planarity,
   * or to fit the polydata cells at the average/minimum/maximum height to
   * the height field, different strategies can be used (i.e., placing the
   * cells). The point-based strategies (e.g, POINT_MINIMUM_HEIGHT) set the
   * cell at the minimum height of the cell's points (and so on). The
   * cell-based strategies (e.g., CELL_MINIMUM_HEIGHT) sample the interior of
   * the cell and place the cell at the minimum height (and so on) of the
   * cell's sampled interior points.
   */
  svtkSetMacro(FittingStrategy, int);
  svtkGetMacro(FittingStrategy, int);
  void SetFittingStrategyToPointProjection() { this->SetFittingStrategy(POINT_PROJECTION); }
  void SetFittingStrategyToPointMinimumHeight() { this->SetFittingStrategy(POINT_MINIMUM_HEIGHT); }
  void SetFittingStrategyToPointMaximumHeight() { this->SetFittingStrategy(POINT_MAXIMUM_HEIGHT); }
  void SetFittingStrategyToAverageHeight() { this->SetFittingStrategy(POINT_AVERAGE_HEIGHT); }
  void SetFittingStrategyToCellMinimumHeight() { this->SetFittingStrategy(CELL_MINIMUM_HEIGHT); }
  void SetFittingStrategyToCellMaximumHeight() { this->SetFittingStrategy(CELL_MAXIMUM_HEIGHT); }
  void SetFittingStrategyToCellAverageHeight() { this->SetFittingStrategy(CELL_AVERAGE_HEIGHT); }
  //@}

  //@{
  /**
   * Indicate whether the z-offset from the image height map should be added
   * to the final result. Some height map images are offset in z-coordinate
   * which is independent of the height map values. By default this value is
   * true.
   */
  svtkSetMacro(UseHeightMapOffset, svtkTypeBool);
  svtkGetMacro(UseHeightMapOffset, svtkTypeBool);
  svtkBooleanMacro(UseHeightMapOffset, svtkTypeBool);
  //@}

protected:
  svtkFitToHeightMapFilter();
  ~svtkFitToHeightMapFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int, svtkInformation*) override;

  int FittingStrategy;
  svtkTypeBool UseHeightMapOffset;
  double Offset;

  void AdjustPoints(svtkPolyData* output, svtkIdType numCells, svtkPoints* newPts);
  void AdjustCells(
    svtkPolyData* output, svtkIdType numCells, double* cellHts, svtkPoints* inPts, svtkPoints* newPts);

private:
  svtkFitToHeightMapFilter(const svtkFitToHeightMapFilter&) = delete;
  void operator=(const svtkFitToHeightMapFilter&) = delete;
};

#endif
