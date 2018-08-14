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

#ifndef CDEVICE9_H
#define CDEVICE9_H

#include "d3d9.h" // Base class: IDirect3DDevice9
#include <boost/container/small_vector.hpp>
#include "Perf_CommandStreamManager.h"

class C9;
class CSwapChain9;
class CVertexBuffer9;
class CIndexBuffer9;

class CDevice9 : public IDirect3DDevice9
{	
public:
	CDevice9(C9* Instance, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS *pPresentationParameters);
	~CDevice9();

	//CSMT
	std::shared_ptr<CommandStreamManager> mCommandStreamManager;
	size_t mInstanceId = 0;
	size_t mId = 0;

	//Creation Parameters
	C9* mInstance = nullptr;
	UINT mAdapter = 0;
	D3DDEVTYPE mDeviceType = D3DDEVTYPE_HAL;
	HWND mFocusWindow = 0;
	DWORD mBehaviorFlags = 0;
	D3DPRESENT_PARAMETERS mPresentationParameters = {};

	//Misc
	ULONG mReferenceCount = 1;
	uint32_t mDisplayCount = 0;	
	boost::container::small_vector<CSwapChain9*, 2> mSwapChains;
	CSurface9* mRenderTargets[4] = {};

	std::vector<CVertexBuffer9*> mVertexBuffers;
	std::vector<CIndexBuffer9*> mIndexBuffers;

	BOOL mIsDirty = true;
	
	PAINTSTRUCT* mPaintInformation = {};

	CSurface9* mDepthStencilSurface = nullptr;

	UINT mAvailableTextureMemory = 0;

public:

	//IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,void  **ppv);
	virtual ULONG STDMETHODCALLTYPE AddRef(void);	
	virtual ULONG STDMETHODCALLTYPE Release(void);

