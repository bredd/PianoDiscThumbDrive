#include <iostream>
#include <windows.h>

#include "FloppyImage.h"
#include "WinHelp.h"

bool ImageFileRead(std::wstring filename, LPBYTE pImage)
{
    HANDLE hFile = CreateFileW(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        std::wcerr << L"Failed to open source file: " << filename << std::endl;
        ReportError(GetLastError());
        return false;
    }
    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize))
    {
        DWORD hResult = GetLastError();
        CloseHandle(hFile);
        std::wcerr << L"Failed to get source file size." << std::endl;
        ReportError(hResult);
        return false;
    }
    if (fileSize.QuadPart != FLOPPY_IMAGE_SIZE)
    {
        CloseHandle(hFile);
        std::wcerr << L"Invalid floppy image file. Size is not " << FLOPPY_IMAGE_SIZE << L" bytes." << std::endl;
        return false;
    }
    DWORD bytesRead;
    if (!ReadFile(hFile, pImage, FLOPPY_IMAGE_SIZE, &bytesRead, NULL))
    {
        DWORD hResult = GetLastError();
        CloseHandle(hFile);
        std::wcerr << L"Failed to read source file." << std::endl;
        ReportError(hResult);
        return false;
    }
    CloseHandle(hFile);
    if (bytesRead != FLOPPY_IMAGE_SIZE)
    {
        std::wcerr << L"Failed to read entire source file." << std::endl;
        return false;
    }
    return true;
}

bool ImageFileWrite(std::wstring filename, LPBYTE pImage, bool overwrite)
{
    HANDLE hFile = CreateFileW(filename.c_str(), GENERIC_WRITE, 0, NULL, overwrite ? CREATE_ALWAYS : CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        std::wcerr << L"Failed to open destination file: " << filename << std::endl;
        ReportError(GetLastError());
        return false;
    }
    DWORD bytesWritten;
    if (!WriteFile(hFile, pImage, FLOPPY_IMAGE_SIZE, &bytesWritten, NULL))
    {
        DWORD hResult = GetLastError();
        CloseHandle(hFile);
        std::wcerr << L"Failed to write destination file." << std::endl;
        ReportError(hResult);
        return false;
    }
    CloseHandle(hFile);
    if (bytesWritten != FLOPPY_IMAGE_SIZE)
    {
        std::wcerr << L"Failed to write entire destination file." << std::endl;
        return false;
    }
    return true;
}