/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBoostDividedEdgeBundling.h

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
 * @class   svtkBoostDividedEdgeBundling
 * @brief   layout graph edges in directed edge bundles
 *
 *
 * Uses the technique by Selassie, Heller, and Heer to route graph edges into directed
 * bundles, with "lanes" for bundled edges moving in each direction. This technique
 * works best for networks whose vertices have been positioned already (geospatial
 * graphs, for example). Note that this scales to a few thousand edges in a reasonable
 * period of time (~1 minute). The time complexity comes mainly from the doubling
 * of edge control points each cycle and the complex set of forces between many pairs of
 * edge points.
 *
 * The algorithm depends on the Boost graph library for its implementation of all-pairs
 * shortest paths, needed here for determining connectivity compatibility.
 *
 * @par Thanks:
 * This algorithm was developed in the paper:
 *   David Selassie, Brandon Heller, Jeffrey Heer. Divided Edge Bundling for Directional
 *   Network Data. Proceedings of IEEE InfoVis 2011.
 */

#ifndef svtkBoostDividedEdgeBundling_h
#define svtkBoostDividedEdgeBundling_h

#include "svtkDirectedGraphAlgorithm.h"
#include "svtkInfovisBoostGraphAlgorithmsModule.h" // For export macro

class SVTKINFOVISBOOSTGRAPHALGORITHMS_EXPORT svtkBoostDividedEdgeBundling
  : public svtkDirectedGraphAlgorithm
{
public:
  static svtkBoostDividedEdgeBundling* New();

  svtkTypeMacro(svtkBoostDividedEdgeBundling, svtkDirectedGraphAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkBoostDividedEdgeBundling();
  ~svtkBoostDividedEdgeBundling() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkBoostDividedEdgeBundling(const svtkBoostDividedEdgeBundling&) = delete;
  void operator=(const svtkBoostDividedEdgeBundling&) = delete;
};

#endif
