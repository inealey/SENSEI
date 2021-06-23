/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPStreamTracer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPStreamTracer.h"

#include "svtkAMRInterpolatedVelocityField.h"
#include "svtkAbstractInterpolatedVelocityField.h"
#include "svtkAppendPolyData.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCharArray.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkMPIController.h"
#include "svtkMath.h"
#include "svtkMultiProcessController.h"
#include "svtkMultiProcessStream.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkOverlappingAMR.h"
#include "svtkParallelAMRUtilities.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkRungeKutta2.h"
#include "svtkSMPTools.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTimerLog.h"
#include "svtkUniformGrid.h"

#include <algorithm>
#include <cassert>
#include <list>
#include <numeric>
#include <vector>

#ifndef NDEBUG
// #define DEBUGTRACE
// #define LOGTRACE
#else
#endif

#ifdef DEBUGTRACE
#define PRINT(x)                                                                                   \
  {                                                                                                \
    cout << this->Rank << ")" << x << endl;                                                        \
  }
#define ALLPRINT(x)                                                                                \
  for (int rank = 0; rank < NumProcs; rank++)                                                      \
  {                                                                                                \
    Controller->Barrier();                                                                         \
    if (rank == Rank)                                                                              \
      cout << "(" << this->Rank << ")" << x << endl;                                               \
  }
#define Assert(a, msg)                                                                             \
  {                                                                                                \
    if (!a)                                                                                        \
    {                                                                                              \
      cerr << msg << endl;                                                                         \
      assert(false);                                                                               \
    }                                                                                              \
  }

#define AssertEq(a, b)                                                                             \
  {                                                                                                \
    if (a != b)                                                                                    \
    {                                                                                              \
      cerr << a << " != " << b << endl;                                                            \
      assert(false);                                                                               \
    }                                                                                              \
  }

#define AssertNe(a, b)                                                                             \
  {                                                                                                \
    if (a == b)                                                                                    \
    {                                                                                              \
      cerr << a << " == " << b << endl;                                                            \
      assert(false);                                                                               \
    }                                                                                              \
  }

#define AssertGe(a, b)                                                                             \
  {                                                                                                \
    if (a < b)                                                                                     \
    {                                                                                              \
      cerr << a << " < " << b << endl;                                                             \
      assert(false);                                                                               \
    }                                                                                              \
  }

#define AssertGt(a, b)                                                                             \
  {                                                                                                \
    if (a <= b)                                                                                    \
    {                                                                                              \
      cerr << a << " < " << b << endl;                                                             \
      assert(false);                                                                               \
    }                                                                                              \
  }
//#define PRINT(id, x)
#else
#define PRINT(x)
#define ALLPRINT(x)

#define AssertEq(a, b)
#define AssertGt(a, b)
#define AssertGe(a, b)
#define Assert(a, b)
#define AssertNe(a, b)
#endif

namespace
{
inline int CNext(int i, int n)
{
  return (i + 1) % n;
}

class MyStream
{
public:
  MyStream(int BufferSize)
    : Size(BufferSize)
  {
    this->Data = new char[Size];
    this->Head = Data;
  }
  ~MyStream() { delete[] this->Data; }
  int GetSize() { return Size; }

  template <class T>
  MyStream& operator<<(T t)
  {
    unsigned int size = sizeof(T);
    char* value = reinterpret_cast<char*>(&t);
    for (unsigned int i = 0; i < size; i++)
    {
      AssertGe(Size, this->Head - this->Data);
      *(this->Head++) = (*(value++));
    }
    return (*this);
  }

  template <class T>
  MyStream& operator>>(T& t)
  {
    unsigned int size = sizeof(T);
    AssertGe(Size, this->Head + size - this->Data);
    t = *(reinterpret_cast<T*>(this->Head));
    this->Head += size;
    return (*this);
  }

  char* GetRawData() { return this->Data; }
  int GetLength() { return this->Head - this->Data; }

  void Reset() { this->Head = this->Data; }

private:
  MyStream(const MyStream&) = delete;
  void operator=(const MyStream&) = delete;

  char* Data;
  char* Head;
  int Size;
};

inline void InitBB(double* Bounds)
{
  Bounds[0] = DBL_MAX;
  Bounds[1] = -DBL_MAX;
  Bounds[2] = DBL_MAX;
  Bounds[3] = -DBL_MAX;
  Bounds[4] = DBL_MAX;
  Bounds[5] = -DBL_MAX;
}

inline bool InBB(const double* x, const double* bounds)
{
  return bounds[0] <= x[0] && x[0] <= bounds[1] && bounds[2] <= x[1] && x[1] <= bounds[3] &&
    bounds[4] <= x[2] && x[2] <= bounds[5];
}

inline void UpdateBB(double* a, const double* b)
{
  for (int i = 0; i <= 4; i += 2)
  {
    if (b[i] < a[i])
    {
      a[i] = b[i];
    }
  }
  for (int i = 1; i <= 5; i += 2)
  {
    if (b[i] > a[i])
    {
      a[i] = b[i];
    }
  }
}
}

class PStreamTracerPoint : public svtkObject
{
public:
  svtkTypeMacro(PStreamTracerPoint, svtkObject);

  static PStreamTracerPoint* New();

  svtkGetMacro(Id, int);
  svtkGetVector3Macro(Seed, double);
  svtkGetVector3Macro(Normal, double);
  svtkGetMacro(Direction, int);
  svtkGetMacro(NumSteps, int);
  svtkGetMacro(Propagation, double);
  svtkGetMacro(Rank, int);
  svtkGetMacro(IntegrationTime, double);

  svtkSetMacro(Id, int);
  svtkSetMacro(Direction, int);
  svtkSetVector3Macro(Seed, double);
  svtkSetMacro(NumSteps, int);
  svtkSetMacro(Propagation, double);
  svtkSetMacro(Rank, int);
  svtkSetMacro(IntegrationTime, double);

  void Reseed(double* seed, double* normal, svtkPolyData* poly, int id, double propagation,
    double integrationTime)
  {
    memcpy(this->Seed, seed, 3 * sizeof(double));
    memcpy(this->Normal, normal, 3 * sizeof(double));

    this->AllocateTail(poly->GetPointData());
    double* x = poly->GetPoints()->GetPoint(id);
    this->Tail->GetPoints()->SetPoint(0, x);
    this->Tail->GetPointData()->CopyData(poly->GetPointData(), id, 0);
    this->Rank = -1; // someone else figure this out
    this->IntegrationTime = integrationTime;
    this->Propagation = propagation;
  }

  svtkPolyData* GetTail() { return this->Tail; }

  void CopyTail(PStreamTracerPoint* other)
  {
    if (other->Tail)
    {
      svtkPointData* pd = other->Tail->GetPointData();
      if (!this->Tail)
      {
        AllocateTail(pd);
      }
      this->Tail->GetPointData()->DeepCopy(pd);
    }
    else
    {
      Tail = nullptr;
    }
  }

