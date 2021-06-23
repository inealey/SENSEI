/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSmartVolumeMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSmartVolumeMapper
 * @brief   Adaptive volume mapper
 *
 * svtkSmartVolumeMapper is a volume mapper that will delegate to a specific
 * volume mapper based on rendering parameters and available hardware. Use the
 * SetRequestedRenderMode() method to control the behavior of the selection.
 * The following options are available:
 *
 * @par svtkSmartVolumeMapper::DefaultRenderMode:
 *          Allow the svtkSmartVolumeMapper to select the best mapper based on
 *          rendering parameters and hardware support. If GPU ray casting is
 *          supported, this mapper will be used for all rendering. If not,
 *          then the svtkFixedPointRayCastMapper will be used exclusively.
 *          This is the default requested render mode, and is generally the
 *          best option. When you use this option, your volume will always
 *          be rendered, but the method used to render it may vary based
 *          on parameters and platform.
 *
 * @par svtkSmartVolumeMapper::RayCastRenderMode:
 *          Use the svtkFixedPointVolumeRayCastMapper for both interactive and
 *          still rendering. When you use this option your volume will always
 *          be rendered with the svtkFixedPointVolumeRayCastMapper.
 *
 * @par svtkSmartVolumeMapper::GPURenderMode:
 *          Use the svtkGPUVolumeRayCastMapper, if supported, for both
 *          interactive and still rendering. If the GPU ray caster is not
 *          supported (due to hardware limitations or rendering parameters)
 *          then no image will be rendered. Use this option only if you have
 *          already checked for supported based on the current hardware,
 *          number of scalar components, and rendering parameters in the
 *          svtkVolumeProperty.
 *
 * @par svtkSmartVolumeMapper::GPURenderMode:
 *  You can adjust the contrast and brightness in the rendered image using the
 *  FinalColorWindow and FinalColorLevel ivars. By default the
 *  FinalColorWindow is set to 1.0, and the FinalColorLevel is set to 0.5,
 *  which applies no correction to the computed image. To apply the window /
 *  level operation to the computer image color, first a Scale and Bias
 *  value are computed:
 *  <pre>
 *  scale = 1.0 / this->FinalColorWindow
 *  bias  = 0.5 - this->FinalColorLevel / this->FinalColorWindow
 *  </pre>
 *  To compute a new color (R', G', B', A') from an existing color (R,G,B,A)
 *  for a pixel, the following equation is used:
 *  <pre>
 *  R' = R*scale + bias*A
 *  G' = G*scale + bias*A
 *  B' = B*scale + bias*A
 *  A' = A
 *  </pre>
 * Note that bias is multiplied by the alpha component before adding because
 * the red, green, and blue component of the color are already pre-multiplied
 * by alpha. Also note that the window / level operation leaves the alpha
 * component unchanged - it only adjusts the RGB values.
 */

#ifndef svtkSmartVolumeMapper_h
#define svtkSmartVolumeMapper_h

#include "svtkImageReslice.h"                 // for SVTK_RESLICE_NEAREST, SVTK_RESLICE_CUBIC
#include "svtkRenderingVolumeOpenGL2Module.h" // For export macro
#include "svtkVolumeMapper.h"

class svtkFixedPointVolumeRayCastMapper;
class svtkGPUVolumeRayCastMapper;
class svtkImageResample;
class svtkMultiBlockVolumeMapper;
class svtkOSPRayVolumeInterface;
class svtkRenderWindow;
class svtkVolume;
class svtkVolumeProperty;
class svtkImageMagnitude;

