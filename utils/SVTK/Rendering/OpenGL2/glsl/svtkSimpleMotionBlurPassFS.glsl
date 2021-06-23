//SVTK::System::Dec

// ============================================================================
//
//  Program:   Visualization Toolkit
//  Module:    svtkTextureObjectFS.glsl
//
//  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
//  All rights reserved.
//  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
//
//     This software is distributed WITHOUT ANY WARRANTY; without even
//     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//     PURPOSE.  See the above copyright notice for more information.
//
// ============================================================================

// Fragment shader used by the gaussian blur filter render pass.

in vec2 tcoordVC;
uniform sampler2D source;
uniform float blendScale;

// the output of this shader
//SVTK::Output::Dec

void main(void)
{
  gl_FragData[0] = blendScale*texture2D(source,tcoordVC);
}
