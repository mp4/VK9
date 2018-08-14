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
 
#ifndef CTEXTURE9_H
#define CTEXTURE9_H

#include <boost/container/small_vector.hpp>
#include "d3d9.h" // Base class: IDirect3DTexture9
#include "Perf_CommandStreamManager.h"

class CSurface9;

class CTexture9 : public IDirect3DTexture9
{
public:
	CTexture9(CDevice9* device,UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, HANDLE *pSharedHandle);
	~CTexture9();

	size_t mId;
	std::shared_ptr<CommandStreamManager> mCommandStreamManager;

	CDevice9* mDevice = nullptr;
	UINT mWidth = 0;
	UINT mHeight = 0;
	UINT mDepth = 0;
	UINT mLevels = 0;
	DWORD mUsage = D3DUSAGE_RENDERTARGET;
	D3DFORMAT mFormat = D3DFMT_UNKNOWN;
	D3DPOOL mPool = D3DPOOL_DEFAULT;
	HANDLE* mSharedHandle = nullptr;

	ULONG mReferenceCount = 1;
	D3DTEXTUREFILTERTYPE mMipFilter = D3DTEXF_NONE;
	D3DTEXTUREFILTERTYPE mMinFilter = D3DTEXF_NONE;
	D3DTEXTUREFILTERTYPE mMagFilter = D3DTEXF_NONE;

	boost::container::small_vector<CSurface9*,5> mSurfaces;

	void Flush();

public:
	//IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,void  **ppv);
	virtual ULONG STDMETHODCALLTYPE AddRef(void);	
	virtual ULONG STDMETHODCALLTYPE Release(void);

	//IDirect3DResource9
	virtual HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9** ppDevice);
	virtual HRESULT STDMETHODCALLTYPE FreePrivateData(REFGUID refguid);
	virtual DWORD STDMETHODCALLTYPE GetPriority();
	virtual HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID refguid, void* pData, DWORD* pSizeOfData);
	virtual D3DRESOURCETYPE STDMETHODCALLTYPE GetType();
	virtual void STDMETHODCALLTYPE PreLoad();
	virtual DWORD STDMETHODCALLTYPE SetPriority(DWORD PriorityNew);
	virtual HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID refguid, const void* pData, DWORD SizeOfData, DWORD Flags);

	//IDirect3DBaseTexture9
	virtual VOID STDMETHODCALLTYPE GenerateMipSubLevels();
	virtual D3DTEXTUREFILTERTYPE STDMETHODCALLTYPE GetAutoGenFilterType();
	virtual DWORD STDMETHODCALLTYPE GetLOD();
	virtual DWORD STDMETHODCALLTYPE GetLevelCount();
	virtual HRESULT STDMETHODCALLTYPE SetAutoGenFilterType(D3DTEXTUREFILTERTYPE FilterType);
	virtual DWORD STDMETHODCALLTYPE SetLOD(DWORD LODNew);
		
	//virtual D3DRESOURCETYPE STDMETHODCALLTYPE GetType(); in IDirect3DResource9

	//IDirect3DTexture9
	virtual HRESULT STDMETHODCALLTYPE AddDirtyRect(const RECT* pDirtyRect);
	virtual HRESULT STDMETHODCALLTYPE GetLevelDesc(UINT Level, D3DSURFACE_DESC* pDesc);
	virtual HRESULT STDMETHODCALLTYPE GetSurfaceLevel(UINT Level, IDirect3DSurface9** ppSurfaceLevel);
	virtual HRESULT STDMETHODCALLTYPE LockRect(UINT Level, D3DLOCKED_RECT* pLockedRect, const RECT* pRect, DWORD Flags);
	virtual HRESULT STDMETHODCALLTYPE UnlockRect(UINT Level);
};

#endif // CTEXTURE9_H
