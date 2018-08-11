/*
Copyright(c) 2016 Christopher Joseph Dean Schaefer

This software is provided 'as-is', without any express or implied
warranty.In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions :

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software.If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#include "Constants"
#include "Structures"
#include "Functions"

layout(std140,binding = 0) uniform LightBlock
{
	Light lights[lightCount];
};

layout(binding = 1) uniform MaterialBlock
{
	Material material;
};

layout(push_constant) uniform UniformBufferObject {
    mat4 totalTransformation;
	mat4 modelTransformation;
} ubo;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 attr1;
layout (location = 2) in vec2 attr2;
layout (location = 0) out vec4 diffuseColor;
layout (location = 1) out vec4 ambientColor;
layout (location = 2) out vec4 specularColor;
layout (location = 3) out vec4 emissiveColor;
layout (location = 4) out vec4 normal;
layout (location = 5) out vec2 texcoord;
layout (location = 6) out vec4 pos;
layout (location = 7) out vec4 globalIllumination;

out gl_PerVertex 
{
        vec4 gl_Position;
};

#include "GlobalIllumination"

void main() 
{	
	gl_Position = ubo.totalTransformation * vec4(position,1.0);
	gl_Position *= vec4(1.0,-1.0,1.0,1.0);
	pos = gl_Position;

	texcoord = attr2;

	if(colorVertex)
	{
		switch(diffuseMaterialSource)
		{
			case D3DMCS_MATERIAL:
				diffuseColor = material.Diffuse;
			break;
			case D3DMCS_COLOR1:
				diffuseColor = vec4(1.0);
			break;
			case D3DMCS_COLOR2:
				diffuseColor = vec4(1.0);
			break;
			default:
				diffuseColor = vec4(1.0);
			break;
		}
		
		switch(ambientMaterialSource)
		{
			case D3DMCS_MATERIAL:
				ambientColor = material.Ambient;
			break;
			case D3DMCS_COLOR1:
				ambientColor = vec4(1.0);
			break;
			case D3DMCS_COLOR2:
				ambientColor = vec4(1.0);
			break;
			default:
				ambientColor = vec4(1.0);
			break;
		}

		switch(specularMaterialSource)
		{
			case D3DMCS_MATERIAL:
				specularColor = material.Specular;
			break;
			case D3DMCS_COLOR1:
				specularColor = vec4(1.0);
			break;
			case D3DMCS_COLOR2:
				specularColor = vec4(1.0);
			break;
			default:
				specularColor = vec4(1.0);
			break;
		}

		switch(emissiveMaterialSource)
		{
			case D3DMCS_MATERIAL:
				emissiveColor = material.Emissive;
			break;
			case D3DMCS_COLOR1:
				emissiveColor = vec4(1.0);
			break;
			case D3DMCS_COLOR2:
				emissiveColor = vec4(1.0);
			break;
			default:
				emissiveColor = vec4(1.0);
			break;
		}		
	}
	else
	{
		diffuseColor = material.Diffuse;
		ambientColor = material.Ambient;
		specularColor = material.Specular;
		emissiveColor = material.Emissive;
	}

	normal = ubo.modelTransformation * vec4(attr1,0);
	normal *= vec4(1.0,-1.0,1.0,1.0);
	normal = normalize(normal);	

	globalIllumination = GetGlobalIllumination();
}