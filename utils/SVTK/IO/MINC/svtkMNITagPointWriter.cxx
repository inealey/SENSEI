/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMNITagPointWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

Copyright (c) 2006 Atamai, Inc.

Use, modification and redistribution of the software, in source or
binary forms, are permitted provided that the following terms and
conditions are met:

1) Redistribution of the source code, in verbatim or modified
   form, must retain the above copyright notice, this license,
   the following disclaimer, and any notices that refer to this
   license and/or the following disclaimer.

2) Redistribution in binary form must include the above copyright
   notice, a copy of this license and the following disclaimer
   in the documentation or with other materials provided with the
   distribution.

3) Modified copies of the source code must be clearly marked as such,
   and must not be misrepresented as verbatim copies of the source code.

THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE SOFTWARE "AS IS"
WITHOUT EXPRESSED OR IMPLIED WARRANTY INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  IN NO EVENT SHALL ANY COPYRIGHT HOLDER OR OTHER PARTY WHO MAY
MODIFY AND/OR REDISTRIBUTE THE SOFTWARE UNDER THE TERMS OF THIS LICENSE
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, LOSS OF DATA OR DATA BECOMING INACCURATE
OR LOSS OF PROFIT OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF
THE USE OR INABILITY TO USE THE SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGES.

=========================================================================*/

#include "svtkMNITagPointWriter.h"

#include "svtkObjectFactory.h"

#include "svtkCommand.h"
#include "svtkDoubleArray.h"
#include "svtkErrorCode.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkMath.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkPoints.h"
#include "svtkStringArray.h"
#include "svtksys/FStream.hxx"

#include <cctype>
#include <cmath>

#if !defined(_WIN32) || defined(__CYGWIN__)
#include <unistd.h> /* unlink */
#else
#include <io.h> /* unlink */
#endif

//--------------------------------------------------------------------------
svtkStandardNewMacro(svtkMNITagPointWriter);

svtkCxxSetObjectMacro(svtkMNITagPointWriter, LabelText, svtkStringArray);
svtkCxxSetObjectMacro(svtkMNITagPointWriter, Weights, svtkDoubleArray);
svtkCxxSetObjectMacro(svtkMNITagPointWriter, StructureIds, svtkIntArray);
svtkCxxSetObjectMacro(svtkMNITagPointWriter, PatientIds, svtkIntArray);

//-------------------------------------------------------------------------
svtkMNITagPointWriter::svtkMNITagPointWriter()
{
  this->Points[0] = nullptr;
  this->Points[1] = nullptr;

  this->LabelText = nullptr;
  this->Weights = nullptr;
  this->StructureIds = nullptr;
  this->PatientIds = nullptr;

  this->Comments = nullptr;

  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(0);

  this->FileName = nullptr;
}

//-------------------------------------------------------------------------
svtkMNITagPointWriter::~svtkMNITagPointWriter()
{
  svtkObject* objects[6];
  objects[0] = this->Points[0];
  objects[1] = this->Points[1];
  objects[2] = this->LabelText;
  objects[3] = this->Weights;
  objects[4] = this->StructureIds;
  objects[5] = this->PatientIds;

  for (int i = 0; i < 6; i++)
  {
    if (objects[i])
    {
      objects[i]->Delete();
    }
  }

  delete[] this->Comments;

  delete[] this->FileName;
}

//-------------------------------------------------------------------------
void svtkMNITagPointWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Points: " << this->Points[0] << " " << this->Points[1] << "\n";
  os << indent << "LabelText: " << this->LabelText << "\n";
  os << indent << "Weights: " << this->Weights << "\n";
  os << indent << "StructureIds: " << this->StructureIds << "\n";
  os << indent << "PatientIds: " << this->PatientIds << "\n";

  os << indent << "Comments: " << (this->Comments ? this->Comments : "none") << "\n";
}

//----------------------------------------------------------------------------
int svtkMNITagPointWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPointSet");
  info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//-------------------------------------------------------------------------
svtkMTimeType svtkMNITagPointWriter::GetMTime()
{
  svtkMTimeType mtime = this->Superclass::GetMTime();

  svtkObject* objects[6];
  objects[0] = this->Points[0];
  objects[1] = this->Points[1];
  objects[2] = this->LabelText;
  objects[3] = this->Weights;
  objects[4] = this->StructureIds;
  objects[5] = this->PatientIds;

  for (int i = 0; i < 6; i++)
  {
    if (objects[i])
    {
      svtkMTimeType m = objects[i]->GetMTime();
      if (m > mtime)
      {
        mtime = m;
      }
    }
  }

  return mtime;
}

