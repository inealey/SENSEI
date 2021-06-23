/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageResample.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageResample
 * @brief   Resamples an image to be larger or smaller.
 *
 * This filter produces an output with different spacing (and extent)
 * than the input.  Linear interpolation can be used to resample the data.
 * The Output spacing can be set explicitly or relative to input spacing
 * with the SetAxisMagnificationFactor method.
 */

#ifndef svtkImageResample_h
#define svtkImageResample_h

#include "svtkImageReslice.h"
#include "svtkImagingCoreModule.h" // For export macro

class SVTKIMAGINGCORE_EXPORT svtkImageResample : public svtkImageReslice
{
public:
  static svtkImageResample* New();
  svtkTypeMacro(svtkImageResample, svtkImageReslice);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set desired spacing.
   * Zero is a reserved value indicating spacing has not been set.
   */
  void SetOutputSpacing(double sx, double sy, double sz) override;
  void SetOutputSpacing(const double spacing[3]) override
  {
    this->SetOutputSpacing(spacing[0], spacing[1], spacing[2]);
  }
  void SetAxisOutputSpacing(int axis, double spacing);
  //@}

  //@{
  /**
   * Set/Get Magnification factors.
   * Zero is a reserved value indicating values have not been computed.
   */
  void SetMagnificationFactors(double fx, double fy, double fz);
  void SetMagnificationFactors(const double f[3])
  {
    this->SetMagnificationFactors(f[0], f[1], f[2]);
  }
  svtkGetVector3Macro(MagnificationFactors, double);
  void SetAxisMagnificationFactor(int axis, double factor);
  //@}

  /**
   * Get the computed magnification factor for a specific axis.
   * The input information is required to compute the value.
   */
  double GetAxisMagnificationFactor(int axis, svtkInformation* inInfo = nullptr);

  //@{
  /**
   * Dimensionality is the number of axes which are considered during
   * execution. To process images dimensionality would be set to 2.
   * This has the same effect as setting the magnification of the third
   * axis to 1.0
   */
  svtkSetMacro(Dimensionality, int);
  svtkGetMacro(Dimensionality, int);
  //@}

protected:
  svtkImageResample();
  ~svtkImageResample() override {}

  double MagnificationFactors[3];
  int Dimensionality;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkImageResample(const svtkImageResample&) = delete;
  void operator=(const svtkImageResample&) = delete;
};

#endif
