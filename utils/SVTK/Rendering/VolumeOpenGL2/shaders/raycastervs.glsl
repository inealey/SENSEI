//SVTK::System::Dec

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    raycastervs.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/// Needed to enable inverse function
#extension GL_ARB_gpu_shader5 : enable

//////////////////////////////////////////////////////////////////////////////
///
/// Uniforms, attributes, and globals
///
//////////////////////////////////////////////////////////////////////////////
//SVTK::CustomUniforms::Dec

//SVTK::Base::Dec

//SVTK::Termination::Dec

//SVTK::Cropping::Dec

//SVTK::Shading::Dec

//////////////////////////////////////////////////////////////////////////////
///
/// Inputs
///
//////////////////////////////////////////////////////////////////////////////
in vec3 in_vertexPos;

//////////////////////////////////////////////////////////////////////////////
///
/// Outputs
///
//////////////////////////////////////////////////////////////////////////////
/// 3D texture coordinates for texture lookup in the fragment shader
out vec3 ip_textureCoords;
out vec3 ip_vertexPos;

void main()
{
  /// Get clipspace position
  //SVTK::ComputeClipPos::Impl

  /// Compute texture coordinates
  //SVTK::ComputeTextureCoords::Impl

  /// Copy incoming vertex position for the fragment shader
  ip_vertexPos = in_vertexPos;
}
