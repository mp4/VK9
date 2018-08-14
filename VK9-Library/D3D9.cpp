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
 
#include "d3d9.h"
#include "C9.h"

//#include "PrivateTypes.h"

IDirect3D9* WINAPI Direct3DCreate9(UINT SDKVersion)
{
	C9* instance = new C9();

	WorkItem* workItem = instance->mCommandStreamManager->GetWorkItem(nullptr);
	//std::lock_guard<std::mutex> lock(workItem->Mutex);
	workItem->WorkItemType = WorkItemType::Instance_Create;
	instance->mId = instance->mCommandStreamManager->RequestWork(workItem);

	//WINAPI to get monitor info
	EnumDisplayMonitors(GetDC(NULL), NULL, MonitorEnumProc, (LPARAM)&(instance->mMonitors));

	return (IDirect3D9*)instance;
}

HRESULT WINAPI Direct3DCreate9Ex(UINT SDKVersion, IDirect3D9Ex** out)
{
	out = nullptr;

	//TODO: Implement, maybe.

	BOOST_LOG_TRIVIAL(warning) << "Direct3DCreate9Ex is not implemented!";

	return E_NOTIMPL;
}

int WINAPI D3DPERF_BeginEvent(DWORD col, LPCWSTR wszName)
{
	return 0;
}

int WINAPI D3DPERF_EndEvent()
{
	return 0;
}

void WINAPI D3DPERF_SetMarker(D3DCOLOR color, const WCHAR *name)
{

}

void WINAPI D3DPERF_SetRegion(D3DCOLOR color, const WCHAR *name)
{

}

BOOL WINAPI D3DPERF_QueryRepeatFrame()
{
	return false;
}

void WINAPI D3DPERF_SetOptions(DWORD options)
{

}

DWORD WINAPI D3DPERF_GetStatus()
{
	return 0;
}

/* 
Other things to possibly implement.

DXVA2CreateDirect3DDeviceManager9
DXVA2CreateVideoService
*/
