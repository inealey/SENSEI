/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkActor.h"
#include "svtkCellArray.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTrivialProducer.h"

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

//----------------------------------------------------------------------------
int TestVBOPointsLines(int, char*[])
{
  // Initialize everything
  svtkNew<svtkActor> actor;
  svtkNew<svtkPolyDataMapper> mapper;
  svtkNew<svtkRenderer> renderer;
  renderer->SetBackground(0.0, 0.0, 0.0);
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->AddRenderer(renderer);
  renderer->AddActor(actor);

  // Basic polydata lines, triangles, points...
  svtkNew<svtkPolyData> polydata;
  svtkNew<svtkPoints> points;
  points->SetNumberOfPoints(7);
  points->SetPoint(0, 0, 0, 0);
  points->SetPoint(1, 1, 0, 0);
  points->SetPoint(2, 0, 1, 0);
  points->SetPoint(3, 1, 1, -1);
  points->SetPoint(4, 1, 2, 1);
  points->SetPoint(5, 4, 1, -9);
  points->SetPoint(6, 3, -2, 1);

  svtkNew<svtkCellArray> verts;
  verts->InsertNextCell(1);
  verts->InsertCellPoint(0);
  verts->InsertNextCell(1);
  verts->InsertCellPoint(1);
  verts->InsertNextCell(1);
  verts->InsertCellPoint(5);
  verts->InsertNextCell(1);
  verts->InsertCellPoint(6);

  svtkNew<svtkCellArray> lines;
  lines->InsertNextCell(2);
  lines->InsertCellPoint(2);
  lines->InsertCellPoint(3);
  lines->InsertNextCell(2);
  lines->InsertCellPoint(0);
  lines->InsertCellPoint(4);

  // Try inserting a polyline now...
  lines->InsertNextCell(4);
  lines->InsertCellPoint(0);
  lines->InsertCellPoint(2);
  lines->InsertCellPoint(3);
  lines->InsertCellPoint(4);

  svtkNew<svtkCellArray> tris;
  tris->InsertNextCell(3);
  tris->InsertCellPoint(0);
  tris->InsertCellPoint(2);
  tris->InsertCellPoint(3);

  polydata->SetPoints(points);
  polydata->SetVerts(verts);
  polydata->SetLines(lines);
  polydata->SetPolys(tris);

  svtkNew<svtkTrivialProducer> prod;
  prod->SetOutput(polydata);

  // Set some properties.
  mapper->SetInputConnection(prod->GetOutputPort());
  actor->GetProperty()->SetPointSize(5);
  actor->GetProperty()->SetLineWidth(2);
  actor->GetProperty()->SetDiffuseColor(1.0, 0.0, 0.0);
  actor->SetMapper(mapper);
  actor->GetProperty()->SetAmbientColor(0.2, 0.2, 1.0);
  actor->GetProperty()->SetDiffuseColor(1.0, 0.65, 0.7);
  actor->GetProperty()->SetSpecularColor(1.0, 1.0, 1.0);
  actor->GetProperty()->SetSpecular(0.5);
  actor->GetProperty()->SetDiffuse(0.7);
  actor->GetProperty()->SetAmbient(0.5);
  actor->GetProperty()->SetSpecularPower(20.0);
  actor->GetProperty()->SetOpacity(1.0);

  // Start.
  svtkNew<svtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow);
  renderWindow->SetMultiSamples(0);
  interactor->Initialize();

  // Ensure we can change properties between renders and trigger updates in the
  // mapper.
  renderWindow->Render();
  actor->GetProperty()->SetPointSize(2.0);
  renderWindow->Render();

  interactor->Start();

  return EXIT_SUCCESS;
}
