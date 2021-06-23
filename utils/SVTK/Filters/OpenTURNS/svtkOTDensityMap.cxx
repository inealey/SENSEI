/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOTDensityMap.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOTDensityMap.h"

#include "svtkContourFilter.h"
#include "svtkDataArray.h"
#include "svtkDataArrayCollection.h"
#include "svtkDataObject.h"
#include "svtkDoubleArray.h"
#include "svtkImageData.h"
#include "svtkImagePermute.h"
#include "svtkInformation.h"
#include "svtkInformationDoubleKey.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkTable.h"

#include "svtkOTIncludes.h"
#include "svtkOTUtilities.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <vector>

svtkInformationKeyMacro(svtkOTDensityMap, DENSITY, Double);
svtkStandardNewMacro(svtkOTDensityMap);

using namespace OT;

class svtkOTDensityMap::OTDistributionCache
{
public:
  Distribution Cache;
};

class svtkOTDensityMap::OTDensityCache
{
public:
  OTDensityCache(Sample* cache)
    : Cache(cache)
  {
  }

  Sample* Cache;
};

//-----------------------------------------------------------------------------
svtkOTDensityMap::svtkOTDensityMap()
{
  this->SetNumberOfOutputPorts(2);
  this->ContourValues = svtkContourValues::New();
  this->GridSubdivisions = 50;
  this->ContourApproximationNumberOfPoints = 600;
  this->DensityLogPDFSampleCache = new svtkOTDensityMap::OTDensityCache(nullptr);
  this->DensityPDFCache = new svtkOTDensityMap::OTDensityCache(nullptr);
  this->DistributionCache = new svtkOTDensityMap::OTDistributionCache();
}

//-----------------------------------------------------------------------------
svtkOTDensityMap::~svtkOTDensityMap()
{
  this->ContourValues->Delete();
  this->ClearCache();
  delete this->DensityLogPDFSampleCache;
  delete this->DensityPDFCache;
  delete this->DistributionCache;
}

//-----------------------------------------------------------------------------
void svtkOTDensityMap::ClearCache()
{
  if (this->DensityLogPDFSampleCache->Cache != nullptr)
  {
    delete this->DensityLogPDFSampleCache->Cache;
    this->DensityLogPDFSampleCache->Cache = nullptr;
  }
  if (this->DensityPDFCache->Cache != nullptr)
  {
    delete this->DensityPDFCache->Cache;
    this->DensityPDFCache->Cache = nullptr;
  }
  this->DensityLogPDFSampleMTime.Modified();
  this->DensityPDFMTime.Modified();
}

//-----------------------------------------------------------------------------
void svtkOTDensityMap::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  this->ContourValues->PrintSelf(os, indent.GetNextIndent());
  os << indent << "GridSubdivisions: " << this->GridSubdivisions << endl;
  os << indent << "ContourApproximationNumberOfPoints: " << this->ContourApproximationNumberOfPoints
     << endl;
}

//-----------------------------------------------------------------------------
int svtkOTDensityMap::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  // Input is a table
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
  return 1;
}

