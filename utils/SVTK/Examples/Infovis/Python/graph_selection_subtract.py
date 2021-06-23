#!/usr/bin/env python
from svtk import *

source = svtkRandomGraphSource()
source.SetNumberOfVertices(10)
#source.SetStartWithTree(True)
source.SetIncludeEdgeWeights(True)
source.SetGeneratePedigreeIds(True)

# selection source
sel1 = svtkSelectionSource()
#svtkSelectionNode :: SELECTIONS,GLOBALIDS,PEDIGREEIDS,VALUES,INDICES,
#                    FRUSTRUM,LOCATIONS,THRESHOLDS,BLOCKS
#sel1.SetContentType( svtkSelectionNode.VALUES )
## sel1.SetContentType( svtkSelectionNode.PEDIGREEIDS )
sel1.SetContentType( svtkSelectionNode.INDICES )
#svtkSelectionNode :: CELL,POINT,FIELD,VERTEX,EDGE,ROW
sel1.SetFieldType( svtkSelectionNode.VERTEX )
#sel1.SetArrayName("vertex id")
sel1.AddID(0, 0)
sel1.AddID(0, 2)
sel1.AddID(0, 3)
sel1.Update()

G = source.GetOutputPort()

selExp0 = svtkExpandSelectedGraph()
selExp0.SetInputConnection(0, sel1.GetOutputPort());
selExp0.SetGraphConnection( G )
selExp0.SetBFSDistance(0)
selExp0.Update()


selExp1 = svtkExpandSelectedGraph()
selExp1.SetInputConnection(0, sel1.GetOutputPort());
selExp1.SetGraphConnection( G )
selExp1.SetBFSDistance(2)
selExp1.Update()


selExp1.GetOutput().Subtract( selExp0.GetOutput() )

view = svtkGraphLayoutView()
view.AddRepresentationFromInputConnection(G)

view.SetVertexLabelArrayName("vertex id")
view.SetVertexColorArrayName("vertex id")
view.SetVertexLabelVisibility(True)
#view.SetColorVertices(True)

view.SetEdgeColorArrayName("edge weight")
view.SetEdgeLabelArrayName("edge weight")
#view.SetEdgeLabelVisibility(True)
view.SetColorEdges(True)

view.SetLayoutStrategyToSimple2D()
view.SetVertexLabelFontSize(20)

# create selection link
annotationLink = svtkAnnotationLink()
view.GetRepresentation(0).SetAnnotationLink(annotationLink)
#annotationLink.SetCurrentSelection(sel1.GetOutput())
annotationLink.SetCurrentSelection(selExp1.GetOutput())

updater = svtkViewUpdater()
updater.AddAnnotationLink(annotationLink)
updater.AddView(view)

# set the theme on the view
theme = svtkViewTheme.CreateOceanTheme()
theme.SetLineWidth(2)
theme.SetPointSize(10)
view.ApplyViewTheme(theme)
theme.FastDelete()

view.GetRenderWindow().SetSize(600, 600)
view.ResetCamera()
view.Render()

view.GetInteractor().Start()
