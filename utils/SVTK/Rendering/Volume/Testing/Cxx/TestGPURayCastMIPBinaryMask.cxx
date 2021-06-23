/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastCompositeMask.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkGPUVolumeRayCastMapper.h"
#include "svtkImageData.h"
#include "svtkPiecewiseFunction.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTestUtilities.h"
#include "svtkTesting.h"
#include "svtkVolume.h"
#include "svtkVolume16Reader.h"
#include "svtkVolumeProperty.h"

int TestGPURayCastMIPBinaryMask(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  svtkSmartPointer<svtkVolume16Reader> reader = svtkSmartPointer<svtkVolume16Reader>::New();
  reader->SetDataDimensions(64, 64);
  reader->SetDataByteOrderToLittleEndian();
  reader->SetImageRange(1, 93);
  reader->SetDataSpacing(3.2, 3.2, 1.5);
  reader->SetFilePrefix(fname);
  reader->SetDataMask(0x7fff);
  reader->Update();

  delete[] fname;

  svtkImageData* input = reader->GetOutput();

  int dim[3];
  double spacing[3], center[3], origin[3];
  input->GetSpacing(spacing);
  input->GetDimensions(dim);
  input->GetCenter(center);
  input->GetOrigin(origin);

  svtkSmartPointer<svtkGPUVolumeRayCastMapper> mapper =
    svtkSmartPointer<svtkGPUVolumeRayCastMapper>::New();
  svtkSmartPointer<svtkVolume> volume = svtkSmartPointer<svtkVolume>::New();
  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->SetMaskTypeToBinary();
  mapper->SetAutoAdjustSampleDistances(0);

  // assume the scalar field is a set of samples taken from a
  // contiguous band-limited volumetric field.
  // assume max frequency is present:
  // min spacing divided by 2. Nyquist-Shannon theorem says so.
  // sample distance could be bigger if we compute the actual max frequency
  // in the data.
  double distance = spacing[0];
  if (distance > spacing[1])
  {
    distance = spacing[1];
  }
  if (distance > spacing[2])
  {
    distance = spacing[2];
  }
  distance = distance / 2.0;

  // This does not take the screen size of a cell into account.
  // distance has to be smaller: min(nyquis,screensize)

  mapper->SetSampleDistance(static_cast<float>(distance));

  svtkSmartPointer<svtkColorTransferFunction> colorFun =
    svtkSmartPointer<svtkColorTransferFunction>::New();
  svtkSmartPointer<svtkPiecewiseFunction> opacityFun = svtkSmartPointer<svtkPiecewiseFunction>::New();

  // Create the property and attach the transfer functions
  svtkSmartPointer<svtkVolumeProperty> property = svtkSmartPointer<svtkVolumeProperty>::New();
  property->SetIndependentComponents(true);
  property->SetColor(colorFun);
  property->SetScalarOpacity(opacityFun);
  property->SetInterpolationTypeToLinear();

  // connect up the volume to the property and the mapper
  volume->SetProperty(property);
  volume->SetMapper(mapper);

  colorFun->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
  colorFun->AddRGBPoint(4095.0, 1, 1, 1);

  opacityFun->AddPoint(0.0, 0.0);
  opacityFun->AddPoint(4095.0, 1.0);

  mapper->SetBlendModeToMaximumIntensity();

  // Make the mask
  svtkSmartPointer<svtkImageData> mask = svtkSmartPointer<svtkImageData>::New();
  mask->SetExtent(input->GetExtent());
  mask->SetSpacing(input->GetSpacing());
  mask->SetOrigin(input->GetOrigin());
  mask->AllocateScalars(SVTK_UNSIGNED_CHAR, 1);

  // Create a simple mask that's split along the X axis
  unsigned char* ptr = static_cast<unsigned char*>(mask->GetScalarPointer());
  const double radiusSq = 70 * 70; // 7cm spherical mask
  center[0] -= origin[0];
  center[1] -= origin[1];
  center[2] -= origin[2];
  for (int z = 0; z < dim[2]; z++)
  {
    for (int y = 0; y < dim[1]; y++)
    {
      for (int x = 0; x < dim[0]; x++)
      {
        const double distanceSq =
          ((double)x * spacing[0] - center[0]) * ((double)x * spacing[0] - center[0]) +
          ((double)y * spacing[1] - center[1]) * ((double)y * spacing[1] - center[1]) +
          ((double)z * spacing[2] - center[2]) * ((double)z * spacing[2] - center[2]);
        *ptr = (distanceSq < radiusSq ? 255 : 0);
        ++ptr;
      }
    }
  }

  mapper->SetMaskInput(mask);

  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->SetSize(300, 300);
  iren->SetRenderWindow(renWin);

  svtkSmartPointer<svtkRenderer> ren = svtkSmartPointer<svtkRenderer>::New();
  renWin->AddRenderer(ren);
  renWin->Render();

  if (!mapper->IsRenderSupported(renWin, property))
  {
    cout << "Required extensions not supported." << endl;
    return EXIT_SUCCESS;
  }

  ren->AddViewProp(volume);
  iren->Initialize();
  ren->GetActiveCamera()->SetPosition(-484.648, 261.986, 144.52);
  ren->GetActiveCamera()->SetViewUp(-0.078112, 0.176042, -0.981279);
  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.5);
  renWin->Render();

  return svtkTesting::InteractorEventLoop(argc, argv, iren);
}
