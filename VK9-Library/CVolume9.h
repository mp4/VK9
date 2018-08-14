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

#ifndef CVOLUME9_H
#define CVOLUME9_H

#include "d3d9.h"
#include "Perf_CommandStreamManager.h"

class CDevice9;
class CVolumeTexture9;

class CVolume9 : public IDirect3DVolume9
{
public:
	CVolume9(CDevice9* device, CVolumeTexture9* texture, UINT Width, UINT Height, UINT Depth, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, HANDLE *pSharedHandle);
	~CVolume9();

	size_t mId;
	size_t mTextureId;
	std::shared_ptr<CommandStreamManager> mCommandStreamManager;

	CDevice9* mDevice = nullptr;
	CVolumeTexture9* mTexture = nullptr;
	UINT mWidth = 0;
	UINT mHeight = 0;
	UINT mDepth = 0;
	DWORD mUsage = D3DUSAGE_RENDERTARGET;
	D3DFORMAT mFormat = D3DFMT_UNKNOWN;
	D3DPOOL mPool = D3DPOOL_DEFAULT;
	HANDLE* mSharedHandle = nullptr;

	BOOL mDiscard = 0;
	BOOL mLockable = 0;

	ULONG mReferenceCount = 1;
	uint32_t mMipIndex = 0;
	uint32_t counter = 0;
	DWORD mFlags = 0;
	uint32_t mTargetLayer = 0;

	void Init();
	void Flush();
public:

	//IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void  **ppv);
	virtual ULONG STDMETHODCALLTYPE AddRef(void);
	virtual ULONG STDMETHODCALLTYPE Release(void);

	/*** IDirect3DVolume9 methods ***/
	virtual HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9** ppDevice);
	virtual HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
	virtual HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID refguid, void* pData, DWORD* pSizeOfData);
	virtual HRESULT STDMETHODCALLTYPE FreePrivateData(REFGUID refguid);
	virtual HRESULT STDMETHODCALLTYPE GetContainer(REFIID riid, void** ppContainer);
	virtual HRESULT STDMETHODCALLTYPE GetDesc(D3DVOLUME_DESC *pDesc);
	virtual HRESULT STDMETHODCALLTYPE LockBox(D3DLOCKED_BOX * pLockedVolume, CONST D3DBOX* pBox, DWORD Flags);
	virtual HRESULT STDMETHODCALLTYPE UnlockBox();
};

#endif // CVOLUME9_H