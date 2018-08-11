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
in a product, an acknowledgement in the product documentation would be
appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include "Utilities.h"

#ifdef _MSC_VER
#include "winres.h"
#endif
#include <stdio.h>
#include <stdlib.h>

void MergeState(const DeviceState& sourceState, DeviceState& targetState, D3DSTATEBLOCKTYPE type, BOOL onlyIfExists)
{
	/*
	Pixel https://msdn.microsoft.com/en-us/library/windows/desktop/bb147351(v=vs.85).aspx
	Vertex https://msdn.microsoft.com/en-us/library/windows/desktop/bb147353(v=vs.85).aspx#Vertex_Pipeline_Render_State
	All https://msdn.microsoft.com/en-us/library/windows/desktop/bb147350(v=vs.85).aspx
	*/

	//I don't see in render target nor depth buffer in docs but applications use them as if they are included in state blocks so I guess I'll roll with it.
	
	//&& (!onlyIfExists || targetState.mRenderTarget != nullptr)
	if (sourceState.mRenderTarget != nullptr  && (type == D3DSBT_ALL))
	{
		targetState.mRenderTarget = sourceState.mRenderTarget;
	}

	//IDirect3DDevice9::LightEnable
	//if ((type == D3DSBT_ALL || type == D3DSBT_VERTEXSTATE))
	//{
	//	BOOST_FOREACH(const auto& pair1, sourceState.mLightSettings)
	//	{
	//		if (!onlyIfExists || targetState.mLightSettings.count(pair1.first) > 0)
	//		{
	//			targetState.mLightSettings[pair1.first] = pair1.second;
	//		}
	//	}
	//}

	//IDirect3DDevice9::SetClipPlane
	//IDirect3DDevice9::SetCurrentTexturePalette

	//IDirect3DDevice9::SetVertexDeclaration
	//IDirect3DDevice9::SetFVF
	if ((sourceState.mHasVertexDeclaration || sourceState.mHasFVF) && (!onlyIfExists || targetState.mHasFVF || targetState.mHasVertexDeclaration) && (type == D3DSBT_ALL || type == D3DSBT_VERTEXSTATE))
	{
		targetState.mFVF = sourceState.mFVF;
		targetState.mHasFVF = sourceState.mHasFVF;

		targetState.mVertexDeclaration = sourceState.mVertexDeclaration;
		targetState.mHasVertexDeclaration = sourceState.mHasVertexDeclaration;
	}

	//IDirect3DDevice9::SetIndices
	if (sourceState.mHasIndexBuffer && (!onlyIfExists || targetState.mHasIndexBuffer) && (type == D3DSBT_ALL))
	{
		targetState.mIndexBuffer = sourceState.mIndexBuffer;
		targetState.mHasIndexBuffer = true;
	}

	if ((type == D3DSBT_ALL || type == D3DSBT_VERTEXSTATE))
	{
		//IDirect3DDevice9::SetLight
		for (size_t i = 0; i < sourceState.mLights.size(); i++)
		{
			if (i < targetState.mLights.size())
			{
				targetState.mLights[i] = sourceState.mLights[i];
			}
			else if (!onlyIfExists)
			{
				targetState.mLights.push_back(sourceState.mLights[i]);
			}
		}
		targetState.mAreLightsDirty = true;

		//IDirect3DDevice9::SetMaterial
		targetState.mMaterial = sourceState.mMaterial;
		targetState.mIsMaterialDirty = true;
	}

	//IDirect3DDevice9::SetNPatchMode
	if (sourceState.mNSegments != -1 && (!onlyIfExists || targetState.mNSegments != -1) && (type == D3DSBT_ALL || type == D3DSBT_VERTEXSTATE))
	{
		targetState.mNSegments = sourceState.mNSegments; //Doesn't matter anyway.
	}

	//IDirect3DDevice9::SetPixelShader
	if (sourceState.mHasPixelShader && (!onlyIfExists || targetState.mHasPixelShader) && (type == D3DSBT_ALL || type == D3DSBT_PIXELSTATE))
	{
		//if (targetState.mPixelShader != nullptr)
		//{
		//	targetState.mPixelShader->Release();
		//}

		//TODO: may leak
		targetState.mPixelShader = sourceState.mPixelShader;
		targetState.mHasPixelShader = true;
	}

	//IDirect3DDevice9::SetPixelShaderConstantB
	//IDirect3DDevice9::SetPixelShaderConstantF
	//IDirect3DDevice9::SetPixelShaderConstantI
	//IDirect3DDevice9::SetRenderState
	if (type == D3DSBT_ALL)
	{
		if (sourceState.hasFogEnable && (targetState.hasFogEnable || !onlyIfExists)) {
			targetState.hasFogEnable = true; targetState.mSpecializationConstants.fogEnable = sourceState.mSpecializationConstants.fogEnable;
		}
		if (sourceState.hasNormalizeNormals && (targetState.hasNormalizeNormals || !onlyIfExists)) {
			targetState.hasNormalizeNormals = true; targetState.mSpecializationConstants.normalizeNormals = sourceState.mSpecializationConstants.normalizeNormals;
		}
		if (sourceState.hasDebugMonitorToken && (targetState.hasDebugMonitorToken || !onlyIfExists)) {
			targetState.hasDebugMonitorToken = true; targetState.mSpecializationConstants.debugMonitorToken = sourceState.mSpecializationConstants.debugMonitorToken;
		}
	}

	if (type == D3DSBT_ALL || type == D3DSBT_VERTEXSTATE)
	{
		if (sourceState.hasCullMode && (targetState.hasCullMode || !onlyIfExists)) {
			targetState.hasCullMode = true;  targetState.mSpecializationConstants.cullMode = sourceState.mSpecializationConstants.cullMode;
		}
		if (sourceState.hasFogColor && (targetState.hasFogColor || !onlyIfExists)) {
			targetState.hasFogColor = true;  targetState.mSpecializationConstants.fogColor = sourceState.mSpecializationConstants.fogColor;
		}
		if (sourceState.hasFogTableMode && (targetState.hasFogTableMode || !onlyIfExists)) {
			targetState.hasFogTableMode = true; targetState.mSpecializationConstants.fogTableMode = sourceState.mSpecializationConstants.fogTableMode;
		}
		if (sourceState.hasFogStart && (targetState.hasFogStart || !onlyIfExists)) {
			targetState.hasFogStart = true; targetState.mSpecializationConstants.fogStart = sourceState.mSpecializationConstants.fogStart;
		}
		if (sourceState.hasFogEnd && (targetState.hasFogEnd || !onlyIfExists)) {
			targetState.hasFogEnd = true;  targetState.mSpecializationConstants.fogEnd = sourceState.mSpecializationConstants.fogEnd;
		}
		if (sourceState.hasFogDensity && (targetState.hasFogDensity || !onlyIfExists)) {
			targetState.hasFogDensity = true; targetState.mSpecializationConstants.fogDensity = sourceState.mSpecializationConstants.fogDensity;
		}
		if (sourceState.hasRangeFogEnable && (targetState.hasRangeFogEnable || !onlyIfExists)) {
			targetState.hasRangeFogEnable = true; targetState.mSpecializationConstants.rangeFogEnable = sourceState.mSpecializationConstants.rangeFogEnable;
		}
		if (sourceState.hasAmbient && (targetState.hasAmbient || !onlyIfExists)) {
			targetState.hasAmbient = true;  targetState.mSpecializationConstants.ambient = sourceState.mSpecializationConstants.ambient;
		}
		if (sourceState.hasColorVertex && (targetState.hasColorVertex || !onlyIfExists)) {
			targetState.hasColorVertex = true; targetState.mSpecializationConstants.colorVertex = sourceState.mSpecializationConstants.colorVertex;
		}
		if (sourceState.hasFogVertexMode && (targetState.hasFogVertexMode || !onlyIfExists)) {
			targetState.hasFogVertexMode = true;  targetState.mSpecializationConstants.fogVertexMode = sourceState.mSpecializationConstants.fogVertexMode;
		}
		if (sourceState.hasClipping && (targetState.hasClipping || !onlyIfExists)) {
			targetState.hasClipping = true;  targetState.mSpecializationConstants.clipping = sourceState.mSpecializationConstants.clipping;
		}
		if (sourceState.hasLighting && (targetState.hasLighting || !onlyIfExists)) {
			targetState.hasLighting = true;  targetState.mSpecializationConstants.lighting = sourceState.mSpecializationConstants.lighting;
		}
		if (sourceState.hasLocalViewer && (targetState.hasLocalViewer || !onlyIfExists)) {
			targetState.hasLocalViewer = true;  targetState.mSpecializationConstants.localViewer = sourceState.mSpecializationConstants.localViewer;
		}
		if (sourceState.hasEmissiveMaterialSource && (targetState.hasEmissiveMaterialSource || !onlyIfExists)) {
			targetState.hasEmissiveMaterialSource = true;  targetState.mSpecializationConstants.emissiveMaterialSource = sourceState.mSpecializationConstants.emissiveMaterialSource;
		}
		if (sourceState.hasAmbientMaterialSource && (targetState.hasAmbientMaterialSource || !onlyIfExists)) {
			targetState.hasAmbientMaterialSource = true;  targetState.mSpecializationConstants.ambientMaterialSource = sourceState.mSpecializationConstants.ambientMaterialSource;
		}
		if (sourceState.hasDiffuseMaterialSource && (targetState.hasDiffuseMaterialSource || !onlyIfExists)) {
			targetState.hasDiffuseMaterialSource = true;  targetState.mSpecializationConstants.diffuseMaterialSource = sourceState.mSpecializationConstants.diffuseMaterialSource;
		}
		if (sourceState.hasSpecularMaterialSource && (targetState.hasSpecularMaterialSource || !onlyIfExists)) {
			targetState.hasSpecularMaterialSource = true; targetState.mSpecializationConstants.specularMaterialSource = sourceState.mSpecializationConstants.specularMaterialSource;
		}
		if (sourceState.hasVertexBlend && (targetState.hasVertexBlend || !onlyIfExists)) {
			targetState.hasVertexBlend = true;  targetState.mSpecializationConstants.vertexBlend = sourceState.mSpecializationConstants.vertexBlend;
		}
		if (sourceState.hasClipPlaneEnable && (targetState.hasClipPlaneEnable || !onlyIfExists)) {
			targetState.hasClipPlaneEnable = true;  targetState.mSpecializationConstants.clipPlaneEnable = sourceState.mSpecializationConstants.clipPlaneEnable;
		}
		if (sourceState.hasPointSize && (targetState.hasPointSize || !onlyIfExists)) {
			targetState.hasPointSize = true;  targetState.mSpecializationConstants.pointSize = sourceState.mSpecializationConstants.pointSize;
		}
		if (sourceState.hasPointSizeMinimum && (targetState.hasPointSizeMinimum || !onlyIfExists)) {
			targetState.hasPointSizeMinimum = true;  targetState.mSpecializationConstants.pointSizeMinimum = sourceState.mSpecializationConstants.pointSizeMinimum;
		}
		if (sourceState.hasPointSpriteEnable && (targetState.hasPointSpriteEnable || !onlyIfExists)) {
			targetState.hasPointSpriteEnable = true;  targetState.mSpecializationConstants.pointSpriteEnable = sourceState.mSpecializationConstants.pointSpriteEnable;
		}
		if (sourceState.hasPointScaleEnable && (targetState.hasPointScaleEnable || !onlyIfExists)) {
			targetState.hasPointScaleEnable = true; targetState.mSpecializationConstants.pointScaleEnable = sourceState.mSpecializationConstants.pointScaleEnable;
		}
		if (sourceState.hasPointScaleA && (targetState.hasPointScaleA || !onlyIfExists)) {
			targetState.hasPointScaleA = true; targetState.mSpecializationConstants.pointScaleA = sourceState.mSpecializationConstants.pointScaleA;
		}
		if (sourceState.hasPointScaleB && (targetState.hasPointScaleB || !onlyIfExists)) {
			targetState.hasPointScaleB = true; targetState.mSpecializationConstants.pointScaleB = sourceState.mSpecializationConstants.pointScaleB;
		}
		if (sourceState.hasPointScaleC && (targetState.hasPointScaleC || !onlyIfExists)) {
			targetState.hasPointScaleC = true; targetState.mSpecializationConstants.pointScaleC = sourceState.mSpecializationConstants.pointScaleC;
		}
		if (sourceState.hasMultisampleAntiAlias && (targetState.hasMultisampleAntiAlias || !onlyIfExists)) {
			targetState.hasMultisampleAntiAlias = true; targetState.mSpecializationConstants.multisampleAntiAlias = sourceState.mSpecializationConstants.multisampleAntiAlias;
		}
		if (sourceState.hasMultisampleMask && (targetState.hasMultisampleMask || !onlyIfExists)) {
			targetState.hasMultisampleMask = true;  targetState.mSpecializationConstants.multisampleMask = sourceState.mSpecializationConstants.multisampleMask;
		}
		if (sourceState.hasPatchEdgeStyle && (targetState.hasPatchEdgeStyle || !onlyIfExists)) {
			targetState.hasPatchEdgeStyle = true;  targetState.mSpecializationConstants.patchEdgeStyle = sourceState.mSpecializationConstants.patchEdgeStyle;
		}
		if (sourceState.hasPointSizeMaximum && (targetState.hasPointSizeMaximum || !onlyIfExists)) {
			targetState.hasPointSizeMaximum = true; targetState.mSpecializationConstants.pointSizeMaximum = sourceState.mSpecializationConstants.pointSizeMaximum;
		}
		if (sourceState.hasIndexedVertexBlendEnable && (targetState.hasIndexedVertexBlendEnable || !onlyIfExists)) {
			targetState.hasIndexedVertexBlendEnable = true; targetState.mSpecializationConstants.indexedVertexBlendEnable = sourceState.mSpecializationConstants.indexedVertexBlendEnable;
		}
		if (sourceState.hasTweenFactor && (targetState.hasTweenFactor || !onlyIfExists)) {
			targetState.hasTweenFactor = true;  targetState.mSpecializationConstants.tweenFactor = sourceState.mSpecializationConstants.tweenFactor;
		}
		if (sourceState.hasPositionDegree && (targetState.hasPositionDegree || !onlyIfExists)) {
			targetState.hasPositionDegree = true; targetState.mSpecializationConstants.positionDegree = sourceState.mSpecializationConstants.positionDegree;
		}
		if (sourceState.hasNormalDegree && (targetState.hasNormalDegree || !onlyIfExists)) {
			targetState.hasNormalDegree = true;  targetState.mSpecializationConstants.normalDegree = sourceState.mSpecializationConstants.normalDegree;
		}
		if (sourceState.hasMinimumTessellationLevel && (targetState.hasMinimumTessellationLevel || !onlyIfExists)) {
			targetState.hasMinimumTessellationLevel = true; targetState.mSpecializationConstants.minimumTessellationLevel = sourceState.mSpecializationConstants.minimumTessellationLevel;
		}
		if (sourceState.hasMaximumTessellationLevel && (targetState.hasMaximumTessellationLevel || !onlyIfExists)) {
			targetState.hasMaximumTessellationLevel = true; targetState.mSpecializationConstants.maximumTessellationLevel = sourceState.mSpecializationConstants.maximumTessellationLevel;
		}
		if (sourceState.hasAdaptivetessX && (targetState.hasAdaptivetessX || !onlyIfExists)) {
			targetState.hasAdaptivetessX = true; targetState.mSpecializationConstants.adaptivetessX = sourceState.mSpecializationConstants.adaptivetessX;
		}
		if (sourceState.hasAdaptivetessY && (targetState.hasAdaptivetessY || !onlyIfExists)) {
			targetState.hasAdaptivetessY = true;  targetState.mSpecializationConstants.adaptivetessY = sourceState.mSpecializationConstants.adaptivetessY;
		}
		if (sourceState.hasAdaptivetessZ && (targetState.hasAdaptivetessZ || !onlyIfExists)) {
			targetState.hasAdaptivetessZ = true;  targetState.mSpecializationConstants.adaptivetessZ = sourceState.mSpecializationConstants.adaptivetessZ;
		}
		if (sourceState.hasAdaptivetessW && (targetState.hasAdaptivetessW || !onlyIfExists)) {
			targetState.hasAdaptivetessW = true; targetState.mSpecializationConstants.adaptivetessW = sourceState.mSpecializationConstants.adaptivetessW;
		}
		if (sourceState.hasEnableAdaptiveTessellation && (targetState.hasEnableAdaptiveTessellation || !onlyIfExists)) {
			targetState.hasEnableAdaptiveTessellation = true;  targetState.mSpecializationConstants.enableAdaptiveTessellation = sourceState.mSpecializationConstants.enableAdaptiveTessellation;
		}
	}

	if (type == D3DSBT_ALL || type == D3DSBT_PIXELSTATE)
	{
		if (sourceState.hasZEnable && (targetState.hasZEnable || !onlyIfExists)) {
			targetState.hasZEnable = true;  targetState.mSpecializationConstants.zEnable = sourceState.mSpecializationConstants.zEnable;
		}
		if (sourceState.hasSpecularEnable && (targetState.hasSpecularEnable || !onlyIfExists)) {
			targetState.hasSpecularEnable = true;  targetState.mSpecializationConstants.specularEnable = sourceState.mSpecializationConstants.specularEnable;
		}
		if (sourceState.hasFillMode && (targetState.hasFillMode || !onlyIfExists)) {
			targetState.hasFillMode = true;  targetState.mSpecializationConstants.fillMode = sourceState.mSpecializationConstants.fillMode;
		}
		if (sourceState.hasShadeMode && (targetState.hasShadeMode || !onlyIfExists)) {
			targetState.hasShadeMode = true;  targetState.mSpecializationConstants.shadeMode = sourceState.mSpecializationConstants.shadeMode;
		}
		if (sourceState.hasZWriteEnable && (targetState.hasZWriteEnable || !onlyIfExists)) {
			targetState.hasZWriteEnable = true;  targetState.mSpecializationConstants.zWriteEnable = sourceState.mSpecializationConstants.zWriteEnable;
		}
		if (sourceState.hasAlphaTestEnable && (targetState.hasAlphaTestEnable || !onlyIfExists)) {
			targetState.hasAlphaTestEnable = true; targetState.mSpecializationConstants.alphaTestEnable = sourceState.mSpecializationConstants.alphaTestEnable;
		}
		if (sourceState.hasLastPixel && (targetState.hasLastPixel || !onlyIfExists)) {
			targetState.hasLastPixel = true; targetState.mSpecializationConstants.lastPixel = sourceState.mSpecializationConstants.lastPixel;
		}
		if (sourceState.hasSourceBlend && (targetState.hasSourceBlend || !onlyIfExists)) {
			targetState.hasSourceBlend = true;  targetState.mSpecializationConstants.sourceBlend = sourceState.mSpecializationConstants.sourceBlend;
		}
		if (sourceState.hasDestinationBlend && (targetState.hasDestinationBlend || !onlyIfExists)) {
			targetState.hasDestinationBlend = true; targetState.mSpecializationConstants.destinationBlend = sourceState.mSpecializationConstants.destinationBlend;
		}
		if (sourceState.hasZFunction && (targetState.hasZFunction || !onlyIfExists)) {
			targetState.hasZFunction = true; targetState.mSpecializationConstants.zFunction = sourceState.mSpecializationConstants.zFunction;
		}
		if (sourceState.hasAlphaReference && (targetState.hasAlphaReference || !onlyIfExists)) {
			targetState.hasAlphaReference = true;  targetState.mSpecializationConstants.alphaReference = sourceState.mSpecializationConstants.alphaReference;
		}
		if (sourceState.hasAlphaFunction && (targetState.hasAlphaFunction || !onlyIfExists)) {
			targetState.hasAlphaFunction = true;  targetState.mSpecializationConstants.alphaFunction = sourceState.mSpecializationConstants.alphaFunction;
		}
		if (sourceState.hasDitherEnable && (targetState.hasDitherEnable || !onlyIfExists)) {
			targetState.hasDitherEnable = true;  targetState.mSpecializationConstants.ditherEnable = sourceState.mSpecializationConstants.ditherEnable;
		}
		if (sourceState.hasFogStart && (targetState.hasFogStart || !onlyIfExists)) {
			targetState.hasFogStart = true; targetState.mSpecializationConstants.fogStart = sourceState.mSpecializationConstants.fogStart;
		}
		if (sourceState.hasFogEnd && (targetState.hasFogEnd || !onlyIfExists)) {
			targetState.hasFogEnd = true;  targetState.mSpecializationConstants.fogEnd = sourceState.mSpecializationConstants.fogEnd;
		}
		if (sourceState.hasFogDensity && (targetState.hasFogDensity || !onlyIfExists)) {
			targetState.hasFogDensity = true; targetState.mSpecializationConstants.fogDensity = sourceState.mSpecializationConstants.fogDensity;
		}
		if (sourceState.hasAlphaBlendEnable && (targetState.hasAlphaBlendEnable || !onlyIfExists)) {
			targetState.hasAlphaBlendEnable = true;  targetState.mSpecializationConstants.alphaBlendEnable = sourceState.mSpecializationConstants.alphaBlendEnable;
		}
		if (sourceState.hasDepthBias && (targetState.hasDepthBias || !onlyIfExists)) {
			targetState.hasDepthBias = true;  targetState.mSpecializationConstants.depthBias = sourceState.mSpecializationConstants.depthBias;
		}
		if (sourceState.hasStencilEnable && (targetState.hasStencilEnable || !onlyIfExists)) {
			targetState.hasStencilEnable = true;  targetState.mSpecializationConstants.stencilEnable = sourceState.mSpecializationConstants.stencilEnable;
		}
		if (sourceState.hasStencilFail && (targetState.hasStencilFail || !onlyIfExists)) {
			targetState.hasStencilFail = true;  targetState.mSpecializationConstants.stencilFail = sourceState.mSpecializationConstants.stencilFail;
		}
		if (sourceState.hasStencilZFail && (targetState.hasStencilZFail || !onlyIfExists)) {
			targetState.hasStencilZFail = true;  targetState.mSpecializationConstants.stencilZFail = sourceState.mSpecializationConstants.stencilZFail;
		}
		if (sourceState.hasStencilPass && (targetState.hasStencilPass || !onlyIfExists)) {
			targetState.hasStencilPass = true;  targetState.mSpecializationConstants.stencilPass = sourceState.mSpecializationConstants.stencilPass;
		}
		if (sourceState.hasStencilFunction && (targetState.hasStencilFunction || !onlyIfExists)) {
			targetState.hasStencilFunction = true;  targetState.mSpecializationConstants.stencilFunction = sourceState.mSpecializationConstants.stencilFunction;
		}
		if (sourceState.hasStencilReference && (targetState.hasStencilReference || !onlyIfExists)) {
			targetState.hasStencilReference = true; targetState.mSpecializationConstants.stencilReference = sourceState.mSpecializationConstants.stencilReference;
		}
		if (sourceState.hasStencilMask && (targetState.hasStencilMask || !onlyIfExists)) {
			targetState.hasStencilMask = true; targetState.mSpecializationConstants.stencilMask = sourceState.mSpecializationConstants.stencilMask;
		}
		if (sourceState.hasStencilWriteMask && (targetState.hasStencilWriteMask || !onlyIfExists)) {
			targetState.hasStencilWriteMask = true; targetState.mSpecializationConstants.stencilWriteMask = sourceState.mSpecializationConstants.stencilWriteMask;
		}
		if (sourceState.hasTextureFactor && (targetState.hasTextureFactor || !onlyIfExists)) {
			targetState.hasTextureFactor = true;  targetState.mSpecializationConstants.textureFactor = sourceState.mSpecializationConstants.textureFactor;
		}
		if (sourceState.hasWrap0 && (targetState.hasWrap0 || !onlyIfExists)) {
			targetState.hasWrap0 = true;  targetState.mSpecializationConstants.wrap0 = sourceState.mSpecializationConstants.wrap0;
		}
		if (sourceState.hasWrap1 && (targetState.hasWrap1 || !onlyIfExists)) {
			targetState.hasWrap1 = true; targetState.mSpecializationConstants.wrap1 = sourceState.mSpecializationConstants.wrap1;
		}
		if (sourceState.hasWrap2 && (targetState.hasWrap2 || !onlyIfExists)) {
			targetState.hasWrap2 = true;  targetState.mSpecializationConstants.wrap2 = sourceState.mSpecializationConstants.wrap2;
		}
		if (sourceState.hasWrap3 && (targetState.hasWrap3 || !onlyIfExists)) {
			targetState.hasWrap3 = true;  targetState.mSpecializationConstants.wrap3 = sourceState.mSpecializationConstants.wrap3;
		}
		if (sourceState.hasWrap4 && (targetState.hasWrap4 || !onlyIfExists)) {
			targetState.hasWrap4 = true;  targetState.mSpecializationConstants.wrap4 = sourceState.mSpecializationConstants.wrap4;
		}
		if (sourceState.hasWrap5 && (targetState.hasWrap5 || !onlyIfExists)) {
			targetState.hasWrap5 = true; targetState.mSpecializationConstants.wrap5 = sourceState.mSpecializationConstants.wrap5;
		}
		if (sourceState.hasWrap6 && (targetState.hasWrap6 || !onlyIfExists)) {
			targetState.hasWrap6 = true; targetState.mSpecializationConstants.wrap6 = sourceState.mSpecializationConstants.wrap6;
		}
		if (sourceState.hasWrap7 && (targetState.hasWrap7 || !onlyIfExists)) {
			targetState.hasWrap7 = true; targetState.mSpecializationConstants.wrap7 = sourceState.mSpecializationConstants.wrap7;
		}
		if (sourceState.hasWrap8 && (targetState.hasWrap8 || !onlyIfExists)) {
			targetState.hasWrap8 = true;  targetState.mSpecializationConstants.wrap8 = sourceState.mSpecializationConstants.wrap8;
		}
		if (sourceState.hasWrap9 && (targetState.hasWrap9 || !onlyIfExists)) {
			targetState.hasWrap9 = true; targetState.mSpecializationConstants.wrap9 = sourceState.mSpecializationConstants.wrap9;
		}
		if (sourceState.hasWrap10 && (targetState.hasWrap10 || !onlyIfExists)) {
			targetState.hasWrap10 = true; targetState.mSpecializationConstants.wrap10 = sourceState.mSpecializationConstants.wrap10;
		}
		if (sourceState.hasWrap11 && (targetState.hasWrap11 || !onlyIfExists)) {
			targetState.hasWrap11 = true;  targetState.mSpecializationConstants.wrap11 = sourceState.mSpecializationConstants.wrap11;
		}
		if (sourceState.hasWrap12 && (targetState.hasWrap12 || !onlyIfExists)) {
			targetState.hasWrap12 = true;  targetState.mSpecializationConstants.wrap12 = sourceState.mSpecializationConstants.wrap12;
		}
		if (sourceState.hasWrap13 && (targetState.hasWrap13 || !onlyIfExists)) {
			targetState.hasWrap13 = true; targetState.mSpecializationConstants.wrap13 = sourceState.mSpecializationConstants.wrap13;
		}
		if (sourceState.hasWrap14 && (targetState.hasWrap14 || !onlyIfExists)) {
			targetState.hasWrap14 = true; targetState.mSpecializationConstants.wrap14 = sourceState.mSpecializationConstants.wrap14;
		}
		if (sourceState.hasWrap15 && (targetState.hasWrap15 || !onlyIfExists)) {
			targetState.hasWrap15 = true;  targetState.mSpecializationConstants.wrap15 = sourceState.mSpecializationConstants.wrap15;
		}
		if (sourceState.hasLocalViewer && (targetState.hasLocalViewer || !onlyIfExists)) {
			targetState.hasLocalViewer = true;  targetState.mSpecializationConstants.localViewer = sourceState.mSpecializationConstants.localViewer;
		}
		if (sourceState.hasEmissiveMaterialSource && (targetState.hasEmissiveMaterialSource || !onlyIfExists)) {
			targetState.hasEmissiveMaterialSource = true;  targetState.mSpecializationConstants.emissiveMaterialSource = sourceState.mSpecializationConstants.emissiveMaterialSource;
		}
		if (sourceState.hasAmbientMaterialSource && (targetState.hasAmbientMaterialSource || !onlyIfExists)) {
			targetState.hasAmbientMaterialSource = true;  targetState.mSpecializationConstants.ambientMaterialSource = sourceState.mSpecializationConstants.ambientMaterialSource;
		}
		if (sourceState.hasDiffuseMaterialSource && (targetState.hasDiffuseMaterialSource || !onlyIfExists)) {
			targetState.hasDiffuseMaterialSource = true;  targetState.mSpecializationConstants.diffuseMaterialSource = sourceState.mSpecializationConstants.diffuseMaterialSource;
		}
		if (sourceState.hasSpecularMaterialSource && (targetState.hasSpecularMaterialSource || !onlyIfExists)) {
			targetState.hasSpecularMaterialSource = true; targetState.mSpecializationConstants.specularMaterialSource = sourceState.mSpecializationConstants.specularMaterialSource;
		}
		if (sourceState.hasColorWriteEnable && (targetState.hasColorWriteEnable || !onlyIfExists)) {
			targetState.hasColorWriteEnable = true; targetState.mSpecializationConstants.colorWriteEnable = sourceState.mSpecializationConstants.colorWriteEnable;
		}
		if (sourceState.hasBlendOperation && (targetState.hasBlendOperation || !onlyIfExists)) {
			targetState.hasBlendOperation = true; targetState.mSpecializationConstants.blendOperation = sourceState.mSpecializationConstants.blendOperation;
		}
		if (sourceState.hasScissorTestEnable && (targetState.hasScissorTestEnable || !onlyIfExists)) {
			targetState.hasScissorTestEnable = true; targetState.mSpecializationConstants.scissorTestEnable = sourceState.mSpecializationConstants.scissorTestEnable;
		}
		if (sourceState.hasSlopeScaleDepthBias && (targetState.hasSlopeScaleDepthBias || !onlyIfExists)) {
			targetState.hasSlopeScaleDepthBias = true;  targetState.mSpecializationConstants.slopeScaleDepthBias = sourceState.mSpecializationConstants.slopeScaleDepthBias;
		}
		if (sourceState.hasAntiAliasedLineEnable && (targetState.hasAntiAliasedLineEnable || !onlyIfExists)) {
			targetState.hasAntiAliasedLineEnable = true;  targetState.mSpecializationConstants.antiAliasedLineEnable = sourceState.mSpecializationConstants.antiAliasedLineEnable;
		}
		if (sourceState.hasTwoSidedStencilMode && (targetState.hasTwoSidedStencilMode || !onlyIfExists)) {
			targetState.hasTwoSidedStencilMode = true; targetState.mSpecializationConstants.twoSidedStencilMode = sourceState.mSpecializationConstants.twoSidedStencilMode;
		}
		if (sourceState.hasCcwStencilFail && (targetState.hasCcwStencilFail || !onlyIfExists)) {
			targetState.hasCcwStencilFail = true;  targetState.mSpecializationConstants.ccwStencilFail = sourceState.mSpecializationConstants.ccwStencilFail;
		}
		if (sourceState.hasCcwStencilZFail && (targetState.hasCcwStencilZFail || !onlyIfExists)) {
			targetState.hasCcwStencilZFail = true; targetState.mSpecializationConstants.ccwStencilZFail = sourceState.mSpecializationConstants.ccwStencilZFail;
		}
		if (sourceState.hasCcwStencilPass && (targetState.hasCcwStencilPass || !onlyIfExists)) {
			targetState.hasCcwStencilPass = true;  targetState.mSpecializationConstants.ccwStencilPass = sourceState.mSpecializationConstants.ccwStencilPass;
		}
		if (sourceState.hasCcwStencilFunction && (targetState.hasCcwStencilFunction || !onlyIfExists)) {
			targetState.hasCcwStencilFunction = true; targetState.mSpecializationConstants.ccwStencilFunction = sourceState.mSpecializationConstants.ccwStencilFunction;
		}
		if (sourceState.hasColorWriteEnable1 && (targetState.hasColorWriteEnable1 || !onlyIfExists)) {
			targetState.hasColorWriteEnable1 = true; targetState.mSpecializationConstants.colorWriteEnable1 = sourceState.mSpecializationConstants.colorWriteEnable1;
		}
		if (sourceState.hasColorWriteEnable2 && (targetState.hasColorWriteEnable2 || !onlyIfExists)) {
			targetState.hasColorWriteEnable2 = true; targetState.mSpecializationConstants.colorWriteEnable2 = sourceState.mSpecializationConstants.colorWriteEnable2;
		}
		if (sourceState.hasColorWriteEnable3 && (targetState.hasColorWriteEnable3 || !onlyIfExists)) {
			targetState.hasColorWriteEnable3 = true;  targetState.mSpecializationConstants.colorWriteEnable3 = sourceState.mSpecializationConstants.colorWriteEnable3;
		}
		if (sourceState.hasBlendFactor && (targetState.hasBlendFactor || !onlyIfExists)) {
			targetState.hasBlendFactor = true;  targetState.mSpecializationConstants.blendFactor = sourceState.mSpecializationConstants.blendFactor;
		}
		if (sourceState.hasSrgbWriteEnable && (targetState.hasSrgbWriteEnable || !onlyIfExists)) {
			targetState.hasSrgbWriteEnable = true;  targetState.mSpecializationConstants.srgbWriteEnable = sourceState.mSpecializationConstants.srgbWriteEnable;
		}
		if (sourceState.hasSeparateAlphaBlendEnable && (targetState.hasSeparateAlphaBlendEnable || !onlyIfExists)) {
			targetState.hasSeparateAlphaBlendEnable = true; targetState.mSpecializationConstants.separateAlphaBlendEnable = sourceState.mSpecializationConstants.separateAlphaBlendEnable;
		}
		if (sourceState.hasSourceBlendAlpha && (targetState.hasSourceBlendAlpha || !onlyIfExists)) {
			targetState.hasSourceBlendAlpha = true;  targetState.mSpecializationConstants.sourceBlendAlpha = sourceState.mSpecializationConstants.sourceBlendAlpha;
		}
		if (sourceState.hasDestinationBlendAlpha && (targetState.hasDestinationBlendAlpha || !onlyIfExists)) {
			targetState.hasDestinationBlendAlpha = true;  targetState.mSpecializationConstants.destinationBlendAlpha = sourceState.mSpecializationConstants.destinationBlendAlpha;
		}
		if (sourceState.hasBlendOperationAlpha && (targetState.hasBlendOperationAlpha || !onlyIfExists)) {
			targetState.hasBlendOperationAlpha = true;  targetState.mSpecializationConstants.blendOperationAlpha = sourceState.mSpecializationConstants.blendOperationAlpha;
		}
	}

	//IDirect3DDevice9::SetSamplerState
	if (sourceState.mSamplerStates.size())
	{
		BOOST_FOREACH(const auto& pair1, sourceState.mSamplerStates)
		{
			BOOST_FOREACH(const auto& pair2, pair1.second)
			{
				if (!onlyIfExists || (targetState.mSamplerStates.count(pair1.first) > 0 && targetState.mSamplerStates[pair1.first].count(pair2.first) > 0))
				{
					if
						(
						(type == D3DSBT_ALL) ||
							(type == D3DSBT_VERTEXSTATE &&
							(
								pair2.first == D3DSAMP_DMAPOFFSET

								)) ||
								(type == D3DSBT_PIXELSTATE &&
							(
								pair2.first == D3DSAMP_ADDRESSU ||
								pair2.first == D3DSAMP_ADDRESSV ||
								pair2.first == D3DSAMP_ADDRESSW ||
								pair2.first == D3DSAMP_BORDERCOLOR ||
								pair2.first == D3DSAMP_MAGFILTER ||
								pair2.first == D3DSAMP_MINFILTER ||
								pair2.first == D3DSAMP_MIPFILTER ||
								pair2.first == D3DSAMP_MIPMAPLODBIAS ||
								pair2.first == D3DSAMP_MAXMIPLEVEL ||
								pair2.first == D3DSAMP_MAXANISOTROPY ||
								pair2.first == D3DSAMP_SRGBTEXTURE ||
								pair2.first == D3DSAMP_ELEMENTINDEX
								))
							)
					{
						targetState.mSamplerStates[pair1.first][pair2.first] = pair2.second;
					}
				}
			}
		}
	}
	//targetState.mSamplerStates = sourceState.mSamplerStates;

	//IDirect3DDevice9::SetScissorRect
	if ((sourceState.m9Scissor.right != 0 || sourceState.m9Scissor.left != 0) && (!onlyIfExists || targetState.mHasIndexBuffer) && (type == D3DSBT_ALL))
	{
		targetState.m9Scissor = sourceState.m9Scissor;
		targetState.mScissor = sourceState.mScissor;
	}

	//IDirect3DDevice9::SetStreamSource
	if (sourceState.mStreamSources.size())
	{
		BOOST_FOREACH(const auto& pair1, sourceState.mStreamSources)
		{
			if (type == D3DSBT_ALL && (!onlyIfExists || targetState.mStreamSources.count(pair1.first) > 0))
			{
				targetState.mStreamSources[pair1.first] = pair1.second;
			}
		}
	}

	//IDirect3DDevice9::SetStreamSourceFreq
	//IDirect3DDevice9::SetTexture
	//BOOST_FOREACH(const auto& pair1, sourceState.mTextures)
	//{
	//	if ((type == D3DSBT_ALL))
	//	{
	//		targetState.mTextures[pair1.first] = pair1.second;
	//	}
	//}

	for (size_t i = 0; i < 16; i++)
	{
		if ((type == D3DSBT_ALL))
		{
			targetState.mTextures[i] = sourceState.mTextures[i];
		}
	}

	//IDirect3DDevice9::SetTextureStageState
	if (type == D3DSBT_VERTEXSTATE || type == D3DSBT_ALL)
	{
		targetState.mSpecializationConstants.texureCoordinateIndex_0 = sourceState.mSpecializationConstants.texureCoordinateIndex_0;
		targetState.mSpecializationConstants.texureCoordinateIndex_1 = sourceState.mSpecializationConstants.texureCoordinateIndex_1;
		targetState.mSpecializationConstants.texureCoordinateIndex_2 = sourceState.mSpecializationConstants.texureCoordinateIndex_2;
		targetState.mSpecializationConstants.texureCoordinateIndex_3 = sourceState.mSpecializationConstants.texureCoordinateIndex_3;
		targetState.mSpecializationConstants.texureCoordinateIndex_4 = sourceState.mSpecializationConstants.texureCoordinateIndex_4;
		targetState.mSpecializationConstants.texureCoordinateIndex_5 = sourceState.mSpecializationConstants.texureCoordinateIndex_5;
		targetState.mSpecializationConstants.texureCoordinateIndex_6 = sourceState.mSpecializationConstants.texureCoordinateIndex_6;
		targetState.mSpecializationConstants.texureCoordinateIndex_7 = sourceState.mSpecializationConstants.texureCoordinateIndex_7;

		targetState.mSpecializationConstants.textureTransformationFlags_0 = sourceState.mSpecializationConstants.textureTransformationFlags_0;
		targetState.mSpecializationConstants.textureTransformationFlags_1 = sourceState.mSpecializationConstants.textureTransformationFlags_1;
		targetState.mSpecializationConstants.textureTransformationFlags_2 = sourceState.mSpecializationConstants.textureTransformationFlags_2;
		targetState.mSpecializationConstants.textureTransformationFlags_3 = sourceState.mSpecializationConstants.textureTransformationFlags_3;
		targetState.mSpecializationConstants.textureTransformationFlags_4 = sourceState.mSpecializationConstants.textureTransformationFlags_4;
		targetState.mSpecializationConstants.textureTransformationFlags_5 = sourceState.mSpecializationConstants.textureTransformationFlags_5;
		targetState.mSpecializationConstants.textureTransformationFlags_6 = sourceState.mSpecializationConstants.textureTransformationFlags_6;
		targetState.mSpecializationConstants.textureTransformationFlags_7 = sourceState.mSpecializationConstants.textureTransformationFlags_7;
	}

	if (type == D3DSBT_PIXELSTATE || type == D3DSBT_ALL)
	{
		targetState.mSpecializationConstants.colorOperation_0 = sourceState.mSpecializationConstants.colorOperation_0;
		targetState.mSpecializationConstants.colorOperation_1 = sourceState.mSpecializationConstants.colorOperation_1;
		targetState.mSpecializationConstants.colorOperation_2 = sourceState.mSpecializationConstants.colorOperation_2;
		targetState.mSpecializationConstants.colorOperation_3 = sourceState.mSpecializationConstants.colorOperation_3;
		targetState.mSpecializationConstants.colorOperation_4 = sourceState.mSpecializationConstants.colorOperation_4;
		targetState.mSpecializationConstants.colorOperation_5 = sourceState.mSpecializationConstants.colorOperation_5;
		targetState.mSpecializationConstants.colorOperation_6 = sourceState.mSpecializationConstants.colorOperation_6;
		targetState.mSpecializationConstants.colorOperation_7 = sourceState.mSpecializationConstants.colorOperation_7;

		targetState.mSpecializationConstants.colorArgument1_0 = sourceState.mSpecializationConstants.colorArgument1_0;
		targetState.mSpecializationConstants.colorArgument1_1 = sourceState.mSpecializationConstants.colorArgument1_1;
		targetState.mSpecializationConstants.colorArgument1_2 = sourceState.mSpecializationConstants.colorArgument1_2;
		targetState.mSpecializationConstants.colorArgument1_3 = sourceState.mSpecializationConstants.colorArgument1_3;
		targetState.mSpecializationConstants.colorArgument1_4 = sourceState.mSpecializationConstants.colorArgument1_4;
		targetState.mSpecializationConstants.colorArgument1_5 = sourceState.mSpecializationConstants.colorArgument1_5;
		targetState.mSpecializationConstants.colorArgument1_6 = sourceState.mSpecializationConstants.colorArgument1_6;
		targetState.mSpecializationConstants.colorArgument1_7 = sourceState.mSpecializationConstants.colorArgument1_7;

		targetState.mSpecializationConstants.colorArgument2_0 = sourceState.mSpecializationConstants.colorArgument2_0;
		targetState.mSpecializationConstants.colorArgument2_1 = sourceState.mSpecializationConstants.colorArgument2_1;
		targetState.mSpecializationConstants.colorArgument2_2 = sourceState.mSpecializationConstants.colorArgument2_2;
		targetState.mSpecializationConstants.colorArgument2_3 = sourceState.mSpecializationConstants.colorArgument2_3;
		targetState.mSpecializationConstants.colorArgument2_4 = sourceState.mSpecializationConstants.colorArgument2_4;
		targetState.mSpecializationConstants.colorArgument2_5 = sourceState.mSpecializationConstants.colorArgument2_5;
		targetState.mSpecializationConstants.colorArgument2_6 = sourceState.mSpecializationConstants.colorArgument2_6;
		targetState.mSpecializationConstants.colorArgument2_7 = sourceState.mSpecializationConstants.colorArgument2_7;

		targetState.mSpecializationConstants.alphaOperation_0 = sourceState.mSpecializationConstants.alphaOperation_0;
		targetState.mSpecializationConstants.alphaOperation_0 = sourceState.mSpecializationConstants.alphaOperation_0;
		targetState.mSpecializationConstants.alphaOperation_0 = sourceState.mSpecializationConstants.alphaOperation_0;
		targetState.mSpecializationConstants.alphaOperation_0 = sourceState.mSpecializationConstants.alphaOperation_0;
		targetState.mSpecializationConstants.alphaOperation_0 = sourceState.mSpecializationConstants.alphaOperation_0;
		targetState.mSpecializationConstants.alphaOperation_0 = sourceState.mSpecializationConstants.alphaOperation_0;
		targetState.mSpecializationConstants.alphaOperation_0 = sourceState.mSpecializationConstants.alphaOperation_0;
		targetState.mSpecializationConstants.alphaOperation_0 = sourceState.mSpecializationConstants.alphaOperation_0;

		targetState.mSpecializationConstants.alphaOperation_0 = sourceState.mSpecializationConstants.alphaOperation_0;
		targetState.mSpecializationConstants.alphaOperation_1 = sourceState.mSpecializationConstants.alphaOperation_1;
		targetState.mSpecializationConstants.alphaOperation_2 = sourceState.mSpecializationConstants.alphaOperation_2;
		targetState.mSpecializationConstants.alphaOperation_3 = sourceState.mSpecializationConstants.alphaOperation_3;
		targetState.mSpecializationConstants.alphaOperation_4 = sourceState.mSpecializationConstants.alphaOperation_4;
		targetState.mSpecializationConstants.alphaOperation_5 = sourceState.mSpecializationConstants.alphaOperation_5;
		targetState.mSpecializationConstants.alphaOperation_6 = sourceState.mSpecializationConstants.alphaOperation_6;
		targetState.mSpecializationConstants.alphaOperation_7 = sourceState.mSpecializationConstants.alphaOperation_7;

		targetState.mSpecializationConstants.alphaArgument1_0 = sourceState.mSpecializationConstants.alphaArgument1_0;
		targetState.mSpecializationConstants.alphaArgument1_1 = sourceState.mSpecializationConstants.alphaArgument1_1;
		targetState.mSpecializationConstants.alphaArgument1_2 = sourceState.mSpecializationConstants.alphaArgument1_2;
		targetState.mSpecializationConstants.alphaArgument1_3 = sourceState.mSpecializationConstants.alphaArgument1_3;
		targetState.mSpecializationConstants.alphaArgument1_4 = sourceState.mSpecializationConstants.alphaArgument1_4;
		targetState.mSpecializationConstants.alphaArgument1_5 = sourceState.mSpecializationConstants.alphaArgument1_5;
		targetState.mSpecializationConstants.alphaArgument1_6 = sourceState.mSpecializationConstants.alphaArgument1_6;
		targetState.mSpecializationConstants.alphaArgument1_7 = sourceState.mSpecializationConstants.alphaArgument1_7;

		targetState.mSpecializationConstants.alphaArgument2_0 = sourceState.mSpecializationConstants.alphaArgument2_0;
		targetState.mSpecializationConstants.alphaArgument2_1 = sourceState.mSpecializationConstants.alphaArgument2_1;
		targetState.mSpecializationConstants.alphaArgument2_2 = sourceState.mSpecializationConstants.alphaArgument2_2;
		targetState.mSpecializationConstants.alphaArgument2_3 = sourceState.mSpecializationConstants.alphaArgument2_3;
		targetState.mSpecializationConstants.alphaArgument2_4 = sourceState.mSpecializationConstants.alphaArgument2_4;
		targetState.mSpecializationConstants.alphaArgument2_5 = sourceState.mSpecializationConstants.alphaArgument2_5;
		targetState.mSpecializationConstants.alphaArgument2_6 = sourceState.mSpecializationConstants.alphaArgument2_6;
		targetState.mSpecializationConstants.alphaArgument2_7 = sourceState.mSpecializationConstants.alphaArgument2_7;

		targetState.mSpecializationConstants.bumpMapMatrix00_0 = sourceState.mSpecializationConstants.bumpMapMatrix00_0;
		targetState.mSpecializationConstants.bumpMapMatrix00_1 = sourceState.mSpecializationConstants.bumpMapMatrix00_1;
		targetState.mSpecializationConstants.bumpMapMatrix00_2 = sourceState.mSpecializationConstants.bumpMapMatrix00_2;
		targetState.mSpecializationConstants.bumpMapMatrix00_3 = sourceState.mSpecializationConstants.bumpMapMatrix00_3;
		targetState.mSpecializationConstants.bumpMapMatrix00_4 = sourceState.mSpecializationConstants.bumpMapMatrix00_4;
		targetState.mSpecializationConstants.bumpMapMatrix00_5 = sourceState.mSpecializationConstants.bumpMapMatrix00_5;
		targetState.mSpecializationConstants.bumpMapMatrix00_6 = sourceState.mSpecializationConstants.bumpMapMatrix00_6;
		targetState.mSpecializationConstants.bumpMapMatrix00_7 = sourceState.mSpecializationConstants.bumpMapMatrix00_7;

		targetState.mSpecializationConstants.bumpMapMatrix01_0 = sourceState.mSpecializationConstants.bumpMapMatrix01_0;
		targetState.mSpecializationConstants.bumpMapMatrix01_1 = sourceState.mSpecializationConstants.bumpMapMatrix01_1;
		targetState.mSpecializationConstants.bumpMapMatrix01_2 = sourceState.mSpecializationConstants.bumpMapMatrix01_2;
		targetState.mSpecializationConstants.bumpMapMatrix01_3 = sourceState.mSpecializationConstants.bumpMapMatrix01_3;
		targetState.mSpecializationConstants.bumpMapMatrix01_4 = sourceState.mSpecializationConstants.bumpMapMatrix01_4;
		targetState.mSpecializationConstants.bumpMapMatrix01_5 = sourceState.mSpecializationConstants.bumpMapMatrix01_5;
		targetState.mSpecializationConstants.bumpMapMatrix01_6 = sourceState.mSpecializationConstants.bumpMapMatrix01_6;
		targetState.mSpecializationConstants.bumpMapMatrix01_7 = sourceState.mSpecializationConstants.bumpMapMatrix01_7;

		targetState.mSpecializationConstants.bumpMapMatrix10_0 = sourceState.mSpecializationConstants.bumpMapMatrix10_0;
		targetState.mSpecializationConstants.bumpMapMatrix10_1 = sourceState.mSpecializationConstants.bumpMapMatrix10_1;
		targetState.mSpecializationConstants.bumpMapMatrix10_2 = sourceState.mSpecializationConstants.bumpMapMatrix10_2;
		targetState.mSpecializationConstants.bumpMapMatrix10_3 = sourceState.mSpecializationConstants.bumpMapMatrix10_3;
		targetState.mSpecializationConstants.bumpMapMatrix10_4 = sourceState.mSpecializationConstants.bumpMapMatrix10_4;
		targetState.mSpecializationConstants.bumpMapMatrix10_5 = sourceState.mSpecializationConstants.bumpMapMatrix10_5;
		targetState.mSpecializationConstants.bumpMapMatrix10_6 = sourceState.mSpecializationConstants.bumpMapMatrix10_6;
		targetState.mSpecializationConstants.bumpMapMatrix10_7 = sourceState.mSpecializationConstants.bumpMapMatrix10_7;

		targetState.mSpecializationConstants.bumpMapMatrix11_0 = sourceState.mSpecializationConstants.bumpMapMatrix11_0;
		targetState.mSpecializationConstants.bumpMapMatrix11_1 = sourceState.mSpecializationConstants.bumpMapMatrix11_1;
		targetState.mSpecializationConstants.bumpMapMatrix11_2 = sourceState.mSpecializationConstants.bumpMapMatrix11_2;
		targetState.mSpecializationConstants.bumpMapMatrix11_3 = sourceState.mSpecializationConstants.bumpMapMatrix11_3;
		targetState.mSpecializationConstants.bumpMapMatrix11_4 = sourceState.mSpecializationConstants.bumpMapMatrix11_4;
		targetState.mSpecializationConstants.bumpMapMatrix11_5 = sourceState.mSpecializationConstants.bumpMapMatrix11_5;
		targetState.mSpecializationConstants.bumpMapMatrix11_6 = sourceState.mSpecializationConstants.bumpMapMatrix11_6;
		targetState.mSpecializationConstants.bumpMapMatrix11_7 = sourceState.mSpecializationConstants.bumpMapMatrix11_7;

		targetState.mSpecializationConstants.texureCoordinateIndex_0 = sourceState.mSpecializationConstants.texureCoordinateIndex_0;
		targetState.mSpecializationConstants.texureCoordinateIndex_1 = sourceState.mSpecializationConstants.texureCoordinateIndex_1;
		targetState.mSpecializationConstants.texureCoordinateIndex_2 = sourceState.mSpecializationConstants.texureCoordinateIndex_2;
		targetState.mSpecializationConstants.texureCoordinateIndex_3 = sourceState.mSpecializationConstants.texureCoordinateIndex_3;
		targetState.mSpecializationConstants.texureCoordinateIndex_4 = sourceState.mSpecializationConstants.texureCoordinateIndex_4;
		targetState.mSpecializationConstants.texureCoordinateIndex_5 = sourceState.mSpecializationConstants.texureCoordinateIndex_5;
		targetState.mSpecializationConstants.texureCoordinateIndex_6 = sourceState.mSpecializationConstants.texureCoordinateIndex_6;
		targetState.mSpecializationConstants.texureCoordinateIndex_7 = sourceState.mSpecializationConstants.texureCoordinateIndex_7;

		targetState.mSpecializationConstants.bumpMapScale_0 = sourceState.mSpecializationConstants.bumpMapScale_0;
		targetState.mSpecializationConstants.bumpMapScale_1 = sourceState.mSpecializationConstants.bumpMapScale_1;
		targetState.mSpecializationConstants.bumpMapScale_2 = sourceState.mSpecializationConstants.bumpMapScale_2;
		targetState.mSpecializationConstants.bumpMapScale_3 = sourceState.mSpecializationConstants.bumpMapScale_3;
		targetState.mSpecializationConstants.bumpMapScale_4 = sourceState.mSpecializationConstants.bumpMapScale_4;
		targetState.mSpecializationConstants.bumpMapScale_5 = sourceState.mSpecializationConstants.bumpMapScale_5;
		targetState.mSpecializationConstants.bumpMapScale_6 = sourceState.mSpecializationConstants.bumpMapScale_6;
		targetState.mSpecializationConstants.bumpMapScale_7 = sourceState.mSpecializationConstants.bumpMapScale_7;

		targetState.mSpecializationConstants.bumpMapOffset_0 = sourceState.mSpecializationConstants.bumpMapOffset_0;
		targetState.mSpecializationConstants.bumpMapOffset_1 = sourceState.mSpecializationConstants.bumpMapOffset_1;
		targetState.mSpecializationConstants.bumpMapOffset_2 = sourceState.mSpecializationConstants.bumpMapOffset_2;
		targetState.mSpecializationConstants.bumpMapOffset_3 = sourceState.mSpecializationConstants.bumpMapOffset_3;
		targetState.mSpecializationConstants.bumpMapOffset_4 = sourceState.mSpecializationConstants.bumpMapOffset_4;
		targetState.mSpecializationConstants.bumpMapOffset_5 = sourceState.mSpecializationConstants.bumpMapOffset_5;
		targetState.mSpecializationConstants.bumpMapOffset_6 = sourceState.mSpecializationConstants.bumpMapOffset_6;
		targetState.mSpecializationConstants.bumpMapOffset_7 = sourceState.mSpecializationConstants.bumpMapOffset_7;

		targetState.mSpecializationConstants.textureTransformationFlags_0 = sourceState.mSpecializationConstants.textureTransformationFlags_0;
		targetState.mSpecializationConstants.textureTransformationFlags_1 = sourceState.mSpecializationConstants.textureTransformationFlags_1;
		targetState.mSpecializationConstants.textureTransformationFlags_2 = sourceState.mSpecializationConstants.textureTransformationFlags_2;
		targetState.mSpecializationConstants.textureTransformationFlags_3 = sourceState.mSpecializationConstants.textureTransformationFlags_3;
		targetState.mSpecializationConstants.textureTransformationFlags_4 = sourceState.mSpecializationConstants.textureTransformationFlags_4;
		targetState.mSpecializationConstants.textureTransformationFlags_5 = sourceState.mSpecializationConstants.textureTransformationFlags_5;
		targetState.mSpecializationConstants.textureTransformationFlags_6 = sourceState.mSpecializationConstants.textureTransformationFlags_6;
		targetState.mSpecializationConstants.textureTransformationFlags_7 = sourceState.mSpecializationConstants.textureTransformationFlags_7;

		targetState.mSpecializationConstants.colorArgument0_0 = sourceState.mSpecializationConstants.colorArgument0_0;
		targetState.mSpecializationConstants.colorArgument0_1 = sourceState.mSpecializationConstants.colorArgument0_1;
		targetState.mSpecializationConstants.colorArgument0_2 = sourceState.mSpecializationConstants.colorArgument0_2;
		targetState.mSpecializationConstants.colorArgument0_3 = sourceState.mSpecializationConstants.colorArgument0_3;
		targetState.mSpecializationConstants.colorArgument0_4 = sourceState.mSpecializationConstants.colorArgument0_4;
		targetState.mSpecializationConstants.colorArgument0_5 = sourceState.mSpecializationConstants.colorArgument0_5;
		targetState.mSpecializationConstants.colorArgument0_6 = sourceState.mSpecializationConstants.colorArgument0_6;
		targetState.mSpecializationConstants.colorArgument0_7 = sourceState.mSpecializationConstants.colorArgument0_7;

		targetState.mSpecializationConstants.alphaArgument0_0 = sourceState.mSpecializationConstants.alphaArgument0_0;
		targetState.mSpecializationConstants.alphaArgument0_1 = sourceState.mSpecializationConstants.alphaArgument0_1;
		targetState.mSpecializationConstants.alphaArgument0_2 = sourceState.mSpecializationConstants.alphaArgument0_2;
		targetState.mSpecializationConstants.alphaArgument0_3 = sourceState.mSpecializationConstants.alphaArgument0_3;
		targetState.mSpecializationConstants.alphaArgument0_4 = sourceState.mSpecializationConstants.alphaArgument0_4;
		targetState.mSpecializationConstants.alphaArgument0_5 = sourceState.mSpecializationConstants.alphaArgument0_5;
		targetState.mSpecializationConstants.alphaArgument0_6 = sourceState.mSpecializationConstants.alphaArgument0_6;
		targetState.mSpecializationConstants.alphaArgument0_7 = sourceState.mSpecializationConstants.alphaArgument0_7;

		targetState.mSpecializationConstants.Result_0 = sourceState.mSpecializationConstants.Result_0;
		targetState.mSpecializationConstants.Result_1 = sourceState.mSpecializationConstants.Result_1;
		targetState.mSpecializationConstants.Result_2 = sourceState.mSpecializationConstants.Result_2;
		targetState.mSpecializationConstants.Result_3 = sourceState.mSpecializationConstants.Result_3;
		targetState.mSpecializationConstants.Result_4 = sourceState.mSpecializationConstants.Result_4;
		targetState.mSpecializationConstants.Result_5 = sourceState.mSpecializationConstants.Result_5;
		targetState.mSpecializationConstants.Result_6 = sourceState.mSpecializationConstants.Result_6;
		targetState.mSpecializationConstants.Result_7 = sourceState.mSpecializationConstants.Result_7;
	}

	//IDirect3DDevice9::SetTransform
	if (type == D3DSBT_ALL)
	{
		BOOST_FOREACH(const auto& pair1, sourceState.mTransforms)
		{
			if (!onlyIfExists || targetState.mTransforms.count(pair1.first) > 0)
			{
				targetState.mTransforms[pair1.first] = pair1.second;
			}
		}
	}

	//IDirect3DDevice9::SetViewport
	if ((sourceState.m9Viewport.Width != 0) && (!onlyIfExists || targetState.m9Viewport.Width != 0) && (type == D3DSBT_ALL))
	{
		targetState.m9Viewport = sourceState.m9Viewport;
		targetState.mViewport = sourceState.mViewport;
	}

	//IDirect3DDevice9::SetVertexShader
	if (sourceState.mHasVertexShader && (!onlyIfExists || targetState.mHasVertexShader) && (type == D3DSBT_ALL || type == D3DSBT_VERTEXSTATE))
	{
		//if (targetState.mVertexShader != nullptr)
		//{
		//	targetState.mVertexShader->Release();
		//}

		targetState.mVertexShader = sourceState.mVertexShader;
		targetState.mHasVertexShader = true;
	}

	//IDirect3DDevice9::SetVertexShaderConstantB
	//IDirect3DDevice9::SetVertexShaderConstantF
	//IDirect3DDevice9::SetVertexShaderConstantI
}

