/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCubeAxes3.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkBYUReader.h"
#include "svtkCamera.h"
#include "svtkCubeAxesActor.h"
#include "svtkLODActor.h"
#include "svtkLight.h"
#include "svtkNew.h"
#include "svtkOutlineFilter.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyDataNormals.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTestUtilities.h"
#include "svtkTextProperty.h"

//----------------------------------------------------------------------------
int TestCubeAxesSticky(int argc, char* argv[])
{
  svtkNew<svtkBYUReader> fohe;
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/teapot.g");
  fohe->SetGeometryFileName(fname);
  delete[] fname;

  svtkNew<svtkPolyDataNormals> normals;
  normals->SetInputConnection(fohe->GetOutputPort());

  svtkNew<svtkPolyDataMapper> foheMapper;
  foheMapper->SetInputConnection(normals->GetOutputPort());

  svtkNew<svtkLODActor> foheActor;
  foheActor->SetMapper(foheMapper);
  foheActor->GetProperty()->SetDiffuseColor(0.7, 0.3, 0.0);

  svtkNew<svtkOutlineFilter> outline;
  outline->SetInputConnection(normals->GetOutputPort());

  svtkNew<svtkPolyDataMapper> mapOutline;
  mapOutline->SetInputConnection(outline->GetOutputPort());

  svtkNew<svtkActor> outlineActor;
  outlineActor->SetMapper(mapOutline);
  outlineActor->GetProperty()->SetColor(0.0, 0.0, 0.0);

  svtkNew<svtkCamera> camera;
  camera->SetClippingRange(1.0, 100.0);
  double xShift = -5;
  double yShift = -1;
  double zShift = 1;
  camera->SetFocalPoint(0.9 + xShift, 1.0 + yShift, 0.0 + zShift);
  camera->SetPosition(8.63 + xShift, 6.0 + yShift, 3.77 + zShift);

  svtkNew<svtkLight> light;
  light->SetFocalPoint(0.21406, 1.5, 0.0);
  light->SetPosition(8.3761, 4.94858, 4.12505);

  svtkNew<svtkRenderer> ren2;
  ren2->SetActiveCamera(camera);
  ren2->AddLight(light);

  svtkNew<svtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(ren2);
  renWin->SetWindowName("Cube Axes");
  renWin->SetSize(800, 400);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  ren2->AddViewProp(foheActor);
  ren2->AddViewProp(outlineActor);
  ren2->SetBackground(0.1, 0.2, 0.4);

  normals->Update();

  svtkNew<svtkCubeAxesActor> axes;
  axes->SetBounds(normals->GetOutput()->GetBounds());
  axes->SetXAxisRange(20, 300);
  axes->SetYAxisRange(-.01, .01);
  axes->SetCamera(ren2->GetActiveCamera());
  axes->SetXLabelFormat("%6.1f");
  axes->SetYLabelFormat("%6.1f");
  axes->SetZLabelFormat("%6.1f");
  axes->SetScreenSize(15.);
  axes->SetFlyModeToClosestTriad();
  axes->SetCornerOffset(.0);
  axes->SetStickyAxes(1);
  axes->SetCenterStickyAxes(0);

  // Use red color for X axis
  axes->GetXAxesLinesProperty()->SetColor(1., 0., 0.);
  axes->GetTitleTextProperty(0)->SetColor(1., 0., 0.);
  axes->GetLabelTextProperty(0)->SetColor(.8, 0., 0.);

  // Use green color for Y axis
  axes->GetYAxesLinesProperty()->SetColor(0., 1., 0.);
  axes->GetTitleTextProperty(1)->SetColor(0., 1., 0.);
  axes->GetLabelTextProperty(1)->SetColor(0., .8, 0.);

  ren2->AddViewProp(axes);
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
