/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContourLoopExtraction.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkContourLoopExtraction.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkExecutive.h"
#include "svtkGarbageCollector.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLine.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolygon.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <cfloat>
#include <vector>

svtkStandardNewMacro(svtkContourLoopExtraction);

//----------------------------------------------------------------------------
namespace
{
// Note on the definition of parametric coordinates: Given a sequence of
// lines segments (vi,vi+1) that form a primitive (e.g., polyline or
// polygon), the parametric coordinate t along the primitive is
// [i,i+1). Any point (like an intersection point on the segment) is i+t,
// where 0 <= t < 1.

// Infrastructure for cropping----------------------------------------------
struct LoopPoint
{
  double T; // parametric coordinate along linked lines
  svtkIdType Id;
  LoopPoint(double t, svtkIdType id)
    : T(t)
    , Id(id)
  {
  }
};

// Special sort operation on primitive parametric coordinate T-------------
bool PointSorter(LoopPoint const& lhs, LoopPoint const& rhs)
{
  return lhs.T < rhs.T;
}

// Vectors are used to hold points.
typedef std::vector<LoopPoint> LoopPointType;

// Update the scalar range-------------------------------------------------
void UpdateRange(svtkDataArray* scalars, svtkIdType pid, double range[2])
{
  if (!scalars)
  {
    return;
  }

  int numComp = scalars->GetNumberOfComponents();
  double s;
  for (int i = 0; i < numComp; ++i)
  {
    s = scalars->GetComponent(pid, i);
    range[0] = (s < range[0] ? s : range[0]);
    range[1] = (s > range[1] ? s : range[1]);
  }
}

// March along connected lines to the end----------------------------------
// pts[0] is assumed to be the starting point and already inserted.
svtkIdType TraverseLoop(double dir, svtkPolyData* polyData, svtkIdType lineId, svtkIdType start,
  LoopPointType& sortedPoints, char* visited, svtkDataArray* scalars, double range[2])
{
  svtkIdType last = start, numInserted = 0;
  double t = 0.0;
  bool terminated = false;
  svtkIdType ncells;
  svtkIdType npts;
  const svtkIdType* pts;
  svtkIdType* cells;
  svtkIdType nei;
  svtkIdType lastCell = lineId;
  polyData->GetCellPoints(lineId, npts, pts);

  // Recall that we are working with 2-pt lines
  while (!terminated)
  {
    last = (pts[0] != last ? pts[0] : pts[1]);
    numInserted++;
    t = dir * static_cast<double>(numInserted);
    sortedPoints.push_back(LoopPoint(t, last));
    UpdateRange(scalars, last, range);

    polyData->GetPointCells(last, ncells, cells);
    if (ncells == 1 || last == start) // this is the last point
    {
      return last;
    }
    else if (ncells == 2) // continue along loop
    {
      nei = (cells[0] != lastCell ? cells[0] : cells[1]);
      polyData->GetCellPoints(nei, npts, pts);
      visited[nei] = 1;
      lastCell = nei;
    }
    else // non-manifold, for now just quit (TODO: break apart loops)
    {
      terminated = true;
      break;
    }
  }

  return last;
}

// March along connected lines to the end----------------------------------
void OutputPolygon(LoopPointType& sortedPoints, svtkPoints* inPts, svtkCellArray* outLines,
  svtkCellArray* outPolys, int loopClosure)
{
  // Check to see that last point is the same as the first. Such a loop is
  // closed and can be directly output. Otherwise, check on the strategy
  // for closing the loop and close as appropriate.
  svtkIdType num = static_cast<svtkIdType>(sortedPoints.size());
  if (sortedPoints[0].Id == sortedPoints[num - 1].Id)
  {
    --num;
    sortedPoints.erase(sortedPoints.begin() + num);
  }

  else if (loopClosure == SVTK_LOOP_CLOSURE_ALL)
  {
    ; // do nothing and it will close between the first and last points
  }

  // If here we assume that the loop begins and ends on the given bounding
  // box (i.e. the boundary of the data). Close the loop by walking the
  // bounding box in the plane defined by the Normal plus the loop start
  // point.
  else if (loopClosure == SVTK_LOOP_CLOSURE_BOUNDARY)
  {
    // First check the simple case, complete the loop along horizontal or
    // vertical lines (assumed on the boundary).
    double p0[3], p1[3], delX, delY;
    inPts->GetPoint(sortedPoints[0].Id, p0);
    inPts->GetPoint(sortedPoints[num - 1].Id, p1);
    delX = fabs(p0[0] - p1[0]);
    delY = fabs(p0[1] - p1[1]);

    // if no change in either the x or y direction just return, the loop will complete
    if (delX < FLT_EPSILON || delY < FLT_EPSILON)
    {
      ; // do nothing loop will complete; points are along same (horizontal or vertical) boundary
        // edge
    }

    // Otherwise check if the points are on the "boundary" and then complete the loop
    // along the shortest path around the boundary.
    else
    {
      return;
    }
  }

  // Don't close
  else // loopClosure == SVTK_LOOP_CLOSURE_OFF
  {
    return;
  }

  // Return if not a valid loop
  if (num < 3)
  {
    return;
  }

  // If here can output the loop
  if (outLines)
  {
    outLines->InsertNextCell(num + 1);
    for (svtkIdType i = 0; i < num; ++i)
    {
      outLines->InsertCellPoint(sortedPoints[i].Id);
    }
    outLines->InsertCellPoint(sortedPoints[0].Id);
  }
  if (outPolys)
  {
    outPolys->InsertNextCell(num);
    for (svtkIdType i = 0; i < num; ++i)
    {
      outPolys->InsertCellPoint(sortedPoints[i].Id);
    }
  }
}

} // anonymous namespace

