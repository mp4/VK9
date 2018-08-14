/*
Copyright(c) 2018 Christopher Joseph Dean Schaefer

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

#include "Perf_CommandStreamManager.h"
#include "Perf_ProcessQueue.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/format.hpp>

#include <iostream>
#include <fstream>

#include <winuser.h>

CommandStreamManager::CommandStreamManager()
	: mWorkerThread(ProcessQueue, this)
{
	//Setup configuration & logging.
	mOptionDescriptions.add_options()
		("LogFile", boost::program_options::value<std::string>(), "The location of the log file.");

	boost::program_options::store(boost::program_options::parse_config_file<char>("VK9.conf", mOptionDescriptions), mOptions);
	boost::program_options::notify(mOptions);

	boost::log::add_console_log(
		std::cout, 
		boost::log::keywords::format = "[%TimeStamp%]: %Message%",
		boost::log::keywords::auto_flush = true
	);

	if (mOptions.count("LogFile"))
	{
		std::ofstream outfile(mOptions["LogFile"].as<std::string>());
		if (!outfile)
		{
			MessageBox(nullptr,
				TEXT("The application does not have permission to write to the log file location. If running on Windows try running as administrator."),
				TEXT("No Write Permission!"),
				IDOK | MB_ICONERROR);
		}
		outfile.close();

		boost::log::add_file_log(
			boost::log::keywords::file_name = mOptions["LogFile"].as<std::string>(),
			boost::log::keywords::format = "[%TimeStamp%]: %Message%",
			boost::log::keywords::auto_flush = true
		);	
	}
	else
	{
		std::ofstream outfile("VK9.log");
		if (!outfile)
		{
			MessageBox(nullptr,
				TEXT("The application does not have permission to write to the log file location. If running on Windows try running as administrator."),
				TEXT("No Write Permission!"),
				IDOK | MB_ICONERROR);
		}
		outfile.close();

		boost::log::add_file_log(
			boost::log::keywords::file_name = "VK9.log",
			boost::log::keywords::format = "[%TimeStamp%]: %Message%",
			boost::log::keywords::auto_flush = true
		);
	}

#ifndef _DEBUG
	boost::log::core::get()->set_filter(boost::log::trivial::severity > boost::log::trivial::info);
#endif

	BOOST_LOG_TRIVIAL(info) << "CommandStreamManager::CommandStreamManager";
}

CommandStreamManager::~CommandStreamManager()
{
	IsRunning = 0;
	mWorkerThread.join();
	BOOST_LOG_TRIVIAL(info) << "CommandStreamManager::~CommandStreamManager";
}

size_t CommandStreamManager::RequestWork(WorkItem* workItem)
{
	if (workItem->Caller != nullptr)
	{
		workItem->Caller->AddRef();
	}

	while (!mWorkItems.push(workItem)) {}
	//while (!mWorkItems.try_enqueue(workItem)) {}

	size_t key = 0;

	//fetching key should be atomic because it's an atomic size_t.
	switch (workItem->WorkItemType)
	{
	case WorkItemType::Instance_Create:
		key = mRenderManager.mStateManager.mInstanceKey++;
		break;
	case WorkItemType::Device_Create:
		key = mRenderManager.mStateManager.mDeviceKey++;
		break;
	case WorkItemType::VertexBuffer_Create:
		key = mRenderManager.mStateManager.mVertexBufferKey++;
		break;
	case WorkItemType::IndexBuffer_Create:
		key = mRenderManager.mStateManager.mIndexBufferKey++;
		break;
	case WorkItemType::Texture_Create:
		key = mRenderManager.mStateManager.mTextureKey++;
		break;
	case WorkItemType::CubeTexture_Create:
		key = mRenderManager.mStateManager.mTextureKey++;
		break;
	case WorkItemType::VolumeTexture_Create:
		key = mRenderManager.mStateManager.mTextureKey++;
		break;
	case WorkItemType::Surface_Create:
		key = mRenderManager.mStateManager.mSurfaceKey++;
		break;
	case WorkItemType::Volume_Create:
		key = mRenderManager.mStateManager.mSurfaceKey++;
		break;
	case WorkItemType::Shader_Create:
		key = mRenderManager.mStateManager.mShaderConverterKey++;
		break;
	case WorkItemType::Query_Create:
		key = mRenderManager.mStateManager.mQueryKey++;
		break;
	default:
		break;
	}

	return key;
}

size_t CommandStreamManager::RequestWorkAndWait(WorkItem* workItem)
{
	size_t result = this->RequestWork(workItem);

	while (!workItem->HasBeenProcessed){}

	return result;
}

WorkItem* CommandStreamManager::GetWorkItem(IUnknown* caller)
{
	WorkItem* returnValue = nullptr;

	//if (!mUnusedWorkItems.try_dequeue(returnValue))
	//{
	//	returnValue = new WorkItem();
	//}
	//else
	//{
	//	returnValue->HasBeenProcessed = false;
	//}

	if (!mUnusedWorkItems.pop(returnValue))
	{
		returnValue = new WorkItem();
	}
	else
	{
		returnValue->HasBeenProcessed = false;
	}

	returnValue->Caller = caller;

	return returnValue;
}