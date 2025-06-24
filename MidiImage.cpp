#include <iostream>
#include <vector>
#include <cstring>
#include <windows.h>

#include "MidiImage.h"
#include "FloppyImage.h"
#include "WinHelp.h"


// References:
// https://www.tavi.co.uk/phobos/fat.html - This has been the most helpful even though the examples are all FAT16 instead of FAT12

const char DiskLabel[] = "Piano_Midi  "; // Pad to at least 11 bytes;
const byte FatHeader[] = { 0xF0, 0xFF, 0xFF };

bool AddFile(LPBYTE pImage, std::wstring filename);
void FormatImage(LPBYTE pImage);
unsigned int GetFAT(LPBYTE pImage, unsigned int au);
void PutFAT(LPBYTE pImage, unsigned int au, unsigned int value);
void To8dot3Filename(const wchar_t* srcFilename, char* dstFilename);
void Uniquify8dot3Filename(char* filename, LPBYTE pImage);
void SystemTimeToFloppyTime(SYSTEMTIME* pSystemTime, FloppyDateTime* pFloppyTime);
void FileTimeToFloppyTime(FILETIME* pFileTime, FloppyDateTime* pFloppyTime);

bool MidiToImage(std::vector<std::wstring> midiPaths, LPBYTE pImage)
{
	FormatImage(pImage);
	for (const auto& path : midiPaths)
	{
		if (!AddFile(pImage, path))
		{
			return false;
		}
	}
	return true;
}

void FormatImage(LPBYTE pImage) {
	// Zero it all
	memset(pImage, 0, FLOPPY_IMAGE_SIZE);

	// Fill in the boot sector (minus serial number and volume label)
	memcpy(pImage, BootSector, FLOPPY_BLOCK_SIZE);

	// Random "serial number"
	{
		// They want me to use GetTickCount64 but for my purposes this one works better
#pragma warning( push )
#pragma warning ( disable: 28159)
		DWORD ticks = GetTickCount();
#pragma warning( pop )
		memcpy(pImage + BOOTSECTOR_SERIALNUM_OFFSET, &ticks, BOOTSECTOR_SERIALNUM_LEN);
	}

	// Volume Label
	memcpy(pImage + BOOTSECTOR_LABEL_OFFSET, DiskLabel, BOOTSECTOR_LABEL_LEN);

	// Init the two FATs
	memcpy(pImage + FLOPPY_FAT0_OFFSET, FatHeader, sizeof(FatHeader));
	memcpy(pImage + FLOPPY_FAT1_OFFSET, FatHeader, sizeof(FatHeader));

	// Init the directory
	FloppyDirectoryEntry* pFirstEntry = (FloppyDirectoryEntry*)(pImage + FLOPPY_ROOT_DIR_OFFSET);
	memcpy(pFirstEntry->Filename, DiskLabel, sizeof(FloppyDirectoryEntry::Filename));
	pFirstEntry->Attributes = 0x08;
	{
		SYSTEMTIME st;
		GetLocalTime(&st);
		SystemTimeToFloppyTime(&st, &(pFirstEntry->DateTime));
	}
}