	//Device
	virtual HRESULT STDMETHODCALLTYPE BeginScene();
	virtual HRESULT STDMETHODCALLTYPE BeginStateBlock();
	virtual HRESULT STDMETHODCALLTYPE Clear(DWORD Count,const D3DRECT *pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil);
	virtual HRESULT STDMETHODCALLTYPE ColorFill(IDirect3DSurface9 *pSurface,const RECT *pRect,D3DCOLOR color);
	virtual HRESULT STDMETHODCALLTYPE CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS *pPresentationParameters,IDirect3DSwapChain9 **ppSwapChain);
	virtual HRESULT STDMETHODCALLTYPE CreateCubeTexture(UINT EdgeLength,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DCubeTexture9 **ppCubeTexture,HANDLE *pSharedHandle);
	virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilSurface(UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Discard,IDirect3DSurface9 **ppSurface,HANDLE *pSharedHandle);
	virtual HRESULT STDMETHODCALLTYPE CreateIndexBuffer(UINT Length,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DIndexBuffer9 **ppIndexBuffer,HANDLE *pSharedHandle);
	virtual HRESULT STDMETHODCALLTYPE CreateOffscreenPlainSurface(UINT Width,UINT Height,D3DFORMAT Format,D3DPOOL Pool,IDirect3DSurface9 **ppSurface,HANDLE *pSharedHandle);
	virtual HRESULT STDMETHODCALLTYPE CreatePixelShader(const DWORD *pFunction,IDirect3DPixelShader9 **ppShader);
	virtual HRESULT STDMETHODCALLTYPE CreateQuery(D3DQUERYTYPE Type,IDirect3DQuery9 **ppQuery);
	virtual HRESULT STDMETHODCALLTYPE CreateRenderTarget(UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Lockable,IDirect3DSurface9 **ppSurface,HANDLE *pSharedHandle);
	virtual HRESULT STDMETHODCALLTYPE CreateStateBlock(D3DSTATEBLOCKTYPE Type,IDirect3DStateBlock9 **ppSB);
	virtual HRESULT STDMETHODCALLTYPE CreateTexture(UINT Width,UINT Height,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DTexture9 **ppTexture,HANDLE *pSharedHandle);
	virtual HRESULT STDMETHODCALLTYPE CreateVertexBuffer(UINT Length,DWORD Usage,DWORD FVF,D3DPOOL Pool,IDirect3DVertexBuffer9 **ppVertexBuffer,HANDLE *pSharedHandle);
	virtual HRESULT STDMETHODCALLTYPE CreateVertexDeclaration(const D3DVERTEXELEMENT9 *pVertexElements,IDirect3DVertexDeclaration9 **ppDecl);
	virtual HRESULT STDMETHODCALLTYPE CreateVertexShader(const DWORD *pFunction,IDirect3DVertexShader9 **ppShader);
	virtual HRESULT STDMETHODCALLTYPE CreateVolumeTexture(UINT Width,UINT Height,UINT Depth,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DVolumeTexture9 **ppVolumeTexture,HANDLE *pSharedHandle);
	virtual HRESULT STDMETHODCALLTYPE DeletePatch(UINT Handle);
	virtual HRESULT STDMETHODCALLTYPE DrawIndexedPrimitive(D3DPRIMITIVETYPE Type,INT BaseVertexIndex,UINT MinIndex,UINT NumVertices,UINT StartIndex,UINT PrimitiveCount);
	virtual HRESULT STDMETHODCALLTYPE DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertices,UINT PrimitiveCount,const void *pIndexData,D3DFORMAT IndexDataFormat,const void *pVertexStreamZeroData,UINT VertexStreamZeroStride);
	virtual HRESULT STDMETHODCALLTYPE DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount);
	virtual HRESULT STDMETHODCALLTYPE DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,const void *pVertexStreamZeroData,UINT VertexStreamZeroStride);
	virtual HRESULT STDMETHODCALLTYPE DrawRectPatch(UINT Handle,const float *pNumSegs,const D3DRECTPATCH_INFO *pRectPatchInfo);
	virtual HRESULT STDMETHODCALLTYPE DrawTriPatch(UINT Handle,const float *pNumSegs,const D3DTRIPATCH_INFO *pTriPatchInfo);
	virtual HRESULT STDMETHODCALLTYPE EndScene();
	virtual HRESULT STDMETHODCALLTYPE EndStateBlock(IDirect3DStateBlock9 **ppSB);
	virtual HRESULT STDMETHODCALLTYPE EvictManagedResources();
	virtual UINT STDMETHODCALLTYPE GetAvailableTextureMem();
	virtual HRESULT STDMETHODCALLTYPE GetBackBuffer(UINT  iSwapChain,UINT BackBuffer,D3DBACKBUFFER_TYPE Type,IDirect3DSurface9 **ppBackBuffer);
	virtual HRESULT STDMETHODCALLTYPE GetClipPlane(DWORD Index,float *pPlane);
	virtual HRESULT STDMETHODCALLTYPE GetClipStatus(D3DCLIPSTATUS9 *pClipStatus);
	virtual HRESULT STDMETHODCALLTYPE GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS *pParameters);
	virtual HRESULT STDMETHODCALLTYPE GetCurrentTexturePalette(UINT *pPaletteNumber);
	virtual HRESULT STDMETHODCALLTYPE GetDepthStencilSurface(IDirect3DSurface9 **ppZStencilSurface);
	virtual HRESULT STDMETHODCALLTYPE GetDeviceCaps(D3DCAPS9 *pCaps);
	virtual HRESULT STDMETHODCALLTYPE GetDirect3D(IDirect3D9 **ppD3D9);
	virtual HRESULT STDMETHODCALLTYPE GetDisplayMode(UINT  iSwapChain,D3DDISPLAYMODE *pMode);
	virtual HRESULT STDMETHODCALLTYPE GetFrontBufferData(UINT  iSwapChain,IDirect3DSurface9 *pDestSurface);
	virtual HRESULT STDMETHODCALLTYPE GetFVF(DWORD *pFVF);
	virtual void STDMETHODCALLTYPE GetGammaRamp(UINT  iSwapChain,D3DGAMMARAMP *pRamp);
	virtual HRESULT STDMETHODCALLTYPE GetIndices(IDirect3DIndexBuffer9 **ppIndexData); //,UINT *pBaseVertexIndex  ?
	virtual HRESULT STDMETHODCALLTYPE GetLight(DWORD Index,D3DLIGHT9 *pLight); /*?documentation lists D3DLight9*/
	virtual HRESULT STDMETHODCALLTYPE GetLightEnable(DWORD Index,BOOL *pEnable);
	virtual HRESULT STDMETHODCALLTYPE GetMaterial(D3DMATERIAL9 *pMaterial);
	virtual FLOAT STDMETHODCALLTYPE GetNPatchMode();
	virtual UINT STDMETHODCALLTYPE GetNumberOfSwapChains();
	virtual HRESULT STDMETHODCALLTYPE GetPaletteEntries(UINT PaletteNumber,PALETTEENTRY *pEntries);
	virtual HRESULT STDMETHODCALLTYPE GetPixelShader(IDirect3DPixelShader9 **ppShader);
	virtual HRESULT STDMETHODCALLTYPE GetPixelShaderConstantB(UINT StartRegister,BOOL *pConstantData,UINT BoolCount);
	virtual HRESULT STDMETHODCALLTYPE GetPixelShaderConstantF(UINT StartRegister,float *pConstantData,UINT Vector4fCount);
	virtual HRESULT STDMETHODCALLTYPE GetPixelShaderConstantI(UINT StartRegister,int *pConstantData,UINT Vector4iCount);
	virtual HRESULT STDMETHODCALLTYPE GetRasterStatus(UINT  iSwapChain,D3DRASTER_STATUS *pRasterStatus);
	virtual HRESULT STDMETHODCALLTYPE GetRenderState(D3DRENDERSTATETYPE State,DWORD *pValue);
	virtual HRESULT STDMETHODCALLTYPE GetRenderTarget(DWORD RenderTargetIndex,IDirect3DSurface9 **ppRenderTarget);
	virtual HRESULT STDMETHODCALLTYPE GetRenderTargetData(IDirect3DSurface9 *pRenderTarget,IDirect3DSurface9 *pDestSurface);
	virtual HRESULT STDMETHODCALLTYPE GetSamplerState(DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD *pValue);
	virtual HRESULT STDMETHODCALLTYPE GetScissorRect(RECT *pRect);
	virtual BOOL STDMETHODCALLTYPE GetSoftwareVertexProcessing();
	virtual HRESULT STDMETHODCALLTYPE GetStreamSource(UINT StreamNumber,IDirect3DVertexBuffer9 **ppStreamData,UINT *pOffsetInBytes,UINT *pStride);
	virtual HRESULT STDMETHODCALLTYPE GetStreamSourceFreq(UINT StreamNumber,UINT *pDivider);
	virtual HRESULT STDMETHODCALLTYPE GetSwapChain(UINT  iSwapChain,IDirect3DSwapChain9 **ppSwapChain);
	virtual HRESULT STDMETHODCALLTYPE GetTexture(DWORD Stage,IDirect3DBaseTexture9 **ppTexture);
	virtual HRESULT STDMETHODCALLTYPE GetTextureStageState(DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD *pValue);
	virtual HRESULT STDMETHODCALLTYPE GetTransform(D3DTRANSFORMSTATETYPE State,D3DMATRIX *pMatrix);
	virtual HRESULT STDMETHODCALLTYPE GetVertexDeclaration(IDirect3DVertexDeclaration9 **ppDecl);
	virtual HRESULT STDMETHODCALLTYPE GetVertexShader(IDirect3DVertexShader9 **ppShader);
	virtual HRESULT STDMETHODCALLTYPE GetVertexShaderConstantB(UINT StartRegister,BOOL *pConstantData,UINT BoolCount);
	virtual HRESULT STDMETHODCALLTYPE GetVertexShaderConstantF(UINT StartRegister,float *pConstantData,UINT Vector4fCount);
	virtual HRESULT STDMETHODCALLTYPE GetVertexShaderConstantI(UINT StartRegister,int *pConstantData,UINT Vector4iCount);
	virtual HRESULT STDMETHODCALLTYPE GetViewport(D3DVIEWPORT9 *pViewport);
	virtual HRESULT STDMETHODCALLTYPE LightEnable(DWORD LightIndex,BOOL bEnable);
	virtual HRESULT STDMETHODCALLTYPE MultiplyTransform(D3DTRANSFORMSTATETYPE State,const D3DMATRIX *pMatrix);
	virtual HRESULT STDMETHODCALLTYPE Present(const RECT *pSourceRect,const RECT *pDestRect,HWND hDestWindowOverride,const RGNDATA *pDirtyRegion);
	virtual HRESULT STDMETHODCALLTYPE ProcessVertices(UINT SrcStartIndex,UINT DestIndex,UINT VertexCount,IDirect3DVertexBuffer9 *pDestBuffer,IDirect3DVertexDeclaration9 *pVertexDecl,DWORD Flags);
	virtual HRESULT STDMETHODCALLTYPE Reset(D3DPRESENT_PARAMETERS *pPresentationParameters);
	virtual HRESULT STDMETHODCALLTYPE SetClipPlane(DWORD Index,const float *pPlane);
	virtual HRESULT STDMETHODCALLTYPE SetClipStatus(const D3DCLIPSTATUS9 *pClipStatus);
	virtual HRESULT STDMETHODCALLTYPE SetCurrentTexturePalette(UINT PaletteNumber);
	virtual void STDMETHODCALLTYPE SetCursorPosition(INT X,INT Y,DWORD Flags);
	virtual HRESULT STDMETHODCALLTYPE SetCursorProperties(UINT XHotSpot,UINT YHotSpot,IDirect3DSurface9 *pCursorBitmap);
	virtual HRESULT STDMETHODCALLTYPE SetDepthStencilSurface(IDirect3DSurface9 *pNewZStencil);
	virtual HRESULT STDMETHODCALLTYPE SetDialogBoxMode(BOOL bEnableDialogs);
	virtual HRESULT STDMETHODCALLTYPE SetFVF(DWORD FVF);
	virtual void STDMETHODCALLTYPE SetGammaRamp(UINT  iSwapChain,DWORD Flags,const D3DGAMMARAMP *pRamp);
	virtual HRESULT STDMETHODCALLTYPE SetIndices(IDirect3DIndexBuffer9 *pIndexData);
	virtual HRESULT STDMETHODCALLTYPE SetLight(DWORD Index,const D3DLIGHT9 *pLight);
	virtual HRESULT STDMETHODCALLTYPE SetMaterial(const D3DMATERIAL9 *pMaterial);
	virtual HRESULT STDMETHODCALLTYPE SetNPatchMode(float nSegments);
	virtual HRESULT STDMETHODCALLTYPE SetPaletteEntries(UINT PaletteNumber,const PALETTEENTRY *pEntries);
	virtual HRESULT STDMETHODCALLTYPE SetPixelShader(IDirect3DPixelShader9 *pShader);
	virtual HRESULT STDMETHODCALLTYPE SetPixelShaderConstantB(UINT StartRegister,const BOOL *pConstantData,UINT BoolCount);
	virtual HRESULT STDMETHODCALLTYPE SetPixelShaderConstantF(UINT StartRegister,const float *pConstantData,UINT Vector4fCount);
	virtual HRESULT STDMETHODCALLTYPE SetPixelShaderConstantI(UINT StartRegister,const int *pConstantData,UINT Vector4iCount);
	virtual HRESULT STDMETHODCALLTYPE SetRenderState(D3DRENDERSTATETYPE State,DWORD Value);
	virtual HRESULT STDMETHODCALLTYPE SetRenderTarget(DWORD RenderTargetIndex,IDirect3DSurface9 *pRenderTarget);
	virtual HRESULT STDMETHODCALLTYPE SetSamplerState(DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD Value);
	virtual HRESULT STDMETHODCALLTYPE SetScissorRect(const RECT *pRect);
	virtual HRESULT STDMETHODCALLTYPE SetSoftwareVertexProcessing(BOOL bSoftware);
	virtual HRESULT STDMETHODCALLTYPE SetStreamSource(UINT StreamNumber,IDirect3DVertexBuffer9 *pStreamData,UINT OffsetInBytes,UINT Stride);
	virtual HRESULT STDMETHODCALLTYPE SetStreamSourceFreq(UINT StreamNumber,UINT FrequencyParameter);
	virtual HRESULT STDMETHODCALLTYPE SetTexture(DWORD Sampler,IDirect3DBaseTexture9 *pTexture);
	virtual HRESULT STDMETHODCALLTYPE SetTextureStageState(DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD Value);
	virtual HRESULT STDMETHODCALLTYPE SetTransform(D3DTRANSFORMSTATETYPE State,const D3DMATRIX *pMatrix);
	virtual HRESULT STDMETHODCALLTYPE SetVertexDeclaration(IDirect3DVertexDeclaration9 *pDecl);
	virtual HRESULT STDMETHODCALLTYPE SetVertexShader(IDirect3DVertexShader9 *pShader);
	virtual HRESULT STDMETHODCALLTYPE SetVertexShaderConstantB(UINT StartRegister,const BOOL *pConstantData,UINT BoolCount);
	virtual HRESULT STDMETHODCALLTYPE SetVertexShaderConstantF(UINT StartRegister,const float *pConstantData,UINT Vector4fCount);
	virtual HRESULT STDMETHODCALLTYPE SetVertexShaderConstantI(UINT StartRegister,const int *pConstantData,UINT Vector4iCount);
	virtual HRESULT STDMETHODCALLTYPE SetViewport(const D3DVIEWPORT9 *pViewport);
	virtual BOOL STDMETHODCALLTYPE ShowCursor(BOOL bShow);
	virtual HRESULT STDMETHODCALLTYPE StretchRect(IDirect3DSurface9 *pSourceSurface,const RECT *pSourceRect,IDirect3DSurface9 *pDestSurface,const RECT *pDestRect,D3DTEXTUREFILTERTYPE Filter);
	virtual HRESULT STDMETHODCALLTYPE TestCooperativeLevel();
	virtual HRESULT STDMETHODCALLTYPE UpdateSurface(IDirect3DSurface9 *pSourceSurface,const RECT *pSourceRect,IDirect3DSurface9 *pDestinationSurface,const POINT *pDestinationPoint);
	virtual HRESULT STDMETHODCALLTYPE UpdateTexture(IDirect3DBaseTexture9* pSourceTexture,IDirect3DBaseTexture9* pDestinationTexture);
	virtual HRESULT STDMETHODCALLTYPE ValidateDevice(DWORD *pNumPasses);
};



#endif // CDEVICE9_H