//----------------------------------------------------------------------------
// Instantiate object with empty loop.
svtkContourLoopExtraction::svtkContourLoopExtraction()
{
  this->LoopClosure = SVTK_LOOP_CLOSURE_BOUNDARY;
  this->ScalarThresholding = false;

  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = 1.0;

  this->Normal[0] = 0.0;
  this->Normal[1] = 0.0;
  this->Normal[2] = 1.0;

  this->OutputMode = SVTK_OUTPUT_POLYGONS;
}

//----------------------------------------------------------------------------
svtkContourLoopExtraction::~svtkContourLoopExtraction() = default;

//----------------------------------------------------------------------------
int svtkContourLoopExtraction::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Initialize and check data
  svtkDebugMacro(<< "Loop extraction...");

  svtkPoints* points = input->GetPoints();
  svtkIdType numPts;
  if (!points || (numPts = input->GetNumberOfPoints()) < 1)
  {
    svtkErrorMacro("Input contains no points");
    return 1;
  }

  svtkCellArray* lines = input->GetLines();
  svtkIdType numLines = lines->GetNumberOfCells();
  if (numLines < 1)
  {
    svtkErrorMacro("Input contains no lines");
    return 1;
  }

  svtkPointData* inPD = input->GetPointData();

  svtkDataArray* scalars = nullptr;
  if (this->ScalarThresholding)
  {
    scalars = inPD->GetScalars();
  }

  // Prepare output
  output->SetPoints(points);
  svtkCellArray *outLines = nullptr, *outPolys = nullptr;
  if (this->OutputMode == SVTK_OUTPUT_POLYLINES || this->OutputMode == SVTK_OUTPUT_BOTH)
  {
    outLines = svtkCellArray::New();
    output->SetLines(outLines);
  }
  if (this->OutputMode == SVTK_OUTPUT_POLYGONS || this->OutputMode == SVTK_OUTPUT_BOTH)
  {
    outPolys = svtkCellArray::New();
    output->SetPolys(outPolys);
  }
  output->GetPointData()->PassData(inPD);

  // Create a clean polydata containing only line segments and without other
  // topological types. This simplifies the filter.
  svtkIdType npts;
  const svtkIdType* pts;
  svtkIdType lineId;
  svtkCellArray* newLines = svtkCellArray::New();
  newLines->AllocateEstimate(numLines, 2);
  for (lineId = 0, lines->InitTraversal(); lines->GetNextCell(npts, pts); ++lineId)
  {
    for (int i = 0; i < (npts - 1); ++i)
    {
      newLines->InsertNextCell(2, pts + i);
    }
  }
  svtkPolyData* polyData = svtkPolyData::New();
  polyData->SetPoints(points);
  polyData->SetLines(newLines);
  polyData->GetPointData()->PassData(inPD);
  polyData->BuildLinks();

  // Keep track of what cells are visited
  numLines = newLines->GetNumberOfCells();
  char* visited = new char[numLines];
  std::fill_n(visited, numLines, 0);

  // Loop over all lines, visit each one. Build a loop from the seed line if
  // not visited.
  svtkIdType start, rightEnd;
  LoopPointType sortedPoints;
  double range[2];
  for (lineId = 0, newLines->InitTraversal(); newLines->GetNextCell(npts, pts); ++lineId)
  {
    if (!visited[lineId])
    {
      visited[lineId] = 1;
      start = pts[0];
      sortedPoints.clear();
      sortedPoints.push_back(LoopPoint(0.0, start));
      range[0] = SVTK_FLOAT_MAX;
      range[1] = SVTK_FLOAT_MIN;
      UpdateRange(scalars, start, range);

      rightEnd = TraverseLoop(1.0, polyData, lineId, start, sortedPoints, visited, scalars, range);

      if (rightEnd == start)
      {
        // extract loop, we've traversed all the way around
        if (!scalars || (range[0] <= this->ScalarRange[1] && range[1] >= this->ScalarRange[0]))
        {
          OutputPolygon(sortedPoints, points, outLines, outPolys, this->LoopClosure);
        }
      }
      else
      {
        // go the other direction and see where we end up
        TraverseLoop(-1.0, polyData, lineId, start, sortedPoints, visited, scalars, range);
        std::sort(sortedPoints.begin(), sortedPoints.end(), &PointSorter);
        OutputPolygon(sortedPoints, points, outLines, outPolys, this->LoopClosure);
      }
    } // if not visited start a loop
  }

  // Clean up
  newLines->Delete();
  if (outLines != nullptr)
  {
    svtkDebugMacro(<< "Generated " << outLines->GetNumberOfCells() << " lines\n");
    outLines->Delete();
  }
  if (outPolys != nullptr)
  {
    svtkDebugMacro(<< "Generated " << outPolys->GetNumberOfCells() << " polygons\n");
    outPolys->Delete();
  }
  polyData->Delete();
  delete[] visited;

  return 1;
}