bool AddFile(LPBYTE pImage, std::wstring filename)
{
	// Generate an 8.3 filename
	char floppyFilename[11];
	To8dot3Filename(filename.c_str(), floppyFilename);
	Uniquify8dot3Filename(floppyFilename, pImage);

	// Find the next available FAT entry
	unsigned int fileFirstAu = 2;
	for (;;)
	{
		if (fileFirstAu >= FLOPPY_DATA_AU_PER_DISK)
		{
			std::wcerr << L"Floppy is full." << std::endl;
			return false;
		}
		if (GetFAT(pImage, fileFirstAu) == 0) break;
		++fileFirstAu;
	}

	// Find the next available Directory entry
	FloppyDirectoryEntry* pDirEntry;
	{
		pDirEntry = (FloppyDirectoryEntry*)(pImage + FLOPPY_ROOT_DIR_OFFSET);
		FloppyDirectoryEntry* pEnd = pDirEntry + FLOPPY_ROOT_DIR_ENTRIES;
		for (;;)
		{
			if (pDirEntry >= pEnd) {
				std::wcerr << L"Floppy directory is full." << std::endl;
				return false;
			}
			char nameFirst = pDirEntry->Filename[0];
			if (nameFirst == '\0' || nameFirst == '\xE5') break;
			++pDirEntry;
		}
	}

	// Open the source file
	HANDLE hFile = CreateFileW(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		std::wcerr << L"Failed to open source file: " << filename << std::endl;
		ReportError(GetLastError());
		return false;
	}

	// Get its size
	LARGE_INTEGER fileSize;
	if (!GetFileSizeEx(hFile, &fileSize))
	{
		DWORD hResult = GetLastError();
		CloseHandle(hFile);
		std::wcerr << L"Failed to get source file size." << std::endl;
		ReportError(hResult);
		return false;
	}

	// See if there's enough room left
	if ((size_t)fileSize.QuadPart > (FLOPPY_DATA_AU_PER_DISK - fileFirstAu) * FLOPPY_AU_SIZE)
	{
		CloseHandle(hFile);
		std::wcerr << L"Insufficient space for source file: " << filename << std::endl;
		return false;
	}

	// Get its date modified
	FILETIME dateModified;
	if (!GetFileTime(hFile, NULL, NULL, &dateModified))
	{
		DWORD hResult = GetLastError();
		CloseHandle(hFile);
		std::wcerr << L"Failed to get source file date modified." << std::endl;
		ReportError(hResult);
		return false;
	}

	// Write the directory entry
	memcpy(pDirEntry->Filename, floppyFilename, sizeof(floppyFilename));
	pDirEntry->Attributes = 0x20; // Archive bit
	FileTimeToFloppyTime(&dateModified, &pDirEntry->DateTime);
	pDirEntry->StartCluster = fileFirstAu;
	pDirEntry->FileSize = fileSize.LowPart;

	// Write the FAT entries
	{
		int fileAus = (int)(fileSize.LowPart / FLOPPY_AU_SIZE); // Round down
		for (int i = 0; i < fileAus; ++i)
			PutFAT(pImage, fileFirstAu + i, fileFirstAu + i + 1);
		PutFAT(pImage, fileFirstAu + fileAus, 0xFFFF); // Last entry
	}

	// Read the data from the file into the image
	DWORD bytesRead;
	if (!ReadFile(hFile, pImage + FLOPPY_DATA_OFFSET + (fileFirstAu - 2) * FLOPPY_AU_SIZE, fileSize.LowPart, &bytesRead, NULL))
	{
		DWORD hResult = GetLastError();
		CloseHandle(hFile);
		std::wcerr << L"Failed to read source file: " << filename << std::endl;
		ReportError(hResult);
		return false;
	}
	CloseHandle(hFile);
	if (bytesRead != fileSize.LowPart)
	{
		std::wcerr << L"Failed to read entire source file:" << filename << std::endl;
		return false;
	}

	return true;
}

unsigned int GetFAT(LPBYTE pImage, unsigned int au)
{
	if (au < FLOPPY_FIRST_DATA_AU || au >= FLOPPY_DATA_AU_PER_DISK) return 0xFFF; // Out of range
	DWORD* pEntry = (DWORD*)(pImage + FLOPPY_FAT0_OFFSET + (au >> 1) * 3);
	return (int)(((au & 0x01) == 0) ? *pEntry & 0x0FFF : (*pEntry >> 12) & 0xFFF);
}

void PutFAT(LPBYTE pImage, unsigned int au, unsigned int value)
{
	if (au < FLOPPY_FIRST_DATA_AU || au >= FLOPPY_DATA_AU_PER_DISK) return; // Out of range
	DWORD* pEntry0 = (DWORD*)(pImage + FLOPPY_FAT0_OFFSET + (au >> 1) * 3);
	DWORD* pEntry1 = (DWORD*)(((BYTE*)pEntry0) + FLOPPY_FAT_SIZE);
	DWORD newEntry;
	if ((au & 0x01) == 0)
	{
		newEntry = (*pEntry0 & 0xFFFFF000) | (((DWORD)value) & 0x00000FFF);
	}
	else {
		newEntry = (*pEntry0 & 0xFF000FFF) | ((((DWORD)value) << 12) & 0x00FFF000);
	}
	*pEntry0 = newEntry;
	*pEntry1 = newEntry;
}

