/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPLinearExtrusionFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPLinearExtrusionFilter
 * @brief   Subclass that handles piece invariance.
 *
 * svtkPLinearExtrusionFilter is a parallel version of svtkLinearExtrusionFilter.
 *
 * @sa
 * svtkLinearExtrusionFilter
 */

#ifndef svtkPLinearExtrusionFilter_h
#define svtkPLinearExtrusionFilter_h

#include "svtkFiltersParallelModule.h" // For export macro
#include "svtkLinearExtrusionFilter.h"

class SVTKFILTERSPARALLEL_EXPORT svtkPLinearExtrusionFilter : public svtkLinearExtrusionFilter
{
public:
  svtkTypeMacro(svtkPLinearExtrusionFilter, svtkLinearExtrusionFilter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Create an object with PieceInvariant off.
   */
  static svtkPLinearExtrusionFilter* New();

  // To get piece invariance, this filter has to request an
  // extra ghost level.  Since piece invariance is not very
  // important for this filter,  it is optional.  Without invariance,
  // Internal surfaces will be generated.  These surface
  // Are hidden by the normal surface generated by this filter.
  // By default, PieceInvariance is off.
  svtkSetMacro(PieceInvariant, svtkTypeBool);
  svtkGetMacro(PieceInvariant, svtkTypeBool);
  svtkBooleanMacro(PieceInvariant, svtkTypeBool);

protected:
  svtkPLinearExtrusionFilter();
  ~svtkPLinearExtrusionFilter() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkTypeBool PieceInvariant;

private:
  svtkPLinearExtrusionFilter(const svtkPLinearExtrusionFilter&) = delete;
  void operator=(const svtkPLinearExtrusionFilter&) = delete;
};

#endif
