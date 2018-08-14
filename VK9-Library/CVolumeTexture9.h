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
 
#ifndef CVOLUMETEXTURE9_H
#define CVOLUMETEXTURE9_H

#include "d3d9.h" // Base class: IDirect3DCubeTexture9
#include <vector>
#include "Perf_CommandStreamManager.h"

class CVolume9;

class CVolumeTexture9 : public IDirect3DVolumeTexture9
{
public:
	CVolumeTexture9(CDevice9* device, UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, HANDLE *pSharedHandle);
	~CVolumeTexture9();

	size_t mId;
	std::shared_ptr<CommandStreamManager> mCommandStreamManager;

	CDevice9* mDevice;
	UINT mWidth;
	UINT mHeight;
	UINT mDepth;
	UINT mLevels;
	DWORD mUsage;
	D3DFORMAT mFormat;
	D3DPOOL mPool;
	HANDLE* mSharedHandle;

	ULONG mReferenceCount;
	VkResult mResult;
	D3DTEXTUREFILTERTYPE mMipFilter = D3DTEXF_NONE;
	D3DTEXTUREFILTERTYPE mMinFilter = D3DTEXF_NONE;
	D3DTEXTUREFILTERTYPE mMagFilter = D3DTEXF_NONE;

	std::vector<CVolume9*> mVolumes;

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

	//IDirect3DVolumeTexture9
	virtual HRESULT STDMETHODCALLTYPE AddDirtyBox(const D3DBOX* pDirtyBox);
	virtual HRESULT STDMETHODCALLTYPE GetLevelDesc(UINT Level, D3DVOLUME_DESC* pDesc);
	virtual HRESULT STDMETHODCALLTYPE GetVolumeLevel(UINT Level, IDirect3DVolume9** ppVolumeLevel);
	virtual HRESULT STDMETHODCALLTYPE LockBox(UINT Level, D3DLOCKED_BOX* pLockedVolume, const D3DBOX* pBox, DWORD Flags);
	virtual HRESULT STDMETHODCALLTYPE UnlockBox(UINT Level);
};

#endif // CVOLUMETEXTURE9_H
