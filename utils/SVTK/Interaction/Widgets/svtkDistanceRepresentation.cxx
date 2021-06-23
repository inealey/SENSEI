/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDistanceRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDistanceRepresentation.h"
#include "svtkBox.h"
#include "svtkCoordinate.h"
#include "svtkEventData.h"
#include "svtkHandleRepresentation.h"
#include "svtkInteractorObserver.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkWindow.h"

svtkCxxSetObjectMacro(svtkDistanceRepresentation, HandleRepresentation, svtkHandleRepresentation);

//----------------------------------------------------------------------
svtkDistanceRepresentation::svtkDistanceRepresentation()
{
  this->HandleRepresentation = nullptr;
  this->Point1Representation = nullptr;
  this->Point2Representation = nullptr;

  this->Tolerance = 5;
  this->Placed = 0;

  this->LabelFormat = new char[8];
  snprintf(this->LabelFormat, 8, "%s", "%-#6.3g");

  this->Scale = 1.0;
  this->RulerMode = 0;
  this->RulerDistance = 1.0;
  this->NumberOfRulerTicks = 5;
}

//----------------------------------------------------------------------
svtkDistanceRepresentation::~svtkDistanceRepresentation()
{
  if (this->HandleRepresentation)
  {
    this->HandleRepresentation->Delete();
  }
  if (this->Point1Representation)
  {
    this->Point1Representation->Delete();
  }
  if (this->Point2Representation)
  {
    this->Point2Representation->Delete();
  }

  delete[] this->LabelFormat;
  this->LabelFormat = nullptr;
}

//----------------------------------------------------------------------
void svtkDistanceRepresentation::InstantiateHandleRepresentation()
{
  if (!this->Point1Representation)
  {
    this->Point1Representation = this->HandleRepresentation->NewInstance();
    this->Point1Representation->ShallowCopy(this->HandleRepresentation);
  }

  if (!this->Point2Representation)
  {
    this->Point2Representation = this->HandleRepresentation->NewInstance();
    this->Point2Representation->ShallowCopy(this->HandleRepresentation);
  }
}

//----------------------------------------------------------------------
void svtkDistanceRepresentation::GetPoint1WorldPosition(double pos[3])
{
  if (this->Point1Representation)
  {
    this->Point1Representation->GetWorldPosition(pos);
  }
}

//----------------------------------------------------------------------
void svtkDistanceRepresentation::GetPoint2WorldPosition(double pos[3])
{
  if (this->Point2Representation)
  {
    this->Point2Representation->GetWorldPosition(pos);
  }
}

//----------------------------------------------------------------------
int svtkDistanceRepresentation::ComputeInteractionState(
  int svtkNotUsed(X), int svtkNotUsed(Y), int svtkNotUsed(modify))
{
  if (this->Point1Representation == nullptr || this->Point2Representation == nullptr)
  {
    this->InteractionState = svtkDistanceRepresentation::Outside;
    return this->InteractionState;
  }

  int h1State = this->Point1Representation->GetInteractionState();
  int h2State = this->Point2Representation->GetInteractionState();
  if (h1State == svtkHandleRepresentation::Nearby)
  {
    this->InteractionState = svtkDistanceRepresentation::NearP1;
  }
  else if (h2State == svtkHandleRepresentation::Nearby)
  {
    this->InteractionState = svtkDistanceRepresentation::NearP2;
  }
  else
  {
    this->InteractionState = svtkDistanceRepresentation::Outside;
  }

  return this->InteractionState;
}

int svtkDistanceRepresentation::ComputeComplexInteractionState(
  svtkRenderWindowInteractor*, svtkAbstractWidget*, unsigned long, void*, int)
{
  if (this->Point1Representation == nullptr || this->Point2Representation == nullptr)
  {
    this->InteractionState = svtkDistanceRepresentation::Outside;
    return this->InteractionState;
  }

  int h1State = this->Point1Representation->GetInteractionState();
  int h2State = this->Point2Representation->GetInteractionState();
  if (h1State == svtkHandleRepresentation::Nearby)
  {
    this->InteractionState = svtkDistanceRepresentation::NearP1;
  }
  else if (h2State == svtkHandleRepresentation::Nearby)
  {
    this->InteractionState = svtkDistanceRepresentation::NearP2;
  }
  else
  {
    this->InteractionState = svtkDistanceRepresentation::Outside;
  }

  return this->InteractionState;
}

//----------------------------------------------------------------------
void svtkDistanceRepresentation::StartWidgetInteraction(double e[2])
{
  double pos[3];
  pos[0] = e[0];
  pos[1] = e[1];
  pos[2] = 0.0;
  this->SetPoint1DisplayPosition(pos);
  this->SetPoint2DisplayPosition(pos);
}

void svtkDistanceRepresentation::StartComplexInteraction(
  svtkRenderWindowInteractor*, svtkAbstractWidget*, unsigned long, void* calldata)
{
  svtkEventData* edata = static_cast<svtkEventData*>(calldata);
  svtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (edd)
  {
    double pos[3];
    edd->GetWorldPosition(pos);
    this->SetPoint1WorldPosition(pos);
    this->SetPoint2WorldPosition(pos);
  }
}

//----------------------------------------------------------------------
void svtkDistanceRepresentation::WidgetInteraction(double e[2])
{
  double pos[3];
  pos[0] = e[0];
  pos[1] = e[1];
  pos[2] = 0.0;
  this->SetPoint2DisplayPosition(pos);
}
void svtkDistanceRepresentation::ComplexInteraction(
  svtkRenderWindowInteractor*, svtkAbstractWidget*, unsigned long, void* calldata)
{
  svtkEventData* edata = static_cast<svtkEventData*>(calldata);
  svtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (edd)
  {
    double pos[3];
    edd->GetWorldPosition(pos);
    this->SetPoint2WorldPosition(pos);
  }
}

//----------------------------------------------------------------------
void svtkDistanceRepresentation::BuildRepresentation()
{
  // Make sure that tolerance is consistent between handles and this representation
  if (this->Point1Representation)
  {
    this->Point1Representation->SetTolerance(this->Tolerance);
  }
  if (this->Point2Representation)
  {
    this->Point2Representation->SetTolerance(this->Tolerance);
  }
}

//----------------------------------------------------------------------
void svtkDistanceRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Distance: " << this->GetDistance() << "\n";
  os << indent << "Tolerance: " << this->Tolerance << "\n";
  os << indent << "Handle Representation: " << this->HandleRepresentation << "\n";

  os << indent << "Label Format: ";
  if (this->LabelFormat)
  {
    os << this->LabelFormat << "\n";
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << "Scale: " << this->GetScale() << "\n";
  os << indent << "Ruler Mode: " << (this->RulerMode ? "On" : "Off") << "\n";
  os << indent << "Ruler Distance: " << this->GetRulerDistance() << "\n";
  os << indent << "Number of Ruler Ticks: " << this->GetNumberOfRulerTicks() << "\n";

  os << indent << "Point1 Representation: ";
  if (this->Point1Representation)
  {
    this->Point1Representation->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << "Point2 Representation: ";
  if (this->Point2Representation)
  {
    this->Point2Representation->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }
}
