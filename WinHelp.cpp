#include <iostream>
#include <Windows.h>
#include "WinHelp.h"

void ReportError(DWORD hResult)
{
    wchar_t* msgBuffer;
    size_t msgLen = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, hResult, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&msgBuffer, 0, NULL);
    std::cerr << msgBuffer << std::endl;
    LocalFree(msgBuffer);
}
