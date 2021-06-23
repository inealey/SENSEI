/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWeightedTransformFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkWeightedTransformFilter.h"

#include "svtkCellData.h"
#include "svtkFieldData.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLinearTransform.h"
#include "svtkMath.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkUnsignedShortArray.h"

#include <vector>

svtkStandardNewMacro(svtkWeightedTransformFilter);

// helper functions.  Can't easily get to these in Matrix4x4 as written.

static inline void LinearTransformVector(double matrix[4][4], double in[3], double out[3])
{
  out[0] = matrix[0][0] * in[0] + matrix[0][1] * in[1] + matrix[0][2] * in[2];
  out[1] = matrix[1][0] * in[0] + matrix[1][1] * in[1] + matrix[1][2] * in[2];
  out[2] = matrix[2][0] * in[0] + matrix[2][1] * in[1] + matrix[2][2] * in[2];
}

static inline void LinearTransformPoint(double mtx[4][4], double in[3], double out[3])
{
  out[0] = mtx[0][0] * in[0] + mtx[0][1] * in[1] + mtx[0][2] * in[2] + mtx[0][3];
  out[1] = mtx[1][0] * in[0] + mtx[1][1] * in[1] + mtx[1][2] * in[2] + mtx[1][3];
  out[2] = mtx[2][0] * in[0] + mtx[2][1] * in[1] + mtx[2][2] * in[2] + mtx[2][3];
}

//----------------------------------------------------------------------------
svtkWeightedTransformFilter::svtkWeightedTransformFilter()
{
  this->AddInputValues = 0;
  this->Transforms = nullptr;
  this->NumberOfTransforms = 0;

  this->CellDataWeightArray = nullptr;
  this->WeightArray = nullptr;
  this->CellDataTransformIndexArray = nullptr;
  this->TransformIndexArray = nullptr;
}

//----------------------------------------------------------------------------
svtkWeightedTransformFilter::~svtkWeightedTransformFilter()
{
  int i;

  if (this->Transforms != nullptr)
  {
    for (i = 0; i < this->NumberOfTransforms; i++)
    {
      if (this->Transforms[i] != nullptr)
      {
        this->Transforms[i]->UnRegister(this);
      }
    }
    delete[] this->Transforms;
  }

  // Setting these strings to nullptr has the side-effect of deleting them.
  this->SetCellDataWeightArray(nullptr);
  this->SetWeightArray(nullptr);
  this->SetCellDataTransformIndexArray(nullptr);
  this->SetTransformIndexArray(nullptr);
}

