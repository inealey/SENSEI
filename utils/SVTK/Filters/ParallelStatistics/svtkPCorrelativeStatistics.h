/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPCorrelativeStatistics.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2011 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
/**
 * @class   svtkPCorrelativeStatistics
 * @brief   A class for parallel bivariate correlative statistics
 *
 * svtkPCorrelativeStatistics is svtkCorrelativeStatistics subclass for parallel datasets.
 * It learns and derives the global statistical model on each node, but assesses each
 * individual data points on the node that owns it.
 *
 * @par Thanks:
 * Thanks to Philippe Pebay from Sandia National Laboratories for implementing this class.
 */

#ifndef svtkPCorrelativeStatistics_h
#define svtkPCorrelativeStatistics_h

#include "svtkCorrelativeStatistics.h"
#include "svtkFiltersParallelStatisticsModule.h" // For export macro

class svtkMultiBlockDataSet;
class svtkMultiProcessController;

class SVTKFILTERSPARALLELSTATISTICS_EXPORT svtkPCorrelativeStatistics
  : public svtkCorrelativeStatistics
{
public:
  static svtkPCorrelativeStatistics* New();
  svtkTypeMacro(svtkPCorrelativeStatistics, svtkCorrelativeStatistics);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the multiprocess controller. If no controller is set,
   * single process is assumed.
   */
  virtual void SetController(svtkMultiProcessController*);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

  /**
   * Execute the parallel calculations required by the Learn option.
   */
  void Learn(svtkTable* inData, svtkTable* inParameters, svtkMultiBlockDataSet* outMeta) override;

  /**
   * Execute the calculations required by the Test option.
   * NB: Not implemented for more than 1 processor
   */
  void Test(svtkTable*, svtkMultiBlockDataSet*, svtkTable*) override;

protected:
  svtkPCorrelativeStatistics();
  ~svtkPCorrelativeStatistics() override;

  svtkMultiProcessController* Controller;

private:
  svtkPCorrelativeStatistics(const svtkPCorrelativeStatistics&) = delete;
  void operator=(const svtkPCorrelativeStatistics&) = delete;
};

#endif
