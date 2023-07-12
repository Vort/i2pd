/*
* Copyright (c) 2013-2020, The PurpleI2P Project
*
* This file is part of Purple i2pd project and licensed under BSD3
*
* See full license text in LICENSE file at top of project tree
*/

#include <stdlib.h>
#include "Daemon.h"

#if defined(QT_GUI_LIB)
namespace i2p
{
namespace qt
{
	int RunQT (int argc, char* argv[]);
}
}

int main( int argc, char* argv[] )
{
	return i2p::qt::RunQT (argc, argv);
}
#else

#include <fstream>
#include <thread>
#include <ctime>
#include <chrono>
#include <mutex>
#include <vector>
#include <iomanip>

std::ofstream crstatfs;
bool timeThreadExiting = false;
time_t lastRequestTime = 0;
const int dumpInterval = 30;
std::shared_ptr<std::thread> timeThread;

namespace i2p
{
	extern std::mutex g_CrResultsMutex;
	extern std::vector<std::string> g_CrResults;
}

void DumpProfileData()
{
	std::unique_lock<std::mutex> l(i2p::g_CrResultsMutex);
	for (int i = 0; i < i2p::g_CrResults.size(); i++)
		crstatfs << i2p::g_CrResults[i] << std::endl;
	i2p::g_CrResults.clear();
}

void StartProfiling()
{
	lastRequestTime = std::time(nullptr);
	timeThread = std::make_shared<std::thread>([] {
		for (;;)
		{
			time_t now = std::time(nullptr);
			if (now / dumpInterval > lastRequestTime / dumpInterval)
			{
				lastRequestTime = now;
				DumpProfileData();
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(250));
			if (timeThreadExiting)
				return;
		}
	});

#ifdef _WIN32
	crstatfs.open("crstat.txt", std::ios::app);
#else
	crstatfs.open("/tmp/crstat.txt", std::ios::app);
#endif
}

void StopProfiling()
{
	crstatfs.close();

	timeThreadExiting = true;
	timeThread->join();
	timeThread = nullptr;
}

int main( int argc, char* argv[] )
{
	StartProfiling();
	if (Daemon.init(argc, argv))
	{
		if (Daemon.start())
			Daemon.run ();
		else
			return EXIT_FAILURE;
		Daemon.stop();
	}
	StopProfiling();
	return EXIT_SUCCESS;
}
#endif

#ifdef _WIN32
#include <windows.h>

int CALLBACK WinMain(
	_In_ HINSTANCE hInstance,
	_In_ HINSTANCE hPrevInstance,
	_In_ LPSTR     lpCmdLine,
	_In_ int       nCmdShow
	)
{
	int r = main(__argc, __argv);
	return r;
}
#endif
