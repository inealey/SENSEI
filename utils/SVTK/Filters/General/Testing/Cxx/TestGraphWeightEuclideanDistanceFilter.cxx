#include <svtkDataSetAttributes.h>
#include <svtkFloatArray.h>
#include <svtkMutableUndirectedGraph.h>
#include <svtkPoints.h>
#include <svtkSmartPointer.h>

#include "svtkGraphWeightEuclideanDistanceFilter.h"

template <typename T>
static bool FuzzyCompare(const T a, const T b);

int TestGraphWeightEuclideanDistanceFilter(int, char*[])
{
  // Create a graph
  svtkSmartPointer<svtkMutableUndirectedGraph> g = svtkSmartPointer<svtkMutableUndirectedGraph>::New();

  // Add 4 vertices to the graph
  svtkIdType v1 = g->AddVertex();
  svtkIdType v2 = g->AddVertex();
  svtkIdType v3 = g->AddVertex();
  svtkIdType v4 = g->AddVertex();

  // Add 3 edges to the graph
  g->AddEdge(v1, v2);
  g->AddEdge(v1, v3);
  g->AddEdge(v1, v4);

  // Create 4 points - one for each vertex
  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  points->InsertNextPoint(0.0, 0.0, 0.0);
  points->InsertNextPoint(1.0, 0.0, 0.0);
  points->InsertNextPoint(0.0, 1.0, 0.0);
  points->InsertNextPoint(0.0, 0.0, 2.0);

  // Add the coordinates of the points to the graph
  g->SetPoints(points);

  svtkSmartPointer<svtkGraphWeightEuclideanDistanceFilter> weightFilter =
    svtkSmartPointer<svtkGraphWeightEuclideanDistanceFilter>::New();
  weightFilter->SetInputData(g);
  weightFilter->Update();

  svtkFloatArray* weights =
    svtkArrayDownCast<svtkFloatArray>(weightFilter->GetOutput()->GetEdgeData()->GetArray("Weights"));

  std::vector<float> correctWeights;
  correctWeights.push_back(1.0f);
  correctWeights.push_back(1.0f);
  correctWeights.push_back(2.0f);

  for (svtkIdType i = 0; i < weights->GetNumberOfTuples(); ++i)
  {
    float w = weights->GetValue(i);
    if (!FuzzyCompare<float>(w, correctWeights[i]))
    {
      std::cerr << "Weight " << i << " was " << w << " and should have been " << correctWeights[i]
                << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

template <typename T>
static bool FuzzyCompare(const T a, const T b)
{
  if (fabs(static_cast<float>(a) - static_cast<float>(b)) < 1e-4)
  {
    return true;
  }
  return false;
}
