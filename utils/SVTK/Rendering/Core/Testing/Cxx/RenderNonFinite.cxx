/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SurfacePlusEdges.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// This test draws a stick with non-finite values.  The topology of the stick is
// as follows.
//
//  +---+  INF  Red
//  |   |
//  +---+  1.0  Red
//  |   |
//  +---+  0.5  Green
//  |   |
//  +---+  NAN  Magenta
//  |   |
//  +---+  0.5  Green
//  |   |
//  +---+  0.0  Blue
//  |   |
//  +---+  -INF Blue
//
// These values are mapped to the spectrum colors from blue (low) to red (high).
// -INF should be blue, INF should be red.  Since these are near extrema,
// whatever interpolation used should be constant.  NAN should be drawn as
// magenta.  The interpolation to NAN is ill defined in a texture map.  I would
// expect a sharp transition to the NAN color, but that might depend on graphics
// hardware.

#include "svtkActor.h"
#include "svtkCellArray.h"
#include "svtkColorTransferFunction.h"
#include "svtkDiscretizableColorTransferFunction.h"
#include "svtkDoubleArray.h"
#include "svtkLogLookupTable.h"
#include "svtkLookupTable.h"
#include "svtkMath.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, var) svtkSmartPointer<type> var = svtkSmartPointer<type>::New()

// Create the data described above.
static svtkSmartPointer<svtkPolyData> CreateData()
{
  const int cellsHigh = 6;
  const int pointsHigh = cellsHigh + 1;
  const double pointValues[pointsHigh] = { svtkMath::NegInf(), 0.0, 0.5, svtkMath::Nan(), 0.5, 1.0,
    svtkMath::Inf() };

  SVTK_CREATE(svtkPolyData, polyData);

  SVTK_CREATE(svtkPoints, points);
  for (int y = 0; y < pointsHigh; y++)
  {
    for (int x = 0; x < 2; x++)
    {
      points->InsertNextPoint(static_cast<double>(x), static_cast<double>(y), 0.0);
    }
  }
  polyData->SetPoints(points);

  SVTK_CREATE(svtkCellArray, cells);
  for (int c = 0; c < cellsHigh; c++)
  {
    cells->InsertNextCell(4);
    cells->InsertCellPoint(2 * c);
    cells->InsertCellPoint(2 * c + 1);
    cells->InsertCellPoint(2 * c + 3);
    cells->InsertCellPoint(2 * c + 2);
  }
  polyData->SetPolys(cells);

  SVTK_CREATE(svtkDoubleArray, scalars);
  for (int height = 0; height < pointsHigh; height++)
  {
    scalars->InsertNextTuple1(pointValues[height]);
    scalars->InsertNextTuple1(pointValues[height]);
  }
  polyData->GetPointData()->SetScalars(scalars);

  return polyData;
}

static svtkSmartPointer<svtkLookupTable> CreateLookupTable()
{
  SVTK_CREATE(svtkLookupTable, lut);

  lut->SetRampToLinear();
  lut->SetScaleToLinear();
  lut->SetTableRange(0.0, 1.0);
  lut->SetHueRange(0.6, 0.0);
  lut->SetNanColor(1.0, 0.0, 1.0, 1.0);

  return lut;
}

static svtkSmartPointer<svtkLogLookupTable> CreateLogLookupTable()
{
  SVTK_CREATE(svtkLogLookupTable, lut);

  lut->SetRampToLinear();
  lut->SetScaleToLinear();
  lut->SetTableRange(0.0, 1.0);
  lut->SetHueRange(0.6, 0.0);
  lut->SetNanColor(1.0, 0.0, 1.0, 1.0);

  return lut;
}

static svtkSmartPointer<svtkColorTransferFunction> CreateColorTransferFunction()
{
  SVTK_CREATE(svtkColorTransferFunction, ctf);

  ctf->SetColorSpaceToHSV();
  ctf->HSVWrapOff();
  ctf->AddHSVSegment(0.0, 0.6, 1.0, 1.0, 1.0, 0.0, 1.0, 1.0);
  ctf->SetNanColor(1.0, 0.0, 1.0);

  return ctf;
}

static svtkSmartPointer<svtkDiscretizableColorTransferFunction>
CreateDiscretizableColorTransferFunction()
{
  SVTK_CREATE(svtkDiscretizableColorTransferFunction, ctf);

  ctf->DiscretizeOn();
  ctf->SetColorSpaceToHSV();
  ctf->HSVWrapOff();
  ctf->AddHSVSegment(0.0, 0.6, 1.0, 1.0, 1.0, 0.0, 1.0, 1.0);
  ctf->SetNanColor(1.0, 0.0, 1.0);
  ctf->Build();

  return ctf;
}

static svtkSmartPointer<svtkRenderer> CreateRenderer(
  svtkPolyData* input, svtkScalarsToColors* lut, int interpolate)
{
  SVTK_CREATE(svtkPolyDataMapper, mapper);
  mapper->SetInputData(input);
  mapper->SetLookupTable(lut);
  mapper->SetInterpolateScalarsBeforeMapping(interpolate);

  SVTK_CREATE(svtkActor, actor);
  actor->SetMapper(mapper);

  SVTK_CREATE(svtkRenderer, renderer);
  renderer->AddActor(actor);
  renderer->ResetCamera();

  return renderer;
}

const int NUM_RENDERERS = 8;
static void AddRenderer(svtkRenderer* renderer, svtkRenderWindow* renwin)
{
  static int rencount = 0;
  renderer->SetViewport(static_cast<double>(rencount) / NUM_RENDERERS, 0.0,
    static_cast<double>(rencount + 1) / NUM_RENDERERS, 1.0);
  renwin->AddRenderer(renderer);
  rencount++;
}

int RenderNonFinite(int argc, char* argv[])
{
  svtkSmartPointer<svtkPolyData> input = CreateData();

  SVTK_CREATE(svtkRenderWindow, renwin);
  renwin->SetSize(300, 200);

  svtkSmartPointer<svtkRenderer> renderer;

  renderer = CreateRenderer(input, CreateLookupTable(), 0);
  AddRenderer(renderer, renwin);

  renderer = CreateRenderer(input, CreateLookupTable(), 1);
  AddRenderer(renderer, renwin);

  renderer = CreateRenderer(input, CreateLogLookupTable(), 0);
  AddRenderer(renderer, renwin);

  renderer = CreateRenderer(input, CreateLogLookupTable(), 1);
  AddRenderer(renderer, renwin);

  renderer = CreateRenderer(input, CreateColorTransferFunction(), 0);
  AddRenderer(renderer, renwin);

  renderer = CreateRenderer(input, CreateColorTransferFunction(), 1);
  AddRenderer(renderer, renwin);

  renderer = CreateRenderer(input, CreateDiscretizableColorTransferFunction(), 0);
  AddRenderer(renderer, renwin);

  renderer = CreateRenderer(input, CreateDiscretizableColorTransferFunction(), 1);
  AddRenderer(renderer, renwin);

  renwin->Render();

  int retVal = svtkRegressionTestImage(renwin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    SVTK_CREATE(svtkRenderWindowInteractor, iren);
    iren->SetRenderWindow(renwin);
    iren->Initialize();
    iren->Start();
    retVal = svtkRegressionTester::PASSED;
  }

  return (retVal == svtkRegressionTester::PASSED) ? 0 : 1;
}