//----------------------------------------------------------------------------
const char* svtkContourLoopExtraction::GetLoopClosureAsString()
{
  if (this->LoopClosure == SVTK_LOOP_CLOSURE_OFF)
  {
    return "LoopClosureOff";
  }
  else if (this->LoopClosure == SVTK_LOOP_CLOSURE_BOUNDARY)
  {
    return "LoopClosureBoundary";
  }
  else
  {
    return "LoopClosureAll";
  }
}

//----------------------------------------------------------------------------
const char* svtkContourLoopExtraction::GetOutputModeAsString()
{
  if (this->OutputMode == SVTK_OUTPUT_POLYGONS)
  {
    return "OutputModePolygons";
  }
  else if (this->OutputMode == SVTK_OUTPUT_POLYLINES)
  {
    return "OutputModePolylines";
  }
  else
  {
    return "OutputModeBoth";
  }
}

//----------------------------------------------------------------------------
void svtkContourLoopExtraction::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Loop Closure: ";
  os << this->GetLoopClosureAsString() << "\n";

  os << indent << "Scalar Thresholding: " << (this->ScalarThresholding ? "On\n" : "Off\n");

  double* range = this->GetScalarRange();
  os << indent << "Scalar Range: (" << range[0] << ", " << range[1] << ")\n";

  double* n = this->GetNormal();
  os << indent << "Normal: (" << n[0] << ", " << n[1] << ", " << n[2] << ")\n";

  os << indent << "Output Mode: ";
  os << this->GetOutputModeAsString() << "\n";
}