//-------------------------------------------------------------------------
void svtkMNITagPointWriter::SetPoints(int port, svtkPoints* points)
{
  if (port < 0 || port > 1)
  {
    return;
  }
  if (this->Points[port] == points)
  {
    return;
  }
  if (this->Points[port])
  {
    this->Points[port]->Delete();
  }
  this->Points[port] = points;
  if (this->Points[port])
  {
    this->Points[port]->Register(this);
  }
  this->Modified();
}

//-------------------------------------------------------------------------
svtkPoints* svtkMNITagPointWriter::GetPoints(int port)
{
  if (port < 0 || port > 1)
  {
    return nullptr;
  }
  return this->Points[port];
}

//-------------------------------------------------------------------------
void svtkMNITagPointWriter::WriteData(svtkPointSet* inputs[2])
{
  static const char* arrayNames[3] = { "Weights", "StructureIds", "PatientIds" };

  svtkPoints* points[2];

  svtkStringArray* labels = nullptr;
  svtkDataArray* darray[3];
  darray[0] = nullptr;
  darray[1] = nullptr;
  darray[2] = nullptr;

  svtkDataArray* ivarArrays[3];
  ivarArrays[0] = this->Weights;
  ivarArrays[1] = this->StructureIds;
  ivarArrays[2] = this->PatientIds;

  for (int ii = 1; ii >= 0; --ii)
  {
    points[ii] = nullptr;
    if (inputs[ii])
    {
      points[ii] = inputs[ii]->GetPoints();

      svtkStringArray* stringArray =
        svtkArrayDownCast<svtkStringArray>(inputs[ii]->GetPointData()->GetAbstractArray("LabelText"));
      if (stringArray)
      {
        labels = stringArray;
      }

      for (int j = 0; j < 3; j++)
      {
        svtkDataArray* dataArray = inputs[ii]->GetPointData()->GetArray(arrayNames[j]);
        if (dataArray)
        {
          darray[j] = dataArray;
        }
      }
    }

    if (this->Points[ii])
    {
      points[ii] = this->Points[ii];
    }
  }

  if (this->LabelText)
  {
    labels = this->LabelText;
  }

  for (int j = 0; j < 3; j++)
  {
    if (ivarArrays[j])
    {
      darray[j] = ivarArrays[j];
    }
  }

  if (points[0] == nullptr)
  {
    svtkErrorMacro("No input points have been provided");
    return;
  }

  // numVolumes is 1 if there is only one set of points
  int numVolumes = 1;
  svtkIdType n = points[0]->GetNumberOfPoints();
  if (points[1])
  {
    numVolumes = 2;
    if (points[1]->GetNumberOfPoints() != n)
    {
      svtkErrorMacro(
        "Input point counts do not match: " << n << " versus " << points[1]->GetNumberOfPoints());
      return;
    }
  }

  // labels is null if there are no labels
  if (labels && labels->GetNumberOfValues() != n)
  {
    svtkErrorMacro("LabelText count does not match point count: " << labels->GetNumberOfValues()
                                                                 << " versus " << n);
    return;
  }

  // dataArrays is null if there are no data arrays
  svtkDataArray** dataArrays = nullptr;
  for (int jj = 0; jj < 3; jj++)
  {
    if (darray[jj])
    {
      dataArrays = darray;
      if (darray[jj]->GetNumberOfTuples() != n)
      {
        svtkErrorMacro("" << arrayNames[jj] << " count does not match point count: "
                         << darray[jj]->GetNumberOfTuples() << " versus " << n);
        return;
      }
    }
  }

  // If we got this far, the data seems to be okay
  ostream* outfilep = this->OpenFile();
  if (!outfilep)
  {
    return;
  }

  ostream& outfile = *outfilep;

  // Write the header
  outfile << "MNI Tag Point File\n";
  outfile << "Volumes = " << numVolumes << ";\n";

  // Write user comments
  if (this->Comments)
  {
    char* cp = this->Comments;
    while (*cp)
    {
      if (*cp != '%')
      {
        outfile << "% ";
      }
      while (*cp && *cp != '\n')
      {
        if (isprint(*cp) || *cp == '\t')
        {
          outfile << *cp;
        }
        cp++;
      }
      outfile << "\n";
      if (*cp == '\n')
      {
        cp++;
      }
    }
  }
  else
  {
    for (int k = 0; k < numVolumes; k++)
    {
      outfile << "% Volume " << (k + 1) << " produced by SVTK\n";
    }
  }

  // Add a blank line
  outfile << "\n";

  // Write the points
  outfile << "Points =\n";

  char text[256];
  for (int i = 0; i < n; i++)
  {
    for (int kk = 0; kk < 2; kk++)
    {
      if (points[kk])
      {
        double point[3];
        points[kk]->GetPoint(i, point);
        snprintf(text, sizeof(text), " %.15g %.15g %.15g", point[0], point[1], point[2]);
        outfile << text;
      }
    }

    if (dataArrays)
    {
      double w = 0.0;
      int s = -1;
      int p = -1;
      if (dataArrays[0])
      {
        w = dataArrays[0]->GetComponent(i, 0);
      }
      if (dataArrays[1])
      {
        s = static_cast<int>(dataArrays[1]->GetComponent(i, 0));
      }
      if (dataArrays[2])
      {
        p = static_cast<int>(dataArrays[2]->GetComponent(i, 0));
      }

      snprintf(text, sizeof(text), " %.15g %d %d", w, s, p);
      outfile << text;
    }

    if (labels)
    {
      svtkStdString l = labels->GetValue(i);
      outfile << " \"";
      for (std::string::iterator si = l.begin(); si != l.end(); ++si)
      {
        if (isprint(*si) && *si != '\"' && *si != '\\')
        {
          outfile.put(*si);
        }
        else
        {
          outfile.put('\\');
          char c = '\0';
          static char ctrltable[] = { '\a', 'a', '\b', 'b', '\f', 'f', '\n', 'n', '\r', 'r', '\t',
            't', '\v', 'v', '\\', '\\', '\"', '\"', '\0', '\0' };
          for (int ci = 0; ctrltable[ci] != '\0'; ci += 2)
          {
            if (*si == ctrltable[ci])
            {
              c = ctrltable[ci + 1];
              break;
            }
          }
          if (c != '\0')
          {
            outfile.put(c);
          }
          else
          {
            snprintf(text, sizeof(text), "x%2.2x", (static_cast<int>(*si) & 0x00ff));
            outfile << text;
          }
        }
      }

      outfile << "\"";
    }

    if (i < n - 1)
    {
      outfile << "\n";
    }
  }

  outfile << ";\n";
  outfile.flush();

  // Close the file
  this->CloseFile(outfilep);

  // Delete the file if an error occurred
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    svtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    unlink(this->FileName);
  }
}

