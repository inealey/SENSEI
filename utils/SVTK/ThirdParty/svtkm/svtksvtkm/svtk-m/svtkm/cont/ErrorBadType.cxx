//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/ErrorBadType.h>

#include <string>

namespace svtkm
{
namespace cont
{

void throwFailedDynamicCast(const std::string& baseType, const std::string& derivedType)
{ //Should we support typeid() instead of className?

  const std::string msg = "Cast failed: " + baseType + " --> " + derivedType;
  throw svtkm::cont::ErrorBadType(msg);
}
}
}
