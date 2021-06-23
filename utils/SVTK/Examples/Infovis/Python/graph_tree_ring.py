#!/usr/bin/env python
from svtk import *

addStringLabel = svtkProgrammableFilter()

def computeLabel():
  input = addStringLabel.GetInput()
  output = addStringLabel.GetOutput()

  output.ShallowCopy(input)

  # Create output array
  vertexArray = svtkStringArray()
  vertexArray.SetName("label")
  vertexArray.SetNumberOfTuples(output.GetNumberOfVertices())

  # Loop through all the vertices setting the degree for the new attribute array
  for i in range(output.GetNumberOfVertices()):
      label = '%02d' % (i)
      vertexArray.SetValue(i, label)

  # Add the new attribute array to the output graph
  output.GetVertexData().AddArray(vertexArray)

addStringLabel.SetExecuteMethod(computeLabel)

source = svtkRandomGraphSource()
source.SetNumberOfVertices(15)
source.SetIncludeEdgeWeights(True)

addStringLabel.SetInputConnection(source.GetOutputPort())


conn_comp = svtkBoostConnectedComponents()
bi_conn_comp = svtkBoostBiconnectedComponents()

conn_comp.SetInputConnection(addStringLabel.GetOutputPort())
bi_conn_comp.SetInputConnection(conn_comp.GetOutputPort())


# Cleave off part of the graph
vertexDataTable = svtkDataObjectToTable()
vertexDataTable.SetInputConnection(bi_conn_comp.GetOutputPort())
vertexDataTable.SetFieldType(3) # Vertex data

# Make a tree out of connected/biconnected components
toTree = svtkTableToTreeFilter()
toTree.AddInputConnection(vertexDataTable.GetOutputPort())
tree1 = svtkGroupLeafVertices()
tree1.AddInputConnection(toTree.GetOutputPort())
tree1.SetInputArrayToProcess(0,0, 0, 4, "component")
tree1.SetInputArrayToProcess(1,0, 0, 4, "label")
tree2 = svtkGroupLeafVertices()
tree2.AddInputConnection(tree1.GetOutputPort())
tree2.SetInputArrayToProcess(0,0, 0, 4, "biconnected component")
tree2.SetInputArrayToProcess(1,0, 0, 4, "label")


# Create a tree ring view on connected/biconnected components
view1 = svtkTreeRingView()
view1.SetTreeFromInputConnection(tree2.GetOutputPort())
view1.SetGraphFromInputConnection(bi_conn_comp.GetOutputPort())
view1.SetLabelPriorityArrayName("GraphVertexDegree")
view1.SetAreaColorArrayName("VertexDegree")
view1.SetAreaLabelArrayName("label")
view1.SetAreaHoverArrayName("label")
view1.SetAreaLabelVisibility(True)
view1.SetBundlingStrength(.5)
view1.SetLayerThickness(.5)
view1.Update()
view1.SetColorEdges(True)
view1.SetEdgeColorArrayName("edge weight")

view2 = svtkGraphLayoutView()
view2.AddRepresentationFromInputConnection(bi_conn_comp.GetOutputPort())
view2.SetVertexLabelArrayName("label")
view2.SetVertexLabelVisibility(True)
view2.SetVertexColorArrayName("label")
view2.SetColorVertices(True)
view2.SetLayoutStrategyToSimple2D()


# Apply a theme to the views
theme = svtkViewTheme.CreateOceanTheme()
view1.ApplyViewTheme(theme)
view2.ApplyViewTheme(theme)
theme.FastDelete()

view1.GetRenderWindow().SetSize(600, 600)
view1.ResetCamera()
view1.Render()

view2.GetRenderWindow().SetSize(600, 600)
view2.ResetCamera()
view2.Render()


view1.GetInteractor().Start()