//-----------------------------------------------------------------------------
int svtkOTDensityMap::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Recover output
  svtkMultiBlockDataSet* output = svtkMultiBlockDataSet::GetData(outputVector, 0);
  svtkImageData* imageOutput = svtkImageData::GetData(outputVector, 1);

  // Create Sample from input data array
  svtkDataArray* xArray = this->GetInputArrayToProcess(0, inputVector);
  svtkDataArray* yArray = this->GetInputArrayToProcess(1, inputVector);
  if (xArray == nullptr || yArray == nullptr)
  {
    svtkErrorMacro("Please define numeric arrays to process");
    return 0;
  }
  const char* xArrayName = xArray->GetName();
  const char* yArrayName = yArray->GetName();
  svtkNew<svtkDataArrayCollection> arrays;
  arrays->AddItem(xArray);
  arrays->AddItem(yArray);
  Sample* input = svtkOTUtilities::SingleDimArraysToSample(arrays);

  // Create the PDF Grid
  OT::Indices pointNumber(2, this->GridSubdivisions);
  Point pointMin;
  Point pointMax;
  pointMin = input->getMin();
  pointMax = input->getMax();

  // Check Density PDF cache time
  svtkMTimeType lastBuildTime = this->BuildTime.GetMTime();
  if (this->DensityPDFMTime.GetMTime() > lastBuildTime ||
    inputVector[0]->GetMTime() > lastBuildTime)
  {
    // Clear cache
    this->ClearCache();

    // Compute OpenTURNS PDF
    KernelSmoothing* ks = new KernelSmoothing();
    this->DistributionCache->Cache = ks->build(*input);
    Sample gridX(this->GridSubdivisions * this->GridSubdivisions, 2);
    this->DensityPDFCache->Cache =
      new Sample(this->DistributionCache->Cache.getImplementation()->computePDF(
        pointMin, pointMax, pointNumber, gridX));
    delete ks;
    // this->DensityPDFCache->Cache is now a this->GridSubdivisions*this->GridSubdivisions
    // serialized grid, containing density value for each point of the grid
  }

  // Check Density Log PDF sample cache time
  if (this->DensityLogPDFSampleMTime.GetMTime() > lastBuildTime)
  {
    if (this->DensityLogPDFSampleCache->Cache == nullptr)
    {
      const Sample xSample(
        this->DistributionCache->Cache.getSample(this->ContourApproximationNumberOfPoints));
      this->DensityLogPDFSampleCache->Cache =
        new Sample(this->DistributionCache->Cache.computeLogPDF(xSample));
    }
    else
    {
      // Here we reuse the previous values
      const int oldSize = this->DensityLogPDFSampleCache->Cache->getSize();
      const int newSize = this->ContourApproximationNumberOfPoints;
      // Test if we are asking for more points
      if (newSize > oldSize)
      {
        const Sample xSample(this->DistributionCache->Cache.getSample(newSize - oldSize));
        this->DensityLogPDFSampleCache->Cache->add(
          Sample(this->DistributionCache->Cache.computeLogPDF(xSample)));
      }
      else if (newSize < oldSize)
      {
        // This method keeps only the newSize first elements into the sample and returns the
        // remaining ones
        this->DensityLogPDFSampleCache->Cache->split(newSize);
      }
    }
  }

  // Store the density in a image
  svtkNew<svtkImageData> image;
  image->SetDimensions(this->GridSubdivisions, this->GridSubdivisions, 1);
  image->SetOrigin(pointMin[0], pointMin[1], 0);
  image->SetSpacing((pointMax[0] - pointMin[0]) / this->GridSubdivisions,
    (pointMax[1] - pointMin[1]) / this->GridSubdivisions, 0);

  svtkDataArray* density = svtkOTUtilities::SampleToArray(this->DensityPDFCache->Cache);
  density->SetName("Density");
  image->GetPointData()->SetScalars(density);
  density->Delete();

  // Create contour and set contour values
  svtkNew<svtkContourFilter> contour;
  contour->SetInputData(image);
  int numContours = this->ContourValues->GetNumberOfContours();
  contour->SetNumberOfContours(numContours);
  double* contourValues = this->ContourValues->GetValues();
  std::vector<double> densityPDFContourValues(numContours);

  for (int i = 0; i < numContours; i++)
  {
    double val =
      std::exp(this->DensityLogPDFSampleCache->Cache->computeQuantile(1.0 - contourValues[i])[0]);
    contour->SetValue(i, val);
    densityPDFContourValues[i] = val;
  }

  // Compute contour
  contour->Update();
  svtkPolyData* contourPd = contour->GetOutput();

  // A map to temporarily store the output
  std::multimap<double, svtkSmartPointer<svtkTable> > contoursMap;

  // Build contours tables
  this->BuildContours(contourPd, numContours, contourValues, densityPDFContourValues.data(),
    xArrayName, yArrayName, contoursMap);

  // Recover iterator to cache by creating the multiblock tree output
  // Initialize to maximum number of blocks
  output->SetNumberOfBlocks(contoursMap.size());
  int nBlock = 0;
  for (std::multimap<double, svtkSmartPointer<svtkTable> >::iterator it =
         contoursMap.begin(); // Iterate over multimap keys
       it != contoursMap.end(); it = contoursMap.upper_bound(it->first))
  {
    // For each key recover range of tables
    std::pair<std::multimap<double, svtkSmartPointer<svtkTable> >::iterator,
      std::multimap<double, svtkSmartPointer<svtkTable> >::iterator>
      range;
    range = contoursMap.equal_range(it->first);
    svtkNew<svtkMultiBlockDataSet> block;
    block->SetNumberOfBlocks(contoursMap.size());
    int nChildBlock = 0;
    // Put table for the same density in the some block
    for (std::multimap<double, svtkSmartPointer<svtkTable> >::iterator it2 = range.first;
         it2 != range.second; ++it2)
    {
      block->SetBlock(nChildBlock, it2->second);
      block->GetMetaData(nChildBlock)->Set(svtkOTDensityMap::DENSITY(), it2->first);
      nChildBlock++;
    }

    // Store block in output
    block->SetNumberOfBlocks(nChildBlock);
    output->SetBlock(nBlock, block);
    std::ostringstream strs;
    strs << it->first;
    output->GetMetaData(nBlock)->Set(svtkCompositeDataSet::NAME(), strs.str().c_str());
    nBlock++;
  }
  output->SetNumberOfBlocks(nBlock);

  svtkNew<svtkImagePermute> flipImage;
  flipImage->SetInputData(image);
  flipImage->SetFilteredAxes(1, 0, 2);
  flipImage->Update();
  imageOutput->ShallowCopy(flipImage->GetOutput());

  // Store Build Time for cache
  this->BuildTime.Modified();

  delete input;
  return 1;
}

