/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTemporalStreamTracer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTemporalStreamTracer
 * @brief   A Parallel Particle tracer for unsteady vector fields
 *
 * svtkTemporalStreamTracer is a filter that integrates a vector field to generate
 *
 *
 * @sa
 * svtkRibbonFilter svtkRuledSurfaceFilter svtkInitialValueProblemSolver
 * svtkRungeKutta2 svtkRungeKutta4 svtkRungeKutta45 svtkStreamTracer
 *
 * This class is deprecated.
 * Use instead one of the following classes: svtkParticleTracerBase
 * svtkParticleTracer svtkParticlePathFilter svtkStreaklineFilter
 * See https://blog.kitware.com/improvements-in-path-tracing-in-svtk/
 */

#ifndef svtkTemporalStreamTracer_h
#define svtkTemporalStreamTracer_h

#include "svtkConfigure.h" // For legacy defines
#include "svtkSetGet.h"    // For legacy macros
#ifndef SVTK_LEGACY_REMOVE

#include "svtkFiltersFlowPathsModule.h" // For export macro
#include "svtkSmartPointer.h"           // For protected ivars.
#include "svtkStreamTracer.h"

#include <list>   // STL Header
#include <vector> // STL Header

class svtkMultiProcessController;

class svtkMultiBlockDataSet;
class svtkDataArray;
class svtkDoubleArray;
class svtkGenericCell;
class svtkIntArray;
class svtkTemporalInterpolatedVelocityField;
class svtkPoints;
class svtkCellArray;
class svtkDoubleArray;
class svtkFloatArray;
class svtkIntArray;
class svtkCharArray;
class svtkAbstractParticleWriter;

namespace svtkTemporalStreamTracerNamespace
{
typedef struct
{
  double x[4];
} Position;
typedef struct
{
  // These are used during iteration
  Position CurrentPosition;
  int CachedDataSetId[2];
  svtkIdType CachedCellId[2];
  int LocationState;
  // These are computed scalars we might display
  int SourceID;
  int TimeStepAge;
  int InjectedPointId;
  int InjectedStepId;
  int UniqueParticleId;
  // These are useful to track for debugging etc
  int ErrorCode;
  float age;
  // these are needed across time steps to compute vorticity
  float rotation;
  float angularVel;
  float time;
  float speed;
} ParticleInformation;

typedef std::vector<ParticleInformation> ParticleVector;
typedef ParticleVector::iterator ParticleIterator;
typedef std::list<ParticleInformation> ParticleDataList;
typedef ParticleDataList::iterator ParticleListIterator;
};

class SVTKFILTERSFLOWPATHS_EXPORT svtkTemporalStreamTracer : public svtkStreamTracer
{
public:
  svtkTypeMacro(svtkTemporalStreamTracer, svtkStreamTracer);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object using 2nd order Runge Kutta
   */
  static svtkTemporalStreamTracer* New();

  //@{
  /**
   * Set/Get the TimeStep. This is the primary means of advancing
   * the particles. The TimeStep should be animated and this will drive
   * the pipeline forcing timesteps to be fetched from upstream.
   */
  svtkSetMacro(TimeStep, unsigned int);
  svtkGetMacro(TimeStep, unsigned int);
  //@}

  //@{
  /**
   * To get around problems with the Paraview Animation controls
   * we can just animate the time step and ignore the TIME_ requests
   */
  svtkSetMacro(IgnorePipelineTime, svtkTypeBool);
  svtkGetMacro(IgnorePipelineTime, svtkTypeBool);
  svtkBooleanMacro(IgnorePipelineTime, svtkTypeBool);
  //@}

  //@{
  /**
   * If the data source does not have the correct time values
   * present on each time step - setting this value to non unity can
   * be used to adjust the time step size from 1s pre step to
   * 1x_TimeStepResolution : Not functional in this version.
   * Broke it @todo, put back time scaling
   */
  svtkSetMacro(TimeStepResolution, double);
  svtkGetMacro(TimeStepResolution, double);
  //@}

  //@{
  /**
   * When animating particles, it is nice to inject new ones every Nth step
   * to produce a continuous flow. Setting ForceReinjectionEveryNSteps to a
   * non zero value will cause the particle source to reinject particles
   * every Nth step even if it is otherwise unchanged.
   * Note that if the particle source is also animated, this flag will be
   * redundant as the particles will be reinjected whenever the source changes
   * anyway
   */
  svtkSetMacro(ForceReinjectionEveryNSteps, int);
  svtkGetMacro(ForceReinjectionEveryNSteps, int);
  //@}

  enum Units
  {
    TERMINATION_TIME_UNIT,
    TERMINATION_STEP_UNIT
  };

  //@{
  /**
   * Setting TerminationTime to a positive value will cause particles
   * to terminate when the time is reached. Use a vlue of zero to
   * disable termination. The units of time should be consistent with the
   * primary time variable.
   */
  svtkSetMacro(TerminationTime, double);
  svtkGetMacro(TerminationTime, double);
  //@}

