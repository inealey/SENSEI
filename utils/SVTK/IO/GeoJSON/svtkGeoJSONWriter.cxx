/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGeoJSONWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkGeoJSONWriter.h"

#include "svtkCellArray.h"
#include "svtkInformation.h"
#include "svtkLookupTable.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtksys/FStream.hxx"

#include <sstream>

svtkStandardNewMacro(svtkGeoJSONWriter);

#define SVTK_GJWRITER_MAXPOINTS 32000

class svtkGeoJSONWriter::Internals
{
public:
  Internals()
  {
    this->MaxBufferSize = 128;
    this->Buffer = new char[this->MaxBufferSize];
    this->Top = this->Buffer;
  };
  ~Internals() { delete[] this->Buffer; }
  inline size_t GetSize() { return this->Top - this->Buffer; }
  void Clear() { this->Top = this->Buffer; }
  inline void Grow()
  {
    this->MaxBufferSize *= 2;
    // cerr << "GROW " << this->MaxBufferSize << endl;
    char* biggerBuffer = new char[this->MaxBufferSize];
    size_t curSize = this->Top - this->Buffer;
    memcpy(biggerBuffer, this->Buffer, curSize);
    delete[] this->Buffer;
    this->Buffer = biggerBuffer;
    this->Top = this->Buffer + curSize;
  }
  inline void append(const char* newcontent)
  {
    while (this->Top + strlen(newcontent) >= this->Buffer + this->MaxBufferSize)
    {
      this->Grow();
    }
    int nchars = snprintf(this->Top, this->MaxBufferSize, "%s", newcontent);
    this->Top += nchars;
  }
  inline void append(const double newcontent)
  {
    snprintf(this->NumBuffer, 64, "%g", newcontent);
    while (this->Top + strlen(NumBuffer) >= this->Buffer + this->MaxBufferSize)
    {
      this->Grow();
    }
    int nchars = snprintf(this->Top, this->MaxBufferSize, "%s", this->NumBuffer);
    this->Top += nchars;
  }
  char* Buffer;
  char* Top;
  size_t MaxBufferSize;
  char NumBuffer[64];
};

//------------------------------------------------------------------------------
svtkGeoJSONWriter::svtkGeoJSONWriter()
{
  this->FileName = nullptr;
  this->OutputString = nullptr;
  this->SetNumberOfOutputPorts(0);
  this->WriteToOutputString = false;
  this->ScalarFormat = 2;
  this->LookupTable = nullptr;
  this->WriterHelper = new svtkGeoJSONWriter::Internals();
}

//------------------------------------------------------------------------------
svtkGeoJSONWriter::~svtkGeoJSONWriter()
{
  this->SetFileName(nullptr);
  delete[] this->OutputString;
  this->SetLookupTable(nullptr);
  delete this->WriterHelper;
}

//------------------------------------------------------------------------------
void svtkGeoJSONWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "NONE") << endl;
  os << indent << "WriteToOutputString: " << (this->WriteToOutputString ? "True" : "False") << endl;
  os << indent << "ScalarFormat: " << this->ScalarFormat << endl;
}

//------------------------------------------------------------------------------
int svtkGeoJSONWriter::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPolyData");
  }
  return 1;
}

//------------------------------------------------------------------------------
ostream* svtkGeoJSONWriter::OpenFile()
{
  svtkDebugMacro(<< "Opening file\n");

  ostream* fptr;

  if (!this->WriteToOutputString)
  {
    if (!this->FileName)
    {
      svtkErrorMacro(<< "No FileName specified! Can't write!");
      return nullptr;
    }

    fptr = new svtksys::ofstream(this->FileName, ios::out);
  }
  else
  {
    // Get rid of any old output string.
    if (this->OutputString)
    {
      delete[] this->OutputString;
      this->OutputString = nullptr;
      this->OutputStringLength = 0;
    }
    fptr = new std::ostringstream;
  }

  if (fptr->fail())
  {
    svtkErrorMacro(<< "Unable to open file: " << this->FileName);
    delete fptr;
    return nullptr;
  }

  return fptr;
}

