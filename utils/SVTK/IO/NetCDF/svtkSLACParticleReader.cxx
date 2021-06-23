// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSLACParticleReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "svtkSLACParticleReader.h"

#include "svtkCallbackCommand.h"
#include "svtkCellArray.h"
#include "svtkCompositeDataIterator.h"
#include "svtkDataArraySelection.h"
#include "svtkDoubleArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

#include "svtk_netcdf.h"

//=============================================================================
#define CALL_NETCDF(call)                                                                          \
  {                                                                                                \
    int errorcode = call;                                                                          \
    if (errorcode != NC_NOERR)                                                                     \
    {                                                                                              \
      svtkErrorMacro(<< "netCDF Error: " << nc_strerror(errorcode));                                \
      return 0;                                                                                    \
    }                                                                                              \
  }

#define WRAP_NETCDF(call)                                                                          \
  {                                                                                                \
    int errorcode = call;                                                                          \
    if (errorcode != NC_NOERR)                                                                     \
      return errorcode;                                                                            \
  }

#ifdef SVTK_USE_64BIT_IDS
#ifdef NC_INT64
// This may or may not work with the netCDF 4 library reading in netCDF 3 files.
#if SVTK_ID_TYPE_IMPL == SVTK_LONG
#define nc_get_vars_svtkIdType nc_get_vars_long
#else
#define nc_get_vars_svtkIdType nc_get_vars_longlong
#endif
#else  // NC_INT64
static int nc_get_vars_svtkIdType(int ncid, int varid, const size_t start[], const size_t count[],
  const ptrdiff_t stride[], svtkIdType* ip)
{
  // Step 1, figure out how many entries in the given variable.
  int numdims;
  WRAP_NETCDF(nc_inq_varndims(ncid, varid, &numdims));
  svtkIdType numValues = 1;
  for (int dim = 0; dim < numdims; dim++)
  {
    numValues *= count[dim];
  }

  // Step 2, read the data in as 32 bit integers.  Recast the input buffer
  // so we do not have to create a new one.
  long* smallIp = reinterpret_cast<long*>(ip);
  WRAP_NETCDF(nc_get_vars_long(ncid, varid, start, count, stride, smallIp));

  // Step 3, recast the data from 32 bit integers to 64 bit integers.  Since we
  // are storing both in the same buffer, we need to be careful to not overwrite
  // uncopied 32 bit numbers with 64 bit numbers.  We can do that by copying
  // backwards.
  for (svtkIdType i = numValues - 1; i >= 0; i--)
  {
    ip[i] = static_cast<svtkIdType>(smallIp[i]);
  }

  return NC_NOERR;
}
#endif // NC_INT64
#else  // SVTK_USE_64_BIT_IDS
#define nc_get_vars_svtkIdType nc_get_vars_int
#endif // SVTK_USE_64BIT_IDS

// //=============================================================================
// static int NetCDFTypeToSVTKType(nc_type type)
// {
//   switch (type)
//     {
//     case NC_BYTE: return SVTK_UNSIGNED_CHAR;
//     case NC_CHAR: return SVTK_CHAR;
//     case NC_SHORT: return SVTK_SHORT;
//     case NC_INT: return SVTK_INT;
//     case NC_FLOAT: return SVTK_FLOAT;
//     case NC_DOUBLE: return SVTK_DOUBLE;
//     default:
//       svtkGenericWarningMacro(<< "Unknown netCDF variable type "
//                              << type);
//       return -1;
//     }
// }

//=============================================================================
// This class automatically closes a netCDF file descripter when it goes out
// of scope.  This allows us to exit on error without having to close the
// file at every instance.
class svtkSLACParticleReaderAutoCloseNetCDF
{
public:
  svtkSLACParticleReaderAutoCloseNetCDF(const char* filename, int omode, bool quiet = false)
  {
    int errorcode = nc_open(filename, omode, &this->fd);
    if (errorcode != NC_NOERR)
    {
      if (!quiet)
      {
        svtkGenericWarningMacro(<< "Could not open " << filename << endl << nc_strerror(errorcode));
      }
      this->fd = -1;
    }
  }
  ~svtkSLACParticleReaderAutoCloseNetCDF()
  {
    if (this->fd != -1)
    {
      nc_close(this->fd);
    }
  }
  int operator()() const { return this->fd; }
  bool Valid() const { return this->fd != -1; }

protected:
  int fd;

private:
  svtkSLACParticleReaderAutoCloseNetCDF() = delete;
  svtkSLACParticleReaderAutoCloseNetCDF(const svtkSLACParticleReaderAutoCloseNetCDF&) = delete;
  void operator=(const svtkSLACParticleReaderAutoCloseNetCDF&) = delete;
};

//=============================================================================
svtkStandardNewMacro(svtkSLACParticleReader);

//-----------------------------------------------------------------------------
svtkSLACParticleReader::svtkSLACParticleReader()
{
  this->SetNumberOfInputPorts(0);

  this->FileName = nullptr;
}

svtkSLACParticleReader::~svtkSLACParticleReader()
{
  this->SetFileName(nullptr);
}

void svtkSLACParticleReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->FileName)
  {
    os << indent << "FileName: " << this->FileName << endl;
  }
  else
  {
    os << indent << "FileName: (null)\n";
  }
}

//-----------------------------------------------------------------------------
int svtkSLACParticleReader::CanReadFile(const char* filename)
{
  svtkSLACParticleReaderAutoCloseNetCDF ncFD(filename, NC_NOWRITE, true);
  if (!ncFD.Valid())
    return 0;

  // Check for the existence of several arrays we know should be in the file.
  int dummy;
  if (nc_inq_varid(ncFD(), "particlePos", &dummy) != NC_NOERR)
    return 0;
  if (nc_inq_varid(ncFD(), "particleInfo", &dummy) != NC_NOERR)
    return 0;
  if (nc_inq_varid(ncFD(), "time", &dummy) != NC_NOERR)
    return 0;

  return 1;
}