//----------------------------------------------------------------------------
void svtkWeightedTransformFilter::SetNumberOfTransforms(int num)
{
  int i;
  svtkAbstractTransform** newTransforms;

  if (num < 0)
  {
    svtkErrorMacro(<< "Cannot set transform count below zero");
    return;
  }

  if (this->Transforms == nullptr)
  {
    // first time
    this->Transforms = new svtkAbstractTransform*[num];
    for (i = 0; i < num; i++)
    {
      this->Transforms[i] = nullptr;
    }
    this->NumberOfTransforms = num;
    return;
  }

  if (num == this->NumberOfTransforms)
  {
    return;
  }

  if (num < this->NumberOfTransforms)
  {
    // create a smaller array, free up references to cut-off elements,
    // and copy other elements
    for (i = num; i < this->NumberOfTransforms; i++)
    {
      if (this->Transforms[i] != nullptr)
      {
        this->Transforms[i]->UnRegister(this);
        this->Transforms[i] = nullptr;
      }
    }
    newTransforms = new svtkAbstractTransform*[num];
    for (i = 0; i < num; i++)
    {
      newTransforms[i] = this->Transforms[i];
    }
    delete[] this->Transforms;
    this->Transforms = newTransforms;
  }
  else
  {
    // create a new array and copy elements, no unregistering needed.
    newTransforms = new svtkAbstractTransform*[num];
    for (i = 0; i < this->NumberOfTransforms; i++)
    {
      newTransforms[i] = this->Transforms[i];
    }

    for (i = this->NumberOfTransforms; i < num; i++)
    {
      newTransforms[i] = nullptr;
    }
    delete[] this->Transforms;
    this->Transforms = newTransforms;
  }

  this->NumberOfTransforms = num;
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkWeightedTransformFilter::SetTransform(svtkAbstractTransform* trans, int num)
{
  if (num < 0)
  {
    svtkErrorMacro(<< "Transform number must be greater than 0");
    return;
  }

  if (num >= this->NumberOfTransforms)
  {
    svtkErrorMacro(<< "Transform number exceeds maximum of " << this->NumberOfTransforms);
    return;
  }
  if (this->Transforms[num] != nullptr)
  {
    this->Transforms[num]->UnRegister(this);
  }
  this->Transforms[num] = trans;
  if (trans != nullptr)
  {
    trans->Register(this);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
svtkAbstractTransform* svtkWeightedTransformFilter::GetTransform(int num)
{
  if (num < 0)
  {
    svtkErrorMacro(<< "Transform number must be greater than 0");
    return nullptr;
  }

  if (num >= this->NumberOfTransforms)
  {
    svtkErrorMacro(<< "Transform number exceeds maximum of " << this->NumberOfTransforms);
    return nullptr;
  }

  return this->Transforms[num];
}

//----------------------------------------------------------------------------
int svtkWeightedTransformFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPointSet* input = svtkPointSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPointSet* output = svtkPointSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkPoints* inPts;
  svtkPoints* newPts;
  svtkDataArray *inVectors, *inCellVectors;
  svtkDataArray *inNormals, *inCellNormals;
  svtkFloatArray *newVectors = nullptr, *newCellVectors = nullptr;
  svtkFloatArray *newNormals = nullptr, *newCellNormals = nullptr;
  svtkIdType numPts, numCells, p;
  int activeTransforms, allLinear;
  int i, c, tidx;
  int pdComponents, cdComponents;
  double inVec[3], inPt[3], inNorm[3];
  double xformNorm[3], cumNorm[3];
  double xformPt[3], cumPt[3];
  double xformVec[3], cumVec[3];
  double derivMatrix[3][3];
  float* weights = nullptr;
  unsigned short* transformIndices = nullptr;
  double thisWeight;
  svtkDataArray *pdArray, *cdArray;
  svtkUnsignedShortArray *tiArray, *cdtiArray;
  svtkFieldData* fd;
  svtkPointData *pd = input->GetPointData(), *outPD = output->GetPointData();
  svtkCellData *cd = input->GetCellData(), *outCD = output->GetCellData();
  svtkLinearTransform* linearTransform;

  svtkDebugMacro(<< "Executing weighted transform filter");

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  // Check input
  //
  if (this->Transforms == nullptr || this->NumberOfTransforms == 0)
  {
    svtkErrorMacro(<< "No transform defined!");
    return 1;
  }

  activeTransforms = 0;
  for (c = 0; c < this->NumberOfTransforms; c++)
  {
    if (this->Transforms[c] != nullptr)
    {
      activeTransforms++;
    }
  }

  if (activeTransforms == 0)
  {
    svtkErrorMacro(<< "No transform defined!");
    return 1;
  }

  std::vector<double*> linearPtMtx(this->NumberOfTransforms, nullptr);       // non-owning ptr
  std::vector<std::vector<double> > linearNormMtx(this->NumberOfTransforms); // owns data
  allLinear = 1;
  for (c = 0; c < this->NumberOfTransforms; c++)
  {
    if (this->Transforms[c] == nullptr)
    {
      continue;
    }

    this->Transforms[c]->Update();

    if (!this->Transforms[c]->IsA("svtkLinearTransform"))
    {
      allLinear = 0;
      continue;
    }
    linearTransform = svtkLinearTransform::SafeDownCast(this->Transforms[c]);
    linearPtMtx[c] = linearTransform->GetMatrix()->GetData();
    linearNormMtx[c].resize(16);
    svtkMatrix4x4::DeepCopy(linearNormMtx[c].data(), linearTransform->GetMatrix());
    svtkMatrix4x4::Invert(linearNormMtx[c].data(), linearNormMtx[c].data());
    svtkMatrix4x4::Transpose(linearNormMtx[c].data(), linearNormMtx[c].data());
  }

  pdArray = nullptr;
  pdComponents = 0;
  if (this->WeightArray != nullptr && this->WeightArray[0] != '\0')
  {
    fd = pd;
    if (fd != nullptr)
    {
      pdArray = fd->GetArray(this->WeightArray);
    }
    if (pdArray == nullptr)
    {
      fd = input->GetFieldData();
      if (fd != nullptr)
      {
        pdArray = fd->GetArray(this->WeightArray);
      }
    }
    if (pdArray == nullptr)
    {
      svtkErrorMacro(<< "WeightArray " << this->WeightArray << " "
                    << "doesn't exist");
      return 1;
    }

    pdComponents = pdArray->GetNumberOfComponents();
    if (pdComponents > this->NumberOfTransforms)
    {
      pdComponents = this->NumberOfTransforms;
    }
  }

  tiArray = nullptr;
  if (this->TransformIndexArray != nullptr && this->TransformIndexArray[0] != '\0')
  {
    fd = pd;
    if (fd != nullptr)
    {
      tiArray = reinterpret_cast<svtkUnsignedShortArray*>(fd->GetArray(this->TransformIndexArray));
    }
    if (tiArray == nullptr)
    {
      fd = input->GetFieldData();
      if (fd != nullptr)
      {
        tiArray = reinterpret_cast<svtkUnsignedShortArray*>(fd->GetArray(this->TransformIndexArray));
      }
    }
    if (tiArray == nullptr)
    {
      svtkErrorMacro(<< "TransformIndexArray " << this->TransformIndexArray << " "
                    << "doesn't exist");
      return 1;
    }

    if (pdComponents != tiArray->GetNumberOfComponents())
    {
      svtkWarningMacro(<< "TransformIndexArray " << this->TransformIndexArray << " "
                      << "does not have the same number of components as WeightArray "
                      << this->WeightArray);
      tiArray = nullptr;
    }
    if (tiArray->GetDataType() != SVTK_UNSIGNED_SHORT)
    {
      svtkWarningMacro(<< "TransformIndexArray " << this->TransformIndexArray << " "
                      << " is not of type unsigned short, ignoring.");
      tiArray = nullptr;
    }
  }

  cdArray = nullptr;
  cdComponents = 0;
  if (this->CellDataWeightArray != nullptr && this->CellDataWeightArray[0] != '\0')
  {
    fd = cd;
    if (fd != nullptr)
    {
      cdArray = fd->GetArray(this->CellDataWeightArray);
    }
    if (cdArray == nullptr)
    {
      fd = input->GetFieldData();
      if (fd != nullptr)
      {
        cdArray = fd->GetArray(this->CellDataWeightArray);
      }
    }
    if (cdArray == nullptr)
    {
      svtkErrorMacro(<< "CellDataWeightArray " << this->CellDataWeightArray << " "
                    << "doesn't exist");
      return 1;
    }
    cdComponents = cdArray->GetNumberOfComponents();
    if (cdComponents > this->NumberOfTransforms)
    {
      cdComponents = this->NumberOfTransforms;
    }
  }

  cdtiArray = nullptr;
  if (this->CellDataTransformIndexArray != nullptr && this->CellDataTransformIndexArray[0] != '\0')
  {
    fd = pd;
    if (fd != nullptr)
    {
      cdtiArray =
        reinterpret_cast<svtkUnsignedShortArray*>(fd->GetArray(this->CellDataTransformIndexArray));
    }
    if (cdtiArray == nullptr)
    {
      fd = input->GetFieldData();
      if (fd != nullptr)
      {
        cdtiArray =
          reinterpret_cast<svtkUnsignedShortArray*>(fd->GetArray(this->CellDataTransformIndexArray));
      }
    }
    if (cdtiArray == nullptr)
    {
      svtkErrorMacro(<< "CellDataTransformIndexArray " << this->CellDataTransformIndexArray << " "
                    << "doesn't exist");
      return 1;
    }

    if (cdComponents != cdtiArray->GetNumberOfComponents())
    {
      svtkWarningMacro(<< "CellDataTransformIndexArray " << this->CellDataTransformIndexArray << " "
                      << "does not have the same number of components as "
                      << "CellDataWeightArray " << this->WeightArray);
      cdtiArray = nullptr;
    }
    if (cdtiArray && (cdtiArray->GetDataType() != SVTK_UNSIGNED_SHORT))
    {
      svtkWarningMacro(<< "CellDataTransformIndexArray " << this->CellDataTransformIndexArray << " "
                      << " is not of type unsigned short, ignoring.");
      cdtiArray = nullptr;
    }
  }

  inPts = input->GetPoints();
  inVectors = pd->GetVectors();
  inNormals = pd->GetNormals();
  inCellVectors = cd->GetVectors();
  inCellNormals = cd->GetNormals();

  if (!inPts)
  {
    svtkErrorMacro(<< "No input data");
    return 1;
  }

  numPts = inPts->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();

  newPts = svtkPoints::New();
  newPts->Allocate(numPts);
  if (inVectors)
  {
    newVectors = svtkFloatArray::New();
    newVectors->SetNumberOfComponents(3);
    newVectors->Allocate(3 * numPts);
  }
  if (inNormals)
  {
    newNormals = svtkFloatArray::New();
    newNormals->SetNumberOfComponents(3);
    newNormals->Allocate(3 * numPts);
  }

  this->UpdateProgress(.2);
  // Loop over all points, updating position
  //

  // since we may be doing multiple transforms, we must duplicate
  // work done in svtkTransform

  // -------------------------- POINT DATA -------------------------------
  if (pdArray != nullptr)
  {
    transformIndices = nullptr;
    // do points
    for (p = 0; p < numPts; p++)
    {
      // -------- points init ---------------
      inPts->GetPoint(p, inPt);
      if (this->AddInputValues)
      {
        cumPt[0] = inPt[0];
        cumPt[1] = inPt[1];
        cumPt[2] = inPt[2];
      }
      else
      {
        cumPt[0] = 0.0;
        cumPt[1] = 0.0;
        cumPt[2] = 0.0;
      }
      // -------- vectors init ---------------
      if (inVectors)
      {
        inVectors->GetTuple(p, inVec);
        if (this->AddInputValues)
        {
          cumVec[0] = inVec[0];
          cumVec[1] = inVec[1];
          cumVec[2] = inVec[2];
        }
        else
        {
          cumVec[0] = 0.0;
          cumVec[1] = 0.0;
          cumVec[2] = 0.0;
        }
      }
      // -------- normals init ---------------
      if (inNormals)
      {
        inNormals->GetTuple(p, inNorm);
        if (this->AddInputValues)
        {
          cumNorm[0] = inNorm[0];
          cumNorm[1] = inNorm[1];
          cumNorm[2] = inNorm[2];
        }
        else
        {
          cumNorm[0] = 0.0;
          cumNorm[1] = 0.0;
          cumNorm[2] = 0.0;
        }
      }

      weights = reinterpret_cast<svtkFloatArray*>(pdArray)->GetPointer(p * pdComponents);

      if (tiArray != nullptr)
      {
        transformIndices =
          reinterpret_cast<svtkUnsignedShortArray*>(tiArray)->GetPointer(p * pdComponents);
      }

      // for each transform...
      for (c = 0; c < pdComponents; c++)
      {
        if (transformIndices != nullptr)
        {
          tidx = transformIndices[c];
        }
        else
        {
          tidx = c;
        }
        if (tidx >= this->NumberOfTransforms || tidx < 0)
        {
          svtkWarningMacro(<< "transform index " << tidx << " outside valid range, ignoring");
          continue;
        }
        thisWeight = weights[c];
        if (this->Transforms[tidx] == nullptr || thisWeight == 0.0)
        {
          continue;
        }

        if (linearPtMtx[tidx] != nullptr)
        {
          // -------------------- linear fast path ------------------------
          LinearTransformPoint((double(*)[4])linearPtMtx[tidx], inPt, xformPt);

          if (inVectors)
          {
            LinearTransformVector((double(*)[4])linearPtMtx[tidx], inVec, xformVec);
          }

          if (inNormals)
          {
            LinearTransformVector((double(*)[4])linearNormMtx[tidx].data(), inNorm, xformNorm);
            // normalize below
          }
        }
        else
        {
          // -------------------- general, slow path ------------------------
          this->Transforms[tidx]->InternalTransformDerivative(inPt, xformPt, derivMatrix);
          if (inVectors)
          {
            svtkMath::Multiply3x3(derivMatrix, inVec, xformVec);
          }

          if (inNormals)
          {
            svtkMath::Transpose3x3(derivMatrix, derivMatrix);
            svtkMath::LinearSolve3x3(derivMatrix, inNorm, xformNorm);
            // normalize below
          }
        }

        // ------ accumulate the results into respective tuples -------
        cumPt[0] += xformPt[0] * thisWeight;
        cumPt[1] += xformPt[1] * thisWeight;
        cumPt[2] += xformPt[2] * thisWeight;

        if (inVectors)
        {
          cumVec[0] += xformVec[0] * thisWeight;
          cumVec[1] += xformVec[1] * thisWeight;
          cumVec[2] += xformVec[2] * thisWeight;
        }

        if (inNormals)
        {
          svtkMath::Normalize(xformNorm);
          cumNorm[0] += xformNorm[0] * thisWeight;
          cumNorm[1] += xformNorm[1] * thisWeight;
          cumNorm[2] += xformNorm[2] * thisWeight;
        }
      }

      // assign components
      newPts->InsertNextPoint(cumPt);

      if (inVectors)
      {
        newVectors->InsertNextTuple(cumVec);
      }

      if (inNormals)
      {
        // normalize normal again
        svtkMath::Normalize(cumNorm);
        newNormals->InsertNextTuple(cumNorm);
      }
    }
  }

  this->UpdateProgress(.6);

  // -------------------------- CELL DATA -------------------------------

  // can only work on cell data if the transforms are all linear
  if (cdArray != nullptr && allLinear)
  {
    if (inCellVectors)
    {
      newCellVectors = svtkFloatArray::New();
      newCellVectors->SetNumberOfComponents(3);
      newCellVectors->Allocate(3 * numCells);
    }
    if (inCellNormals)
    {
      newCellNormals = svtkFloatArray::New();
      newCellNormals->SetNumberOfComponents(3);
      newCellNormals->Allocate(3 * numCells);
    }
    transformIndices = nullptr;
    for (p = 0; p < numCells; p++)
    {
      // -------- normals init ---------------
      if (inCellNormals)
      {
        inCellNormals->GetTuple(p, inNorm);
        if (this->AddInputValues)
        {
          for (i = 0; i < 3; i++)
          {
            cumNorm[i] = inNorm[i];
          }
        }
        else
        {
          for (i = 0; i < 3; i++)
          {
            cumNorm[i] = 0.0;
          }
        }
      }
      // -------- vectors init ---------------
      if (inVectors)
      {
        inVectors->GetTuple(p, inVec);
        if (this->AddInputValues)
        {
          for (i = 0; i < 3; i++)
          {
            cumVec[i] = inVec[i];
          }
        }
        else
        {
          for (i = 0; i < 3; i++)
          {
            cumVec[i] = 0.0;
          }
        }
      }

      weights = reinterpret_cast<svtkFloatArray*>(cdArray)->GetPointer(p * cdComponents);
      if (cdtiArray != nullptr)
      {
        transformIndices =
          reinterpret_cast<svtkUnsignedShortArray*>(cdtiArray)->GetPointer(p * cdComponents);
      }

      // for each transform...
      for (c = 0; c < cdComponents; c++)
      {
        if (transformIndices != nullptr)
        {
          tidx = transformIndices[c];
        }
        else
        {
          tidx = c;
        }
        if (tidx >= this->NumberOfTransforms || tidx < 0)
        {
          svtkWarningMacro(<< "transform index " << tidx << " outside valid range, ignoring");
          continue;
        }
        thisWeight = weights[c];
        if (linearPtMtx[tidx] == nullptr || thisWeight == 0.0)
        {
          continue;
        }

        if (inCellNormals)
        {
          LinearTransformVector((double(*)[4])linearNormMtx[tidx].data(), inNorm, xformNorm);

          svtkMath::Normalize(xformNorm);
          cumNorm[0] += xformNorm[0] * thisWeight;
          cumNorm[1] += xformNorm[1] * thisWeight;
          cumNorm[2] += xformNorm[2] * thisWeight;
        }

        if (inVectors)
        {
          LinearTransformVector((double(*)[4])linearPtMtx[tidx], inVec, xformVec);
          cumVec[0] += xformVec[0] * thisWeight;
          cumVec[1] += xformVec[1] * thisWeight;
          cumVec[2] += xformVec[2] * thisWeight;
        }
      }

      if (inCellNormals)
      {
        // normalize normal again
        svtkMath::Normalize(cumNorm);
        newCellNormals->InsertNextTuple(cumNorm);
      }

      if (inCellVectors)
      {
        newCellVectors->InsertNextTuple(cumVec);
      }
    }
  }

  this->UpdateProgress(0.8);

  // Update ourselves and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();

  if (newNormals)
  {
    outPD->SetNormals(newNormals);
    outPD->CopyNormalsOff();
    newNormals->Delete();
  }

  if (newVectors)
  {
    outPD->SetVectors(newVectors);
    outPD->CopyVectorsOff();
    newVectors->Delete();
  }

  if (newCellNormals)
  {
    outCD->SetNormals(newCellNormals);
    outCD->CopyNormalsOff();
    newCellNormals->Delete();
  }

  if (newCellVectors)
  {
    outCD->SetVectors(newCellVectors);
    outCD->CopyVectorsOff();
    newCellVectors->Delete();
  }

  outPD->PassData(pd);
  outCD->PassData(cd);

  return 1;
}

//----------------------------------------------------------------------------
svtkMTimeType svtkWeightedTransformFilter::GetMTime()
{
  int i;
  svtkMTimeType mTime = this->MTime.GetMTime();
  svtkMTimeType transMTime;

  if (this->Transforms)
  {
    for (i = 0; i < this->NumberOfTransforms; i++)
    {
      if (this->Transforms[i])
      {
        transMTime = this->Transforms[i]->GetMTime();
        mTime = (transMTime > mTime ? transMTime : mTime);
      }
    }
  }

  return mTime;
}

//----------------------------------------------------------------------------
void svtkWeightedTransformFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  int i;
  this->Superclass::PrintSelf(os, indent);

  os << indent << "NumberOfTransforms: " << this->NumberOfTransforms << "\n";
  for (i = 0; i < this->NumberOfTransforms; i++)
  {
    os << indent << "Transform " << i << ": " << this->Transforms[i] << "\n";
  }
  os << indent << "AddInputValues: " << (this->AddInputValues ? "On" : "Off") << "\n";
  os << indent << "WeightArray: " << (this->WeightArray ? this->WeightArray : "(none)") << "\n";
  os << indent << "CellDataWeightArray: "
     << (this->CellDataWeightArray ? this->CellDataWeightArray : "(none)") << "\n";
  os << indent << "TransformIndexArray: "
     << (this->TransformIndexArray ? this->TransformIndexArray : "(none)") << "\n";
  os << indent << "CellDataTransformIndexArray: "
     << (this->CellDataTransformIndexArray ? this->CellDataTransformIndexArray : "(none)") << "\n";
}