//----------------------------------------------------------------------------
int svtkOTDensityMap::FillOutputPortInformation(int port, svtkInformation* info)
{
  if (port == 1)
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkImageData");
    return 1;
  }
  return this->Superclass::FillOutputPortInformation(port, info);
}

//----------------------------------------------------------------------------
void svtkOTDensityMap::BuildContours(svtkPolyData* contourPd, int numContours,
  const double* contourValues, const double* densityPDFContourValues, const char* xArrayName,
  const char* yArrayName, std::multimap<double, svtkSmartPointer<svtkTable> >& contoursMap)
{
  std::set<svtkIdType> treatedCells;
  svtkNew<svtkIdList> pointIndices;
  svtkPoints* points = contourPd->GetPoints();

  // Try all cells
  for (svtkIdType cellId = 0; cellId < contourPd->GetNumberOfCells(); cellId++)
  {
    // Pick an untreated cell from the contour polydata
    if (treatedCells.find(cellId) != treatedCells.end())
    {
      continue;
    }

    // Create a table containing the X and Y of the point of this contour
    svtkNew<svtkDoubleArray> x;
    svtkNew<svtkDoubleArray> y;
    svtkSmartPointer<svtkTable> table = svtkSmartPointer<svtkTable>::New();
    table->AddColumn(x);
    table->AddColumn(y);

    // Using neighbor, try to find a cell which is the beginning of the line,
    // or go full circle
    svtkIdType initialCellId = cellId;
    svtkIdType previousCellId = -1;
    svtkIdType nextCellId;
    bool inverted;
    do
    {
      nextCellId = this->FindNextCellId(contourPd, initialCellId, previousCellId, inverted);
      previousCellId = initialCellId;
      initialCellId = nextCellId;
    } while (initialCellId != -1 && initialCellId != cellId);

    // Using this cell, go along the line to fill up the X and Y arrays
    svtkIdType alongCellId = previousCellId;
    svtkIdType pointId = -1;
    previousCellId = -1;
    do
    {
      // Find the next cell and recover current cell point indices
      pointIndices->Reset();
      nextCellId =
        this->FindNextCellId(contourPd, alongCellId, previousCellId, inverted, false, pointIndices);
      svtkIdType nPoints = pointIndices->GetNumberOfIds();

      // If this is the first or final cell, store all points
      // If not, do not store the last point
      bool allPoints = previousCellId == -1 || nextCellId == -1 || alongCellId == initialCellId;
      if (!allPoints)
      {
        nPoints--;
      }
      for (svtkIdType i = 0; i < nPoints; i++)
      {
        double point[3];

        // Some cells may have inverted points
        if (inverted)
        {
          pointId = pointIndices->GetId(pointIndices->GetNumberOfIds() - 1 - i);
        }
        else
        {
          pointId = pointIndices->GetId(i);
        }
        // Store the point in table
        points->GetPoint(pointId, point);
        x->InsertNextTuple1(point[0]);
        y->InsertNextTuple1(point[1]);
      }

      // Add treated cell to set, and go to next cell
      treatedCells.insert(alongCellId);
      previousCellId = alongCellId;
      alongCellId = nextCellId;
    } while (alongCellId != -1 && previousCellId != initialCellId);

    // Recover contour density value using the data at last point
    double densityVal = contourPd->GetPointData()->GetArray(0)->GetTuple1(pointId);
    double contourValue = -1;

    // Recover contour
    for (int i = 0; i < numContours; i++)
    {
      if (densityVal == densityPDFContourValues[i])
      {
        contourValue = contourValues[i];
        break;
      }
    }
    if (contourValue == -1)
    {
      svtkWarningMacro("Cannot find density in inverted values, metadata will be incorrect");
    }

    // Set arrays name
    x->SetName(xArrayName);
    std::stringstream yArrayFullName;
    yArrayFullName << yArrayName << " " << std::setfill(' ') << std::setw(3)
                   << static_cast<int>(contourValue * 100 + 0.5) << "%";
    y->SetName(yArrayFullName.str().c_str());

    // add table to cache
    contoursMap.insert(std::make_pair(contourValue, table));
  }
}

