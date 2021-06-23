/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBridgePointIteratorOne.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME svtkBridgePointIteratorOne - Implementation of svtkGenericPointIterator.
// .SECTION Description
// It is just an example that show how to implement the Generic. It is also
// used for testing and evaluating the Generic.
// .SECTION See Also
// svtkGenericPointIterator, svtkBridgeDataSet

#include "svtkBridgePointIteratorOne.h"

#include <cassert>

#include "svtkBridgeDataSet.h"
#include "svtkDataSet.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkBridgePointIteratorOne);

//-----------------------------------------------------------------------------
// Description:
// Default constructor.
svtkBridgePointIteratorOne::svtkBridgePointIteratorOne()
{
  this->DataSet = nullptr;
  this->cIsAtEnd = 1;
}

//-----------------------------------------------------------------------------
// Description:
// Destructor.
svtkBridgePointIteratorOne::~svtkBridgePointIteratorOne()
{
  svtkSetObjectBodyMacro(DataSet, svtkBridgeDataSet, 0);
}

//-----------------------------------------------------------------------------
void svtkBridgePointIteratorOne::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
// Description:
// Move iterator to first position if any (loop initialization).
void svtkBridgePointIteratorOne::Begin()
{
  this->cIsAtEnd = 0;
}

//-----------------------------------------------------------------------------
// Description:
// Is there no point at iterator position? (exit condition).
svtkTypeBool svtkBridgePointIteratorOne::IsAtEnd()
{
  return this->cIsAtEnd;
}

//-----------------------------------------------------------------------------
// Description:
// Move iterator to next position. (loop progression).
// \pre not_off: !IsAtEnd()
void svtkBridgePointIteratorOne::Next()
{
  assert("pre: not_off" && !IsAtEnd());
  this->cIsAtEnd = 1;
}

//-----------------------------------------------------------------------------
// Description:
// Point at iterator position.
// \pre not_off: !IsAtEnd()
// \post result_exists: result!=0
double* svtkBridgePointIteratorOne::GetPosition()
{
  assert("pre: not_off" && !IsAtEnd());

  double* result = this->DataSet->Implementation->GetPoint(this->Id);

  assert("post: result_exists" && result != nullptr);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Point at iterator position.
// \pre not_off: !IsAtEnd()
// \pre x_exists: x!=0
void svtkBridgePointIteratorOne::GetPosition(double x[3])
{
  assert("pre: not_off" && !IsAtEnd());
  assert("pre: x_exists" && x != nullptr);
  this->DataSet->Implementation->GetPoint(this->Id, x);
}

//-----------------------------------------------------------------------------
// Description:
// Unique identifier for the point, could be non-contiguous
// \pre not_off: !IsAtEnd()
svtkIdType svtkBridgePointIteratorOne::GetId()
{
  assert("pre: not_off" && !IsAtEnd());

  return this->Id;
}

//-----------------------------------------------------------------------------
// Description:
// Used internally by svtkBridgeDataSet.
// Iterate over one point of identifier `id' on dataset `ds'.
// \pre ds_can_be_null: ds!=0 || ds==0
// \pre valid_id: svtkImplies(ds!=0,(id>=0)&&(id<=ds->GetNumberOfCells()))
void svtkBridgePointIteratorOne::InitWithOnePoint(svtkBridgeDataSet* ds, svtkIdType id)
{
  assert("pre: valid_id" &&
    ((ds == nullptr) || ((id >= 0) && (id <= ds->GetNumberOfCells())))); // A=>B: !A||B

  svtkSetObjectBodyMacro(DataSet, svtkBridgeDataSet, ds);
  this->Id = id;
}