HMODULE GetModule(HMODULE module)
{
	static HMODULE dllModule = 0;

	if (module != 0)
	{
		dllModule = module;
	}

	return dllModule;
}

vk::ShaderModule LoadShaderFromFile(vk::Device device, const char *filename)
{
	vk::ShaderModuleCreateInfo moduleCreateInfo;
	vk::ShaderModule module;
	vk::Result result;
	FILE *fp = fopen(filename, "rb");
	if (fp != nullptr)
	{
		fseek(fp, 0L, SEEK_END);
		int32_t dataSize = ftell(fp);
		fseek(fp, 0L, SEEK_SET);
		void* data = (uint32_t*)malloc(dataSize);
		if (data != nullptr && fread(data, dataSize, 1, fp))
		{
			moduleCreateInfo.codeSize = dataSize;
			moduleCreateInfo.pCode = (uint32_t*)data; //Why is this uint32_t* if the size is in bytes?

			result = device.createShaderModule(&moduleCreateInfo, nullptr, &module);
			if (result != vk::Result::eSuccess)
			{
				BOOST_LOG_TRIVIAL(fatal) << "LoadShaderFromFile vkCreateShaderModule failed with return code of " << GetResultString((VkResult)result);
			}
			else
			{
				BOOST_LOG_TRIVIAL(info) << "LoadShaderFromFile vkCreateShaderModule succeeded. " << filename;
			}
		}
		else
		{
			BOOST_LOG_TRIVIAL(fatal) << "LoadShaderFromFile unable to read file. " << filename;
		}
		free(data);
		fclose(fp);
	}
	else
	{
		BOOST_LOG_TRIVIAL(fatal) << "LoadShaderFromFile unable to open file. " << filename;
	}

	return module;
}

