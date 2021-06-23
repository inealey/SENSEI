#!/usr/bin/env python
import svtk
from svtk.util.misc import svtkGetDataRoot
SVTK_DATA_ROOT = svtkGetDataRoot()

# Remove cullers so single vertex will render
ren1 = svtk.svtkRenderer()
ren1.GetCullers().RemoveAllItems()
renWin = svtk.svtkRenderWindow()
renWin.AddRenderer(ren1)
iren = svtk.svtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
cell = svtk.svtkGenericCell()
ptIds = svtk.svtkIdList()
# 0D
ZeroDPts = svtk.svtkPoints()
ZeroDPts.SetNumberOfPoints(1)
ZeroDPts.SetPoint(0,0,0,0)
ZeroDGrid = svtk.svtkStructuredGrid()
ZeroDGrid.SetDimensions(1,1,1)
ZeroDGrid.SetPoints(ZeroDPts)
ZeroDGrid.GetCell(0)
ZeroDGrid.GetCell(0,cell)
ZeroDGrid.GetCellPoints(0,ptIds)
ZeroDGeom = svtk.svtkStructuredGridGeometryFilter()
ZeroDGeom.SetInputData(ZeroDGrid)
ZeroDGeom.SetExtent(0,2,0,2,0,2)
ZeroDMapper = svtk.svtkPolyDataMapper()
ZeroDMapper.SetInputConnection(ZeroDGeom.GetOutputPort())
ZeroDActor = svtk.svtkActor()
ZeroDActor.SetMapper(ZeroDMapper)
ZeroDActor.SetPosition(0,0,0)
ren1.AddActor(ZeroDActor)
# 1D - X
XPts = svtk.svtkPoints()
XPts.SetNumberOfPoints(2)
XPts.SetPoint(0,0,0,0)
XPts.SetPoint(1,1,0,0)
XGrid = svtk.svtkStructuredGrid()
XGrid.SetDimensions(2,1,1)
XGrid.SetPoints(XPts)
XGrid.GetCell(0)
XGrid.GetCell(0,cell)
XGrid.GetCellPoints(0,ptIds)
XGeom = svtk.svtkStructuredGridGeometryFilter()
XGeom.SetInputData(XGrid)
XGeom.SetExtent(0,2,0,2,0,2)
XMapper = svtk.svtkPolyDataMapper()
XMapper.SetInputConnection(XGeom.GetOutputPort())
XActor = svtk.svtkActor()
XActor.SetMapper(XMapper)
XActor.SetPosition(2,0,0)
ren1.AddActor(XActor)
# 1D - Y
YPts = svtk.svtkPoints()
YPts.SetNumberOfPoints(2)
YPts.SetPoint(0,0,0,0)
YPts.SetPoint(1,0,1,0)
YGrid = svtk.svtkStructuredGrid()
YGrid.SetDimensions(1,2,1)
YGrid.SetPoints(YPts)
YGrid.GetCell(0)
YGrid.GetCell(0,cell)
YGrid.GetCellPoints(0,ptIds)
YGeom = svtk.svtkStructuredGridGeometryFilter()
YGeom.SetInputData(YGrid)
YGeom.SetExtent(0,2,0,2,0,2)
YMapper = svtk.svtkPolyDataMapper()
YMapper.SetInputConnection(YGeom.GetOutputPort())
YActor = svtk.svtkActor()
YActor.SetMapper(YMapper)
YActor.SetPosition(4,0,0)
ren1.AddActor(YActor)
# 1D - Z
ZPts = svtk.svtkPoints()
ZPts.SetNumberOfPoints(2)
ZPts.SetPoint(0,0,0,0)
ZPts.SetPoint(1,0,0,1)
ZGrid = svtk.svtkStructuredGrid()
ZGrid.SetDimensions(1,1,2)
ZGrid.SetPoints(ZPts)
ZGrid.GetCell(0)
ZGrid.GetCell(0,cell)
ZGrid.GetCellPoints(0,ptIds)
ZGeom = svtk.svtkStructuredGridGeometryFilter()
ZGeom.SetInputData(ZGrid)
ZGeom.SetExtent(0,2,0,2,0,2)
ZMapper = svtk.svtkPolyDataMapper()
ZMapper.SetInputConnection(ZGeom.GetOutputPort())
ZActor = svtk.svtkActor()
ZActor.SetMapper(ZMapper)
ZActor.SetPosition(6,0,0)
ren1.AddActor(ZActor)
# 2D - XY
XYPts = svtk.svtkPoints()
XYPts.SetNumberOfPoints(4)
XYPts.SetPoint(0,0,0,0)
XYPts.SetPoint(1,1,0,0)
XYPts.SetPoint(2,0,1,0)
XYPts.SetPoint(3,1,1,0)
XYGrid = svtk.svtkStructuredGrid()
XYGrid.SetDimensions(2,2,1)
XYGrid.SetPoints(XYPts)
XYGrid.GetCell(0)
XYGrid.GetCell(0,cell)
XYGrid.GetCellPoints(0,ptIds)
XYGeom = svtk.svtkStructuredGridGeometryFilter()
XYGeom.SetInputData(XYGrid)
XYGeom.SetExtent(0,2,0,2,0,2)
XYMapper = svtk.svtkPolyDataMapper()
XYMapper.SetInputConnection(XYGeom.GetOutputPort())
XYActor = svtk.svtkActor()
XYActor.SetMapper(XYMapper)
XYActor.SetPosition(0,2,0)
ren1.AddActor(XYActor)
# 2D - YZ
YZPts = svtk.svtkPoints()
YZPts.SetNumberOfPoints(4)
YZPts.SetPoint(0,0,0,0)
YZPts.SetPoint(1,0,1,0)
YZPts.SetPoint(2,0,0,1)
YZPts.SetPoint(3,0,1,1)
YZGrid = svtk.svtkStructuredGrid()
YZGrid.SetDimensions(1,2,2)
YZGrid.SetPoints(YZPts)
YZGrid.GetCell(0)
YZGrid.GetCell(0,cell)
YZGrid.GetCellPoints(0,ptIds)
YZGeom = svtk.svtkStructuredGridGeometryFilter()
YZGeom.SetInputData(YZGrid)
YZGeom.SetExtent(0,2,0,2,0,2)
YZMapper = svtk.svtkPolyDataMapper()
YZMapper.SetInputConnection(YZGeom.GetOutputPort())
YZActor = svtk.svtkActor()
YZActor.SetMapper(YZMapper)
YZActor.SetPosition(2,2,0)
ren1.AddActor(YZActor)
# 2D - XZ
XZPts = svtk.svtkPoints()
XZPts.SetNumberOfPoints(4)
XZPts.SetPoint(0,0,0,0)
XZPts.SetPoint(1,1,0,0)
XZPts.SetPoint(2,0,0,1)
XZPts.SetPoint(3,1,0,1)
XZGrid = svtk.svtkStructuredGrid()
XZGrid.SetDimensions(2,1,2)
XZGrid.SetPoints(XZPts)
XZGrid.GetCell(0)
XZGrid.GetCell(0,cell)
XZGrid.GetCellPoints(0,ptIds)
XZGeom = svtk.svtkStructuredGridGeometryFilter()
XZGeom.SetInputData(XZGrid)
XZGeom.SetExtent(0,2,0,2,0,2)
XZMapper = svtk.svtkPolyDataMapper()
XZMapper.SetInputConnection(XZGeom.GetOutputPort())
XZActor = svtk.svtkActor()
XZActor.SetMapper(XZMapper)
XZActor.SetPosition(4,2,0)
ren1.AddActor(XZActor)
# 3D
XYZPts = svtk.svtkPoints()
XYZPts.SetNumberOfPoints(8)
XYZPts.SetPoint(0,0,0,0)
XYZPts.SetPoint(1,1,0,0)
XYZPts.SetPoint(2,0,1,0)
XYZPts.SetPoint(3,1,1,0)
XYZPts.SetPoint(4,0,0,1)
XYZPts.SetPoint(5,1,0,1)
XYZPts.SetPoint(6,0,1,1)
XYZPts.SetPoint(7,1,1,1)
XYZGrid = svtk.svtkStructuredGrid()
XYZGrid.SetDimensions(2,2,2)
XYZGrid.SetPoints(XYZPts)
XYZGrid.GetCell(0)
XYZGrid.GetCell(0,cell)
XYZGrid.GetCellPoints(0,ptIds)
XYZGeom = svtk.svtkStructuredGridGeometryFilter()
XYZGeom.SetInputData(XYZGrid)
XYZGeom.SetExtent(0,2,0,2,0,2)
XYZMapper = svtk.svtkPolyDataMapper()
XYZMapper.SetInputConnection(XYZGeom.GetOutputPort())
XYZActor = svtk.svtkActor()
XYZActor.SetMapper(XYZMapper)
XYZActor.SetPosition(6,2,0)
ren1.AddActor(XYZActor)
# render the image
#
renWin.SetSize(300,150)
cam1 = ren1.GetActiveCamera()
cam1.SetClippingRange(2.27407,14.9819)
cam1.SetFocalPoint(3.1957,1.74012,0.176603)
cam1.SetPosition(-0.380779,6.13894,5.59404)
cam1.SetViewUp(0.137568,0.811424,-0.568037)
renWin.Render()
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
