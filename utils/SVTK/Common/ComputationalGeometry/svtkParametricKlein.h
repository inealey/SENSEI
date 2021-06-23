/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParametricKlein.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkParametricKlein
 * @brief   Generates a "classical" representation of a Klein bottle.
 *
 * svtkParametricKlein generates a "classical" representation of a Klein
 * bottle.  A Klein bottle is a closed surface with no interior and only one
 * surface.  It is unrealisable in 3 dimensions without intersecting
 * surfaces.  It can be
 * realised in 4 dimensions by considering the map \f$F:R^2 \rightarrow R^4\f$  given by:
 *
 * - \f$f(u,v) = ((r*cos(v)+a)*cos(u),(r*cos(v)+a)*sin(u),r*sin(v)*cos(u/2),r*sin(v)*sin(u/2))\f$
 *
 * The classical representation of the immersion in \f$R^3\f$ is returned by this function.
 *
 *
 * For further information about this surface, please consult the
 * technical description "Parametric surfaces" in http://www.svtk.org/publications
 * in the "SVTK Technical Documents" section in the VTk.org web pages.
 *
 * @par Thanks:
 * Andrew Maclean andrew.amaclean@gmail.com for creating and contributing the
 * class.
 *
 */

#ifndef svtkParametricKlein_h
#define svtkParametricKlein_h

#include "svtkCommonComputationalGeometryModule.h" // For export macro
#include "svtkParametricFunction.h"

class SVTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT svtkParametricKlein : public svtkParametricFunction
{
public:
  svtkTypeMacro(svtkParametricKlein, svtkParametricFunction);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct a Klein Bottle with the following parameters:
   * MinimumU = 0, MaximumU = 2*Pi,
   * MinimumV = -Pi, MaximumV = Pi,
   * JoinU = 0, JoinV = 1,
   * TwistU = 0, TwistV = 0,
   * ClockwiseOrdering = 0,
   * DerivativesAvailable = 1,
   */
  static svtkParametricKlein* New(); //! Initialise the parameters for the Klein bottle

  /**
   * Return the parametric dimension of the class.
   */
  int GetDimension() override { return 2; }

  /**
   * A Klein bottle.

   * This function performs the mapping \f$f(u,v) \rightarrow (x,y,x)\f$, returning it
   * as Pt. It also returns the partial derivatives Du and Dv.
   * \f$Pt = (x, y, z), Du = (dx/du, dy/du, dz/du), Dv = (dx/dv, dy/dv, dz/dv)\f$ .
   * Then the normal is \f$N = Du X Dv\f$ .
   */
  void Evaluate(double uvw[3], double Pt[3], double Duvw[9]) override;

  /**
   * Calculate a user defined scalar using one or all of uvw, Pt, Duvw.

   * uvw are the parameters with Pt being the cartesian point,
   * Duvw are the derivatives of this point with respect to u, v and w.
   * Pt, Duvw are obtained from Evaluate().

   * This function is only called if the ScalarMode has the value
   * svtkParametricFunctionSource::SCALAR_FUNCTION_DEFINED

   * If the user does not need to calculate a scalar, then the
   * instantiated function should return zero.
   */
  double EvaluateScalar(double uvw[3], double Pt[3], double Duvw[9]) override;

protected:
  svtkParametricKlein();
  ~svtkParametricKlein() override;

private:
  svtkParametricKlein(const svtkParametricKlein&) = delete;
  void operator=(const svtkParametricKlein&) = delete;
};

#endif
