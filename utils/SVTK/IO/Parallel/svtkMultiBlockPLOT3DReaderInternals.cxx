
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMultiBlockPLOT3DReaderInternals.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkMultiBlockPLOT3DReaderInternals.h"

#include "svtkMultiProcessController.h"
#include <cassert>

int svtkMultiBlockPLOT3DReaderInternals::ReadInts(FILE* fp, int n, int* val)
{
  int retVal = static_cast<int>(fread(val, sizeof(int), n, fp));
  if (this->Settings.ByteOrder == svtkMultiBlockPLOT3DReader::FILE_LITTLE_ENDIAN)
  {
    svtkByteSwap::Swap4LERange(val, n);
  }
  else
  {
    svtkByteSwap::Swap4BERange(val, n);
  }
  return retVal;
}

void svtkMultiBlockPLOT3DReaderInternals::CheckBinaryFile(FILE* fp, size_t fileSize)
{
  rewind(fp);
  this->Settings.BinaryFile = 0;

  // The shortest binary file is 12 files: 2 ints for block dims + 1 float for
  // a coordinate.
  if (fileSize < 12)
  {
    return;
  }

  char bytes[12];
  if (fread(bytes, 1, 12, fp) != 12)
  {
    return;
  }
  // Check the first 12 bytes. If we find non-ascii characters, then we
  // assume that it is binary.
  for (int i = 0; i < 12; i++)
  {
    if (!(isdigit(bytes[i]) || bytes[i] == '.' || bytes[i] == ' ' || bytes[i] == '\r' ||
          bytes[i] == '\n' || bytes[i] == '\t'))
    {
      this->Settings.BinaryFile = 1;
      return;
    }
  }
}

int svtkMultiBlockPLOT3DReaderInternals::CheckByteOrder(FILE* fp)
{
  rewind(fp);
  int i;
  if (fread(&i, sizeof(int), 1, fp) < 1)
  {
    return 0;
  }
  char* cpy = (char*)&i;

  // If binary, we can assume that the first value is going to be either a
  // count (Fortran) or a number of block or a dimension. We assume that
  // this number will be smaller than 2^24. Then we check the first byte.
  // If it is 0 and the last byte is not 0, it is likely that this is big
  // endian.
  if (cpy[0] == 0 && cpy[3] != 0)
  {
    this->Settings.ByteOrder = svtkMultiBlockPLOT3DReader::FILE_BIG_ENDIAN;
  }
  else
  {
    this->Settings.ByteOrder = svtkMultiBlockPLOT3DReader::FILE_LITTLE_ENDIAN;
  }
  return 1;
}

int svtkMultiBlockPLOT3DReaderInternals::CheckByteCount(FILE* fp)
{
  rewind(fp);
  // Read the first integer, then skip by that many bytes, then
  // read the value again. If the two match, it is likely that
  // the file has byte counts.
  int count;
  if (!this->ReadInts(fp, 1, &count))
  {
    return 0;
  }
  if (fseek(fp, count, SEEK_CUR) != 0)
  {
    return 0;
  }
  int count2;
  if (!this->ReadInts(fp, 1, &count2))
  {
    return 0;
  }
  if (count == count2)
  {
    this->Settings.HasByteCount = 1;
  }
  else
  {
    this->Settings.HasByteCount = 0;
  }
  return 1;
}

int svtkMultiBlockPLOT3DReaderInternals::CheckMultiGrid(FILE* fp)
{
  if (this->Settings.HasByteCount)
  {
    rewind(fp);
    // We read the byte count, if it is 4 (1 int),
    // then this is multi-grid because the first
    // value is the number of grids rather than
    // an array of 2 or 3 values.
    int recMarkBeg;
    if (!this->ReadInts(fp, 1, &recMarkBeg))
    {
      return 0;
    }
    if (recMarkBeg == sizeof(int))
    {
      this->Settings.MultiGrid = 1;
    }
    else
    {
      this->Settings.MultiGrid = 0;
    }
    return 1;
  }
  return 0;
}