//------------------------------------------------------------------------------
void svtkGeoJSONWriter::CloseFile(ostream* fp)
{
  svtkDebugMacro(<< "Closing file\n");

  if (fp != nullptr)
  {
    if (this->WriteToOutputString)
    {
      std::ostringstream* ostr = static_cast<std::ostringstream*>(fp);

      delete[] this->OutputString;
      this->OutputStringLength = static_cast<int>(ostr->str().size());
      //+1's account for null terminator
      this->OutputString = new char[ostr->str().size() + 1];
      memcpy(this->OutputString, ostr->str().c_str(), this->OutputStringLength + 1);
    }

    delete fp;
  }
}

//------------------------------------------------------------------------------
void svtkGeoJSONWriter::ConditionalComma(svtkIdType cnt, svtkIdType limit)
{
  if (cnt + 1 != limit)
  {
    this->WriterHelper->append(",");
  }
}

//------------------------------------------------------------------------------
void svtkGeoJSONWriter::WriteScalar(svtkDataArray* da, svtkIdType ptId)
{
  if (this->ScalarFormat == 0)
  {
    return;
  }
  if (da)
  {
    double b = da->GetTuple1(ptId);
    if (this->ScalarFormat == 1)
    {
      svtkLookupTable* lut = this->GetLookupTable();
      if (!lut)
      {
        lut = svtkLookupTable::New();
        lut->SetNumberOfColors(256);
        lut->SetHueRange(0.0, 0.667);
        lut->SetRange(da->GetRange());
        lut->Build();
        this->SetLookupTable(lut);
        lut->Delete();
      }
      const unsigned char* color = lut->MapValue(b);
      this->WriterHelper->append(",");
      this->WriterHelper->append((double)color[0] / 255.0);
      this->WriterHelper->append(",");
      this->WriterHelper->append((double)color[1] / 255.0);
      this->WriterHelper->append(",");
      this->WriterHelper->append((double)color[2] / 255.0);
    }
    else
    {
      if (svtkMath::IsNan(b))
      {
        this->WriterHelper->append(",null");
      }
      else
      {
        this->WriterHelper->append(",");
        this->WriterHelper->append(b);
      }
    }
  }
}

