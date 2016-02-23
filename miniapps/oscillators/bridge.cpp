#include "bridge.h"

#include "dataadaptor.h"

#include <vector>
#include <sensei/ConfigurableAnalysis.h>
#include <vtkDataObject.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>

namespace bridge
{
static vtkSmartPointer<oscillators::DataAdaptor> GlobalDataAdaptor;
static vtkSmartPointer<sensei::ConfigurableAnalysis> GlobalAnalysisAdaptor;

//-----------------------------------------------------------------------------
void initialize(MPI_Comm world,
                size_t window,
                size_t nblocks,
                size_t n_local_blocks,
                int domain_shape_x, int domain_shape_y, int domain_shape_z,
                int* gid,
                int* from_x, int* from_y, int* from_z,
                int* to_x,   int* to_y,   int* to_z,
                const std::string& config_file)
{
  GlobalDataAdaptor = vtkSmartPointer<oscillators::DataAdaptor>::New();
  GlobalDataAdaptor->Initialize(nblocks);
  GlobalDataAdaptor->SetDataTimeStep(-1);
  for (size_t cc=0; cc < n_local_blocks; ++cc)
    {
    GlobalDataAdaptor->SetBlockExtent(gid[cc],
      from_x[cc], to_x[cc],
      from_y[cc], to_y[cc],
      from_z[cc], to_z[cc]);
    }

  GlobalAnalysisAdaptor = vtkSmartPointer<sensei::ConfigurableAnalysis>::New();
  GlobalAnalysisAdaptor->Initialize(world, config_file);
}

//-----------------------------------------------------------------------------
void set_data(int gid, float* data)
{
  GlobalDataAdaptor->SetBlockData(gid, data);
}

//-----------------------------------------------------------------------------
void analyze(float time)
{
  GlobalDataAdaptor->SetDataTime(time);
  GlobalDataAdaptor->SetDataTimeStep(GlobalDataAdaptor->GetDataTimeStep() + 1);
  GlobalAnalysisAdaptor->Execute(GlobalDataAdaptor.GetPointer());
  GlobalDataAdaptor->ReleaseData();
}

//-----------------------------------------------------------------------------
void finalize(size_t k_max, size_t nblocks)
{
  GlobalAnalysisAdaptor = NULL;
  GlobalDataAdaptor = NULL;
}

}
