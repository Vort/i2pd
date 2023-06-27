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
int main( int argc, char* argv[] )
{
	if (Daemon.init(argc, argv))
	{
		if (Daemon.start())
			Daemon.run ();
		else
			return EXIT_FAILURE;
		Daemon.stop();
	}
	return EXIT_SUCCESS;
}
#endif

#ifdef _WIN32
#include <windows.h>
#include <fstream>
#include <thread>
#include <ctime>
#include <mutex>
#include <vector>
#include <iomanip>

std::ofstream failstatfs;
bool timeThreadExiting = false;
time_t lastRequestTime = 0;
const int dumpInterval = 30;
std::shared_ptr<std::thread> timeThread;

namespace i2p
{
	extern std::mutex g_FailsMutex;
	extern std::vector<std::string> g_Fails;
}

void DumpProfileData()
{
	time_t now = std::time(nullptr);
	auto pt = std::put_time(std::gmtime(&now), "%Y_%m_%d_%H_%M_%S");
	std::unique_lock<std::mutex> l(i2p::g_FailsMutex);
	for (int i = 0; i < i2p::g_Fails.size(); i++)
		failstatfs << "[" << pt << "]: " << i2p::g_Fails[i] << std::endl;
	i2p::g_Fails.clear();
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
			Sleep(250);
			if (timeThreadExiting)
				return;
		}
	});

	failstatfs.open("failstat.txt", std::ios::app);
}

void StopMemoryProfiling()
{
	failstatfs.close();

	timeThreadExiting = true;
	timeThread->join();
	timeThread = nullptr;
}

int CALLBACK WinMain(
	_In_ HINSTANCE hInstance,
	_In_ HINSTANCE hPrevInstance,
	_In_ LPSTR     lpCmdLine,
	_In_ int       nCmdShow
	)
{
	StartProfiling();
	int r = main(__argc, __argv);
	StopMemoryProfiling();
	return r;
}
#endif
