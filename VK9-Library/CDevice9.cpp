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

#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX

#include "C9.h"
#include "CDevice9.h"
#include "CCubeTexture9.h"
#include "CBaseTexture9.h"
#include "CTexture9.h"
#include "CVolumeTexture9.h"
#include "CSurface9.h"
#include "CVertexDeclaration9.h"
#include "CVertexShader9.h"
#include "CStateBlock9.h"
#include "CSwapChain9.h"
#include "CVertexBuffer9.h"
#include "CIndexBuffer9.h"
#include "CPixelShader9.h"
#include "CVertexShader9.h"
#include "CQuery9.h"
#include "CRenderTargetSurface9.h"
#include "CVolume9.h"

#include "Utilities.h"

CDevice9::CDevice9(C9* Instance, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS *pPresentationParameters)
	:
	mInstance(Instance),
	mAdapter(Adapter),
	mDeviceType(DeviceType),
	mFocusWindow(hFocusWindow),
	mBehaviorFlags(BehaviorFlags)
{
	BOOST_LOG_TRIVIAL(info) << "CDevice9::CDevice9";

	memcpy(&mPresentationParameters, pPresentationParameters, sizeof(D3DPRESENT_PARAMETERS));
	mCommandStreamManager = mInstance->mCommandStreamManager;
	mInstanceId = mInstance->mId;

	if (!mPresentationParameters.Windowed)
	{
		DEVMODE newSettings = {};
		EnumDisplaySettings(0, 0, &newSettings);
		newSettings.dmPelsWidth = mPresentationParameters.BackBufferWidth;
		newSettings.dmPelsHeight = mPresentationParameters.BackBufferHeight;
		newSettings.dmBitsPerPel = 32;
		newSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
		ChangeDisplaySettings(&newSettings, CDS_FULLSCREEN);
		SetWindowPos(hFocusWindow, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		InvalidateRect(hFocusWindow, 0, true);
	}
}

CDevice9::~CDevice9()
{
	//Delete any temp buffers used to handle the UP draw calls.
	for (auto buffer : mVertexBuffers)
	{
		delete buffer;
	}
	for (auto buffer : mIndexBuffers)
	{
		delete buffer;
	}

	WorkItem* workItem = mCommandStreamManager->GetWorkItem(nullptr);
	workItem->WorkItemType = WorkItemType::Device_Destroy;
	workItem->Id = mId;
	mCommandStreamManager->RequestWorkAndWait(workItem);

	BOOST_LOG_TRIVIAL(info) << "CDevice9::~CDevice9";

	for (size_t i = 0; i < mSwapChains.size(); i++)
	{
		delete mSwapChains[i];
	}
}

HRESULT STDMETHODCALLTYPE CDevice9::Clear(DWORD Count, const D3DRECT *pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_Clear;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(Count);
	workItem->Argument2 = bit_cast<void*>(pRects);
	workItem->Argument3 = bit_cast<void*>(Flags);
	workItem->Argument4 = bit_cast<void*>(Color);
	workItem->Argument5 = bit_cast<void*>(Z);
	workItem->Argument6 = bit_cast<void*>(Stencil);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::BeginScene() //
{
	//According to a tip from the Nine team games don't always use the begin/end scene functions correctly.
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_BeginScene;
	workItem->Id = mId;
	mCommandStreamManager->RequestWork(workItem);

	return D3D_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::EndScene()
{
	//According to a tip from the Nine team games don't always use the begin/end scene functions correctly.

	return D3D_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::Present(const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	//std::lock_guard<std::mutex> lock(workItem->Mutex);
	workItem->WorkItemType = WorkItemType::Device_Present;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(pSourceRect);
	workItem->Argument2 = bit_cast<void*>(pDestRect);
	workItem->Argument3 = bit_cast<void*>(hDestWindowOverride);
	workItem->Argument4 = bit_cast<void*>(pDirtyRegion);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	/*
	This might be a good spot for a co-routine so that the buffers can be freed after the render target is filled but before the swap chain present finishes.
	Whether or not the present has to wait long enough depends on what presentation methods are available so this may or may not improve performance.
	Ultimately someone will have to tinker with this to see what the effect is on performance.
	*/

	//Delete any temp buffers used to handle the UP draw calls.
	for (auto buffer : mVertexBuffers)
	{
		if (buffer != nullptr)
		{
			buffer->Release();
		}
	}
	mVertexBuffers.clear();

	for (auto buffer : mIndexBuffers)
	{
		if (buffer != nullptr)
		{
			buffer->Release();
		}
	}
	mIndexBuffers.clear();

	return D3D_OK;
}

ULONG STDMETHODCALLTYPE CDevice9::AddRef(void)
{
	return InterlockedIncrement(&mReferenceCount);
}

HRESULT STDMETHODCALLTYPE CDevice9::QueryInterface(REFIID riid, void  **ppv)
{
	if (ppv == nullptr)
	{
		return E_POINTER;
	}

	if (IsEqualGUID(riid, IID_IDirect3DDevice9))
	{
		(*ppv) = this;
		this->AddRef();
		return S_OK;
	}

	if (IsEqualGUID(riid, IID_IUnknown))
	{
		(*ppv) = this;
		this->AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CDevice9::Release(void)
{
	ULONG ref = InterlockedDecrement(&mReferenceCount);

	if (ref == 0)
	{
		delete this;
	}

	return ref;
}


HRESULT STDMETHODCALLTYPE CDevice9::BeginStateBlock()
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_BeginStateBlock;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(this);
	mCommandStreamManager->RequestWork(workItem);

	return S_OK;
}



HRESULT STDMETHODCALLTYPE CDevice9::ColorFill(IDirect3DSurface9 *pSurface, const RECT *pRect, D3DCOLOR color)
{
	//TODO: Implement.

	BOOST_LOG_TRIVIAL(warning) << "CDevice9::ColorFill is not implemented!";

	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDevice9::CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS *pPresentationParameters, IDirect3DSwapChain9 **ppSwapChain)
{
	auto ptr = new CSwapChain9(this, pPresentationParameters);
	ptr->Init();
	(*ppSwapChain) = ptr;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::CreateCubeTexture(UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture9 **ppCubeTexture, HANDLE *pSharedHandle)
{
	HRESULT result = S_OK;

	CCubeTexture9* obj = new CCubeTexture9(this, EdgeLength, Levels, Usage, Format, Pool, pSharedHandle);

	obj->mCommandStreamManager = this->mCommandStreamManager;
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->Id = this->mId;
	workItem->WorkItemType = WorkItemType::CubeTexture_Create;
	workItem->Argument1 = (void*)obj;
	obj->mId = mCommandStreamManager->RequestWorkAndWait(workItem);

	for (size_t i = 0; i < 6; i++)
	{
		for (size_t j = 0; j < obj->mLevels; j++)
		{
			obj->mSurfaces[i][j]->Init();
		}
	}

	(*ppCubeTexture) = (IDirect3DCubeTexture9*)obj;

	return result;
}

HRESULT STDMETHODCALLTYPE CDevice9::CreateDepthStencilSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard, IDirect3DSurface9 **ppSurface, HANDLE *pSharedHandle)
{
	HRESULT result = S_OK;

	CSurface9* obj = new CSurface9(this, (CTexture9*)nullptr, Width, Height, Format, MultiSample, MultisampleQuality, Discard, pSharedHandle);

	obj->Init();

	(*ppSurface) = (IDirect3DSurface9*)obj;

	return result;
}

HRESULT STDMETHODCALLTYPE CDevice9::CreateIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer9 **ppIndexBuffer, HANDLE *pSharedHandle)
{
	HRESULT result = S_OK;

	CIndexBuffer9* obj = new CIndexBuffer9(this, Length, Usage, Format, Pool, pSharedHandle);

	obj->mCommandStreamManager = this->mCommandStreamManager;
	WorkItem* workItem = this->mCommandStreamManager->GetWorkItem(this);
	workItem->Id = this->mId;
	workItem->WorkItemType = WorkItemType::IndexBuffer_Create;
	workItem->Argument1 = obj;
	obj->mId = this->mCommandStreamManager->RequestWork(workItem);

	(*ppIndexBuffer) = (IDirect3DIndexBuffer9*)obj;

	return result;
}

HRESULT STDMETHODCALLTYPE CDevice9::CreateOffscreenPlainSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9 **ppSurface, HANDLE *pSharedHandle)
{
	HRESULT result = S_OK;

	CSurface9* ptr = new CSurface9(this, (CTexture9*)nullptr, Width, Height, 1, 0, Format, Pool, pSharedHandle);

	ptr->Init();

	(*ppSurface) = (IDirect3DSurface9*)ptr;

	return result;
}

HRESULT STDMETHODCALLTYPE CDevice9::CreatePixelShader(const DWORD *pFunction, IDirect3DPixelShader9 **ppShader)
{
	HRESULT result = S_OK;

	CPixelShader9* obj = new CPixelShader9(this, pFunction);

	obj->mCommandStreamManager = this->mCommandStreamManager;
	WorkItem* workItem = this->mCommandStreamManager->GetWorkItem(this);
	workItem->Id = this->mId;
	workItem->WorkItemType = WorkItemType::Shader_Create;
	workItem->Argument1 = (void*)obj->mFunction;
	workItem->Argument2 = (void*)false;
	workItem->Argument3 = (void*)&obj->mSize;
	obj->mId = this->mCommandStreamManager->RequestWorkAndWait(workItem);

	(*ppShader) = (IDirect3DPixelShader9*)obj;

	return result;
}

HRESULT STDMETHODCALLTYPE CDevice9::CreateQuery(D3DQUERYTYPE Type, IDirect3DQuery9 **ppQuery)
{
	/*
	https://msdn.microsoft.com/en-us/library/windows/desktop/bb174360(v=vs.85).aspx
	*/

	//If null is passed the call is checking to see if a query type is supported.
	if (ppQuery == nullptr)
	{
		if (Type == D3DQUERYTYPE_OCCLUSION || Type == D3DQUERYTYPE_TIMESTAMP || Type == D3DQUERYTYPE_TIMESTAMPDISJOINT || Type == D3DQUERYTYPE_TIMESTAMPFREQ)
		{
			return S_OK;
		}
		else
		{
			return D3DERR_NOTAVAILABLE;
		}
	}

	if (Type == D3DQUERYTYPE_OCCLUSION || Type == D3DQUERYTYPE_TIMESTAMP || Type == D3DQUERYTYPE_TIMESTAMPDISJOINT || Type == D3DQUERYTYPE_TIMESTAMPFREQ)
	{
		HRESULT result = S_OK;

		CQuery9* obj = new CQuery9(this, Type);

		(*ppQuery) = (IDirect3DQuery9*)obj;

		obj->mCommandStreamManager = mCommandStreamManager;
		WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
		workItem->Id = mId;
		workItem->WorkItemType = WorkItemType::Query_Create;
		workItem->Argument1 = this;
		obj->mId = obj->mCommandStreamManager->RequestWorkAndWait(workItem);

		return result;
	}
	else
	{
		return D3DERR_NOTAVAILABLE;
	}
}

HRESULT STDMETHODCALLTYPE CDevice9::CreateRenderTarget(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9 **ppSurface, HANDLE *pSharedHandle)
{
	HRESULT result = S_OK;

	//I added an extra int at the end so the signature would be different for this version. Locakable/Discard are both BOOL.
	CRenderTargetSurface9* obj = new CRenderTargetSurface9(this, Width, Height, Format);

	(*ppSurface) = (IDirect3DSurface9*)obj;

	return result;
}

HRESULT STDMETHODCALLTYPE CDevice9::CreateStateBlock(D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9 **ppSB)
{
	HRESULT result = S_OK;

	CStateBlock9* obj = new CStateBlock9(this, Type);

	obj->mCommandStreamManager = this->mCommandStreamManager;
	WorkItem* workItem = this->mCommandStreamManager->GetWorkItem(this);
	workItem->Id = this->mId;
	workItem->WorkItemType = WorkItemType::StateBlock_Create;
	workItem->Argument1 = (void*)obj;
	obj->mId = this->mCommandStreamManager->RequestWorkAndWait(workItem);

	(*ppSB) = (IDirect3DStateBlock9*)obj;

	return result;
}

HRESULT STDMETHODCALLTYPE CDevice9::CreateTexture(UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9 **ppTexture, HANDLE *pSharedHandle)
{
	HRESULT result = S_OK;

	CTexture9* obj = new CTexture9(this, Width, Height, Levels, Usage, Format, Pool, pSharedHandle);

	obj->mCommandStreamManager = this->mCommandStreamManager;
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->Id = this->mId;
	workItem->WorkItemType = WorkItemType::Texture_Create;
	workItem->Argument1 = (void*)obj;
	obj->mId = mCommandStreamManager->RequestWorkAndWait(workItem);

	for (size_t i = 0; i < obj->mLevels; i++)
	{
		obj->mSurfaces[i]->Init();
	}

	(*ppTexture) = (IDirect3DTexture9*)obj;

	return result;
}

HRESULT STDMETHODCALLTYPE CDevice9::CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9 **ppVertexBuffer, HANDLE *pSharedHandle)
{
	HRESULT result = S_OK;

	CVertexBuffer9* obj = new CVertexBuffer9(this, Length, Usage, FVF, Pool, pSharedHandle);

	obj->mCommandStreamManager = this->mCommandStreamManager;
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->Id = this->mId;
	workItem->WorkItemType = WorkItemType::VertexBuffer_Create;
	workItem->Argument1 = (void*)obj;
	obj->mId = mCommandStreamManager->RequestWorkAndWait(workItem);

	(*ppVertexBuffer) = (IDirect3DVertexBuffer9*)obj;

	return result;
}

HRESULT STDMETHODCALLTYPE CDevice9::CreateVertexDeclaration(const D3DVERTEXELEMENT9 *pVertexElements, IDirect3DVertexDeclaration9 **ppDecl)
{
	HRESULT result = S_OK;

	CVertexDeclaration9* obj = new CVertexDeclaration9(this, pVertexElements);

	(*ppDecl) = (IDirect3DVertexDeclaration9*)obj;

	return result;
}

HRESULT STDMETHODCALLTYPE CDevice9::CreateVertexShader(const DWORD *pFunction, IDirect3DVertexShader9 **ppShader)
{
	HRESULT result = S_OK;

	CVertexShader9* obj = new CVertexShader9(this, pFunction);

	obj->mCommandStreamManager = this->mCommandStreamManager;
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->Id = this->mId;
	workItem->WorkItemType = WorkItemType::Shader_Create;
	workItem->Argument1 = (void*)obj->mFunction;
	workItem->Argument2 = (void*)true;
	workItem->Argument3 = (void*)&obj->mSize;
	obj->mId = mCommandStreamManager->RequestWorkAndWait(workItem);

	(*ppShader) = (IDirect3DVertexShader9*)obj;

	return result;
}

HRESULT STDMETHODCALLTYPE CDevice9::CreateVolumeTexture(UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9 **ppVolumeTexture, HANDLE *pSharedHandle)
{
	HRESULT result = S_OK;

	CVolumeTexture9* obj = new CVolumeTexture9(this, Width, Height, Depth, Levels, Usage, Format, Pool, pSharedHandle);

	obj->mCommandStreamManager = this->mCommandStreamManager;
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->Id = this->mId;
	workItem->WorkItemType = WorkItemType::VolumeTexture_Create;
	workItem->Argument1 = (void*)obj;
	obj->mId = mCommandStreamManager->RequestWorkAndWait(workItem);

	for (size_t i = 0; i < obj->mLevels; i++)
	{
		obj->mVolumes[i]->Init();
	}

	(*ppVolumeTexture) = (IDirect3DVolumeTexture9*)obj;

	return result;
}

HRESULT STDMETHODCALLTYPE CDevice9::DeletePatch(UINT Handle)
{
	//TODO: Implement.

	BOOST_LOG_TRIVIAL(warning) << "CDevice9::DeletePatch is not implemented!";

	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDevice9::DrawIndexedPrimitive(D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinIndex, UINT NumVertices, UINT StartIndex, UINT PrimitiveCount)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_DrawIndexedPrimitive;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(Type);
	workItem->Argument2 = bit_cast<void*>(BaseVertexIndex);
	workItem->Argument3 = bit_cast<void*>(MinIndex);
	workItem->Argument4 = bit_cast<void*>(NumVertices);
	workItem->Argument5 = bit_cast<void*>(StartIndex);
	workItem->Argument6 = bit_cast<void*>(PrimitiveCount);
	mCommandStreamManager->RequestWork(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, const void *pIndexData, D3DFORMAT IndexDataFormat, const void *pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
	//Figure out the current buffer setup.
	IDirect3DIndexBuffer9* oldIndexBuffer = nullptr;

	GetIndices(&oldIndexBuffer);

	IDirect3DVertexBuffer9* oldVertexBuffer = nullptr;
	UINT oldOffsetInBytes = 0;
	UINT oldStride = 0;

	GetStreamSource(0, &oldVertexBuffer, &oldOffsetInBytes, &oldStride);

	//Setup temp buffers
	UINT indexLength = 0;
	UINT vertexLength = NumVertices * VertexStreamZeroStride;
	DWORD Usage = 0;

	switch (PrimitiveType)
	{
	case D3DPT_POINTLIST:
		indexLength = (PrimitiveCount) * (IndexDataFormat == D3DFMT_INDEX16) ? 2 : 4;
		break;
	case D3DPT_LINELIST:
		indexLength = (PrimitiveCount * 2) * (IndexDataFormat == D3DFMT_INDEX16) ? 2 : 4;
		break;
	case D3DPT_LINESTRIP:
		indexLength = (PrimitiveCount + 1) * (IndexDataFormat == D3DFMT_INDEX16) ? 2 : 4;
		break;
	case D3DPT_TRIANGLELIST:
		indexLength = (PrimitiveCount * 3) * (IndexDataFormat == D3DFMT_INDEX16) ? 2 : 4;
		break;
	case D3DPT_TRIANGLESTRIP:
		indexLength = (PrimitiveCount + 2) * (IndexDataFormat == D3DFMT_INDEX16) ? 2 : 4;
		break;
	case D3DPT_TRIANGLEFAN:
		indexLength = (PrimitiveCount + 2) * (IndexDataFormat == D3DFMT_INDEX16) ? 2 : 4;
		break;
	}

	CIndexBuffer9* indexBuffer = nullptr;
	CVertexBuffer9* vertexBuffer = nullptr;
	void* buffer = nullptr;

	CreateIndexBuffer(indexLength, Usage, IndexDataFormat, D3DPOOL_DEFAULT, (IDirect3DIndexBuffer9**)&indexBuffer, nullptr);
	CreateVertexBuffer(vertexLength, Usage, D3DFVF_XYZ, D3DPOOL_DEFAULT, (IDirect3DVertexBuffer9**)&vertexBuffer, nullptr);

	vertexBuffer->mSize = NumVertices;

	if (VertexStreamZeroStride != 12)
	{
		BOOST_LOG_TRIVIAL(warning) << "CDevice9::DrawIndexedPrimitiveUP unhandled stride " << VertexStreamZeroStride;
	}

	vertexBuffer->Lock(0, 0, (void**)&buffer, 0);
	memcpy(buffer, pVertexStreamZeroData, vertexLength);
	vertexBuffer->Unlock();

	indexBuffer->Lock(0, 0, (void**)&buffer, 0);
	memcpy(buffer, pIndexData, indexLength);
	indexBuffer->Unlock();

	SetIndices(indexBuffer);
	indexBuffer->Release();
	SetStreamSource(0, vertexBuffer, 0, VertexStreamZeroStride);
	vertexBuffer->Release();
	
	//Queue draw command
	//DrawIndexedPrimitive(PrimitiveType, 0, MinVertexIndex, NumVertices, 0, PrimitiveCount);
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_DrawIndexedPrimitive;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(PrimitiveType);
	workItem->Argument2 = bit_cast<void*>(0);
	workItem->Argument3 = bit_cast<void*>(MinVertexIndex);
	workItem->Argument4 = bit_cast<void*>(NumVertices);
	workItem->Argument5 = bit_cast<void*>(0);
	workItem->Argument6 = bit_cast<void*>(PrimitiveCount);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	//Switch the buffers back.
	SetIndices(oldIndexBuffer);
	SetStreamSource(0, oldVertexBuffer, oldOffsetInBytes, oldStride);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_DrawPrimitive;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(PrimitiveType);
	workItem->Argument2 = bit_cast<void*>(StartVertex);
	workItem->Argument3 = bit_cast<void*>(PrimitiveCount);
	mCommandStreamManager->RequestWork(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, const void *pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
	//Figure out the current buffer setup.
	IDirect3DVertexBuffer9* oldVertexBuffer = nullptr;
	UINT oldOffsetInBytes = 0;
	UINT oldStride = 0;

	GetStreamSource(0, &oldVertexBuffer, &oldOffsetInBytes, &oldStride);

	//Setup temp buffers
	UINT vertexLength = 0;
	DWORD Usage = 0;
	UINT NumVertices;

	switch (PrimitiveType)
	{
	case D3DPT_POINTLIST:
		NumVertices = (PrimitiveCount);
		vertexLength = NumVertices * VertexStreamZeroStride;
		break;
	case D3DPT_LINELIST:
		NumVertices = (PrimitiveCount * 2);
		vertexLength = NumVertices * VertexStreamZeroStride;
		break;
	case D3DPT_LINESTRIP:
		NumVertices = (PrimitiveCount + 1);
		vertexLength = NumVertices * VertexStreamZeroStride;
		break;
	case D3DPT_TRIANGLELIST:
		NumVertices = (PrimitiveCount * 3);
		vertexLength = NumVertices * VertexStreamZeroStride;
		break;
	case D3DPT_TRIANGLESTRIP:
		NumVertices = (PrimitiveCount + 2);
		vertexLength = NumVertices * VertexStreamZeroStride;
		break;
	case D3DPT_TRIANGLEFAN:
		NumVertices = (PrimitiveCount + 2);
		vertexLength = NumVertices * VertexStreamZeroStride;
		break;
	}

	CVertexBuffer9* vertexBuffer = nullptr;
	void* buffer = nullptr;

	CreateVertexBuffer(vertexLength, Usage, D3DFVF_XYZ, D3DPOOL_DEFAULT, (IDirect3DVertexBuffer9**)&vertexBuffer, nullptr);

	vertexBuffer->mSize = NumVertices;

	if (VertexStreamZeroStride != 12)
	{
		BOOST_LOG_TRIVIAL(warning) << "CDevice9::DrawPrimitiveUP unhandled stride " << VertexStreamZeroStride;
	}

	vertexBuffer->Lock(0, 0, (void**)&buffer, 0);
	memcpy(buffer, pVertexStreamZeroData, vertexLength);
	vertexBuffer->Unlock();

	SetStreamSource(0, vertexBuffer, 0, VertexStreamZeroStride);
	vertexBuffer->Release();

	//Queue draw command
	//DrawPrimitive(PrimitiveType, 0, PrimitiveCount);
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_DrawPrimitive;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(PrimitiveType);
	workItem->Argument2 = bit_cast<void*>(0);
	workItem->Argument3 = bit_cast<void*>(PrimitiveCount);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	//Switch the buffers back.
	SetStreamSource(0, oldVertexBuffer, oldOffsetInBytes, oldStride);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::DrawRectPatch(UINT Handle, const float *pNumSegs, const D3DRECTPATCH_INFO *pRectPatchInfo)
{
	//if (!mIsSceneStarted)
	//{
	//	this->StartScene();
	//}

	//TODO: Implement.

	BOOST_LOG_TRIVIAL(warning) << "CDevice9::DrawRectPatch is not implemented!";

	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDevice9::DrawTriPatch(UINT Handle, const float *pNumSegs, const D3DTRIPATCH_INFO *pTriPatchInfo)
{
	//if (!mIsSceneStarted)
	//{
	//	this->StartScene();
	//}

	//TODO: Implement.

	BOOST_LOG_TRIVIAL(warning) << "CDevice9::DrawTriPatch is not implemented!";

	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDevice9::EndStateBlock(IDirect3DStateBlock9** ppSB)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_EndStateBlock;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(ppSB);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::EvictManagedResources()
{
	//TODO: Implement.

	BOOST_LOG_TRIVIAL(warning) << "CDevice9::EvictManagedResources is not implemented!";

	return S_OK;
}

UINT STDMETHODCALLTYPE CDevice9::GetAvailableTextureMem()
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_GetAvailableTextureMem;
	workItem->Id = mId;
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return mAvailableTextureMemory;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetBackBuffer(UINT  iSwapChain, UINT BackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9 **ppBackBuffer)
{
	return mSwapChains[iSwapChain]->GetBackBuffer(BackBuffer, Type, ppBackBuffer);
}

HRESULT STDMETHODCALLTYPE CDevice9::GetClipPlane(DWORD Index, float *pPlane)
{
	//TODO: Implement.

	BOOST_LOG_TRIVIAL(warning) << "CDevice9::GetClipPlane is not implemented!";

	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetClipStatus(D3DCLIPSTATUS9 *pClipStatus)
{
	//TODO: Implement.

	BOOST_LOG_TRIVIAL(warning) << "CDevice9::GetClipStatus is not implemented!";

	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS *pParameters)
{
	pParameters->AdapterOrdinal = this->mAdapter;
	pParameters->DeviceType = this->mDeviceType;
	pParameters->hFocusWindow = this->mFocusWindow;
	pParameters->BehaviorFlags = this->mBehaviorFlags;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetCurrentTexturePalette(UINT *pPaletteNumber)
{
	//TODO: Implement.

	BOOST_LOG_TRIVIAL(warning) << "CDevice9::GetCurrentTexturePalette is not implemented!";

	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetDepthStencilSurface(IDirect3DSurface9 **ppZStencilSurface)
{
	(*ppZStencilSurface) = mDepthStencilSurface;

	if (mDepthStencilSurface != nullptr)
	{
		mDepthStencilSurface->AddRef();
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetDeviceCaps(D3DCAPS9 *pCaps)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Instance_GetDeviceCaps;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(0);
	workItem->Argument2 = bit_cast<void*>(D3DDEVTYPE_HAL);
	workItem->Argument3 = bit_cast<void*>(pCaps);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetDirect3D(IDirect3D9 **ppD3D9)
{
	(*ppD3D9) = (IDirect3D9*)mInstance;

	mInstance->AddRef();

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetDisplayMode(UINT  iSwapChain, D3DDISPLAYMODE *pMode)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_GetDisplayMode;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(iSwapChain);
	workItem->Argument2 = bit_cast<void*>(pMode);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetFrontBufferData(UINT  iSwapChain, IDirect3DSurface9 *pDestSurface)
{
	return mSwapChains[iSwapChain]->GetFrontBufferData(pDestSurface);
}

HRESULT STDMETHODCALLTYPE CDevice9::GetFVF(DWORD *pFVF)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_GetFVF;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(pFVF);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

void STDMETHODCALLTYPE CDevice9::GetGammaRamp(UINT  iSwapChain, D3DGAMMARAMP *pRamp)
{
	//TODO: Implement.
	(*pRamp) = {};

	BOOST_LOG_TRIVIAL(warning) << "CDevice9::GetGammaRamp is not implemented!";

	return;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetIndices(IDirect3DIndexBuffer9 **ppIndexData) //,UINT *pBaseVertexIndex ?
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_GetIndices;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(ppIndexData);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	(*ppIndexData)->AddRef();

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetLight(DWORD Index, D3DLIGHT9 *pLight)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_GetLight;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(Index);
	workItem->Argument2 = bit_cast<void*>(pLight);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetLightEnable(DWORD Index, BOOL *pEnable)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_GetLightEnable;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(Index);
	workItem->Argument2 = bit_cast<void*>(pEnable);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetMaterial(D3DMATERIAL9 *pMaterial)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_GetMaterial;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(pMaterial);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

FLOAT STDMETHODCALLTYPE CDevice9::GetNPatchMode()
{
	FLOAT output;

	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_GetMaterial;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(&output);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return output;
}

UINT STDMETHODCALLTYPE CDevice9::GetNumberOfSwapChains()
{
	//TODO: Implement.

	BOOST_LOG_TRIVIAL(warning) << "CDevice9::GetNumberOfSwapChains is not implemented!";

	return 0;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetPaletteEntries(UINT PaletteNumber, PALETTEENTRY *pEntries)
{
	//TODO: Implement.

	BOOST_LOG_TRIVIAL(warning) << "CDevice9::GetPaletteEntries is not implemented!";

	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetPixelShader(IDirect3DPixelShader9 **ppShader)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_GetPixelShader;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(ppShader);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	(*ppShader)->AddRef();

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetPixelShaderConstantB(UINT StartRegister, BOOL *pConstantData, UINT BoolCount)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_GetPixelShaderConstantB;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(StartRegister);
	workItem->Argument2 = bit_cast<void*>(pConstantData);
	workItem->Argument3 = bit_cast<void*>(BoolCount);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetPixelShaderConstantF(UINT StartRegister, float *pConstantData, UINT Vector4fCount)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_GetPixelShaderConstantF;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(StartRegister);
	workItem->Argument2 = bit_cast<void*>(pConstantData);
	workItem->Argument3 = bit_cast<void*>(Vector4fCount);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetPixelShaderConstantI(UINT StartRegister, int *pConstantData, UINT Vector4iCount)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_GetPixelShaderConstantI;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(StartRegister);
	workItem->Argument2 = bit_cast<void*>(pConstantData);
	workItem->Argument3 = bit_cast<void*>(Vector4iCount);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetRasterStatus(UINT  iSwapChain, D3DRASTER_STATUS *pRasterStatus)
{
	return mSwapChains[iSwapChain]->GetRasterStatus(pRasterStatus);
}

HRESULT STDMETHODCALLTYPE CDevice9::GetRenderState(D3DRENDERSTATETYPE State, DWORD* pValue)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_GetRenderState;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(State);
	workItem->Argument2 = bit_cast<void*>(pValue);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9** ppRenderTarget)
{
	mRenderTargets[RenderTargetIndex]->AddRef();
	(*ppRenderTarget) = mRenderTargets[RenderTargetIndex];

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetRenderTargetData(IDirect3DSurface9 *pRenderTarget, IDirect3DSurface9 *pDestSurface)
{
	//TODO: Implement.

	BOOST_LOG_TRIVIAL(warning) << "CDevice9::GetRenderTargetData is not implemented!";

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD* pValue)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_GetSamplerState;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(Sampler);
	workItem->Argument2 = bit_cast<void*>(Type);
	workItem->Argument3 = bit_cast<void*>(pValue);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetScissorRect(RECT* pRect)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_GetScissorRect;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(pRect);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

BOOL STDMETHODCALLTYPE CDevice9::GetSoftwareVertexProcessing()
{
	//TODO: Implement.

	BOOST_LOG_TRIVIAL(warning) << "CDevice9::GetSoftwareVertexProcessing is not implemented!";

	return true;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9** ppStreamData, UINT* pOffsetInBytes, UINT* pStride)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_GetStreamSource;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(StreamNumber);
	workItem->Argument2 = bit_cast<void*>(ppStreamData);
	workItem->Argument3 = bit_cast<void*>(pOffsetInBytes);
	workItem->Argument4 = bit_cast<void*>(pStride);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetStreamSourceFreq(UINT StreamNumber, UINT *pDivider)
{
	//TODO: Implement.

	BOOST_LOG_TRIVIAL(warning) << "CDevice9::GetStreamSourceFreq is not implemented!";

	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetSwapChain(UINT iSwapChain, IDirect3DSwapChain9** ppSwapChain)
{
	(*ppSwapChain) = (IDirect3DSwapChain9*)mSwapChains[iSwapChain];

	(*ppSwapChain)->AddRef();

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetTexture(DWORD Stage, IDirect3DBaseTexture9** ppTexture)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_GetStreamSource;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(Stage);
	workItem->Argument2 = bit_cast<void*>(ppTexture);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_GetTextureStageState;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(Stage);
	workItem->Argument2 = bit_cast<void*>(Type);
	workItem->Argument3 = bit_cast<void*>(pValue);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetTransform(D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_GetTransform;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(State);
	workItem->Argument2 = bit_cast<void*>(pMatrix);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetVertexDeclaration(IDirect3DVertexDeclaration9** ppDecl)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_GetVertexDeclaration;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(ppDecl);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	(*ppDecl)->AddRef();

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetVertexShader(IDirect3DVertexShader9** ppShader)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_GetVertexShader;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(ppShader);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	(*ppShader)->AddRef();

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetVertexShaderConstantB(UINT StartRegister, BOOL* pConstantData, UINT BoolCount)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_GetVertexShaderConstantB;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(StartRegister);
	workItem->Argument2 = bit_cast<void*>(pConstantData);
	workItem->Argument3 = bit_cast<void*>(BoolCount);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetVertexShaderConstantF(UINT StartRegister, float* pConstantData, UINT Vector4fCount)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_GetVertexShaderConstantF;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(StartRegister);
	workItem->Argument2 = bit_cast<void*>(pConstantData);
	workItem->Argument3 = bit_cast<void*>(Vector4fCount);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetVertexShaderConstantI(UINT StartRegister, int* pConstantData, UINT Vector4iCount)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_GetVertexShaderConstantI;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(StartRegister);
	workItem->Argument2 = bit_cast<void*>(pConstantData);
	workItem->Argument3 = bit_cast<void*>(Vector4iCount);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::GetViewport(D3DVIEWPORT9* pViewport)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_GetViewport;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(pViewport);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::LightEnable(DWORD LightIndex, BOOL bEnable)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_LightEnable;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(LightIndex);
	workItem->Argument2 = bit_cast<void*>(bEnable);
	mCommandStreamManager->RequestWork(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::MultiplyTransform(D3DTRANSFORMSTATETYPE State, const D3DMATRIX *pMatrix)
{
	//TODO: Implement.

	BOOST_LOG_TRIVIAL(warning) << "CDevice9::MultiplyTransform is not implemented!";

	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDevice9::ProcessVertices(UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer9 *pDestBuffer, IDirect3DVertexDeclaration9 *pVertexDecl, DWORD Flags)
{
	//TODO: Implement.

	BOOST_LOG_TRIVIAL(warning) << "CDevice9::ProcessVertices is not implemented!";

	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDevice9::Reset(D3DPRESENT_PARAMETERS *pPresentationParameters)
{
	//TODO: Implement.

	BOOST_LOG_TRIVIAL(warning) << "CDevice9::Reset is not implemented!";

	return S_OK;
}

HRESULT CDevice9::SetClipPlane(DWORD Index, const float *pPlane)
{
	//TODO: Implement.

	BOOST_LOG_TRIVIAL(warning) << "CDevice9::SetClipPlane is not implemented!";

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::SetClipStatus(const D3DCLIPSTATUS9 *pClipStatus)
{
	//TODO: Implement.

	BOOST_LOG_TRIVIAL(warning) << "CDevice9::SetClipStatus is not implemented!";

	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDevice9::SetCurrentTexturePalette(UINT PaletteNumber)
{
	//TODO: Implement.

	BOOST_LOG_TRIVIAL(warning) << "CDevice9::SetCurrentTexturePalette is not implemented!";

	return E_NOTIMPL;
}

void STDMETHODCALLTYPE CDevice9::SetCursorPosition(INT X, INT Y, DWORD Flags)
{
	//TODO: Implement.

	BOOST_LOG_TRIVIAL(warning) << "CDevice9::SetCursorPosition is not implemented!";

	return;
}

HRESULT STDMETHODCALLTYPE CDevice9::SetCursorProperties(UINT XHotSpot, UINT YHotSpot, IDirect3DSurface9 *pCursorBitmap)
{
	//TODO: Implement.

	BOOST_LOG_TRIVIAL(warning) << "CDevice9::SetCursorProperties is not implemented!";

	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDevice9::SetDepthStencilSurface(IDirect3DSurface9* pNewZStencil)
{
	if (pNewZStencil == nullptr)
	{
		BOOST_LOG_TRIVIAL(warning) << "CDevice9::SetDepthStencilSurface passing null should disable the stencil operation but this isn't supported yet.";
		return S_OK;
	}


	if (mDepthStencilSurface != nullptr)
	{
		mDepthStencilSurface->Release();
	}
	mDepthStencilSurface = (CSurface9*)pNewZStencil;
	if (mDepthStencilSurface != nullptr)
	{
		mDepthStencilSurface->AddRef();
	}

	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_SetDepthStencilSurface;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(pNewZStencil);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::SetDialogBoxMode(BOOL bEnableDialogs)
{
	//TODO: Implement.

	BOOST_LOG_TRIVIAL(warning) << "CDevice9::SetDialogBoxMode is not implemented!";

	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDevice9::SetFVF(DWORD FVF)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_SetFVF;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(FVF);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

void STDMETHODCALLTYPE CDevice9::SetGammaRamp(UINT  iSwapChain, DWORD Flags, const D3DGAMMARAMP *pRamp)
{
	//TODO: Implement.

	BOOST_LOG_TRIVIAL(warning) << "CDevice9::SetGammaRamp is not implemented!";
}

HRESULT STDMETHODCALLTYPE CDevice9::SetIndices(IDirect3DIndexBuffer9* pIndexData)
{
	if (pIndexData != nullptr)
	{
		mIndexBuffers.push_back((CIndexBuffer9*)pIndexData);
		pIndexData->AddRef();
	}

	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_SetIndices;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(pIndexData);
	mCommandStreamManager->RequestWork(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::SetLight(DWORD Index, const D3DLIGHT9* pLight)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_SetLight;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(Index);
	workItem->Argument2 = bit_cast<void*>(pLight);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::SetMaterial(const D3DMATERIAL9* pMaterial)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_SetMaterial;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(pMaterial);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::SetNPatchMode(float nSegments)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_SetNPatchMode;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(nSegments);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::SetPaletteEntries(UINT PaletteNumber, const PALETTEENTRY *pEntries)
{
	//TODO: Implement.

	BOOST_LOG_TRIVIAL(warning) << "CDevice9::SetPaletteEntries is not implemented!";

	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDevice9::SetPixelShader(IDirect3DPixelShader9* pShader)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_SetPixelShader;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(pShader);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::SetPixelShaderConstantB(UINT StartRegister, const BOOL* pConstantData, UINT BoolCount)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_SetPixelShaderConstantB;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(StartRegister);
	workItem->Argument2 = bit_cast<void*>(pConstantData);
	workItem->Argument3 = bit_cast<void*>(BoolCount);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::SetPixelShaderConstantF(UINT StartRegister, const float *pConstantData, UINT Vector4fCount)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_SetPixelShaderConstantF;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(StartRegister);
	workItem->Argument2 = bit_cast<void*>(pConstantData);
	workItem->Argument3 = bit_cast<void*>(Vector4fCount);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::SetPixelShaderConstantI(UINT StartRegister, const int *pConstantData, UINT Vector4iCount)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_SetPixelShaderConstantI;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(StartRegister);
	workItem->Argument2 = bit_cast<void*>(pConstantData);
	workItem->Argument3 = bit_cast<void*>(Vector4iCount);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::SetRenderState(D3DRENDERSTATETYPE State, DWORD Value)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_SetRenderState;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(State);
	workItem->Argument2 = bit_cast<void*>(Value);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::SetRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9* pRenderTarget)
{
	mRenderTargets[RenderTargetIndex] = (CSurface9*)pRenderTarget;

	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_SetRenderTarget;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(RenderTargetIndex);
	workItem->Argument2 = bit_cast<void*>(pRenderTarget);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::SetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_SetSamplerState;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(Sampler);
	workItem->Argument2 = bit_cast<void*>(Type);
	workItem->Argument3 = bit_cast<void*>(Value);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::SetScissorRect(const RECT* pRect)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_SetScissorRect;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(pRect);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::SetSoftwareVertexProcessing(BOOL bSoftware)
{
	//TODO: Implement.

	BOOST_LOG_TRIVIAL(warning) << "CDevice9::SetSoftwareVertexProcessing is not implemented!";

	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDevice9::SetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride)
{
	if (pStreamData != nullptr)
	{
		mVertexBuffers.push_back((CVertexBuffer9*)pStreamData);
		pStreamData->AddRef();
	}

	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_SetStreamSource;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(StreamNumber);
	workItem->Argument2 = bit_cast<void*>(pStreamData);
	workItem->Argument3 = bit_cast<void*>(OffsetInBytes);
	workItem->Argument4 = bit_cast<void*>(Stride);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::SetStreamSourceFreq(UINT StreamNumber, UINT FrequencyParameter)
{
	//TODO: Implement.

	BOOST_LOG_TRIVIAL(warning) << "CDevice9::SetStreamSourceFreq is not implemented!";

	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDevice9::SetTexture(DWORD Sampler, IDirect3DBaseTexture9* pTexture)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_SetTexture;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(Sampler);
	workItem->Argument2 = bit_cast<void*>(pTexture);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_SetTextureStageState;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(Stage);
	workItem->Argument2 = bit_cast<void*>(Type);
	workItem->Argument3 = bit_cast<void*>(Value);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::SetTransform(D3DTRANSFORMSTATETYPE State, const D3DMATRIX* pMatrix)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_SetTransform;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(State);
	workItem->Argument2 = bit_cast<void*>(pMatrix);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::SetVertexDeclaration(IDirect3DVertexDeclaration9* pDecl)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_SetVertexDeclaration;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(pDecl);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::SetVertexShader(IDirect3DVertexShader9* pShader)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_SetVertexShader;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(pShader);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::SetVertexShaderConstantB(UINT StartRegister, const BOOL* pConstantData, UINT BoolCount)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_SetVertexShaderConstantB;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(StartRegister);
	workItem->Argument2 = bit_cast<void*>(pConstantData);
	workItem->Argument3 = bit_cast<void*>(BoolCount);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::SetVertexShaderConstantF(UINT StartRegister, const float* pConstantData, UINT Vector4fCount)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_SetVertexShaderConstantF;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(StartRegister);
	workItem->Argument2 = bit_cast<void*>(pConstantData);
	workItem->Argument3 = bit_cast<void*>(Vector4fCount);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::SetVertexShaderConstantI(UINT StartRegister, const int* pConstantData, UINT Vector4iCount)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_SetVertexShaderConstantI;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(StartRegister);
	workItem->Argument2 = bit_cast<void*>(pConstantData);
	workItem->Argument3 = bit_cast<void*>(Vector4iCount);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::SetViewport(const D3DVIEWPORT9* pViewport)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_SetViewport;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(pViewport);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

BOOL STDMETHODCALLTYPE CDevice9::ShowCursor(BOOL bShow)
{
	//TODO: Implement.

	BOOST_LOG_TRIVIAL(warning) << "CDevice9::ShowCursor is not implemented!";

	return TRUE;
}

HRESULT STDMETHODCALLTYPE CDevice9::StretchRect(IDirect3DSurface9 *pSourceSurface, const RECT *pSourceRect, IDirect3DSurface9 *pDestSurface, const RECT *pDestRect, D3DTEXTUREFILTERTYPE Filter)
{
	//TODO: Implement.

	BOOST_LOG_TRIVIAL(warning) << "CDevice9::StretchRect is not implemented!";

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::TestCooperativeLevel()
{
	//TODO: Implement.

	BOOST_LOG_TRIVIAL(warning) << "CDevice9::TestCooperativeLevel is not implemented!";

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::UpdateSurface(IDirect3DSurface9 *pSourceSurface, const RECT *pSourceRect, IDirect3DSurface9 *pDestinationSurface, const POINT *pDestinationPoint)
{
	//TODO: Implement.

	BOOST_LOG_TRIVIAL(warning) << "CDevice9::UpdateSurface is not implemented!";

	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDevice9::UpdateTexture(IDirect3DBaseTexture9* pSourceTexture, IDirect3DBaseTexture9* pDestinationTexture)
{
	WorkItem* workItem = mCommandStreamManager->GetWorkItem(this);
	workItem->WorkItemType = WorkItemType::Device_UpdateTexture;
	workItem->Id = mId;
	workItem->Argument1 = bit_cast<void*>(pSourceTexture);
	workItem->Argument2 = bit_cast<void*>(pDestinationTexture);
	mCommandStreamManager->RequestWorkAndWait(workItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDevice9::ValidateDevice(DWORD *pNumPasses)
{
	//TODO: Implement.

	BOOST_LOG_TRIVIAL(warning) << "CDevice9::ValidateDevice is not implemented!";

	return S_OK;
}