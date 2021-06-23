/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCellPickerImage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests picking of images with svtkCellPicker
//
// The command line arguments are:
// -I        => run in interactive mode

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkCamera.h"
#include "svtkCellPicker.h"
#include "svtkConeSource.h"
#include "svtkDataSetMapper.h"
#include "svtkImageData.h"
#include "svtkImageProperty.h"
#include "svtkImageReader2.h"
#include "svtkImageResliceMapper.h"
#include "svtkImageSlice.h"
#include "svtkImageSliceMapper.h"
#include "svtkInteractorStyleImage.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

// A function to point an actor along a vector
void PointCone(svtkActor* actor, double nx, double ny, double nz)
{
  if (nx < 0.0)
  {
    actor->RotateWXYZ(180, 0, 1, 0);
    actor->RotateWXYZ(180, (nx - 1.0) * 0.5, ny * 0.5, nz * 0.5);
  }
  else
  {
    actor->RotateWXYZ(180, (nx + 1.0) * 0.5, ny * 0.5, nz * 0.5);
  }
}

int TestCellPickerImage(int argc, char* argv[])
{
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  svtkInteractorStyle* style = svtkInteractorStyleImage::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->SetMultiSamples(0);
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);
  renWin->Delete();
  style->Delete();

  svtkImageReader2* reader = svtkImageReader2::New();
  reader->SetDataByteOrderToLittleEndian();
  reader->SetDataExtent(0, 63, 0, 63, 1, 93);
  // use negative spacing to strengthen the testing
  reader->SetDataSpacing(3.2, 3.2, -1.5);
  // a nice random-ish origin for testing
  reader->SetDataOrigin(2.5, -13.6, 2.8);

  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  reader->SetFilePrefix(fname);
  reader->Update();
  delete[] fname;

  svtkRenderer* renderers[4];
  for (int i = 0; i < 4; i++)
  {
    svtkRenderer* renderer = svtkRenderer::New();
    renderers[i] = renderer;
    svtkCamera* camera = renderer->GetActiveCamera();
    renderer->SetBackground(0.1, 0.2, 0.4);
    renderer->SetViewport(0.5 * (i & 1), 0.25 * (i & 2), 0.5 + 0.5 * (i & 1), 0.5 + 0.25 * (i & 2));
    renWin->AddRenderer(renderer);
    renderer->Delete();

    svtkImageSliceMapper* imageMapper = svtkImageSliceMapper::New();
    imageMapper->SetInputConnection(reader->GetOutputPort());
    imageMapper->SliceAtFocalPointOn();

    const double* bounds = imageMapper->GetBounds();
    double point[3];
    point[0] = 0.5 * (bounds[0] + bounds[1]);
    point[1] = 0.5 * (bounds[2] + bounds[3]);
    point[2] = 0.5 * (bounds[4] + bounds[5]);

    if (i < 3)
    {
      imageMapper->SetOrientation(i);
    }

    point[imageMapper->GetOrientation()] += 30.0;
    camera->SetFocalPoint(point);
    point[imageMapper->GetOrientation()] += 470.0;
    camera->SetPosition(point);
    camera->SetClippingRange(250, 750);
    camera->ParallelProjectionOn();
    camera->SetParallelScale(120.0);
    if (imageMapper->GetOrientation() != 2)
    {
      camera->SetViewUp(0.0, 0.0, 1.0);
    }

    if (i == 3)
    {
      camera->Azimuth(30);
      camera->Elevation(40);
    }

    svtkImageSlice* image = svtkImageSlice::New();
    image->SetMapper(imageMapper);
    imageMapper->Delete();
    renderer->AddViewProp(image);

    image->GetProperty()->SetColorWindow(2000);
    image->GetProperty()->SetColorLevel(1000);

    image->Delete();
  }

  renWin->SetSize(400, 400);
  renWin->Render();

  // a cone source that points along the z axis
  svtkConeSource* coneSource = svtkConeSource::New();
  coneSource->CappingOn();
  coneSource->SetHeight(24);
  coneSource->SetRadius(8);
  coneSource->SetResolution(31);
  coneSource->SetCenter(12, 0, 0);
  coneSource->SetDirection(-1, 0, 0);

  svtkCellPicker* picker = svtkCellPicker::New();
  picker->SetTolerance(1e-6);

  const static int pickpos[4][2] = { { 120, 90 }, { 278, 99 }, { 90, 310 }, { 250, 260 } };

  bool pickSuccess = true;
  for (int i = 0; i < 4; i++)
  {
    svtkRenderer* renderer = renderers[i];

    // Pick the image
    picker->Pick(pickpos[i][0], pickpos[i][1], 0.0, renderer);
    double p[3], n[3];
    picker->GetPickPosition(p);
    picker->GetPickNormal(n);
    if (svtkImageSlice::SafeDownCast(picker->GetProp3D()) == nullptr)
    {
      cerr << "Pick did not get an image.\n";
      pickSuccess = false;
    }
    if (svtkImageSliceMapper::SafeDownCast(picker->GetMapper()) == nullptr)
    {
      cerr << "Pick did not get a mapper.\n";
      pickSuccess = false;
    }

    // Draw a cone where the pick occurred
    svtkActor* coneActor = svtkActor::New();
    coneActor->PickableOff();
    svtkDataSetMapper* coneMapper = svtkDataSetMapper::New();
    coneMapper->SetInputConnection(coneSource->GetOutputPort());
    coneActor->SetMapper(coneMapper);
    coneActor->GetProperty()->SetColor(1, 0, 0);
    coneActor->SetPosition(p[0], p[1], p[2]);
    PointCone(coneActor, n[0], n[1], n[2]);
    renderer->AddViewProp(coneActor);
    coneMapper->Delete();
    coneActor->Delete();
  }

  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  iren->Delete();

  coneSource->Delete();
  picker->Delete();
  reader->Delete();

  return (!retVal || !pickSuccess);
}
