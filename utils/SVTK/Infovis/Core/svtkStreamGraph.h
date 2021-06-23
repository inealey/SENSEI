/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStreamGraph.h

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
/**
 * @class   svtkStreamGraph
 * @brief   combines two graphs
 *
 *
 * svtkStreamGraph iteratively collects information from the input graph
 * and combines it in the output graph. It internally maintains a graph
 * instance that is incrementally updated every time the filter is called.
 *
 * Each update, svtkMergeGraphs is used to combine this filter's input with the
 * internal graph.
 *
 * If you can use an edge window array to filter out old edges based on a
 * moving threshold.
 */

#ifndef svtkStreamGraph_h
#define svtkStreamGraph_h

#include "svtkGraphAlgorithm.h"
#include "svtkInfovisCoreModule.h" // For export macro

class svtkBitArray;
class svtkMergeGraphs;
class svtkMutableDirectedGraph;
class svtkMutableGraphHelper;
class svtkStringArray;
class svtkTable;

class SVTKINFOVISCORE_EXPORT svtkStreamGraph : public svtkGraphAlgorithm
{
public:
  static svtkStreamGraph* New();
  svtkTypeMacro(svtkStreamGraph, svtkGraphAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Whether to use an edge window array. The default is to
   * not use a window array.
   */
  svtkSetMacro(UseEdgeWindow, bool);
  svtkGetMacro(UseEdgeWindow, bool);
  svtkBooleanMacro(UseEdgeWindow, bool);
  //@}

  //@{
  /**
   * The edge window array. The default array name is "time".
   */
  svtkSetStringMacro(EdgeWindowArrayName);
  svtkGetStringMacro(EdgeWindowArrayName);
  //@}

  //@{
  /**
   * The time window amount. Edges with values lower
   * than the maximum value minus this window will be
   * removed from the graph. The default edge window is
   * 10000.
   */
  svtkSetMacro(EdgeWindow, double);
  svtkGetMacro(EdgeWindow, double);
  //@}

protected:
  svtkStreamGraph();
  ~svtkStreamGraph() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkMutableGraphHelper* CurrentGraph;
  svtkMergeGraphs* MergeGraphs;
  bool UseEdgeWindow;
  double EdgeWindow;
  char* EdgeWindowArrayName;

private:
  svtkStreamGraph(const svtkStreamGraph&) = delete;
  void operator=(const svtkStreamGraph&) = delete;
};

#endif