  // allocate a one point svtkPolyData whose PointData setup matches pd
  void AllocateTail(svtkPointData* pd)
  {
    if (!this->Tail)
    {
      Tail = svtkSmartPointer<svtkPolyData>::New();
      svtkNew<svtkPoints> points;
      {
        points->SetNumberOfPoints(1);
      }
      Tail->SetPoints(points);
    }

    this->Tail->GetPointData()->CopyAllocate(pd);
  }

  virtual int GetSize()
  {
    int size(0);
    svtkPointData* data = this->GetTail()->GetPointData();
    for (int i = 0; i < data->GetNumberOfArrays(); i++)
    {
      size += data->GetArray(i)->GetNumberOfComponents();
    }
    return size * sizeof(double) + sizeof(PStreamTracerPoint);
  }

  virtual void Read(MyStream& stream)
  {
    stream >> this->Id;
    stream >> this->Seed[0];
    stream >> this->Seed[1];
    stream >> this->Seed[2];
    stream >> this->Direction;
    stream >> this->NumSteps;
    stream >> this->Propagation;
    stream >> this->IntegrationTime;

    char hasTail(0);
    stream >> hasTail;
    if (hasTail)
    {
      double x[3];
      for (int i = 0; i < 3; i++)
      {
        stream >> x[i];
      }
      AssertNe(this->Tail, nullptr); // someone should have allocated it by prototype
      this->Tail->SetPoints(svtkSmartPointer<svtkPoints>::New());
      this->Tail->GetPoints()->InsertNextPoint(x);

      svtkPointData* pointData = this->Tail->GetPointData();
      for (int i = 0; i < pointData->GetNumberOfArrays(); i++)
      {
        int numComponents = pointData->GetArray(i)->GetNumberOfComponents();
        std::vector<double> xi(numComponents);
        for (int j = 0; j < numComponents; j++)
        {
          double& xj(xi[j]);
          stream >> xj;
        }
        pointData->GetArray(i)->InsertNextTuple(&xi[0]);
      }
    }
    else
    {
      this->Tail = nullptr;
    }
  }

  virtual void Write(MyStream& stream)
  {
    stream << this->Id << this->Seed[0] << this->Seed[1] << this->Seed[2] << this->Direction
           << this->NumSteps << this->Propagation << this->IntegrationTime;

    stream << (char)(this->Tail != nullptr);

    if (this->Tail)
    {
      double* x = this->Tail->GetPoints()->GetPoint(0);
      for (int i = 0; i < 3; i++)
      {
        stream << x[i];
      }
      svtkPointData* pData = this->Tail->GetPointData();
      int numArrays(pData->GetNumberOfArrays());
      for (int i = 0; i < numArrays; i++)
      {
        svtkDataArray* arr = pData->GetArray(i);
        int numComponents = arr->GetNumberOfComponents();
        double* y = arr->GetTuple(0);
        for (int j = 0; j < numComponents; j++)
          stream << y[j];
      }
    }
  }

private:
  int Id;
  double Seed[3];
  double Normal[3];
  int Direction;
  int NumSteps;
  double Propagation;
  svtkSmartPointer<svtkPolyData> Tail;
  int Rank;
  double IntegrationTime;

protected:
  PStreamTracerPoint()
    : Id(-1)
    , Direction(0)
    , NumSteps(0)
    , Propagation(0)
    , Rank(-1)
    , IntegrationTime(0)
  {
    this->Seed[0] = this->Seed[1] = this->Seed[2] = -999;
  }
};

svtkStandardNewMacro(PStreamTracerPoint);

class AMRPStreamTracerPoint : public PStreamTracerPoint
{
public:
  svtkTypeMacro(AMRPStreamTracerPoint, PStreamTracerPoint);

  static AMRPStreamTracerPoint* New();
  svtkSetMacro(Level, int);
  svtkGetMacro(Level, int);

  svtkSetMacro(GridId, int);
  svtkGetMacro(GridId, int);

  virtual int GetSize() override { return PStreamTracerPoint::GetSize() + 2 * sizeof(int); }

  virtual void Read(MyStream& stream) override
  {
    PStreamTracerPoint::Read(stream);
    stream >> Level;
    stream >> GridId;
  }
  virtual void Write(MyStream& stream) override
  {
    PStreamTracerPoint::Write(stream);
    stream << Level << GridId;
  }

private:
  AMRPStreamTracerPoint()
    : Level(-1)
    , GridId(-1)
  {
  }
  int Level;
  int GridId;
};

typedef std::vector<svtkSmartPointer<PStreamTracerPoint> > PStreamTracerPointArray;

svtkStandardNewMacro(AMRPStreamTracerPoint);

class ProcessLocator : public svtkObject
{
public:
  svtkTypeMacro(ProcessLocator, svtkObject);
  static ProcessLocator* New();
  void Initialize(svtkCompositeDataSet* data)
  {
    this->Controller = svtkMultiProcessController::GetGlobalController();
    this->Rank = this->Controller->GetLocalProcessId();
    this->NumProcs = this->Controller->GetNumberOfProcesses();
    this->InitBoundingBoxes(this->NumProcs);

    double bb[6];
    InitBB(bb);

    if (data)
    {
      svtkCompositeDataIterator* iter = data->NewIterator();
      iter->InitTraversal();
      while (!iter->IsDoneWithTraversal())
      {
        svtkDataSet* dataSet = svtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
        AssertNe(dataSet, nullptr);
        UpdateBB(bb, dataSet->GetBounds());
        iter->GoToNextItem();
      }
      iter->Delete();
    }

    PRINT(bb[0] << " " << bb[1] << " " << bb[2] << " " << bb[3] << " " << bb[4] << " " << bb[5]);
    this->Controller->AllGather(bb, &this->BoundingBoxes[0], 6);

#ifdef DEBUGTRACE
    cout << "(" << Rank << ") BoundingBoxes: ";
    for (int i = 0; i < NumProcs; i++)
    {
      double* box = this->GetBoundingBox(i);
      cout << box[0] << " " << box[1] << " " << box[2] << " " << box[3] << " " << box[4] << " "
           << box[5] << ";  ";
    }
    cout << endl;
#endif
  }

  bool InCurrentProcess(double* p) { return InBB(p, GetBoundingBox(Rank)); }
  int FindNextProcess(double* p)
  {
    for (int rank = CNext(this->Rank, this->NumProcs); rank != Rank;
         rank = CNext(rank, this->NumProcs))
    {
      if (InBB(p, GetBoundingBox(rank)))
      {
        return rank;
      }
    }
    return -1;
  }

private:
  ProcessLocator()
  {
    this->Controller = nullptr;
    this->NumProcs = 0;
    this->Rank = 0;
  }
  svtkMultiProcessController* Controller;
  int Rank;
  int NumProcs;

  double* GetBoundingBox(int i) { return &BoundingBoxes[6 * i]; }

  void InitBoundingBoxes(int num)
  {
    for (int i = 0; i < 6 * num; i++)
    {
      this->BoundingBoxes.push_back(0);
    }
  }
  std::vector<double> BoundingBoxes;
};
svtkStandardNewMacro(ProcessLocator);

