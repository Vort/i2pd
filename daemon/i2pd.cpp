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

bool timeThreadExiting = false;
time_t lastRequestTime = 0;
const int dumpInterval = 30;
std::shared_ptr<std::thread> timeThread;

namespace i2p
{
	extern std::mutex g_SSU2cResultsMutex;
	extern std::vector<std::string> g_SSU2cResults;
}

void StartProfiling()
{
	lastRequestTime = std::time(nullptr);
	timeThread = std::make_shared<std::thread>([] 
	{
		std::ofstream ssu2cstatfs;
#ifdef _WIN32
		ssu2cstatfs.open("ssu2cstat.txt", std::ios::app);
#else
		ssu2cstatfs.open("/tmp/ssu2cstat.txt", std::ios::app);
#endif
		for (;;)
		{
			time_t now = std::time(nullptr);
			if (now / dumpInterval > lastRequestTime / dumpInterval)
			{
				lastRequestTime = now;
				std::unique_lock<std::mutex> l(i2p::g_SSU2cResultsMutex);
				for (size_t i = 0; i < i2p::g_SSU2cResults.size(); i++)
					ssu2cstatfs << i2p::g_SSU2cResults[i] << std::endl;
				i2p::g_SSU2cResults.clear();
				ssu2cstatfs << std::flush;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(250));
			if (timeThreadExiting)
				break;
		}
		ssu2cstatfs.close();
	});
}

void StopProfiling()
{
	timeThreadExiting = true;
	timeThread->join();
	timeThread = nullptr;
}

int main( int argc, char* argv[] )
{
	if (Daemon.init(argc, argv))
	{
		if (Daemon.start())
		{
			StartProfiling();
			Daemon.run ();
			StopProfiling();
		}
		else
			return EXIT_FAILURE;
		Daemon.stop();
	}
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
	return main(__argc, __argv);
}
#endif