//-----------------------------------------------------------------------------
svtkIdType svtkSLACParticleReader::GetNumTuplesInVariable(
  int ncFD, int varId, int expectedNumComponents)
{
  int numDims;
  CALL_NETCDF(nc_inq_varndims(ncFD, varId, &numDims));
  if (numDims != 2)
  {
    char name[NC_MAX_NAME + 1];
    CALL_NETCDF(nc_inq_varname(ncFD, varId, name));
    svtkErrorMacro(<< "Wrong dimensions on " << name);
    return 0;
  }

  int dimIds[2];
  CALL_NETCDF(nc_inq_vardimid(ncFD, varId, dimIds));

  size_t dimLength;
  CALL_NETCDF(nc_inq_dimlen(ncFD, dimIds[1], &dimLength));
  if (static_cast<int>(dimLength) != expectedNumComponents)
  {
    char name[NC_MAX_NAME + 1];
    CALL_NETCDF(nc_inq_varname(ncFD, varId, name));
    svtkErrorMacro(<< "Unexpected tuple size on " << name);
    return 0;
  }

  CALL_NETCDF(nc_inq_dimlen(ncFD, dimIds[0], &dimLength));
  return static_cast<svtkIdType>(dimLength);
}

//-----------------------------------------------------------------------------
int svtkSLACParticleReader::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  if (!this->FileName)
  {
    svtkErrorMacro("No filename specified.");
    return 0;
  }

  svtkSLACParticleReaderAutoCloseNetCDF ncFD(this->FileName, NC_NOWRITE);
  if (!ncFD.Valid())
    return 0;

  int timeVar;
  CALL_NETCDF(nc_inq_varid(ncFD(), "time", &timeVar));
  double timeValue;
  CALL_NETCDF(nc_get_var_double(ncFD(), timeVar, &timeValue));

  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(svtkStreamingDemandDrivenPipeline::TIME_STEPS(), &timeValue, 1);
  double timeRange[2];
  timeRange[0] = timeRange[1] = timeValue;
  outInfo->Set(svtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);

  return 1;
}

//-----------------------------------------------------------------------------
int svtkSLACParticleReader::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  svtkPolyData* output = svtkPolyData::GetData(outputVector);

  if (!this->FileName)
  {
    svtkErrorMacro("No filename specified.");
    return 0;
  }

  svtkSLACParticleReaderAutoCloseNetCDF ncFD(this->FileName, NC_NOWRITE);
  if (!ncFD.Valid())
    return 0;

  SVTK_CREATE(svtkPoints, points);

  int particlePosVar;
  CALL_NETCDF(nc_inq_varid(ncFD(), "particlePos", &particlePosVar));
  svtkIdType numParticles = this->GetNumTuplesInVariable(ncFD(), particlePosVar, 6);

  size_t start[2], count[2];
  start[0] = 0;
  count[0] = numParticles;
  start[1] = 0;
  count[1] = 3;

  SVTK_CREATE(svtkDoubleArray, coords);
  coords->SetNumberOfComponents(3);
  coords->SetNumberOfTuples(numParticles);
  CALL_NETCDF(
    nc_get_vars_double(ncFD(), particlePosVar, start, count, nullptr, coords->GetPointer(0)));
  points->SetData(coords);
  output->SetPoints(points);

  SVTK_CREATE(svtkDoubleArray, momentum);
  momentum->SetName("Momentum");
  momentum->SetNumberOfComponents(3);
  momentum->SetNumberOfTuples(numParticles);
  start[1] = 3;
  CALL_NETCDF(
    nc_get_vars_double(ncFD(), particlePosVar, start, count, nullptr, momentum->GetPointer(0)));
  output->GetPointData()->AddArray(momentum);

  int particleInfoVar;
  CALL_NETCDF(nc_inq_varid(ncFD(), "particleInfo", &particleInfoVar));
  start[1] = 0;
  count[1] = 1;

  SVTK_CREATE(svtkIdTypeArray, ids);
  ids->SetName("ParticleIds");
  ids->SetNumberOfComponents(1);
  ids->SetNumberOfTuples(numParticles);
  CALL_NETCDF(
    nc_get_vars_svtkIdType(ncFD(), particleInfoVar, start, count, nullptr, ids->GetPointer(0)));
  output->GetPointData()->SetGlobalIds(ids);

  SVTK_CREATE(svtkIntArray, emissionType);
  emissionType->SetName("EmissionType");
  emissionType->SetNumberOfComponents(1);
  emissionType->SetNumberOfTuples(numParticles);
  start[1] = 1;
  CALL_NETCDF(
    nc_get_vars_int(ncFD(), particleInfoVar, start, count, nullptr, emissionType->GetPointer(0)));
  output->GetPointData()->AddArray(emissionType);

  SVTK_CREATE(svtkCellArray, verts);
  verts->AllocateEstimate(numParticles, 1);
  for (svtkIdType i = 0; i < numParticles; i++)
  {
    verts->InsertNextCell(1, &i);
  }
  output->SetVerts(verts);

  int timeVar;
  CALL_NETCDF(nc_inq_varid(ncFD(), "time", &timeVar));
  double timeValue;
  CALL_NETCDF(nc_get_var_double(ncFD(), timeVar, &timeValue));
  output->GetInformation()->Set(svtkDataObject::DATA_TIME_STEP(), timeValue);

  return 1;
}