class AbstractPStreamTracerUtils : public svtkObject
{
public:
  svtkTypeMacro(AbstractPStreamTracerUtils, svtkObject);

  svtkGetMacro(VecName, char*);
  svtkGetMacro(VecType, int);
  svtkGetMacro(Input0, svtkDataSet*);

  virtual ProcessLocator* GetProcessLocator() { return nullptr; }

  svtkSmartPointer<PStreamTracerPoint> GetProto() { return this->Proto; }

  virtual void InitializeVelocityFunction(
    PStreamTracerPoint*, svtkAbstractInterpolatedVelocityField*)
  {
    return;
  }

  virtual bool PreparePoint(PStreamTracerPoint*, svtkAbstractInterpolatedVelocityField*)
  {
    return true;
  }

  svtkSmartPointer<svtkIdList> ComputeSeeds(
    svtkDataSet* source, PStreamTracerPointArray& out, int& maxId)
  {
    svtkDataArray* seeds;
    svtkIdList* seedIds;
    svtkIntArray* integrationDirections;
    this->Tracer->InitializeSeeds(seeds, seedIds, integrationDirections, source);

    int numSeeds = seedIds->GetNumberOfIds();
    for (int i = 0; i < numSeeds; i++)
    {
      double seed[3];
      seeds->GetTuple(seedIds->GetId(i), seed);
      svtkSmartPointer<PStreamTracerPoint> point =
        NewPoint(i, seed, integrationDirections->GetValue(i));
      if (this->InBound(point))
      {
        out.push_back(point);
      }
    }
    if (seeds)
    {
      seeds->Delete();
    }
    if (integrationDirections)
    {
      integrationDirections->Delete();
    }

    maxId = numSeeds - 1;
    return svtkSmartPointer<svtkIdList>::Take(seedIds);
  }

  virtual void Initialize(svtkPStreamTracer* tracer)
  {
    this->Tracer = tracer;
    this->Controller = tracer->Controller;
    this->Rank = tracer->Rank;
    this->NumProcs = tracer->NumProcs;
    this->InputData = tracer->InputData;
    this->VecType = 0;
    this->VecName = nullptr;
    this->Input0 = 0;
    if (!tracer->EmptyData)
    {
      svtkCompositeDataIterator* iter = tracer->InputData->NewIterator();
      svtkSmartPointer<svtkCompositeDataIterator> iterP(iter);
      iter->Delete();
      iterP->GoToFirstItem();
      if (!iterP->IsDoneWithTraversal())
      {
        Input0 = svtkDataSet::SafeDownCast(iterP->GetCurrentDataObject());
        // iterP->GotoNextitem();
      }
      svtkDataArray* vectors = tracer->GetInputArrayToProcess(0, this->Input0, this->VecType);
      this->VecName = vectors->GetName();
    }

    if (!tracer->EmptyData)
    {
      this->CreatePrototype(this->Input0->GetPointData(), VecType, VecName);
    }
  }

protected:
  AbstractPStreamTracerUtils()
    : Tracer(nullptr)
    , Controller(nullptr)
    , VecName(nullptr)
    , Input0(nullptr)
    , InputData(nullptr)
  {
  }

  virtual svtkSmartPointer<PStreamTracerPoint> NewPoint(int id, double* x, int dir) = 0;
  virtual bool InBound(PStreamTracerPoint* p) = 0;

  void CreatePrototype(svtkPointData* pointData, int fieldType, const char* vecName)
  {
    this->Proto = NewPoint(-1, nullptr, -1);

    svtkNew<svtkPointData> protoPD;
    protoPD->InterpolateAllocate(pointData, 1);
    svtkSmartPointer<svtkDoubleArray> time = svtkSmartPointer<svtkDoubleArray>::New();
    time->SetName("IntegrationTime");
    protoPD->AddArray(time);

    if (fieldType == svtkDataObject::FIELD_ASSOCIATION_CELLS)
    {
      svtkSmartPointer<svtkDoubleArray> velocityVectors = svtkSmartPointer<svtkDoubleArray>::New();
      velocityVectors->SetName(vecName);
      velocityVectors->SetNumberOfComponents(3);
      protoPD->AddArray(velocityVectors);
    }

    if (Tracer->GetComputeVorticity())
    {
      svtkSmartPointer<svtkDoubleArray> vorticity = svtkSmartPointer<svtkDoubleArray>::New();
      vorticity->SetName("Vorticity");
      vorticity->SetNumberOfComponents(3);
      protoPD->AddArray(vorticity);

      svtkSmartPointer<svtkDoubleArray> rotation = svtkSmartPointer<svtkDoubleArray>::New();
      rotation->SetName("Rotation");
      protoPD->AddArray(rotation);

      svtkSmartPointer<svtkDoubleArray> angularVel = svtkSmartPointer<svtkDoubleArray>::New();
      angularVel->SetName("AngularVelocity");
      protoPD->AddArray(angularVel);
    }

    if (Tracer->GenerateNormalsInIntegrate)
    {
      PRINT("Generate normals prototype");
      svtkSmartPointer<svtkDoubleArray> normals = svtkSmartPointer<svtkDoubleArray>::New();
      normals->SetName("Normals");
      normals->SetNumberOfComponents(3);
      protoPD->AddArray(normals);
    }
    AssertEq(this->Proto->GetTail(), nullptr);
    this->Proto->AllocateTail(protoPD);
  }

  svtkPStreamTracer* Tracer;
  svtkMultiProcessController* Controller;
  svtkSmartPointer<PStreamTracerPoint> Proto;
  int VecType;
  char* VecName;
  svtkDataSet* Input0;
  svtkCompositeDataSet* InputData;

  int Rank;
  int NumProcs;
};

class PStreamTracerUtils : public AbstractPStreamTracerUtils
{
public:
  svtkTypeMacro(PStreamTracerUtils, AbstractPStreamTracerUtils);

  static PStreamTracerUtils* New();

  PStreamTracerUtils() { this->Locator = nullptr; }

  virtual void Initialize(svtkPStreamTracer* tracer) override
  {
    this->Superclass::Initialize(tracer);
    this->Locator = svtkSmartPointer<ProcessLocator>::New();
    this->Locator->Initialize(tracer->InputData);
  }

  virtual ProcessLocator* GetProcessLocator() override { return this->Locator; }

  virtual bool InBound(PStreamTracerPoint*) override { return true; }

  virtual svtkSmartPointer<PStreamTracerPoint> NewPoint(int id, double* x, int dir) override
  {
    svtkSmartPointer<PStreamTracerPoint> p = svtkSmartPointer<PStreamTracerPoint>::New();
    p->SetId(id);
    if (x)
    {
      p->SetSeed(x);
    }
    p->SetDirection(dir);
    return p;
  }

private:
  svtkSmartPointer<ProcessLocator> Locator;
};

svtkStandardNewMacro(PStreamTracerUtils);

class AMRPStreamTracerUtils : public AbstractPStreamTracerUtils
{
public:
  svtkTypeMacro(AMRPStreamTracerUtils, AbstractPStreamTracerUtils);
  static AMRPStreamTracerUtils* New();
  svtkSetMacro(AMR, svtkOverlappingAMR*);