vk::ShaderModule LoadShaderFromResource(vk::Device device, WORD resource)
{
	vk::ShaderModuleCreateInfo moduleCreateInfo;
	vk::ShaderModule module;
	vk::Result result;
	HMODULE dllModule = NULL;

	//dllModule = GetModule();
	BOOL res = GetModuleHandleEx(0, TEXT("d3d9"), &dllModule);

	if (res == FALSE)
	{
		BOOST_LOG_TRIVIAL(fatal) << "LoadShaderFromResource dllModule is null.";
	}
	else
	{
		HRSRC hRes = FindResource(dllModule, MAKEINTRESOURCE(resource), TEXT("Shader"));
		if (NULL != hRes)
		{
			HGLOBAL hData = LoadResource(dllModule, hRes);
			if (NULL != hData)
			{
				int32_t dataSize = SizeofResource(dllModule, hRes);
				uint32_t* data = (uint32_t*)LockResource(hData);

				moduleCreateInfo.codeSize = dataSize;
				moduleCreateInfo.pCode = data; //Why is this uint32_t* if the size is in bytes?

				result = device.createShaderModule(&moduleCreateInfo, nullptr, &module);
				if (result != vk::Result::eSuccess)
				{
					BOOST_LOG_TRIVIAL(fatal) << "LoadShaderFromResource vkCreateShaderModule failed with return code of " << GetResultString((VkResult)result);
				}
				else
				{
					BOOST_LOG_TRIVIAL(info) << "LoadShaderFromResource vkCreateShaderModule succeeded.";
				}
			}
			else
			{
				BOOST_LOG_TRIVIAL(fatal) << "LoadShaderFromResource resource data is null.";
			}
		}
		else
		{
			BOOST_LOG_TRIVIAL(fatal) << "LoadShaderFromResource resource not found.";
		}

		FreeLibrary(dllModule);
	}

	return module;
}

