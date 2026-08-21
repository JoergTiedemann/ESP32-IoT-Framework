#ifndef BOARDINFO_H
#define BOARDINFO_H

#include <Arduino.h>
#include "webServer.h"

class CBoardsInformation
{
public : 
    void PrintBoardInformation();
    CBoardsInformation()
	{
	}
    ~CBoardsInformation()
	{
	}
	void print_used_libraries();
	static void SendLibraryVersion( WebserverLibraryInfo LibraryVersionInfo);

	String m_strPlatformVersion;
	String m_strFrameworkVersion;
	String m_strSDKVersion;
	String m_strLibraryVersion;
};
extern CBoardsInformation BoardInformation;

#endif
