// PianoDiscThumbDrive.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <vector>
#include <windows.h>
#include <cwctype>

#include "FloppyImage.h"
#include "WinHelp.h"
#include "ImageFile.h"
#include "ThumbDriveImage.h"
#include "MidiImage.h"

extern const wchar_t* g_syntax;
bool g_reportSyntax = false;
bool g_verbose = false;
std::vector<std::wstring> g_srcMidiPaths;
std::wstring g_srcImg;
std::wstring g_dstImg;
std::wstring g_dstDir;

void syntax();
int parseCommandLine(int argc, wchar_t* argv[]);
int addToMidiPaths(const wchar_t* pattern);
void winSlash(wchar_t* str); // Substitute windows slashes for forward slashes
bool tryParseThumbDriveImageNum(const wchar_t* name, wchar_t* driveLetter, int* imageNumber);

int wmain( int argc, wchar_t *argv[])
{
    int result = parseCommandLine(argc, argv);
    if (g_reportSyntax) syntax();
    if (result != 0 || g_reportSyntax) return result;

    if (g_verbose) {
        if (g_srcMidiPaths.size() > 0) {
            for (const auto& path : g_srcMidiPaths) {
                std::wcout << L"-midi " << path << std::endl;
            }
        }
        if (g_srcImg.length() > 0) {
            std::wcout << L"-simg " << g_srcImg << std::endl;
        }
        if (g_dstImg.length() > 0) {
            std::wcout << L"-dimg " << g_dstImg << std::endl;
        }
        if (g_dstDir.length() > 0) {
            std::wcout << L"-ddir " << g_dstDir << std::endl;
        }
        std::wcout << std::endl;
    }

    if (g_srcMidiPaths.size() > 0 && g_srcImg.length() > 0)
    {
        std::wcerr << L"Error: Both MIDI and image sources specified. Use either -midi or -simg but not both. (-h for help)" << std::endl;
        return -1;
    }
    if (g_dstImg.length() > 0 && g_dstDir.length() > 0)
    {
        std::wcerr << L"Error: Both image and directory destinations specified. Use either -dimg or -ddir but not both. (-h for help)" << std::endl;
        return -1;
    }
    
    // Allocate a page-aligned buffer for reading and writing
    LPBYTE pImage = (LPBYTE)VirtualAlloc(NULL, FLOPPY_IMAGE_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    // === Get the image =======
    if (g_srcMidiPaths.size() > 0)
    {
        if (!MidiToImage(g_srcMidiPaths, pImage)) {
            return -1; // Error already reported
        }
    }
    else if (g_srcImg.length() > 0)
    {
        std::wcout << L"Reading from: " << g_srcImg << std::endl;
        wchar_t driveLetter;
        int imageNum;
        if (tryParseThumbDriveImageNum(g_srcImg.c_str(), &driveLetter, &imageNum))
        {
            if (!ThumbDriveRead(driveLetter, imageNum, pImage))
            {
                return -1; // Error already reported
            }
        }
        else {
            if (!ImageFileRead(g_srcImg, pImage))
            {
                return -1;
            }
        }
    }
    else
    {
        std::wcerr << L"Error: Neither MIDI nor image source specified. Use either -midi or -simg. (-h for help)" << std::endl;
        return -1;
    }

    // === Store the image ======
    if (g_dstImg.length() > 0)
    {
        std::wcout << L"Writing to: " << g_dstImg << std::endl;
        wchar_t driveLetter;
        int imageNum;
        if (tryParseThumbDriveImageNum(g_dstImg.c_str(), &driveLetter, &imageNum))
        {
            if (!ThumbDriveWrite(driveLetter, imageNum, pImage))
            {
                return -1; // Error already reported
            }
        }
        else {
            if (!ImageFileWrite(g_dstImg, pImage))
            {
                return -1;
            }
        }
    }
    else if (g_dstDir.length() > 0)
    {
        std::wcerr << L"Directory destination not yet implemented. Use either -dimg or -ddir. (-h for help)" << std::endl;
        return -1;
    }
    else
    {
        std::wcerr << L"Error: Neither image nor directory destination specified. Use either -dimg or -ddir. (-h for help)" << std::endl;
        return -1;
    }

    std::wcout << L"Success.";
    return 0;
}

void syntax() {
    std::wcerr << g_syntax;
}

int parseCommandLine(int argc, wchar_t* argv[]) {
    if (argc < 2) {
        g_reportSyntax = true;
        return 0;
    }
    for (int i = 1; i < argc; ++i) {
        if (0 == _wcsicmp(argv[i], L"-h")) {
            g_reportSyntax = true;
            return 0;
        }
        else if (0 == _wcsicmp(argv[i], L"-v")) {
            g_verbose = true;
        }
        else if (0 == _wcsicmp(argv[i], L"-midi")) {
            // Advance to the next string and check for end
            ++i;
            if (i >= argc) {
                std::wcerr << L"No value for argument '-midi'." << std::endl;
                return -1;
            }
            winSlash(argv[i]);

            int findCount = 0;

            // Check for wildcards
            if (NULL != wcschr(argv[i], L'*') || NULL != wcschr(argv[i], L'?'))
            {
                findCount = addToMidiPaths(argv[i]);
            }
            else
            {
                // Get the attributes
                DWORD attributes = GetFileAttributesW(argv[i]);
                if (attributes == INVALID_FILE_ATTRIBUTES)
                {
                    findCount = 0;
                }
                else if ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
                {
                    findCount = addToMidiPaths((std::wstring(argv[i]) + L"\\*.mid").c_str());
                }
                else {
                    g_srcMidiPaths.push_back(std::wstring(argv[i]));
                    findCount = 1;
                }
            }
            if (findCount <= 0) {
                std::wcerr << L"No matches found for -midi '" << argv[i] << "'." << std::endl;
                return -1;
            }
        }
        else if (0 == _wcsicmp(argv[i], L"-simg")) {
            // Advance to the next string and check for end
            ++i;
            if (i >= argc) {
                std::wcerr << L"No value for argument '-simg'." << std::endl;
                return -1;
            }
            g_srcImg = argv[i];
        }
        else if (0 == _wcsicmp(argv[i], L"-dimg")) {
            // Advance to the next string and check for end
            ++i;
            if (i >= argc) {
                std::wcerr << L"No value for argument '-dimg'." << std::endl;
                return -1;
            }
            g_dstImg = argv[i];
        }
        else if (0 == _wcsicmp(argv[i], L"-ddir")) {
            // Advance to the next string and check for end
            ++i;
            if (i >= argc) {
                std::wcerr << L"No value for argument '-ddir'." << std::endl;
                return -1;
            }
            g_dstImg = argv[i];
        }
        else {
            std::wcerr << L"Unexpected argument: " << argv[i] << std::endl;
        }
    }
    return 0;
}

// Substitute windows slashes for forward slashes
void winSlash(wchar_t* str) {
    for (; *str != '\0'; ++str) {
        if (*str == '/') *str = '\\';
    }
}

int addToMidiPaths(const wchar_t* pattern)
{
    int findCount = 0;

    // Find the directory prefix (if any)
    std::wstring prefix;
    wchar_t* lastSlash = wcsrchr((wchar_t*)pattern, '\\');
    if (NULL != lastSlash) {
        prefix = std::wstring(pattern, lastSlash + 1 - pattern);
    }
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(pattern, &findData);
    while (hFind != INVALID_HANDLE_VALUE)
    {
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            std::wstring path = prefix + findData.cFileName;
            g_srcMidiPaths.push_back(path);
            ++findCount;
        }
        if (!FindNextFileW(hFind, &findData))
        {
            FindClose(hFind);
            hFind = INVALID_HANDLE_VALUE;
        }
    }

    return findCount;
}