  //@{
  /**
   * The units of TerminationTime may be actual 'Time' units as described
   * by the data, or just TimeSteps of iteration.
   */
  svtkSetMacro(TerminationTimeUnit, int);
  svtkGetMacro(TerminationTimeUnit, int);
  void SetTerminationTimeUnitToTimeUnit() { this->SetTerminationTimeUnit(TERMINATION_TIME_UNIT); }
  void SetTerminationTimeUnitToStepUnit() { this->SetTerminationTimeUnit(TERMINATION_STEP_UNIT); }
  //@}

  //@{
  /**
   * if StaticSeeds is set and the mesh is static,
   * then every time particles are injected we can re-use the same
   * injection information. We classify particles according to
   * processor just once before start.
   * If StaticSeeds is set and a moving seed source is specified
   * the motion will be ignored and results will not be as expected.
   */
  svtkSetMacro(StaticSeeds, svtkTypeBool);
  svtkGetMacro(StaticSeeds, svtkTypeBool);
  svtkBooleanMacro(StaticSeeds, svtkTypeBool);
  //@}

  //@{
  /**
   * if StaticMesh is set, many optimizations for cell caching
   * can be assumed. if StaticMesh is not set, the algorithm
   * will attempt to find out if optimizations can be used, but
   * setting it to true will force all optimizations.
   * Do not Set StaticMesh to true if a dynamic mesh is being used
   * as this will invalidate all results.
   */
  svtkSetMacro(StaticMesh, svtkTypeBool);
  svtkGetMacro(StaticMesh, svtkTypeBool);
  svtkBooleanMacro(StaticMesh, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the Writer associated with this Particle Tracer
   * Ideally a parallel IO capable svtkH5PartWriter should be used
   * which will collect particles from all parallel processes
   * and write them to a single HDF5 file.
   */
  virtual void SetParticleWriter(svtkAbstractParticleWriter* pw);
  svtkGetObjectMacro(ParticleWriter, svtkAbstractParticleWriter);
  //@}

  //@{
  /**
   * Set/Get the filename to be used with the particle writer when
   * dumping particles to disk
   */
  svtkSetStringMacro(ParticleFileName);
  svtkGetStringMacro(ParticleFileName);
  //@}

  //@{
  /**
   * Set/Get the filename to be used with the particle writer when
   * dumping particles to disk
   */
  svtkSetMacro(EnableParticleWriting, svtkTypeBool);
  svtkGetMacro(EnableParticleWriting, svtkTypeBool);
  svtkBooleanMacro(EnableParticleWriting, svtkTypeBool);
  //@}

  //@{
  /**
   * Provide support for multiple see sources
   */
  void AddSourceConnection(svtkAlgorithmOutput* input);
  void RemoveAllSources();
  //@}

protected:
  SVTK_LEGACY(svtkTemporalStreamTracer());
  ~svtkTemporalStreamTracer() override;

  //
  // Make sure the pipeline knows what type we expect as input
  //
  int FillInputPortInformation(int port, svtkInformation* info) override;

  //
  // The usual suspects
  //
  svtkTypeBool ProcessRequest(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  //
  // Store any information we need in the output and fetch what we can
  // from the input
  //
  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  //
  // Compute input time steps given the output step
  //
  int RequestUpdateExtent(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  //
  // what the pipeline calls for each time step
  //
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  //
  // these routines are internally called to actually generate the output
  //
  virtual int ProcessInput(svtkInformationVector** inputVector);

  virtual int GenerateOutput(
    svtkInformationVector** inputVector, svtkInformationVector* outputVector);

  //
  // Initialization of input (vector-field) geometry
  //
  int InitializeInterpolator();
  int SetTemporalInput(svtkDataObject* td, int index);

  //

  /**
   * inside our data. Add good ones to passed list and set count to the
   * number that passed
   */
  void TestParticles(svtkTemporalStreamTracerNamespace::ParticleVector& candidates,
    svtkTemporalStreamTracerNamespace::ParticleVector& passed, int& count);

  /**
   * all the injection/seed points according to which processor
   * they belong to. This saves us retesting at every injection time
   * providing 1) The volumes are static, 2) the seed points are static
   * If either are non static, then this step is skipped.
   */
  virtual void AssignSeedsToProcessors(svtkDataSet* source, int sourceID, int ptId,
    svtkTemporalStreamTracerNamespace::ParticleVector& LocalSeedPoints, int& LocalAssignedCount);

  /**
   * give each one a unique ID. We need to use MPI to find out
   * who is using which numbers.
   */
  virtual void AssignUniqueIds(svtkTemporalStreamTracerNamespace::ParticleVector& LocalSeedPoints);

  /**
   * and sending between processors, into a list, which is used as the master
   * list on this processor
   */
  void UpdateParticleList(svtkTemporalStreamTracerNamespace::ParticleVector& candidates);

  /**
   * this is used during classification of seed points and also between iterations
   * of the main loop as particles leave each processor domain
   */
  virtual void TransmitReceiveParticles(
    svtkTemporalStreamTracerNamespace::ParticleVector& outofdomain,
    svtkTemporalStreamTracerNamespace::ParticleVector& received, bool removeself);

  /**
   * particle between the two times supplied.
   */
  void IntegrateParticle(svtkTemporalStreamTracerNamespace::ParticleListIterator& it,
    double currenttime, double terminationtime, svtkInitialValueProblemSolver* integrator);

  /**
   * and sent to the other processors for possible continuation.
   * These routines manage the collection and sending after each main iteration.
   * RetryWithPush adds a small pusj to aparticle along it's current velocity
   * vector, this helps get over cracks in dynamic/rotating meshes
   */
  bool RetryWithPush(
    svtkTemporalStreamTracerNamespace::ParticleInformation& info, double velocity[3], double delT);

  // if the particle is added to send list, then returns value is 1,
  // if it is kept on this process after a retry return value is 0
  bool SendParticleToAnotherProcess(
    svtkTemporalStreamTracerNamespace::ParticleInformation& info, double point1[4], double delT);

  void AddParticleToMPISendList(svtkTemporalStreamTracerNamespace::ParticleInformation& info);

  /**
   * In dnamic meshes, particles might leave the domain and need to be extrapolated across
   * a gap between the meshes before they re-renter another domain
   * dodgy rotating meshes need special care....
   */
  bool ComputeDomainExitLocation(
    double pos[4], double p2[4], double intersection[4], svtkGenericCell* cell);

  //

  //
  // Track internally which round of RequestData it is--between 0 and 2
  int RequestIndex;

  // Track which process we are
  int UpdatePieceId;
  int UpdateNumPieces;

  // Important for Caching of Cells/Ids/Weights etc
  int AllFixedGeometry;
  svtkTypeBool StaticMesh;
  svtkTypeBool StaticSeeds;

  // Support 'pipeline' time or manual SetTimeStep
  unsigned int TimeStep;
  unsigned int ActualTimeStep;
  svtkTypeBool IgnorePipelineTime;
  unsigned int NumberOfInputTimeSteps;

  std::vector<double> InputTimeValues;
  std::vector<double> OutputTimeValues;

  // more time management
  double EarliestTime;
  double CurrentTimeSteps[2];
  double TimeStepResolution;

  // Particle termination after time
  double TerminationTime;
  int TerminationTimeUnit;

  // Particle injection+Reinjection
  int ForceReinjectionEveryNSteps;
  bool ReinjectionFlag;
  int ReinjectionCounter;
  svtkTimeStamp ParticleInjectionTime;

  // Particle writing to disk
  svtkAbstractParticleWriter* ParticleWriter;
  char* ParticleFileName;
  svtkTypeBool EnableParticleWriting;

  // The main lists which are held during operation- between time step updates
  unsigned int NumberOfParticles;
  svtkTemporalStreamTracerNamespace::ParticleDataList ParticleHistories;
  svtkTemporalStreamTracerNamespace::ParticleVector LocalSeeds;

  //
  // Scalar arrays that are generated as each particle is updated
  //
  svtkSmartPointer<svtkFloatArray> ParticleAge;
  svtkSmartPointer<svtkIntArray> ParticleIds;
  svtkSmartPointer<svtkCharArray> ParticleSourceIds;
  svtkSmartPointer<svtkIntArray> InjectedPointIds;
  svtkSmartPointer<svtkIntArray> InjectedStepIds;
  svtkSmartPointer<svtkIntArray> ErrorCodeArray;
  svtkSmartPointer<svtkFloatArray> ParticleVorticity;
  svtkSmartPointer<svtkFloatArray> ParticleRotation;
  svtkSmartPointer<svtkFloatArray> ParticleAngularVel;
  svtkSmartPointer<svtkDoubleArray> cellVectors;
  svtkSmartPointer<svtkPointData> OutputPointData;
  int InterpolationCount;

  // The output geometry
  svtkSmartPointer<svtkCellArray> ParticleCells;
  svtkSmartPointer<svtkPoints> OutputCoordinates;

  // List used for transmitting between processors during parallel operation
  svtkTemporalStreamTracerNamespace::ParticleVector MPISendList;

  // The velocity interpolator
  svtkSmartPointer<svtkTemporalInterpolatedVelocityField> Interpolator;

  // The input datasets which are stored by time step 0 and 1
  svtkSmartPointer<svtkMultiBlockDataSet> InputDataT[2];
  svtkSmartPointer<svtkDataSet> DataReferenceT[2];

  // Cache bounds info for each dataset we will use repeatedly
  typedef struct
  {
    double b[6];
  } bounds;
  std::vector<bounds> CachedBounds[2];

  // utility function we use to test if a point is inside any of our local datasets
  bool InsideBounds(double point[]);

  // global Id counter used to give particles a stamp
  svtkIdType UniqueIdCounter;
  svtkIdType UniqueIdCounterMPI;
  // for debugging only;
  int substeps;

private:
  /**
   * Hide this because we require a new interpolator type
   */
  void SetInterpolatorPrototype(svtkAbstractInterpolatedVelocityField*) {}

private:
  svtkTemporalStreamTracer(const svtkTemporalStreamTracer&) = delete;
  void operator=(const svtkTemporalStreamTracer&) = delete;
};

#endif // SVTK_LEGACY_REMOVE

#endif
