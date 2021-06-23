//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================
/**
 * @class   svtkmPointElevation
 * @brief   generate a scalar field along a specified direction
 *
 * svtkmPointElevation is a filter that generates a scalar field along a specified
 * direction. The scalar field values lie within a user specified range, and are
 * generated by computing a projection of each dataset point onto a line. The line
 * can be oriented arbitrarily. A typical example is to generate scalars based
 * on elevation or height above a plane.
 *
 */

#ifndef svtkmPointElevation_h
#define svtkmPointElevation_h

#include "svtkAcceleratorsSVTKmModule.h" // required for correct export
#include "svtkElevationFilter.h"

class SVTKACCELERATORSSVTKM_EXPORT svtkmPointElevation : public svtkElevationFilter
{
public:
  svtkTypeMacro(svtkmPointElevation, svtkElevationFilter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkmPointElevation* New();

protected:
  svtkmPointElevation();
  ~svtkmPointElevation() override;

  virtual int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkmPointElevation(const svtkmPointElevation&) = delete;
  void operator=(const svtkmPointElevation&) = delete;
};

#endif // svtkmPointElevation_h

// SVTK-HeaderTest-Exclude: svtkmPointElevation.h