  virtual void InitializeVelocityFunction(
    PStreamTracerPoint* point, svtkAbstractInterpolatedVelocityField* func) override
  {
    AMRPStreamTracerPoint* amrPoint = AMRPStreamTracerPoint::SafeDownCast(point);
    assert(amrPoint);
    svtkAMRInterpolatedVelocityField* amrFunc = svtkAMRInterpolatedVelocityField::SafeDownCast(func);
    assert(amrFunc);
    if (amrPoint->GetLevel() >= 0)
    {
      amrFunc->SetLastDataSet(amrPoint->GetLevel(), amrPoint->GetGridId());
#ifdef DEBUGTRACE
      svtkUniformGrid* grid = this->AMR->GetDataSet(amrPoint->GetLevel(), amrPoint->GetGridId());
      if (!grid || !InBB(amrPoint->GetSeed(), grid->GetBounds()))
      {
        PRINT("WARNING: Bad AMR Point "
          << (grid) << " " << amrPoint->GetSeed()[0] << " " << amrPoint->GetSeed()[1] << " "
          << amrPoint->GetSeed()[2] << " " << amrPoint->GetLevel() << " " << amrPoint->GetGridId());
      }
#endif
    }
  }

  virtual bool PreparePoint(
    PStreamTracerPoint* point, svtkAbstractInterpolatedVelocityField* func) override
  {
    AMRPStreamTracerPoint* amrPoint = AMRPStreamTracerPoint::SafeDownCast(point);
    svtkAMRInterpolatedVelocityField* amrFunc = svtkAMRInterpolatedVelocityField::SafeDownCast(func);
    unsigned int level, id;
    if (amrFunc->GetLastDataSetLocation(level, id))
    {
      amrPoint->SetLevel(level);
      amrPoint->SetId(id);
      int blockIndex = this->AMR->GetCompositeIndex(level, id);
      amrPoint->SetRank(this->BlockProcess[blockIndex]);
      return true;
    }
    else
    {
      PRINT("Invalid AMR : " << point->GetSeed()[0] << " " << point->GetSeed()[1] << " "
                             << point->GetSeed()[2] << " "
                             << "Probably out of bound");
      amrPoint->SetLevel(-1);
      amrPoint->SetGridId(-1);
      amrPoint->SetRank(-1);
      return false;
    }
  }

  // this assume that p's AMR information has been set correctly
  // it makes no attempt to look for it
  virtual bool InBound(PStreamTracerPoint* p) override
  {
    AMRPStreamTracerPoint* amrp = AMRPStreamTracerPoint::SafeDownCast(p);
    if (amrp->GetLevel() < 0)
    {
      return false;
    }
    AssertNe(amrp, nullptr);
    svtkUniformGrid* grid = this->AMR->GetDataSet(amrp->GetLevel(), amrp->GetGridId());
    return grid != nullptr;
  }

  virtual svtkSmartPointer<PStreamTracerPoint> NewPoint(int id, double* x, int dir) override
  {

    svtkSmartPointer<AMRPStreamTracerPoint> amrp = svtkSmartPointer<AMRPStreamTracerPoint>::New();
    svtkSmartPointer<PStreamTracerPoint> p = amrp;
    p->SetId(id);
    if (x)
    {
      p->SetSeed(x);
    }
    p->SetDirection(dir);

    if (x)
    {
      unsigned int level, gridId;
      if (svtkAMRInterpolatedVelocityField::FindGrid(x, this->AMR, level, gridId))
      {
        amrp->SetLevel((int)level);
        amrp->SetGridId((int)gridId);
        int blockIndex = this->AMR->GetCompositeIndex(level, gridId);
        int process = this->BlockProcess[blockIndex];
        AssertGe(process, 0);
        amrp->SetRank(process);
      }
      else
      {
      }
    }

    return p;
  }
  virtual void Initialize(svtkPStreamTracer* tracer) override
  {
    this->Superclass::Initialize(tracer);
    AssertNe(this->InputData, nullptr);
    this->AMR = svtkOverlappingAMR::SafeDownCast(this->InputData);

    svtkParallelAMRUtilities::DistributeProcessInformation(
      this->AMR, this->Controller, BlockProcess);
    this->AMR->GenerateParentChildInformation();
  }

protected:
  AMRPStreamTracerUtils() { this->AMR = nullptr; }
  svtkOverlappingAMR* AMR;

  std::vector<int> BlockProcess; // stores block->process information
};
svtkStandardNewMacro(AMRPStreamTracerUtils);

namespace
{
inline double normvec3(double* x, double* y)
{
  return sqrt(
    (x[0] - y[0]) * (x[0] - y[0]) + (x[1] - y[1]) * (x[1] - y[1]) + (x[2] - y[2]) * (x[2] - y[2]));
}

inline svtkIdType LastPointIndex(svtkPolyData* pathPoly)
{
  svtkCellArray* pathCells = pathPoly->GetLines();
  AssertGt(pathCells->GetNumberOfCells(), 0);
  const svtkIdType* path(0);
  svtkIdType nPoints(0);
  pathCells->InitTraversal();
  pathCells->GetNextCell(nPoints, path);
  int lastPointIndex = path[nPoints - 1];
  return lastPointIndex;
}

#ifdef DEBUGTRACE
inline double ComputeLength(svtkIdList* poly, svtkPoints* pts)
{
  int n = poly->GetNumberOfIds();
  if (n == 0)
    return 0;

  double s(0);
  double p[3];
  pts->GetPoint(poly->GetId(0), p);
  for (int j = 1; j < n; j++)
  {
    int pIndex = poly->GetId(j);
    double q[3];
    pts->GetPoint(pIndex, q);
    s += sqrt(svtkMath::Distance2BetweenPoints(p, q));
    memcpy(p, q, 3 * sizeof(double));
  }
  return s;
}

inline void PrintNames(ostream& out, svtkPointData* a)
{
  for (int i = 0; i < a->GetNumberOfArrays(); i++)
  {
    out << a->GetArray(i)->GetName() << " ";
  }
  out << endl;
}

inline bool SameShape(svtkPointData* a, svtkPointData* b)
{
  if (!a || !b)
  {
    return false;
  }

  if (a->GetNumberOfArrays() != b->GetNumberOfArrays())
  {
    PrintNames(cerr, a);
    PrintNames(cerr, b);
    return false;
  }

  int numArrays(a->GetNumberOfArrays());
  for (int i = 0; i < numArrays; i++)
  {
    if (a->GetArray(i)->GetNumberOfComponents() != b->GetArray(i)->GetNumberOfComponents())
    {
      return false;
    }
  }

  return true;
}
#endif

class MessageBuffer
{
public:
  MessageBuffer(int size) { this->Stream = new MyStream(size); }

  ~MessageBuffer() { delete this->Stream; }

  svtkMPICommunicator::Request& GetRequest() { return this->Request; }
  MyStream& GetStream() { return *(this->Stream); }

private:
  svtkMPICommunicator::Request Request;
  MyStream* Stream;