int svtkMultiBlockPLOT3DReaderInternals::Check2DGeom(FILE* fp)
{
  if (this->Settings.HasByteCount)
  {
    rewind(fp);
    int recMarkBeg, recMarkEnd;
    int numGrids = 1;
    if (this->Settings.MultiGrid)
    {
      if (!this->ReadInts(fp, 1, &recMarkBeg) || !this->ReadInts(fp, 1, &numGrids) ||
        !this->ReadInts(fp, 1, &recMarkEnd))
      {
        return 0;
      }
    }
    if (!this->ReadInts(fp, 1, &recMarkBeg))
    {
      return 0;
    }
    int nMax = 3 * numGrids;
    int ndims;
    if (recMarkBeg == static_cast<int>(nMax * sizeof(int) + 2 * sizeof(int)))
    {
      ndims = 3;
    }
    else
    {
      if (recMarkBeg == static_cast<int>(nMax * sizeof(int)))
      {
        ndims = 3;
      }
      else
      {
        ndims = 2;
      }
    }
    this->Settings.NumberOfDimensions = ndims;
    return 1;
  }
  return 0;
}

int svtkMultiBlockPLOT3DReaderInternals::CheckBlankingAndPrecision(FILE* fp)
{
  int recMarkBeg, recMarkEnd, numGrids = 1, nMax, totPts;

  rewind(fp);
  if (this->Settings.MultiGrid)
  {
    if (!this->ReadInts(fp, 1, &recMarkBeg) || !this->ReadInts(fp, 1, &numGrids) ||
      !this->ReadInts(fp, 1, &recMarkEnd))
    {
      return 0;
    }
  }
  if (!this->ReadInts(fp, 1, &recMarkBeg))
  {
    return 0;
  }
  nMax = this->Settings.NumberOfDimensions * numGrids;
  std::vector<int> jmax(numGrids * 3); // allocate memory for jmax
  if (!this->ReadInts(fp, nMax, jmax.data()) || !this->ReadInts(fp, 1, &recMarkEnd))
  {
    return 0;
  }
  totPts = 1;
  for (int i = 0; i < this->Settings.NumberOfDimensions; i++)
  {
    totPts *= jmax[i];
  }
  this->ReadInts(fp, 1, &recMarkBeg);
  // single precision, with iblanking
  if (recMarkBeg == totPts * (this->Settings.NumberOfDimensions * 4 + 4))
  {
    this->Settings.Precision = 4;
    this->Settings.IBlanking = 1;
    return 1;
  }
  // double precision, with iblanking
  else if (recMarkBeg == totPts * (this->Settings.NumberOfDimensions * 8 + 4))
  {
    this->Settings.Precision = 8;
    this->Settings.IBlanking = 1;
    return 1;
  }
  // single precision, no iblanking
  else if (recMarkBeg == totPts * this->Settings.NumberOfDimensions * 4)
  {
    this->Settings.Precision = 4;
    this->Settings.IBlanking = 0;
    return 1;
  }
  // double precision, no iblanking
  else if (recMarkBeg == totPts * this->Settings.NumberOfDimensions * 8)
  {
    this->Settings.Precision = 8;
    this->Settings.IBlanking = 0;
    return 1;
  }
  return 0;
}