//------------------------------------------------------------------------------
void svtkGeoJSONWriter::WriteData()
{
  ostream* fp;
  svtkPolyData* input = svtkPolyData::SafeDownCast(this->GetInput());

  svtkDebugMacro(<< "Writing svtk polygonal data to geojson file...");
  fp = this->OpenFile();
  if (!fp)
  {
    return;
  }

  this->WriterHelper->append("{\n");
  this->WriterHelper->append("\"type\": \"Feature\",\n");
  svtkDataArray* da = input->GetPointData()->GetScalars();
  if (!da)
  {
    da = input->GetPointData()->GetArray(0);
  }
  if (da)
  {
    switch (this->ScalarFormat)
    {
      case 0:
        this->WriterHelper->append("\"properties\": {\"ScalarFormat\": \"none\"},\n");
        break;
      case 1:
        this->WriterHelper->append("\"properties\": {\"ScalarFormat\": \"rgb\"},\n");
        break;
      case 2:
        double rng[2];
        da->GetRange(rng);
        this->WriterHelper->append(
          "\"properties\": {\"ScalarFormat\": \"values\", \"ScalarRange\": [");
        this->WriterHelper->append(rng[0]);
        this->WriterHelper->append(",");
        this->WriterHelper->append(rng[1]);
        this->WriterHelper->append("] },\n");
        break;
    }
  }
  else
  {
    this->WriterHelper->append("\"properties\": {\"ScalarFormat\": \"none\"},\n");
  }
  this->WriterHelper->append("\"geometry\":\n");
  this->WriterHelper->append("{\n");
  this->WriterHelper->append("\"type\": \"GeometryCollection\",\n");
  this->WriterHelper->append("\"geometries\":\n");
  this->WriterHelper->append("[\n");

  const svtkIdType* cellPts = nullptr;
  svtkIdType cellSize = 0;
  svtkIdType numlines, numpolys;
  numlines = input->GetLines()->GetNumberOfCells();
  numpolys = input->GetPolys()->GetNumberOfCells();

  // VERTS
  svtkCellArray* ca;
  ca = input->GetVerts();
  if (ca && ca->GetNumberOfCells())
  {
    bool done = false;
    svtkIdType inCell = 0;
    svtkIdType ptCnt = 0;
    do // loop to break into sections with < SVTK_GJWRITER_MAXPOINTS points
    {
      this->WriterHelper->append("{\n");
      this->WriterHelper->append("\"type\": \"MultiPoint\",\n");
      this->WriterHelper->append("\"coordinates\":\n");
      this->WriterHelper->append("[\n");
      for (; inCell < ca->GetNumberOfCells() && ptCnt < SVTK_GJWRITER_MAXPOINTS; inCell++)
      {
        ca->GetCellAtId(inCell, cellSize, cellPts);
        ptCnt += cellSize;
        svtkIdType inPt;
        for (inPt = 0; inPt < cellSize; inPt++)
        {
          double coords[3];
          input->GetPoint(cellPts[inPt], coords);
          this->WriterHelper->append("[");
          for (int i = 0; i < 3; i++)
          {
            if (svtkMath::IsNan(coords[i]))
            {
              this->WriterHelper->append("null");
            }
            else
            {
              this->WriterHelper->append(coords[i]);
            }
            if (i != 2)
            {
              this->WriterHelper->append(",");
            }
          }
          this->WriteScalar(da, cellPts[inPt]);
          this->WriterHelper->append("]");
          this->ConditionalComma(inPt, cellSize);
        }
        if (ptCnt < SVTK_GJWRITER_MAXPOINTS)
        {
          this->ConditionalComma(inCell, ca->GetNumberOfCells());
        }
        this->WriterHelper->append("\n");
      }
      this->WriterHelper->append("]"); // coordinates for this cell array
      if (inCell < ca->GetNumberOfCells())
      {
        ptCnt = 0;
        this->WriterHelper->append(",\n");
      }
      else
      {
        if (numlines || numpolys)
        {
          this->WriterHelper->append(",");
        }
        done = true;
      }
    } while (!done);
  }

  // lines
  ca = input->GetLines();
  if (ca && ca->GetNumberOfCells())
  {
    bool done = false;
    svtkIdType inCell = 0;
    svtkIdType ptCnt = 0;
    do // loop to break into sections with < SVTK_GJWRITER_MAXPOINTS points
    {
      this->WriterHelper->append("{\n");
      this->WriterHelper->append("\"type\": \"MultiLineString\",\n");
      this->WriterHelper->append("\"coordinates\":\n");
      this->WriterHelper->append("[\n");
      for (; inCell < ca->GetNumberOfCells() && ptCnt < SVTK_GJWRITER_MAXPOINTS; inCell++)
      {
        this->WriterHelper->append("[ "); // one cell
        ca->GetCellAtId(inCell, cellSize, cellPts);
        ptCnt += cellSize;
        svtkIdType inPt;
        for (inPt = 0; inPt < cellSize; inPt++)
        {
          double coords[3];
          input->GetPoint(cellPts[inPt], coords);
          this->WriterHelper->append("[");
          for (int i = 0; i < 3; i++)
          {
            if (svtkMath::IsNan(coords[i]))
            {
              this->WriterHelper->append("null");
            }
            else
            {
              this->WriterHelper->append(coords[i]);
            }
            if (i != 2)
            {
              this->WriterHelper->append(",");
            }
          }
          this->WriteScalar(da, cellPts[inPt]);
          this->WriterHelper->append("]");
          this->ConditionalComma(inPt, cellSize);
        }
        this->WriterHelper->append("]"); // one cell
        if (ptCnt < SVTK_GJWRITER_MAXPOINTS)
        {
          this->ConditionalComma(inCell, ca->GetNumberOfCells());
        }
        this->WriterHelper->append("\n");
      }
      this->WriterHelper->append("]"); // coordinates for this cell array
      this->WriterHelper->append("\n");
      this->WriterHelper->append("}\n"); // this cell array
      if (inCell < ca->GetNumberOfCells())
      {
        ptCnt = 0;
        this->WriterHelper->append(",\n");
      }
      else
      {
        if (numpolys)
        {
          this->WriterHelper->append(",");
        }
        done = true;
      }
    } while (!done);
  }
  // polygons
  ca = input->GetPolys();
  if (ca && ca->GetNumberOfCells())
  {
    bool done = false;
    svtkIdType inCell = 0;
    svtkIdType ptCnt = 0;
    do // loop to break into sections with < SVTK_GJWRITER_MAXPOINTS points
    {
      this->WriterHelper->append("{\n");
      this->WriterHelper->append("\"type\": \"MultiPolygon\",\n");
      this->WriterHelper->append("\"coordinates\":\n");
      this->WriterHelper->append("[\n");
      for (; inCell < ca->GetNumberOfCells() && ptCnt < SVTK_GJWRITER_MAXPOINTS; inCell++)
      {
        this->WriterHelper->append("[[ "); // one cell
        ca->GetCellAtId(inCell, cellSize, cellPts);
        ptCnt += cellSize;
        svtkIdType inPt;
        for (inPt = 0; inPt < cellSize; inPt++)
        {
          double coords[3];
          input->GetPoint(cellPts[inPt], coords);
          this->WriterHelper->append("[");
          for (int i = 0; i < 3; i++)
          {
            if (svtkMath::IsNan(coords[i]))
            {
              this->WriterHelper->append("null");
            }
            else
            {
              this->WriterHelper->append(coords[i]);
            }
            if (i != 2)
            {
              this->WriterHelper->append(",");
            }
          }
          this->WriteScalar(da, cellPts[inPt]);
          this->WriterHelper->append("]");
          this->ConditionalComma(inPt, cellSize);
        }
        this->WriterHelper->append(" ]]"); // one cell
        if (ptCnt < SVTK_GJWRITER_MAXPOINTS)
        {
          this->ConditionalComma(inCell, ca->GetNumberOfCells());
        }
        this->WriterHelper->append("\n");
      }
      this->WriterHelper->append("]"); // coordinates for this cell array
      this->WriterHelper->append("\n");
      this->WriterHelper->append("}\n"); // this cell array
      if (inCell < ca->GetNumberOfCells())
      {
        ptCnt = 0;
        this->WriterHelper->append(",\n");
      }
      else
      {
        done = true;
      }
    } while (!done);
  }

  this->WriterHelper->append("]\n"); // feature.geometry.GeometryCollection.geometries
  this->WriterHelper->append("}\n"); // feature.geometry
  this->WriterHelper->append("}\n"); // feature

  fp->write(this->WriterHelper->Buffer, this->WriterHelper->GetSize());
  this->WriterHelper->Clear();

  fp->flush();
  if (fp->fail())
  {
    svtkErrorMacro("Problem writing result check disk space.");
    delete fp;
    fp = nullptr;
  }

  this->CloseFile(fp);
}

//------------------------------------------------------------------------------
char* svtkGeoJSONWriter::RegisterAndGetOutputString()
{
  char* tmp = this->OutputString;

  this->OutputString = nullptr;
  this->OutputStringLength = 0;

  return tmp;
}

//------------------------------------------------------------------------------
svtkStdString svtkGeoJSONWriter::GetOutputStdString()
{
  return svtkStdString(this->OutputString, this->OutputStringLength);
}

//------------------------------------------------------------------------------
svtkCxxSetObjectMacro(svtkGeoJSONWriter, LookupTable, svtkLookupTable);
