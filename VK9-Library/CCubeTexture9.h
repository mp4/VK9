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
 
#ifndef CCUBETEXTURE9_H
#define CCUBETEXTURE9_H

#include <boost/container/small_vector.hpp>
#include "d3d9.h" // Base class: IDirect3DCubeTexture9
#include "CBaseTexture9.h"
#include "CSurface9.h"
#include "Perf_CommandStreamManager.h"

class CCubeTexture9 : public IDirect3DCubeTexture9
{		
public:
	CCubeTexture9(CDevice9* device,UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, HANDLE *pSharedHandle);
	~CCubeTexture9();

	size_t mId;
	std::shared_ptr<CommandStreamManager> mCommandStreamManager;

	CDevice9* mDevice;
	UINT mEdgeLength = 0;
	UINT mLevels = 0;
	DWORD mUsage = D3DUSAGE_RENDERTARGET;
	D3DFORMAT mFormat = D3DFMT_UNKNOWN;
	D3DPOOL mPool = D3DPOOL_DEFAULT;
	HANDLE* mSharedHandle = nullptr;

	ULONG mReferenceCount = 1;
	VkResult mResult = VK_SUCCESS;
	D3DTEXTUREFILTERTYPE mMipFilter = D3DTEXF_NONE;
	D3DTEXTUREFILTERTYPE mMinFilter = D3DTEXF_NONE;
	D3DTEXTUREFILTERTYPE mMagFilter = D3DTEXF_NONE;

	boost::container::small_vector<boost::container::small_vector<CSurface9*, 5>, 6> mSurfaces;

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

	//IDirect3DCubeTexture9
	virtual HRESULT STDMETHODCALLTYPE AddDirtyRect(D3DCUBEMAP_FACES FaceType, const RECT* pDirtyRect);
	virtual HRESULT STDMETHODCALLTYPE GetCubeMapSurface(D3DCUBEMAP_FACES FaceType, UINT Level, IDirect3DSurface9** ppCubeMapSurface);
	virtual HRESULT STDMETHODCALLTYPE GetLevelDesc(UINT Level, D3DSURFACE_DESC* pDesc);
	virtual HRESULT STDMETHODCALLTYPE LockRect(D3DCUBEMAP_FACES FaceType, UINT Level, D3DLOCKED_RECT* pLockedRect, const RECT* pRect, DWORD Flags);
	virtual HRESULT STDMETHODCALLTYPE UnlockRect(D3DCUBEMAP_FACES FaceType, UINT Level);
};

#endif // CCUBETEXTURE9_H