void SystemTimeToFloppyTime(SYSTEMTIME* pSystemTime, FloppyDateTime* pFloppyTime)
{
	pFloppyTime->twoSecond = pSystemTime->wSecond / 2;
	pFloppyTime->minute = pSystemTime->wMinute;
	pFloppyTime->hour = pSystemTime->wHour;
	pFloppyTime->day = pSystemTime->wDay;
	pFloppyTime->month = pSystemTime->wMonth;
	pFloppyTime->year = pSystemTime->wYear - 1980;
}

void FileTimeToFloppyTime(FILETIME* pFileTime, FloppyDateTime* pFloppyTime)
{
	SYSTEMTIME st;
	if (!FileTimeToSystemTime(pFileTime, &st)) {
		*((DWORD*)pFloppyTime) = 0;
		return;
	}
	SystemTimeToFloppyTime(&st, pFloppyTime);
}

void CopyWStringToFnString(const wchar_t* pSrc, const wchar_t* pSrcEnd, char* pDst, const char* pDstEnd)
{
	char* p = pDst;
	while (pSrc < pSrcEnd && p < pDstEnd)
	{
		// If in ASCII printable range and not a special character
		wchar_t c = *pSrc++;
		if ((c > L' ' && c <= '~')
			&& c != '/' && c != '\\' && c != '"' && c != '<' && c != '>' && c != ':' && c != '|' && c != '?' && c != '*')
		{
			*p++ = std::toupper((char)c);
		}
	}

	// Put something in if all was out of range
	if (p == pDst)
		*p++ = '0';

	// Pad with spaces
	while (p < pDstEnd)
		*p++ = ' ';
}

void To8dot3Filename(const wchar_t* srcPath, char* dstFilename)
{
	// Find the filename (it is likely to be a path)
	const wchar_t* pSrc = wcsrchr(srcPath, L'\\');
	if (NULL == pSrc)
		pSrc = srcPath;
	else
		++pSrc;

	// Find the end
	const wchar_t* pSrcEnd = pSrc + wcslen(pSrc);

	// Find the extension (if any)
	const wchar_t* pSrcExt = wcsrchr(pSrc, L'.');
	if (NULL == pSrcExt)
		pSrcExt = pSrcEnd;

	// Copy name and extension
	CopyWStringToFnString(pSrc, pSrcExt, dstFilename, dstFilename + 8);
	CopyWStringToFnString(pSrcExt + 1, pSrcEnd, dstFilename + 8, dstFilename + 11); // this is OK if there's no extension and pSrcExt+1 is greater than pSrcEnd because the copier will just exit the loop
}

void Uniquify8dot3Filename(char* filename, LPBYTE pImage)
{
	FloppyDirectoryEntry* pDir = (FloppyDirectoryEntry*)(pImage + FLOPPY_ROOT_DIR_OFFSET);
	FloppyDirectoryEntry* pDirEnd = pDir + FLOPPY_ROOT_DIR_ENTRIES;

	// Keep going until it's unique
	for (;;)
	{
		FloppyDirectoryEntry* pEntry;
		for (pEntry = pDir; pEntry < pDirEnd; ++pEntry)
		{
			if (pEntry->Filename[0] == '\0')
			{
				// No more used entries (a used but freed entry has 0xE5 in the first byte)
				pDirEnd = pEntry;
				break;
			}
			if (0 == memcmp(filename, pEntry->Filename, sizeof(FloppyDirectoryEntry::Filename)))
			{
				// Found a match
				break;
			}
		}
		if (pEntry >= pDirEnd)
		{
			return; // it's unique
		}

		// Fill in any padding
		for (char* p = filename; p < filename + 8; ++p)
		{
			if (*p == ' ') *p = '-';
		}

		// Increment digit with carry
		for (char* p = filename + 7; p >= filename; --p)
		{
			// Not a number
			if (*p < '0' || *p > '9') {
				*p = '0';
				break; // No carry
			}
			if (*p != '9') {
				*p = (*p) + 1;
				break; // No carry
			}
			*p = '0';
			// Carry to previous digit
		}

		// Loop to try again
	}

}