class SVTKRENDERINGVOLUMEOPENGL2_EXPORT svtkSmartVolumeMapper : public svtkVolumeMapper
{
public:
  static svtkSmartVolumeMapper* New();
  svtkTypeMacro(svtkSmartVolumeMapper, svtkVolumeMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the final color window. This controls the contrast of
   * the image. The default value is 1.0. The Window can be
   * negative (this causes a "negative" effect on the image)
   * Although Window can be set to 0.0, any value less than
   * 0.00001 and greater than or equal to 0.0 will be set to
   * 0.00001, and any value greater than -0.00001 but less
   * than or equal to 0.0 will be set to -0.00001.
   * Initial value is 1.0.
   */
  svtkSetMacro(FinalColorWindow, float);
  //@}

  //@{
  /**
   * Get the final color window. Initial value is 1.0.
   */
  svtkGetMacro(FinalColorWindow, float);
  //@}

  //@{
  /**
   * Set the final color level. The level controls the
   * brightness of the image. The final color window will
   * be centered at the final color level, and together
   * represent a linear remapping of color values. The
   * default value for the level is 0.5.
   */
  svtkSetMacro(FinalColorLevel, float);
  //@}

  //@{
  /**
   * Get the final color level.
   */
  svtkGetMacro(FinalColorLevel, float);
  //@}

  // The possible values for the default and current render mode ivars
  enum
  {
    DefaultRenderMode = 0,
    RayCastRenderMode = 1,
    GPURenderMode = 2,
    OSPRayRenderMode = 3,
    UndefinedRenderMode = 4,
    InvalidRenderMode = 5
  };

  /**
   * Set the requested render mode. The default is
   * svtkSmartVolumeMapper::DefaultRenderMode.
   */
  void SetRequestedRenderMode(int mode);

  /**
   * Set the requested render mode to svtkSmartVolumeMapper::DefaultRenderMode.
   * This is the best option for an application that must adapt to different
   * data types, hardware, and rendering parameters.
   */
  void SetRequestedRenderModeToDefault();

  /**
   * Set the requested render mode to svtkSmartVolumeMapper::RayCastRenderMode.
   * This option will use software rendering exclusively. This is a good option
   * if you know there is no hardware acceleration.
   */
  void SetRequestedRenderModeToRayCast();

  /**
   * Set the requested render mode to svtkSmartVolumeMapper::GPURenderMode.
   * This option will use hardware accelerated rendering exclusively. This is a
   * good option if you know there is hardware acceleration.
   */
  void SetRequestedRenderModeToGPU();

  /**
   * Set the requested render mode to svtkSmartVolumeMapper::OSPRayRenderMode.
   * This option will use intel OSPRay to do software rendering exclusively.
   */
  void SetRequestedRenderModeToOSPRay();

  //@{
  /**
   * Get the requested render mode.
   */
  svtkGetMacro(RequestedRenderMode, int);
  //@}

  /**
   * This will return the render mode used during the previous call to
   * Render().
   */
  int GetLastUsedRenderMode();

  //@{
  /**
   * Value passed to the GPU mapper. Ignored by other mappers.
   * Maximum size of the 3D texture in GPU memory.
   * Will default to the size computed from the graphics
   * card. Can be adjusted by the user.
   * Useful if the automatic detection is defective or missing.
   */
  svtkSetMacro(MaxMemoryInBytes, svtkIdType);
  svtkGetMacro(MaxMemoryInBytes, svtkIdType);
  //@}

  //@{
  /**
   * Value passed to the GPU mapper. Ignored by other mappers.
   * Maximum fraction of the MaxMemoryInBytes that should
   * be used to hold the texture. Valid values are 0.1 to
   * 1.0.
   */
  svtkSetClampMacro(MaxMemoryFraction, float, 0.1f, 1.0f);
  svtkGetMacro(MaxMemoryFraction, float);
  //@}

  //@{
  /**
   * Set interpolation mode for downsampling (lowres GPU)
   * (initial value: cubic).
   */
  svtkSetClampMacro(InterpolationMode, int, SVTK_RESLICE_NEAREST, SVTK_RESLICE_CUBIC);
  svtkGetMacro(InterpolationMode, int);
  void SetInterpolationModeToNearestNeighbor();
  void SetInterpolationModeToLinear();
  void SetInterpolationModeToCubic();
  //@}

  /**
   * This method can be used to render a representative view of the input data
   * into the supplied image given the supplied blending mode, view direction,
   * and view up vector.
   */
  void CreateCanonicalView(svtkRenderer* ren, svtkVolume* volume, svtkVolume* volume2,
    svtkImageData* image, int blend_mode, double viewDirection[3], double viewUp[3]);

  //@{
  /**
   * If the DesiredUpdateRate of the svtkRenderWindow that caused the Render
   * falls at or above this rate, the render is considered interactive and
   * the mapper may be adjusted (depending on the render mode).
   * Initial value is 1.0.
   */
  svtkSetClampMacro(InteractiveUpdateRate, double, 1.0e-10, 1.0e10);
  //@}

  //@{
  /**
   * Get the update rate at or above which this is considered an
   * interactive render.
   * Initial value is 1.0.
   */
  svtkGetMacro(InteractiveUpdateRate, double);
  //@}

  //@{
  /**
   * If the InteractiveAdjustSampleDistances flag is enabled,
   * svtkSmartVolumeMapper interactively sets and resets the
   * AutoAdjustSampleDistances flag on the internal volume mapper. This flag
   * along with InteractiveUpdateRate is useful to adjust volume mapper sample
   * distance based on whether the render is interactive or still.
   * By default, InteractiveAdjustSampleDistances is enabled.
   */
  svtkSetClampMacro(InteractiveAdjustSampleDistances, svtkTypeBool, 0, 1);
  svtkGetMacro(InteractiveAdjustSampleDistances, svtkTypeBool);
  svtkBooleanMacro(InteractiveAdjustSampleDistances, svtkTypeBool);
  //@}

  //@{
  /**
   * If AutoAdjustSampleDistances is on, the ImageSampleDistance
   * will be varied to achieve the allocated render time of this
   * prop (controlled by the desired update rate and any culling in
   * use).
   * Note that, this flag is ignored when InteractiveAdjustSampleDistances is
   * enabled. To explicitly set and use this flag, one must disable
   * InteractiveAdjustSampleDistances.
   */
  svtkSetClampMacro(AutoAdjustSampleDistances, svtkTypeBool, 0, 1);
  svtkGetMacro(AutoAdjustSampleDistances, svtkTypeBool);
  svtkBooleanMacro(AutoAdjustSampleDistances, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the distance between samples used for rendering
   * when AutoAdjustSampleDistances is off, or when this mapper
   * has more than 1 second allocated to it for rendering.
   * If SampleDistance is negative, it will be computed based on the dataset
   * spacing. Initial value is -1.0.
   */
  svtkSetMacro(SampleDistance, float);
  svtkGetMacro(SampleDistance, float);
  //@}

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * Initialize rendering for this volume.
   */
  void Render(svtkRenderer*, svtkVolume*) override;

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

  //@{
  /**
   * VectorMode is a special rendering mode for 3-component vectors which makes
   * use of GPURayCastMapper's independent-component capabilities. In this mode,
   * a single component in the vector can be selected for rendering. In addition,
   * the mapper can compute a scalar field representing the magnitude of this vector
   * using a svtkImageMagnitude object (MAGNITUDE mode).
   */
  enum VectorModeType
  {
    DISABLED = -1,
    MAGNITUDE = 0,
    COMPONENT = 1,
  };

  void SetVectorMode(int mode);
  svtkGetMacro(VectorMode, int);

  svtkSetClampMacro(VectorComponent, int, 0, 3);
  svtkGetMacro(VectorComponent, int);
  //@}

protected:
  svtkSmartVolumeMapper();
  ~svtkSmartVolumeMapper() override;

  /**
   * Connect input of the svtkSmartVolumeMapper to the input of the
   * internal volume mapper by doing a shallow to avoid memory leaks.
   * \pre m_exists: m!=0
   */
  void ConnectMapperInput(svtkVolumeMapper* m);

  /**
   * Connect input of the svtkSmartVolumeMapper to the input of the
   * internal resample filter by doing a shallow to avoid memory leaks.
   * \pre m_exists: f!=0
   */
  void ConnectFilterInput(svtkImageResample* f);

  //@{
  /**
   * Window / level ivars
   */
  float FinalColorWindow;
  float FinalColorLevel;
  //@}

  //@{
  /**
   * GPU mapper-specific memory ivars.
   */
  svtkIdType MaxMemoryInBytes;
  float MaxMemoryFraction;
  //@}

  /**
   * Used for downsampling.
   */
  int InterpolationMode;

  //@{
  /**
   * The requested render mode is used to compute the current render mode. Note
   * that the current render mode can be invalid if the requested mode is not
   * supported.
   */
  int RequestedRenderMode;
  int CurrentRenderMode;
  //@}

  //@{
  /**
   * Initialization variables.
   */
  int Initialized;
  svtkTimeStamp SupportStatusCheckTime;
  int GPUSupported;
  int RayCastSupported;
  int LowResGPUNecessary;
  //@}

  /**
   * This is the resample filter that may be used if we need to
   * create a low resolution version of the volume for GPU rendering
   */
  svtkImageResample* GPUResampleFilter;

  //@{
  /**
   * This filter is used to compute the magnitude of 3-component data. MAGNITUDE
   * is one of the supported modes when rendering separately a single independent
   * component.
   *
   * \note
   * This feature was added specifically for ParaView so it might eventually be
   * moved into a derived mapper in ParaView.
   */
  svtkImageMagnitude* ImageMagnitude;
  svtkImageData* InputDataMagnitude;
  //@}

  /**
   * The initialize method. Called from ComputeRenderMode whenever something
   * relevant has changed.
   */
  void Initialize(svtkRenderer* ren, svtkVolume* vol);

  /**
   * The method that computes the render mode from the requested render mode
   * based on the support status for each render method.
   */
  void ComputeRenderMode(svtkRenderer* ren, svtkVolume* vol);

  /**
   * Expose GPU mapper for additional customization.
   */
  friend class svtkMultiBlockVolumeMapper;
  svtkGetObjectMacro(GPUMapper, svtkGPUVolumeRayCastMapper);

  //@{
  /**
   * The three potential mappers
   */
  svtkGPUVolumeRayCastMapper* GPULowResMapper;
  svtkGPUVolumeRayCastMapper* GPUMapper;
  svtkFixedPointVolumeRayCastMapper* RayCastMapper;
  //@}

  /**
   * We need to keep track of the blend mode we had when we initialized
   * because we need to reinitialize (and recheck hardware support) if
   * it changes
   */
  int InitializedBlendMode;

  /**
   * The distance between sample points along the ray
   */
  float SampleDistance;

  /**
   * Set whether or not the sample distance should be automatically calculated
   * within the internal volume mapper
   */
  svtkTypeBool AutoAdjustSampleDistances;

  /**
   * If the DesiredUpdateRate of the svtkRenderWindow causing the Render is at
   * or above this value, the render is considered interactive. Otherwise it is
   * considered still.
   */
  double InteractiveUpdateRate;

  /**
   * If the InteractiveAdjustSampleDistances flag is enabled,
   * svtkSmartVolumeMapper interactively sets and resets the
   * AutoAdjustSampleDistances flag on the internal volume mapper. This flag
   * along with InteractiveUpdateRate is useful to adjust volume mapper sample
   * distance based on whether the render is interactive or still.
   */
  svtkTypeBool InteractiveAdjustSampleDistances;

  //@{
  /**
   * VectorMode is a special rendering mode for 3-component vectors which makes
   * use of GPURayCastMapper's independent-component capabilities. In this mode,
   * a single component in the vector can be selected for rendering. In addition,
   * the mapper can compute a scalar field representing the magnitude of this vector
   * using a svtkImageMagnitude object (MAGNITUDE mode).
   */
  int VectorMode;
  int VectorComponent;
  svtkTimeStamp MagnitudeUploadTime;
  //@}

private:
  //@{
  /**
   * Adjust the GPUMapper's parameters (ColorTable, Weights, etc.) to render
   * a single component of a dataset.
   */
  void SetupVectorMode(svtkVolume* vol);
  /**
   * svtkImageMagnitude is used to compute the norm of the input multi-component
   * array. svtkImageMagnitude can only process point data, so in the case of cell
   * data it is first transformed to points.
   */
  void ComputeMagnitudeCellData(svtkImageData* input, svtkDataArray* arr);
  void ComputeMagnitudePointData(svtkImageData* input, svtkDataArray* arr);
  //@}

  svtkSmartVolumeMapper(const svtkSmartVolumeMapper&) = delete;
  void operator=(const svtkSmartVolumeMapper&) = delete;

  svtkOSPRayVolumeInterface* OSPRayMapper;
};

#endif