  MessageBuffer(const MessageBuffer&) = delete;
  void operator=(const MessageBuffer&) = delete;
};

typedef MyStream MessageStream;

class Task : public svtkObject
{
public:
  static Task* New();

  int GetId() { return this->Point->GetId(); }
  svtkGetMacro(TraceExtended, bool);
  svtkGetMacro(TraceTerminated, bool);
  svtkSetMacro(TraceExtended, bool);
  svtkSetMacro(TraceTerminated, bool);

  PStreamTracerPoint* GetPoint() { return this->Point; }
  void IncHop() { this->NumHops++; }

private:
  svtkSmartPointer<PStreamTracerPoint> Point;

  int NumPeeks;
  int NumHops;
  bool TraceTerminated;
  bool TraceExtended;

  Task()
    : NumPeeks(0)
    , NumHops(0)
    , TraceTerminated(false)
    , TraceExtended(false)
  {
  }
  friend class TaskManager;
  friend MessageStream& operator<<(MessageStream& stream, const Task& task);
};
svtkStandardNewMacro(Task);

MessageStream& operator<<(MessageStream& stream, const Task& t)
{
  t.Point->Write(stream);
  stream << t.NumPeeks;
  stream << t.NumHops;
  return stream;
}

// Description:
// Manages the communication of traces between processes
class TaskManager
{
public:
  enum Message
  {
    NewTask,
    NoMoreTasks,
    TaskFinished
  };

  TaskManager(ProcessLocator* locator, PStreamTracerPoint* proto)
    : Locator(locator)
    , Proto(proto)
  {
    this->Controller =
      svtkMPIController::SafeDownCast(svtkMultiProcessController::GetGlobalController());
    AssertNe(this->Controller, nullptr);
    this->NumProcs = this->Controller->GetNumberOfProcesses();
    this->Rank = this->Controller->GetLocalProcessId();

    int prototypeSize = Proto == nullptr ? 0 : Proto->GetSize();
    this->MessageSize = prototypeSize + sizeof(Task);
    this->ReceiveBuffer = nullptr;

    this->NumSends = 0;
    this->Timer = svtkSmartPointer<svtkTimerLog>::New();
    this->ReceiveTime = 0;
  }

  void Initialize(bool hasData, const PStreamTracerPointArray& seeds, int MaxId)
  {
    AssertGe(MaxId, 0);
    int numSeeds = static_cast<int>(seeds.size());
    this->HasData.resize(this->NumProcs);
    std::fill(this->HasData.begin(), this->HasData.end(), 0);
    {
      const int self_hasdata = hasData ? 1 : 0;
      this->Controller->AllGather(&self_hasdata, &this->HasData[0], 1);
    }

    for (int i = 0; i < NumProcs; i++)
    {
      if (this->HasData[i])
      {
        this->Leader = i;
        break;
      }
    }

    std::vector<int> processMap0(MaxId + 1, -1);
    for (int i = 0; i < numSeeds; i++)
    {
      int rank = seeds[i]->GetRank();
      int id = seeds[i]->GetId();
      if (rank < 0 && this->Locator)
      {
        rank = this->Locator->InCurrentProcess(seeds[i]->GetSeed()) ? this->Rank : -1;
      }
      processMap0[id] = rank;
    }

    std::vector<int> processMap(MaxId + 1);
    this->Controller->AllReduce(
      &processMap0[0], &processMap[0], MaxId + 1, svtkCommunicator::MAX_OP);

    int totalNumTasks = std::accumulate(processMap.begin(), processMap.end(), 0,
      [](int accumlatedSum, int b) { return accumlatedSum + (b >= 0 ? 1 : 0); });

    this->TotalNumTasks = Rank == this->Leader
      ? totalNumTasks
      : INT_MAX; // only the master process knows how many are left

    for (int i = 0; i < numSeeds; i++)
    {
      int id = seeds[i]->GetId();
      if (processMap[id] == Rank)
      {
        svtkNew<Task> task;
        task->Point = seeds[i];
        NTasks.push_back(task);
      }
    }
    ALLPRINT(NTasks.size() << " initial seeds out of " << totalNumTasks);
  }

  Task* NextTask()
  {
    if (!this->HasData[Rank])
    {
      return nullptr;
    }

    //---------------------------------------------------------
    // Send messages
    //---------------------------------------------------------

    while (!this->PTasks.empty())
    {
      svtkSmartPointer<Task> task = PTasks.back();
      PTasks.pop_back();

      if (task->GetTraceTerminated())
      {
        // send to the master process
        this->Send(TaskFinished, this->Leader, task);
      }
      else
      {
        if (!task->GetTraceExtended())
        {
          // increment the peak
          task->NumPeeks++;
          PRINT("Skip " << task->GetId() << " with " << task->NumPeeks << " Peeks");
        }
        else
        {
          task->NumPeeks = 1;
        }
        int nextProcess = -1;
        if (task->NumPeeks < this->NumProcs)
        {
          nextProcess = NextProcess(task);
          if (nextProcess >= 0)
          {
            task->IncHop();
            // send it to the next guy
            this->Send(NewTask, NextProcess(task), task);
          }
        }

        if (nextProcess < 0)
        {
          this->Send(TaskFinished, this->Leader, task); // no one can do it, norminally finished
          PRINT("Bail on " << task->GetId());
        }
      }
    }

    //---------------------------------------------------------
    // Receive messages
    //---------------------------------------------------------

    do
    {
      this->Receive(this->TotalNumTasks != 0 && this->Msgs.empty() &&
        NTasks.empty()); // wait if there is nothing to do
      while (!this->Msgs.empty())
      {
        Message msg = this->Msgs.back();
        this->Msgs.pop_back();
        switch (msg)
        {
          case NewTask:
            break;
          case TaskFinished:
            AssertEq(Rank, this->Leader);
            this->TotalNumTasks--;
            PRINT(TotalNumTasks << " tasks left");
            break;
          case NoMoreTasks:
            AssertNe(Rank, this->Leader);
            this->TotalNumTasks = 0;
            break;
          default:
            assert(false);
        }
      }
    } while (this->TotalNumTasks != 0 && NTasks.empty());

    svtkSmartPointer<Task> nextTask;
    if (NTasks.empty())
    {
      AssertEq(this->TotalNumTasks, 0);
      if (this->Rank == this->Leader) // let everyeone know
      {
        for (int i = (this->Rank + 1) % NumProcs; i != this->Rank; i = (i + 1) % NumProcs)
        {
          if (this->HasData[i])
          {
            this->Send(NoMoreTasks, i, 0);
          }
        }
      }
    }
    else
    {
      nextTask = this->NTasks.back();
      this->NTasks.pop_back();
      this->PTasks.push_back(nextTask);
    }

    return nextTask;
  }