//-------------------------------------------------------------------------
int svtkMNITagPointWriter::Write()
{
  // Allow writer to work when no inputs are provided
  this->Modified();
  this->Update();
  return 1;
}

//-------------------------------------------------------------------------
int svtkMNITagPointWriter::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector*)
{
  this->SetErrorCode(svtkErrorCode::NoError);

  svtkInformation* inInfo[2];
  inInfo[0] = inputVector[0]->GetInformationObject(0);
  inInfo[1] = inputVector[1]->GetInformationObject(0);

  svtkPointSet* input[2];
  input[0] = nullptr;
  input[1] = nullptr;

  svtkMTimeType lastUpdateTime = 0;
  for (int idx = 0; idx < 2; ++idx)
  {
    if (inInfo[idx])
    {
      input[idx] = svtkPointSet::SafeDownCast(inInfo[idx]->Get(svtkDataObject::DATA_OBJECT()));
      if (input[idx])
      {
        svtkMTimeType updateTime = input[idx]->GetUpdateTime();
        if (updateTime > lastUpdateTime)
        {
          lastUpdateTime = updateTime;
        }
      }
    }
  }

  if (lastUpdateTime < this->WriteTime && this->GetMTime() < this->WriteTime)
  {
    // we are up to date
    return 1;
  }

  this->InvokeEvent(svtkCommand::StartEvent, nullptr);
  this->WriteData(input);
  this->InvokeEvent(svtkCommand::EndEvent, nullptr);

  this->WriteTime.Modified();

  return 1;
}

//-------------------------------------------------------------------------
ostream* svtkMNITagPointWriter::OpenFile()
{
  ostream* fptr;

  if (!this->FileName)
  {
    svtkErrorMacro(<< "No FileName specified! Can't write!");
    this->SetErrorCode(svtkErrorCode::NoFileNameError);
    return nullptr;
  }

  svtkDebugMacro(<< "Opening file for writing...");

  fptr = new svtksys::ofstream(this->FileName, ios::out);

  if (fptr->fail())
  {
    svtkErrorMacro(<< "Unable to open file: " << this->FileName);
    this->SetErrorCode(svtkErrorCode::CannotOpenFileError);
    delete fptr;
    return nullptr;
  }

  return fptr;
}

//-------------------------------------------------------------------------
void svtkMNITagPointWriter::CloseFile(ostream* fp)
{
  svtkDebugMacro(<< "Closing file\n");

  delete fp;
}
