/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMRCReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkNew.h"
#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkCamera.h"
#include "svtkImageData.h"
#include "svtkImageMathematics.h"
#include "svtkImageProperty.h"
#include "svtkImageSlice.h"
#include "svtkImageSliceMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

#include "svtkMRCReader.h"

#include <string>

static const char* dispfile = "Data/mrc/emd_1056.mrc";

static void TestDisplay(svtkRenderWindow* renwin, const char* infile)
{
  svtkNew<svtkMRCReader> reader;

  reader->SetFileName(infile);
  reader->Update();

  int size[3];
  double center[3], spacing[3];
  reader->GetOutput()->GetDimensions(size);
  reader->GetOutput()->GetCenter(center);
  reader->GetOutput()->GetSpacing(spacing);
  double center1[3] = { center[0], center[1], center[2] };
  if (size[2] % 2 == 1)
  {
    center1[2] += 0.5 * spacing[2];
  }
  double vrange[2];
  reader->GetOutput()->GetScalarRange(vrange);

  svtkNew<svtkImageSliceMapper> map1;
  map1->BorderOn();
  map1->SliceAtFocalPointOn();
  map1->SliceFacesCameraOn();
  map1->SetInputConnection(reader->GetOutputPort());

  svtkNew<svtkImageSlice> slice1;
  slice1->SetMapper(map1);
  slice1->GetProperty()->SetColorWindow(vrange[1] - vrange[0]);
  slice1->GetProperty()->SetColorLevel(0.5 * (vrange[0] + vrange[1]));

  svtkNew<svtkRenderer> ren1;
  ren1->SetViewport(0, 0, 1.0, 1.0);

  ren1->AddViewProp(slice1);

  svtkCamera* cam1 = ren1->GetActiveCamera();
  cam1->ParallelProjectionOn();
  cam1->SetParallelScale(0.5 * spacing[1] * size[1]);
  cam1->SetFocalPoint(center1[0], center1[1], center1[2]);
  cam1->SetPosition(center1[0], center1[1], center1[2] - 100.0);

  renwin->SetSize(size[0], size[1]);
  renwin->AddRenderer(ren1);
};

int TestMRCReader(int argc, char* argv[])
{
  // perform the display test
  char* infile = svtkTestUtilities::ExpandDataFileName(argc, argv, dispfile);
  if (!infile)
  {
    cerr << "Could not locate input file " << dispfile << "\n";
    return 1;
  }
  std::string inpath = infile;
  delete[] infile;

  svtkNew<svtkRenderWindow> renwin;
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renwin);

  TestDisplay(renwin, inpath.c_str());

  int retVal = svtkRegressionTestImage(renwin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    renwin->Render();
    iren->Start();
    retVal = svtkRegressionTester::PASSED;
  }

  return (retVal != svtkRegressionTester::PASSED);
}