  ~TaskManager()
  {
    for (BufferList::iterator itr = SendBuffers.begin(); itr != SendBuffers.end(); ++itr)
    {
      MessageBuffer* buf = *itr;
      AssertNe(buf->GetRequest().Test(), 0);
      delete buf;
    }
    if (this->ReceiveBuffer)
    {
      this->ReceiveBuffer->GetRequest().Cancel();
      delete ReceiveBuffer;
    }
  }

private:
  ProcessLocator* Locator;
  svtkSmartPointer<PStreamTracerPoint> Proto;
  svtkMPIController* Controller;
  std::vector<svtkSmartPointer<Task> > NTasks;
  std::vector<svtkSmartPointer<Task> > PTasks;
  std::vector<Message> Msgs;
  int NumProcs;
  int Rank;
  int TotalNumTasks;
  int MessageSize;
  std::vector<int> HasData;
  int Leader;
  typedef std::list<MessageBuffer*> BufferList;
  BufferList SendBuffers;
  MessageBuffer* ReceiveBuffer;

  void Send(int msg, int rank, Task* task)
  {
    if (task && (msg == TaskFinished))
    {
      PRINT("Done in " << task->Point->GetNumSteps() << " steps " << task->NumHops << " hops");
    }
    if (rank == this->Rank)
    {
      switch (msg)
      {
        case TaskFinished:
          this->TotalNumTasks--;
          PRINT(TotalNumTasks << " tasks left");
          break;
        default:
          PRINT("Unhandled message " << msg);
          assert(false);
      }
    }
    else
    {
      MessageBuffer& buf = this->NewSendBuffer();
      MessageStream& outStream(buf.GetStream());

      outStream << msg << this->Rank;
      AssertNe(this->Rank, rank);

      if (task)
      {
        outStream << (*task);
      }

      AssertGe(this->MessageSize, outStream.GetLength());
      this->Controller->NoBlockSend(
        outStream.GetRawData(), outStream.GetLength(), rank, 561, buf.GetRequest());

      NumSends++;
      if (task)
      {
        PRINT("Send " << msg << "; task "
                      << task->GetId()); //<<" "<<task->Seed[0]<<" "<<task->Seed[1]<<"
                                         //"<<task->Seed[2]<<" to "<<rank);
      }
      else
      {
        PRINT("Send " << msg);
      }
    }
  }
  int NextProcess(Task* task)
  {
    PStreamTracerPoint* p = task->GetPoint();
    int rank = p->GetRank();
    if (rank >= 0)
    {
      return rank;
    }

    if (this->Locator)
    {
      rank = this->Locator->FindNextProcess(p->GetSeed());
    }
    AssertNe(rank, Rank);
    return rank;
  }

  svtkSmartPointer<Task> NewTaskInstance()
  {
    svtkSmartPointer<Task> task = svtkSmartPointer<Task>::New();

    task->Point = this->Proto->NewInstance();
    task->Point->CopyTail(this->Proto);
    task->Point->Delete();
    return task;
  }

  void Read(MessageStream& stream, Task& task)
  {
    task.Point->Read(stream);
    stream >> task.NumPeeks;
    stream >> task.NumHops;
  }

  MessageBuffer& NewSendBuffer()
  {
    // remove all empty buffers
    BufferList::iterator itr = SendBuffers.begin();
    while (itr != SendBuffers.end())
    {
      MessageBuffer* buf(*itr);
      BufferList::iterator next = itr;
      ++next;
      if (buf->GetRequest().Test())
      {
        delete buf;
        SendBuffers.erase(itr);
      }
      itr = next;
    }

    MessageBuffer* buf = new MessageBuffer(this->MessageSize);
    SendBuffers.push_back(buf);
    return *buf;
  }

  void Receive(bool wait = false)
  {
    int msg = -1;
    int sender(0);

#ifdef DEBUGTRACE
    this->StartTimer();
#endif
    if (ReceiveBuffer && wait)
    {
      ReceiveBuffer->GetRequest().Wait();
    }

    if (ReceiveBuffer && ReceiveBuffer->GetRequest().Test())
    {
      MyStream& inStream(ReceiveBuffer->GetStream());
      inStream >> msg >> sender;
      this->Msgs.push_back((Message)msg);
      if (msg == NewTask)
      {
        PRINT("Received message " << msg << " from " << sender)

        svtkSmartPointer<Task> task = this->NewTaskInstance();
        this->Read(inStream, *task);
        PRINT("Received task "
          << task->GetId()); //<<" "<<task->Seed[0]<<" "<<task->Seed[1]<<" "<<task->Seed[2]);
        this->NTasks.push_back(task);
      }
      delete ReceiveBuffer;
      ReceiveBuffer = nullptr;
    }
    if (ReceiveBuffer == nullptr)
    {
      ReceiveBuffer = new MessageBuffer(this->MessageSize);
      MyStream& inStream(ReceiveBuffer->GetStream());
      this->Controller->NoBlockReceive(inStream.GetRawData(), inStream.GetSize(),
        svtkMultiProcessController::ANY_SOURCE, 561, ReceiveBuffer->GetRequest());
    }

#ifdef DEBUGTRACE
    double time = this->StopTimer();
    if (msg >= 0)
    {
      this->ReceiveTime += time;
    }
#endif
  }

  int NumSends;
  double ReceiveTime;
  svtkSmartPointer<svtkTimerLog> Timer;
};

};

svtkCxxSetObjectMacro(svtkPStreamTracer, Controller, svtkMultiProcessController);
svtkCxxSetObjectMacro(svtkPStreamTracer, Interpolator, svtkAbstractInterpolatedVelocityField);
svtkStandardNewMacro(svtkPStreamTracer);

svtkPStreamTracer::svtkPStreamTracer()
{
  this->Controller = svtkMultiProcessController::GetGlobalController();
  if (this->Controller)
  {
    this->Controller->Register(this);
  }
  this->Interpolator = 0;
  this->GenerateNormalsInIntegrate = 0;

  this->EmptyData = 0;
}

svtkPStreamTracer::~svtkPStreamTracer()
{
  if (this->Controller)
  {
    this->Controller->UnRegister(this);
    this->Controller = 0;
  }
  this->SetInterpolator(0);
}

int svtkPStreamTracer::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  int piece = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numPieces = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  int ghostLevel = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  int numInputs = this->GetNumberOfInputConnections(0);
  for (int idx = 0; idx < numInputs; ++idx)
  {
    svtkInformation* info = inputVector[0]->GetInformationObject(idx);
    if (info)
    {
      info->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
      info->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
      info->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevel);
    }
  }

  svtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  if (sourceInfo)
  {
    sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
    sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
    sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevel);
  }

  return 1;
}

