/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkImageMapper.h"

#include "svtkActor2D.h"
#include "svtkExecutive.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkViewport.h"
#include "svtkWindow.h"

//----------------------------------------------------------------------------
// Return nullptr if no override is supplied.
svtkAbstractObjectFactoryNewMacro(svtkImageMapper);

//----------------------------------------------------------------------------

svtkImageMapper::svtkImageMapper()
{
  svtkDebugMacro(<< "svtkImageMapper::svtkImageMapper");

  this->ColorWindow = 2000;
  this->ColorLevel = 1000;

  this->DisplayExtent[0] = this->DisplayExtent[1] = 0;
  this->DisplayExtent[2] = this->DisplayExtent[3] = 0;
  this->DisplayExtent[4] = this->DisplayExtent[5] = 0;
  this->ZSlice = 0;

  this->RenderToRectangle = 0;
  this->UseCustomExtents = 0;
  this->CustomDisplayExtents[0] = this->CustomDisplayExtents[1] = 0;
  this->CustomDisplayExtents[2] = this->CustomDisplayExtents[3] = 0;
}

svtkImageMapper::~svtkImageMapper() = default;

//----------------------------------------------------------------------------
void svtkImageMapper::SetInputData(svtkImageData* input)
{
  this->SetInputDataInternal(0, input);
}

//----------------------------------------------------------------------------
svtkImageData* svtkImageMapper::GetInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    return nullptr;
  }
  return svtkImageData::SafeDownCast(this->GetExecutive()->GetInputData(0, 0));
}

svtkMTimeType svtkImageMapper::GetMTime()
{
  svtkMTimeType mTime = this->svtkMapper2D::GetMTime();
  return mTime;
}

void svtkImageMapper::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Color Window: " << this->ColorWindow << "\n";
  os << indent << "Color Level: " << this->ColorLevel << "\n";
  os << indent << "ZSlice: " << this->ZSlice << "\n";
  os << indent << "RenderToRectangle: " << this->RenderToRectangle << "\n";
  os << indent << "UseCustomExtents: " << this->UseCustomExtents << "\n";
  os << indent << "CustomDisplayExtents: " << this->CustomDisplayExtents[0] << " "
     << this->CustomDisplayExtents[1] << " " << this->CustomDisplayExtents[2] << " "
     << this->CustomDisplayExtents[3] << "\n";
  //
}

double svtkImageMapper::GetColorShift()
{
  return this->ColorWindow / 2.0 - this->ColorLevel;
}

double svtkImageMapper::GetColorScale()
{
  return 255.0 / this->ColorWindow;
}

void svtkImageMapper::RenderStart(svtkViewport* viewport, svtkActor2D* actor)
{
  svtkDebugMacro(<< "svtkImageMapper::RenderOverlay");

  svtkImageData* data;

  if (!viewport)
  {
    svtkErrorMacro(<< "svtkImageMapper::Render - Null viewport argument");
    return;
  }

  if (!actor)
  {
    svtkErrorMacro(<< "svtkImageMapper::Render - Null actor argument");
    return;
  }

  if (!this->GetInputAlgorithm())
  {
    svtkDebugMacro(<< "svtkImageMapper::Render - Please Set the input.");
    return;
  }

  this->GetInputAlgorithm()->UpdateInformation();

  svtkInformation* inInfo = this->GetInputInformation();

  if (!this->UseCustomExtents)
  {
    // start with the wholeExtent
    int wholeExtent[6];
    inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);
    inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->DisplayExtent);
    // Set The z values to the zslice
    this->DisplayExtent[4] = this->ZSlice;
    this->DisplayExtent[5] = this->ZSlice;

    // scale currently not handled
    // double *scale = actor->GetScale();

    // get the position
    int* pos = actor->GetActualPositionCoordinate()->GetComputedViewportValue(viewport);

    // Get the viewport coordinates
    double vCoords[4];
    vCoords[0] = 0.0;
    vCoords[1] = 0.0;
    vCoords[2] = 1.0;
    vCoords[3] = 1.0;
    viewport->NormalizedViewportToViewport(vCoords[0], vCoords[1]);
    viewport->NormalizedViewportToViewport(vCoords[2], vCoords[3]);
    const int* vSize = viewport->GetSize();

    // the basic formula is that the draw pos equals
    // the pos + extentPos + clippedAmount
    // The concrete subclass will get the pos in display
    // coordinates so we need to provide the extentPos plus
    // clippedAmount in the PositionAdjustment variable

    // Now clip to imager extents
    if (pos[0] + wholeExtent[0] < 0)
    {
      this->DisplayExtent[0] = -pos[0];
    }
    if ((pos[0] + wholeExtent[1]) > vSize[0])
    {
      this->DisplayExtent[1] = vSize[0] - pos[0];
    }
    if (pos[1] + wholeExtent[2] < 0)
    {
      this->DisplayExtent[2] = -pos[1];
    }
    if ((pos[1] + wholeExtent[3]) > vSize[1])
    {
      this->DisplayExtent[3] = vSize[1] - pos[1];
    }

    // check for the condition where no pixels are visible.
    if (this->DisplayExtent[0] > wholeExtent[1] || this->DisplayExtent[1] < wholeExtent[0] ||
      this->DisplayExtent[2] > wholeExtent[3] || this->DisplayExtent[3] < wholeExtent[2] ||
      this->DisplayExtent[4] > wholeExtent[5] || this->DisplayExtent[5] < wholeExtent[4])
    {
      return;
    }

    this->GetInputAlgorithm()->UpdateExtent(this->DisplayExtent);

    // set the position adjustment
    this->PositionAdjustment[0] = this->DisplayExtent[0];
    this->PositionAdjustment[1] = this->DisplayExtent[2];
  }
  else // UseCustomExtents
  {
    this->DisplayExtent[0] = this->CustomDisplayExtents[0];
    this->DisplayExtent[1] = this->CustomDisplayExtents[1];
    this->DisplayExtent[2] = this->CustomDisplayExtents[2];
    this->DisplayExtent[3] = this->CustomDisplayExtents[3];
    this->DisplayExtent[4] = this->ZSlice;
    this->DisplayExtent[5] = this->ZSlice;
    //
    // clear the position adjustment
    this->PositionAdjustment[0] = 0;
    this->PositionAdjustment[1] = 0;
    this->GetInputAlgorithm()->UpdateWholeExtent();
  }

  // Get the region from the input
  data = this->GetInput();
  if (!data)
  {
    svtkErrorMacro(<< "Render: Could not get data from input.");
    return;
  }

  this->RenderData(viewport, data, actor);
}

//----------------------------------------------------------------------------
int svtkImageMapper::GetWholeZMin()
{
  int* extent;

  if (!this->GetInput())
  {
    return 0;
  }
  this->GetInputAlgorithm()->UpdateInformation();
  extent = this->GetInputInformation()->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  return extent[4];
}

//----------------------------------------------------------------------------
int svtkImageMapper::GetWholeZMax()
{
  int* extent;

  if (!this->GetInput())
  {
    return 0;
  }
  this->GetInputAlgorithm()->UpdateInformation();
  extent = this->GetInputInformation()->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  return extent[5];
}

//----------------------------------------------------------------------------
int svtkImageMapper::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  return 1;
}