void ReallyCopyImage(vk::CommandBuffer commandBuffer, vk::Image srcImage, vk::Image dstImage, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t depth, uint32_t srcMip, uint32_t dstMip, uint32_t srcLayer, uint32_t dstLayer)
{
	//vk::Result result;

	vk::ImageSubresourceLayers subResource1;
	subResource1.aspectMask = vk::ImageAspectFlagBits::eColor;
	subResource1.baseArrayLayer = srcLayer;
	subResource1.mipLevel = srcMip;
	subResource1.layerCount = 1;

	vk::ImageSubresourceLayers subResource2;
	subResource2.aspectMask = vk::ImageAspectFlagBits::eColor;
	subResource2.baseArrayLayer = dstLayer;
	subResource2.mipLevel = dstMip;
	subResource2.layerCount = 1;

	vk::ImageCopy region;
	region.srcSubresource = subResource1;
	region.dstSubresource = subResource2;
	region.srcOffset = vk::Offset3D(x, y, 0);
	region.dstOffset = vk::Offset3D(x, y, 0);
	region.extent.width = width;
	region.extent.height = height;
	region.extent.depth = depth;

	commandBuffer.copyImage(
		srcImage, vk::ImageLayout::eTransferSrcOptimal,
		dstImage, vk::ImageLayout::eTransferDstOptimal,
		1, &region);
}

void ReallyCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImage dstImage, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t srcMip, uint32_t dstMip, uint32_t srcLayer, uint32_t dstLayer)
{
	VkResult result = VK_SUCCESS;

	VkImageSubresourceLayers subResource1 = {};
	subResource1.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subResource1.baseArrayLayer = srcLayer;
	subResource1.mipLevel = srcMip;
	subResource1.layerCount = 1;

	VkImageSubresourceLayers subResource2 = {};
	subResource2.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subResource2.baseArrayLayer = dstLayer;
	subResource2.mipLevel = dstMip;
	subResource2.layerCount = 1;

	VkImageCopy region = {};
	region.srcSubresource = subResource1;
	region.dstSubresource = subResource2;
	region.srcOffset = { x, y, 0 };
	region.dstOffset = { x, y, 0 };
	region.extent.width = width;
	region.extent.height = height;
	region.extent.depth = 1;

	vkCmdCopyImage(
		commandBuffer,
		srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &region
	);
}

void ReallySetImageLayout(vk::CommandBuffer commandBuffer, vk::Image image, vk::ImageAspectFlags aspectMask, vk::ImageLayout oldImageLayout, vk::ImageLayout newImageLayout, uint32_t levelCount, uint32_t mipIndex, uint32_t layerCount)
{
	//VkResult result;
	vk::PipelineStageFlags sourceStages; //VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
	vk::PipelineStageFlags destinationStages; //VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT

	if (aspectMask == vk::ImageAspectFlags()) //0
	{
		aspectMask = vk::ImageAspectFlagBits::eColor;
	}

	vk::ImageMemoryBarrier imageMemoryBarrier;
	imageMemoryBarrier.oldLayout = oldImageLayout;
	imageMemoryBarrier.newLayout = newImageLayout;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.image = image;
	imageMemoryBarrier.subresourceRange.aspectMask = aspectMask;
	imageMemoryBarrier.subresourceRange.baseMipLevel = mipIndex;
	imageMemoryBarrier.subresourceRange.levelCount = levelCount;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	imageMemoryBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

	switch (oldImageLayout)
	{
	case vk::ImageLayout::eUndefined:
		break;
	case vk::ImageLayout::eGeneral:
		break;
	case vk::ImageLayout::eColorAttachmentOptimal:
		imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite; //Added based on validation layer complaints.
		sourceStages |= vk::PipelineStageFlagBits::eColorAttachmentOutput;
		break;
	case vk::ImageLayout::eDepthStencilAttachmentOptimal:
		break;
	case vk::ImageLayout::eDepthStencilReadOnlyOptimal:
		break;
	case vk::ImageLayout::eShaderReadOnlyOptimal:
		break;
	case vk::ImageLayout::eTransferSrcOptimal:
		imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
		sourceStages  |= vk::PipelineStageFlagBits::eTransfer;
		break;
	case vk::ImageLayout::eTransferDstOptimal:
		imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		sourceStages |= vk::PipelineStageFlagBits::eTransfer;
		break;
	case vk::ImageLayout::ePreinitialized:
		imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
		sourceStages |= vk::PipelineStageFlagBits::eHost;
		break;
	case vk::ImageLayout::ePresentSrcKHR:
		imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eMemoryRead;
		break;
	default:
		break;
	}

	switch (newImageLayout)
	{
	case vk::ImageLayout::eUndefined:
		break;
	case vk::ImageLayout::eGeneral:
		break;
	case vk::ImageLayout::eColorAttachmentOptimal:
		imageMemoryBarrier.srcAccessMask |= vk::AccessFlagBits::eMemoryRead;
		imageMemoryBarrier.dstAccessMask |= vk::AccessFlagBits::eColorAttachmentWrite;
		destinationStages |= vk::PipelineStageFlagBits::eColorAttachmentOutput;
		break;
	case vk::ImageLayout::eDepthStencilAttachmentOptimal:
		imageMemoryBarrier.dstAccessMask |= vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		destinationStages |= vk::PipelineStageFlagBits::eLateFragmentTests;
		break;
	case vk::ImageLayout::eDepthStencilReadOnlyOptimal:
		break;
	case vk::ImageLayout::eShaderReadOnlyOptimal:
		imageMemoryBarrier.dstAccessMask |= vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eInputAttachmentRead;
		destinationStages |= vk::PipelineStageFlagBits::eFragmentShader;
		break;
	case vk::ImageLayout::eTransferSrcOptimal:
		imageMemoryBarrier.dstAccessMask |= vk::AccessFlagBits::eTransferRead;
		destinationStages |= vk::PipelineStageFlagBits::eTransfer;
		break;
	case vk::ImageLayout::eTransferDstOptimal:
		imageMemoryBarrier.dstAccessMask |= vk::AccessFlagBits::eTransferWrite;
		destinationStages |= vk::PipelineStageFlagBits::eTransfer;
		break;
	case vk::ImageLayout::ePreinitialized:
		imageMemoryBarrier.dstAccessMask |= vk::AccessFlagBits::eHostWrite;
		destinationStages |= vk::PipelineStageFlagBits::eHost;
		break;
	case vk::ImageLayout::ePresentSrcKHR:
		imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eMemoryRead;
		break;
	default:
		break;
	}

	if (sourceStages == vk::PipelineStageFlags())
	{
		sourceStages = vk::PipelineStageFlagBits::eTopOfPipe;
	}

	if (destinationStages == vk::PipelineStageFlags())
	{
		destinationStages = vk::PipelineStageFlagBits::eTopOfPipe;
	}

	commandBuffer.pipelineBarrier(sourceStages, destinationStages, vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
}