int svtkPStreamTracer::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (!svtkMPIController::SafeDownCast(this->Controller) ||
    this->Controller->GetNumberOfProcesses() == 1)
  {
    this->GenerateNormalsInIntegrate = 1;
    int result = svtkStreamTracer::RequestData(request, inputVector, outputVector);
    this->GenerateNormalsInIntegrate = 0;
    return result;
  }

  this->Rank = this->Controller->GetLocalProcessId();
  this->NumProcs = this->Controller->GetNumberOfProcesses();

  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!this->SetupOutput(inInfo, outInfo))
  {
    return 0;
  }

  svtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  svtkDataSet* source = 0;
  if (sourceInfo)
  {
    source = svtkDataSet::SafeDownCast(sourceInfo->Get(svtkDataObject::DATA_OBJECT()));
  }
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // init 'func' with nullptr such that we can check it later to determine
  // if we need to deallocate 'func' in case CheckInputs() fails (note
  // that a process may be assigned no any dataset when the number of
  // processes is greater than that of the blocks)
  svtkAbstractInterpolatedVelocityField* func = nullptr;
  int maxCellSize = 0;
  if (this->CheckInputs(func, &maxCellSize) != SVTK_OK)
  {
    svtkDebugMacro("No appropriate inputs have been found..");
    this->EmptyData = 1;
    PRINT("Has Empty Data")

    // the if-statement below is a MUST since 'func' may be still nullptr
    // when this->InputData is nullptr ---- no any data has been assigned
    // to this process
    if (func)
    {
      func->Delete();
      func = nullptr;
    }
  }
  else
  {
    func->SetCaching(0);
    this->SetInterpolator(func);
    func->Delete();
  }

  if (svtkOverlappingAMR::SafeDownCast(this->InputData))
  {
    this->Utils = svtkSmartPointer<AMRPStreamTracerUtils>::New();
  }
  else
  {
    this->Utils = svtkSmartPointer<PStreamTracerUtils>::New();
  }
  this->Utils->Initialize(this);
  ALLPRINT("Vec Name: " << this->Utils->GetVecName());
  typedef std::vector<svtkSmartPointer<svtkPolyData> > traceOutputsType;
  traceOutputsType traceOutputs;

  TaskManager taskManager(this->Utils->GetProcessLocator(), this->Utils->GetProto());
  PStreamTracerPointArray seedPoints;

  int maxId;
  auto originalSeedIds = this->Utils->ComputeSeeds(source, seedPoints, maxId);
  taskManager.Initialize(this->EmptyData == 0, seedPoints, maxId);

  Task* task(0);
  std::vector<int> traceIds;
  int iterations = 0;
  while ((task = taskManager.NextTask()))
  {
    iterations++;
    PStreamTracerPoint* point = task->GetPoint();

    svtkSmartPointer<svtkPolyData> traceOut;
    this->Trace(this->Utils->GetInput0(), this->Utils->GetVecType(), this->Utils->GetVecName(),
      point, traceOut, func, maxCellSize);

    task->SetTraceExtended(traceOut->GetNumberOfPoints() > 0);

    if (task->GetTraceExtended() && task->GetPoint()->GetTail())
    {
      // if we got this streamline from another process then this
      // process is responsible for filling in the gap over
      // the subdomain boundary
      this->Prepend(traceOut, task->GetPoint()->GetTail());
    }

    int resTerm = svtkStreamTracer::OUT_OF_DOMAIN;
    svtkIntArray* resTermArray =
      svtkArrayDownCast<svtkIntArray>(traceOut->GetCellData()->GetArray("ReasonForTermination"));
    if (resTermArray)
    {
      resTerm = resTermArray->GetValue(0);
    }

    // construct a new seed from the last point
    task->SetTraceTerminated(this->Controller->GetNumberOfProcesses() == 1 ||
      resTerm != svtkStreamTracer::OUT_OF_DOMAIN ||
      point->GetPropagation() > this->MaximumPropagation ||
      point->GetNumSteps() >= this->MaximumNumberOfSteps);
    if (task->GetTraceExtended() && !task->GetTraceTerminated())
    {
      task->SetTraceTerminated(
        !this->TraceOneStep(traceOut, func, point)); // we don't know where to go, just terminate it
    }
    if (!task->GetTraceTerminated())
    {
      task->SetTraceTerminated(!this->Utils->PreparePoint(point, func));
    }

    traceIds.push_back(task->GetId());
    traceOutputs.push_back(traceOut);
  }

  this->Controller->Barrier();

#ifdef LOGTRACE
  double receiveTime = taskManager.ComputeReceiveTime();
  if (this->Rank == 0)
  {
    PRINT("Total receive time: " << receiveTime)
  }
  this->Controller->Barrier();
#endif

  PRINT("Done");

  // The parallel integration adds all streamlines to traceOutputs
  // container. We append them all together here.
  svtkNew<svtkAppendPolyData> append;
  for (traceOutputsType::iterator it = traceOutputs.begin(); it != traceOutputs.end(); ++it)
  {
    svtkPolyData* inp = it->GetPointer();
    if (inp->GetNumberOfCells() > 0)
    {
      append->AddInputData(inp);
    }
  }
  if (append->GetNumberOfInputConnections(0) > 0)
  {
    append->Update();
    svtkPolyData* appoutput = append->GetOutput();
    output->CopyStructure(appoutput);
    output->GetPointData()->PassData(appoutput->GetPointData());
    output->GetCellData()->PassData(appoutput->GetCellData());
  }

  this->InputData->UnRegister(this);

  // Fix seed ids. The seed ids that the parallel algorithm uses are not really
  // seed ids but seed indices. We need to restore original seed ids so that
  // a full streamline gets the same seed id for forward and backward
  // directions.
  if (auto seedIds = svtkIntArray::SafeDownCast(output->GetCellData()->GetArray("SeedIds")))
  {
    svtkSMPTools::For(0, seedIds->GetNumberOfTuples(),
      [&originalSeedIds, &seedIds](svtkIdType start, svtkIdType end) {
        for (svtkIdType cc = start; cc < end; ++cc)
        {
          const auto seedIdx = seedIds->GetTypedComponent(cc, 0);
          assert(seedIdx < originalSeedIds->GetNumberOfIds());
          seedIds->SetTypedComponent(cc, 0, originalSeedIds->GetId(seedIdx));
        }
      });
  }

#ifdef DEBUGTRACE
  int maxSeeds(maxId + 1);
  std::vector<double> lengths(maxSeeds);
  for (int i = 0; i < maxSeeds; i++)
  {
    lengths[i] = 0;
  }

  AssertEq(traceOutputs.size(), traceIds.size());
  for (unsigned int i = 0; i < traceOutputs.size(); i++)
  {
    svtkPolyData* poly = traceOutputs[i];
    int id = traceIds[i];
    double length(0);
    svtkCellArray* lines = poly->GetLines();
    if (lines)
    {
      lines->InitTraversal();
      svtkNew<svtkIdList> trace;
      lines->GetNextCell(trace);
      length = ComputeLength(trace, poly->GetPoints());
    }
    lengths[id] += length;
  }
  std::vector<double> totalLengths(maxSeeds);
  this->Controller->AllReduce(&lengths[0], &totalLengths[0], maxSeeds, svtkCommunicator::SUM_OP);

  int numNonZeros(0);
  double totalLength(0);
  for (int i = 0; i < maxSeeds; i++)
  {
    totalLength += totalLengths[i];
    if (totalLengths[i] > 0)
    {
      numNonZeros++;
    }
  }

  if (this->Rank == 0)
  {
    PRINT("Summary: " << maxSeeds << " seeds," << numNonZeros << " traces"
                      << " total length " << totalLength);
  }

#endif
  PRINT("Done in " << iterations << " iterations");

  traceOutputs.erase(traceOutputs.begin(), traceOutputs.end());
  return 1;
}

