//SVTK::System::Dec

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSphereMapperGS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Template for the polydata mappers geometry shader

// primitiveID
//SVTK::PrimID::Dec

// optional color passed in from the vertex shader, vertexColor
//SVTK::Color::Dec

layout(points) in;
layout(triangle_strip, max_vertices = 3) out;

uniform int cameraParallel;

uniform mat4 VCDCMatrix;

in float radiusVCVSOutput[];
out float radiusVCGSOutput;

out vec4 vertexVCGSOutput;
out vec3 centerVCGSOutput;

// clipping plane vars
//SVTK::Clip::Dec

// picking support
//SVTK::Picking::Dec

void main()
{
  radiusVCGSOutput = radiusVCVSOutput[0];

  int i = 0;
  vec4 offset;

  vec4 base1 = vec4(1.0,0.0,0.0,0.0);
  vec4 base2 = vec4(0.0,1.0,0.0,0.0);

  // make the triangle face the camera
  if (cameraParallel == 0)
  {
    vec3 dir = normalize(-gl_in[0].gl_Position.xyz);
    base2 = vec4(normalize(cross(dir,vec3(1.0,0.0,0.0))), 0.0);
    base1 = vec4(cross(base2.xyz,dir),0.0);
  }

  //SVTK::PrimID::Impl

  //SVTK::Clip::Impl

  //SVTK::Color::Impl

  centerVCGSOutput = gl_in[0].gl_Position.xyz/gl_in[0].gl_Position.w;

  //SVTK::Picking::Impl

  // note 1.73205 = 2.0*cos(30)

  offset = vec4(-1.73205*radiusVCGSOutput, -radiusVCGSOutput, 0.0, 0.0);
  vertexVCGSOutput = gl_in[0].gl_Position + offset.x*base1 + offset.y*base2;
  gl_Position = VCDCMatrix * vertexVCGSOutput;
  EmitVertex();

  offset = vec4(1.73205*radiusVCGSOutput, -radiusVCGSOutput, 0.0, 0.0);
  vertexVCGSOutput = gl_in[0].gl_Position + offset.x*base1 + offset.y*base2;
  gl_Position = VCDCMatrix * vertexVCGSOutput;
  EmitVertex();

  offset = vec4(0.0, 2.0*radiusVCGSOutput, 0.0, 0.0);
  vertexVCGSOutput = gl_in[0].gl_Position + offset.x*base1 + offset.y*base2;
  gl_Position = VCDCMatrix * vertexVCGSOutput;
  EmitVertex();

  EndPrimitive();
}