// Unfortunately, a Plot3D file written in C is trickier
// to check because it has no byte count markers. We need
// to do a bit more brute force checks based on reading
// data and estimating file size.
int svtkMultiBlockPLOT3DReaderInternals::CheckCFile(FILE* fp, size_t fileSize)
{
  int precisions[2] = { 4, 8 };
  int blankings[2] = { 0, 1 };
  int dimensions[2] = { 2, 3 };

  rewind(fp);
  int gridDims[3];
  if (this->ReadInts(fp, 3, gridDims) != 3)
  {
    return 0;
  }

  // Single grid
  for (int i = 0; i < 2; i++)
  {
    int precision = precisions[i];
    for (int j = 0; j < 2; j++)
    {
      int blanking = blankings[j];
      for (int k = 0; k < 2; k++)
      {
        int dimension = dimensions[k];

        if (fileSize ==
          this->CalculateFileSize(false, precision, blanking, dimension,
            false, // always
            1, gridDims))
        {
          this->Settings.MultiGrid = 0;
          this->Settings.Precision = precision;
          this->Settings.IBlanking = blanking;
          this->Settings.NumberOfDimensions = dimension;
          return 1;
        }
      }
    }
  }

  // Multi grid
  int nGrids;
  rewind(fp);
  if (!this->ReadInts(fp, 1, &nGrids))
  {
    return 0;
  }
  std::vector<int> gridDims2(3 * nGrids);
  if (this->ReadInts(fp, nGrids * 3, gridDims2.data()) != nGrids * 3)
  {
    return 0;
  }

  for (int i = 0; i < 2; i++)
  {
    int precision = precisions[i];
    for (int j = 0; j < 2; j++)
    {
      int blanking = blankings[j];
      for (int k = 0; k < 2; k++)
      {
        int dimension = dimensions[k];

        if (fileSize ==
          this->CalculateFileSize(true, precision, blanking, dimension,
            false, // always
            nGrids, gridDims2.data()))
        {
          this->Settings.MultiGrid = 1;
          this->Settings.Precision = precision;
          this->Settings.IBlanking = blanking;
          this->Settings.NumberOfDimensions = dimension;
          return 1;
        }
      }
    }
  }
  return 0;
}

size_t svtkMultiBlockPLOT3DReaderInternals::CalculateFileSize(int mgrid,
  int precision, // in bytes
  int blanking, int ndims, int hasByteCount, int nGrids, int* gridDims)
{
  size_t size = 0; // the header portion, 3 ints

  // N grids
  if (mgrid)
  {
    size += 4; // int for nblocks
    if (hasByteCount)
    {
      size += 2 * 4; // byte counts for nblocks
    }
  }
  // Header
  size += nGrids * ndims * 4;

  if (hasByteCount)
  {
    size += 2 * 4; // byte counts for grid dims
  }
  for (int i = 0; i < nGrids; i++)
  {
    size += this->CalculateFileSizeForBlock(
      precision, blanking, ndims, hasByteCount, gridDims + ndims * i);
  }
  return size;
}

size_t svtkMultiBlockPLOT3DReaderInternals::CalculateFileSizeForBlock(int precision, // in bytes
  int blanking, int ndims, int hasByteCount, int* gridDims)
{
  size_t size = 0;
  // x, y, (z)
  size_t npts = 1;
  for (int i = 0; i < ndims; i++)
  {
    npts *= gridDims[i];
  }
  size += npts * ndims * precision;

  if (blanking)
  {
    size += npts * 4;
  }

  if (hasByteCount)
  {
    size += 2 * 4;
  }
  return size;
}

