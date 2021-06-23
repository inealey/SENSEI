/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestColumnTree.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkDataSetAttributes.h"
#include "svtkDoubleArray.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkNew.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkTree.h"
#include "svtkTreeHeatmapItem.h"

#include "svtkContextInteractorStyle.h"
#include "svtkContextMouseEvent.h"
#include "svtkContextScene.h"
#include "svtkContextTransform.h"
#include "svtkContextView.h"
#include "svtkNew.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

#include "svtkRegressionTestImage.h"

//----------------------------------------------------------------------------
int TestColumnTree(int argc, char* argv[])
{
  // Construct a tree
  svtkNew<svtkMutableDirectedGraph> graph;
  svtkIdType root = graph->AddVertex();
  svtkIdType internalOne = graph->AddChild(root);
  svtkIdType internalTwo = graph->AddChild(internalOne);
  svtkIdType a = graph->AddChild(internalTwo);
  svtkIdType b = graph->AddChild(internalTwo);
  svtkIdType c = graph->AddChild(internalOne);

  svtkNew<svtkDoubleArray> weights;
  weights->SetNumberOfTuples(5);
  weights->SetValue(graph->GetEdgeId(root, internalOne), 1.0f);
  weights->SetValue(graph->GetEdgeId(internalOne, internalTwo), 2.0f);
  weights->SetValue(graph->GetEdgeId(internalTwo, a), 1.0f);
  weights->SetValue(graph->GetEdgeId(internalTwo, b), 1.0f);
  weights->SetValue(graph->GetEdgeId(internalOne, c), 3.0f);

  weights->SetName("weight");
  graph->GetEdgeData()->AddArray(weights);

  svtkNew<svtkStringArray> names;
  names->SetNumberOfTuples(6);
  names->SetValue(a, "a");
  names->SetValue(b, "b");
  names->SetValue(c, "c");

  names->SetName("node name");
  graph->GetVertexData()->AddArray(names);

  svtkNew<svtkDoubleArray> nodeWeights;
  nodeWeights->SetNumberOfTuples(6);
  nodeWeights->SetValue(root, 0.0f);
  nodeWeights->SetValue(internalOne, 1.0f);
  nodeWeights->SetValue(internalTwo, 3.0f);
  nodeWeights->SetValue(a, 4.0f);
  nodeWeights->SetValue(b, 4.0f);
  nodeWeights->SetValue(c, 4.0f);
  nodeWeights->SetName("node weight");
  graph->GetVertexData()->AddArray(nodeWeights);

  svtkNew<svtkTree> tree;
  tree->ShallowCopy(graph);

  svtkNew<svtkTree> tree2;
  tree2->DeepCopy(tree);

  // Construct a table
  svtkNew<svtkTable> table;
  svtkNew<svtkStringArray> tableNames;
  svtkNew<svtkDoubleArray> m1;
  svtkNew<svtkDoubleArray> m2;
  svtkNew<svtkDoubleArray> m3;

  tableNames->SetNumberOfTuples(3);
  tableNames->SetValue(0, "c");
  tableNames->SetValue(1, "b");
  tableNames->SetValue(2, "a");
  tableNames->SetName("name");

  m1->SetNumberOfTuples(3);
  m2->SetNumberOfTuples(3);
  m3->SetNumberOfTuples(3);

  m1->SetName("a");
  m2->SetName("b");
  m3->SetName("c");

  m1->SetValue(0, 1.0f);
  m1->SetValue(1, 3.0f);
  m1->SetValue(2, 1.0f);

  m2->SetValue(0, 2.0f);
  m2->SetValue(1, 2.0f);
  m2->SetValue(2, 2.0f);

  m3->SetValue(0, 3.0f);
  m3->SetValue(1, 1.0f);
  m3->SetValue(2, 3.0f);

  table->AddColumn(tableNames);
  table->AddColumn(m1);
  table->AddColumn(m2);
  table->AddColumn(m3);

  svtkNew<svtkTreeHeatmapItem> treeItem;
  treeItem->SetTree(tree);
  treeItem->SetColumnTree(tree2);
  treeItem->SetTable(table);

  svtkNew<svtkContextTransform> trans;
  trans->SetInteractive(true);
  trans->AddItem(treeItem);

  // center the item within the render window
  trans->Translate(80, 25);
  trans->Scale(1.5, 1.5);

  svtkSmartPointer<svtkContextView> view = svtkSmartPointer<svtkContextView>::New();
  view->GetRenderWindow()->SetSize(400, 200);
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetScene()->AddItem(trans);

  // Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetRenderWindow()->Render();

  // collapse a column subtree
  svtkContextMouseEvent mouseEvent;
  mouseEvent.SetInteractor(view->GetInteractor());
  svtkVector2f pos;
  mouseEvent.SetButton(svtkContextMouseEvent::LEFT_BUTTON);
  pos.Set(62, 81);
  mouseEvent.SetPos(pos);
  treeItem->MouseDoubleClickEvent(mouseEvent);
  view->GetRenderWindow()->Render();

  int retVal = svtkRegressionTestImage(view->GetRenderWindow());
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    view->GetRenderWindow()->Render();
    view->GetInteractor()->Start();
    retVal = svtkRegressionTester::PASSED;
  }
  return !retVal;
}