//----------------------------------------------------------------------------
svtkIdType svtkOTDensityMap::FindNextCellId(svtkPolyData* pd, svtkIdType cellId,
  svtkIdType previousCellId, bool& invertedPoints, bool up, svtkIdList* currentCellPoints)
{
  // Initialize
  invertedPoints = false;
  svtkIdList* localCellPoints = currentCellPoints;
  if (localCellPoints == nullptr)
  {
    localCellPoints = svtkIdList::New();
  }

  // Recover current cell points
  pd->GetCellPoints(cellId, localCellPoints);

  svtkNew<svtkIdList> edgePt;
  svtkNew<svtkIdList> edgeCells;
  svtkIdType nCells;
  svtkIdType nextCellId = -1;
  svtkIdType localNextCellId;

  // If up is true
  // First pass we try with the first point
  // Second pass we try with the last point
  // if up is false
  // First pass we try with the last point
  // Second pass we try with the first point
  for (int nPass = 0; nPass < 2; nPass++)
  {
    // Recover a point index at an extremity
    svtkIdType localPtIndex =
      (up ? nPass : (nPass + 1) % 2) * (localCellPoints->GetNumberOfIds() - 1);
    edgePt->InsertNextId(localCellPoints->GetId(localPtIndex));

    // Recover cell neighbors at this extremity
    pd->GetCellNeighbors(cellId, edgePt, edgeCells);
    edgePt->Reset();
    nCells = edgeCells->GetNumberOfIds();

    // If we get a neighbor
    if (nCells >= 1)
    {
      // recover it's id
      localNextCellId = edgeCells->GetId(0);

      // check it is not previous cell
      if (localNextCellId != previousCellId)
      {
        nextCellId = localNextCellId;
        break;
      }
      else
      {
        // First pass did not work, cell is inverted
        invertedPoints = true;
      }
    }
  }
  if (currentCellPoints == nullptr)
  {
    localCellPoints->Delete();
  }
  return nextCellId;
}

//----------------------------------------------------------------------------
void svtkOTDensityMap::SetGridSubdivisions(int gridSubdivisions)
{
  if (this->GridSubdivisions != gridSubdivisions)
  {
    this->GridSubdivisions = gridSubdivisions;
    this->DensityPDFMTime.Modified();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkOTDensityMap::SetContourApproximationNumberOfPoints(int val)
{
  if (this->ContourApproximationNumberOfPoints != val)
  {
    this->ContourApproximationNumberOfPoints = val;
    this->DensityLogPDFSampleMTime.Modified();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkOTDensityMap::SetValue(int i, double value)
{
  this->ContourValues->SetValue(i, value);
}

//----------------------------------------------------------------------------
double svtkOTDensityMap::GetValue(int i)
{
  return this->ContourValues->GetValue(i);
}

//----------------------------------------------------------------------------
double* svtkOTDensityMap::GetValues()
{
  return this->ContourValues->GetValues();
}

//----------------------------------------------------------------------------
void svtkOTDensityMap::GetValues(double* contourValues)
{
  this->ContourValues->GetValues(contourValues);
}

//----------------------------------------------------------------------------
void svtkOTDensityMap::SetNumberOfContours(int number)
{
  this->ContourValues->SetNumberOfContours(number);
}

//----------------------------------------------------------------------------
int svtkOTDensityMap::GetNumberOfContours()
{
  return this->ContourValues->GetNumberOfContours();
}

//----------------------------------------------------------------------------
svtkMTimeType svtkOTDensityMap::GetMTime()
{
  return svtkMath::Max(this->Superclass::GetMTime(), this->ContourValues->GetMTime());
}