//-----------------------------------------------------------------------------
bool svtkMultiBlockPLOT3DReaderRecord::Initialize(FILE* fp, svtkTypeUInt64 offset,
  const svtkMultiBlockPLOT3DReaderInternals::InternalSettings& settings,
  svtkMultiProcessController* controller)
{
  this->SubRecords.clear();
  if (!settings.BinaryFile || !settings.HasByteCount)
  {
    return true;
  }
  const int rank = controller ? controller->GetLocalProcessId() : 0;
  int error = 0;
  if (rank == 0)
  {
    svtkTypeUInt64 pos = svtk_ftell(fp);
    unsigned int leadingLengthField;
    int signBit = 0;
    try
    {
      do
      {
        svtkSubRecord subrecord;
        subrecord.HeaderOffset = offset;
        svtkTypeUInt64 dataOffset = subrecord.HeaderOffset + sizeof(int);
        svtk_fseek(fp, offset, SEEK_SET);
        if (fread(&leadingLengthField, sizeof(unsigned int), 1, fp) != 1)
        {
          throw Plot3DException();
        }
        if (settings.ByteOrder == svtkMultiBlockPLOT3DReader::FILE_LITTLE_ENDIAN)
        {
          svtkByteSwap::Swap4LE(&leadingLengthField);
        }
        else
        {
          svtkByteSwap::Swap4BE(&leadingLengthField);
        }
        signBit = (0x80000000 & leadingLengthField) ? 1 : 0;
        if (signBit)
        {
          unsigned int length = 0xffffffff ^ (leadingLengthField - 1);
          subrecord.FooterOffset = dataOffset + length;
        }
        else
        {
          subrecord.FooterOffset = dataOffset + leadingLengthField;
        }
        this->SubRecords.push_back(subrecord);
        offset = subrecord.FooterOffset + sizeof(int);
      } while (signBit == 1);
    }
    catch (Plot3DException&)
    {
      error = 1;
    }
    svtk_fseek(fp, pos, SEEK_SET);
  }

  if (controller == nullptr)
  {
    if (error)
    {
      this->SubRecords.clear();
    }
    return error == 0;
  }

  // Communicate record information to all ranks.
  controller->Broadcast(&error, 1, 0);
  if (error)
  {
    this->SubRecords.clear();
    return false;
  }
  if (rank == 0)
  {
    int count = static_cast<int>(this->SubRecords.size());
    controller->Broadcast(&count, 1, 0);
    if (count > 0)
    {
      controller->Broadcast(reinterpret_cast<svtkTypeUInt64*>(&this->SubRecords[0]), count * 2, 0);
    }
  }
  else
  {
    int count;
    controller->Broadcast(&count, 1, 0);
    this->SubRecords.resize(count);
    if (count > 0)
    {
      controller->Broadcast(reinterpret_cast<svtkTypeUInt64*>(&this->SubRecords[0]), count * 2, 0);
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
svtkMultiBlockPLOT3DReaderRecord::SubRecordSeparators
svtkMultiBlockPLOT3DReaderRecord::GetSubRecordSeparators(
  svtkTypeUInt64 startOffset, svtkTypeUInt64 length) const
{
  svtkMultiBlockPLOT3DReaderRecord::SubRecordSeparators markers;
  if (this->SubRecords.size() <= 1)
  {
    return markers;
  }

  // locate the sub-record in which startOffset begins.
  VectorOfSubRecords::const_iterator sr_start = this->SubRecords.begin();
  while (sr_start != this->SubRecords.end() && sr_start->FooterOffset < startOffset)
  {
    ++sr_start;
  }
  assert(sr_start != this->SubRecords.end());

  svtkTypeUInt64 endOffset = startOffset + length;

  VectorOfSubRecords::const_iterator sr_end = sr_start;
  while (sr_end != this->SubRecords.end() && sr_end->FooterOffset < endOffset)
  {
    markers.push_back(sr_end->FooterOffset);
    ++sr_end;
    assert(sr_end != this->SubRecords.end());
    endOffset += svtkMultiBlockPLOT3DReaderRecord::SubRecordSeparatorWidth;
  }
  assert(sr_end != this->SubRecords.end());
  return markers;
}

//-----------------------------------------------------------------------------
std::vector<std::pair<svtkTypeUInt64, svtkTypeUInt64> >
svtkMultiBlockPLOT3DReaderRecord::GetChunksToRead(
  svtkTypeUInt64 start, svtkTypeUInt64 length, const std::vector<svtkTypeUInt64>& markers)
{
  std::vector<std::pair<svtkTypeUInt64, svtkTypeUInt64> > chunks;
  for (size_t cc = 0; cc < markers.size(); ++cc)
  {
    if (start < markers[cc])
    {
      svtkTypeUInt64 chunksize = (markers[cc] - start);
      chunks.push_back(std::pair<svtkTypeUInt64, svtkTypeUInt64>(start, chunksize));
      length -= chunksize;
    }
    start = markers[cc] + svtkMultiBlockPLOT3DReaderRecord::SubRecordSeparatorWidth;
  }
  if (length > 0)
  {
    chunks.push_back(std::pair<svtkTypeUInt64, svtkTypeUInt64>(start, length));
  }
  return chunks;
}

//-----------------------------------------------------------------------------
svtkTypeUInt64 svtkMultiBlockPLOT3DReaderRecord::GetLengthWithSeparators(
  svtkTypeUInt64 start, svtkTypeUInt64 length) const
{
  return this->GetSubRecordSeparators(start, length).size() *
    svtkMultiBlockPLOT3DReaderRecord::SubRecordSeparatorWidth +
    length;
}