bool tryParseThumbDriveImageNum(const wchar_t* name, wchar_t* driveLetter, int* imageNumber) {
    if (wcslen(name) < 3 || name[1] != ':') return false;
    wchar_t letter = towupper(name[0]);
    if (letter < L'A' || letter > L'Z') return false;
    int num = 0;
    const wchar_t* p = name + 2;
    while (*p != L'\0') {
        if (*p < L'0' || *p > L'9') return false;
        num = num * 10 + (*p - L'0');
        ++p;
    }
    *driveLetter = letter;
    *imageNumber = num;
    return true;
}

const wchar_t* g_syntax =
L"Syntax:\n"
"PianoDiscThumbDrive -midi <midiPath> ... -dimg <dstImage>\n"
"  Create an image from MIDI files\n"
"PianoDiscThumbDrive -simg <srcImage> -dimg <dstImage\n"
"  Copy an image\n"
"PianoDiscThumbDrive -simg <srcImage> -ddir <dstDirectory>\n"
"  Unpack an image into the original files\n"
"  (Not yet implemented)\n"
"\n"
"Arguments:\n"
"-midi\n"
"  This argument may be repeated to include multiple sources.\n"
"  Path to one or more MIDI source files. They should have a .mid extension.\n"
"  The filename portion of the path may have wildcards in which case multiple\n"
"  files are included.\n"
"  The path may be to a directory in which case all .mid files in that directory\n"
"  will be included.\n"
"-simg\n"
"  Designation of a source image. It may be in one of two formats.\n"
"  A path to a filename with a .img extension indicates a floppy image file.\n"
"  A drive letter followed by a number (e.g. F:25) indicates a numbered image\n"
"  on a thumb drive intended for use on a floppy disk emulator.\n"
"-dimg\n"
"  Designation of a destination image. It may be in either of the two formats\n"
"  listed for -simg: a path to an image file or a numbered image on a thumb\n"
"  drive intended for use on a floppy disk emulator.\n"
"\n"
"Additional Arguments\n"
"-h\n"
"  Help: Print this syntax.\n"
"-v\n"
"  Verbose: Print extra information.\n";