void svtkPStreamTracer::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}

void svtkPStreamTracer::Trace(svtkDataSet* input, int vecType, const char* vecName,
  PStreamTracerPoint* point, svtkSmartPointer<svtkPolyData>& traceOut,
  svtkAbstractInterpolatedVelocityField* func, int maxCellSize)
{
  double* seedSource = point->GetSeed();
  int direction = point->GetDirection();

  this->Utils->InitializeVelocityFunction(point, func);

  double lastPoint[3];
  svtkSmartPointer<svtkFloatArray> seeds = svtkSmartPointer<svtkFloatArray>::New();
  seeds->SetNumberOfComponents(3);
  seeds->InsertNextTuple(seedSource);

  svtkNew<svtkIdList> seedIds;
  seedIds->InsertNextId(0);

  svtkNew<svtkIntArray> integrationDirections;
  integrationDirections->InsertNextValue(direction);
  traceOut = svtkSmartPointer<svtkPolyData>::New();

  double propagation = point->GetPropagation();
  svtkIdType numSteps = point->GetNumSteps();
  double integrationTime = point->GetIntegrationTime();

  svtkStreamTracer::Integrate(input->GetPointData(), traceOut, seeds, seedIds, integrationDirections,
    lastPoint, func, maxCellSize, vecType, vecName, propagation, numSteps, integrationTime);
  AssertGe(propagation, point->GetPropagation());
  AssertGe(numSteps, point->GetNumSteps());

  point->SetPropagation(propagation);
  point->SetNumSteps(numSteps);
  point->SetIntegrationTime(integrationTime);

  if (this->GenerateNormalsInIntegrate)
  {
    this->GenerateNormals(traceOut, point->GetNormal(), vecName);
  }

  if (traceOut->GetNumberOfPoints() > 0)
  {
    if (traceOut->GetLines()->GetNumberOfCells() == 0)
    {
      PRINT("Fix Single Point Path")
      AssertEq(traceOut->GetNumberOfPoints(), 1); // fix it
      svtkNew<svtkCellArray> newCells;
      svtkIdType cells[2] = { 0, 0 };
      newCells->InsertNextCell(2, cells);
      traceOut->SetLines(newCells);

      // Don't forget to add the ReasonForTermination cell array.
      svtkNew<svtkIntArray> retVals;
      retVals->SetName("ReasonForTermination");
      retVals->SetNumberOfTuples(1);
      retVals->SetValue(0, svtkStreamTracer::OUT_OF_DOMAIN);
      traceOut->GetCellData()->AddArray(retVals);
    }

    svtkNew<svtkIntArray> ids;
    ids->SetName("SeedIds");
    ids->SetNumberOfTuples(1);
    ids->SetValue(0, point->GetId());
    traceOut->GetCellData()->AddArray(ids);
  }
  Assert(SameShape(traceOut->GetPointData(), this->Utils->GetProto()->GetTail()->GetPointData()),
    "trace data does not match prototype");
}
bool svtkPStreamTracer::TraceOneStep(
  svtkPolyData* traceOut, svtkAbstractInterpolatedVelocityField* func, PStreamTracerPoint* point)
{
  double outPoint[3], outNormal[3];

  svtkIdType lastPointIndex = LastPointIndex(traceOut);
  double lastPoint[3];
  // Continue the integration a bit further to obtain a point
  // outside. The main integration step can not always be used
  // for this, specially if the integration is not 2nd order.
  traceOut->GetPoint(lastPointIndex, lastPoint);

  svtkInitialValueProblemSolver* ivp = this->Integrator;
  ivp->Register(this);

  svtkNew<svtkRungeKutta2> tmpSolver;
  this->SetIntegrator(tmpSolver);

  memcpy(outPoint, lastPoint, sizeof(double) * 3);

  double timeStepTaken = this->SimpleIntegrate(0, outPoint, this->LastUsedStepSize, func);
  PRINT("Simple Integrate from :" << lastPoint[0] << " " << lastPoint[1] << " " << lastPoint[2]
                                  << " to " << outPoint[0] << " " << outPoint[1] << " "
                                  << outPoint[2]);
  double d = sqrt(svtkMath::Distance2BetweenPoints(lastPoint, outPoint));

  this->SetIntegrator(ivp);
  ivp->UnRegister(this);

  svtkDataArray* normals = traceOut->GetPointData()->GetArray("Normals");
  if (normals)
  {
    normals->GetTuple(lastPointIndex, outNormal);
  }

  bool res = d > 0;
  if (res)
  {
    Assert(SameShape(traceOut->GetPointData(), this->Utils->GetProto()->GetTail()->GetPointData()),
      "Point data mismatch");
    point->Reseed(outPoint, outNormal, traceOut, lastPointIndex, point->GetPropagation() + d,
      point->GetIntegrationTime() + timeStepTaken);
    AssertEq(point->GetTail()->GetPointData()->GetNumberOfTuples(), 1);
  }

  return res;
}

void svtkPStreamTracer::Prepend(svtkPolyData* pathPoly, svtkPolyData* headPoly)
{
  svtkCellArray* pathCells = pathPoly->GetLines();
  AssertEq(pathCells->GetNumberOfCells(), 1);
  AssertEq(headPoly->GetNumberOfPoints(), 1);

  double* newPoint = headPoly->GetPoint(0);
  AssertEq(
    headPoly->GetPointData()->GetNumberOfArrays(), pathPoly->GetPointData()->GetNumberOfArrays());

  const svtkIdType* path(0);
  svtkIdType nPoints(0);
  pathCells->InitTraversal();
  pathCells->GetNextCell(nPoints, path);
  AssertNe(path, nullptr);
  AssertEq(nPoints, pathPoly->GetNumberOfPoints());

  svtkIdType newPointId = pathPoly->GetPoints()->InsertNextPoint(newPoint);

  svtkPointData* headData = headPoly->GetPointData();
  svtkPointData* pathData = pathPoly->GetPointData();
  Assert(SameShape(headData, pathData), "Prepend failure");
  int numArrays(headData->GetNumberOfArrays());
  for (int i = 0; i < numArrays; i++)
  {
    pathData->CopyTuple(
      headData->GetAbstractArray(i), pathData->GetAbstractArray(i), 0, newPointId);
  }

  PRINT("Prepend Point " << newPointId << " " << newPoint[0] << " " << newPoint[1] << " "
                         << newPoint[2]);
  svtkNew<svtkIdList> newPath;
  newPath->InsertNextId(newPointId);
  for (int i = 0; i < nPoints; i++)
  {
    newPath->InsertNextId(path[i]);
  }

  pathCells->Reset();
  if (newPath->GetNumberOfIds() > 1)
  {
    pathCells->InsertNextCell(newPath);
  }
  AssertEq(pathCells->GetNumberOfCells(), 1);
  svtkIdType newNumPoints(0);
  pathCells->GetNextCell(newNumPoints, path);
  AssertEq(newNumPoints, nPoints + 1);
  AssertEq(newNumPoints, pathPoly->GetNumberOfPoints());
}
