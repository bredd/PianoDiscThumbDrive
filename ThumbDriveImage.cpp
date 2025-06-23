#include <iostream>
#include <windows.h>

#include "ThumbDriveImage.h"
#include "FloppyImage.h"
#include "WinHelp.h"

HANDLE OpenVolumeAndVerify(wchar_t driveLetter);
bool HasFloppyImageHeader(LPBYTE pBuffer);
bool HasFloppyImageHeader(HANDLE hVolume, int imageNum);

bool ThumbDriveRead(wchar_t driveLetter, int imageNum, LPBYTE pImage)
{
	HANDLE hVolume = OpenVolumeAndVerify(driveLetter);
	if (hVolume == INVALID_HANDLE_VALUE)
	{
		// Error has already been reported
		return false;
	}

	bool result = true;

	// Read the image
	LARGE_INTEGER pos;
	pos.QuadPart = FLOPPY_IMAGE_INTERVAL * imageNum;
	if (!SetFilePointerEx(hVolume, pos, NULL, FILE_BEGIN))
	{
		std::wcerr << L"Failed to set read position on volume." << std::endl;
		ReportError(GetLastError());
		result = false;
		goto finally;
	}

	DWORD bytesRead;
	if (!ReadFile(hVolume, pImage, FLOPPY_IMAGE_SIZE, &bytesRead, NULL))
	{
		std::wcerr << L"Failed to read full image from volume." << std::endl;
		ReportError(GetLastError());
		result = false;
		goto finally;
	}

	if (bytesRead != FLOPPY_IMAGE_SIZE)
	{
		std::wcerr << L"Failed to read full floppy image from thumb drive." << std::endl;
		result = false;
		goto finally;
	}

	// Check whether this is a 1.44 MB floppy boot sector
	if (!HasFloppyImageHeader(pImage))
	{
		std::wcerr << L"Invalid header on floppy image. Drive " << driveLetter << L": image " << imageNum << std::endl;
		result = false;
		goto finally;
	}

finally:
	CloseHandle(hVolume);
	return result;
}

bool ThumbDriveWrite(wchar_t driveLetter, int imageNum, LPBYTE pImage)
{
	// Make sure this is a valid image being written
	if (!HasFloppyImageHeader(pImage))
	{
		std::wcerr << L"Source image is not a valid floppy image." << std::endl;
		return false;
	}

	HANDLE hVolume = OpenVolumeAndVerify(driveLetter);
	if (hVolume == INVALID_HANDLE_VALUE)
	{
		// Error has already been reported
		return false;
	}

	bool result = true;

	// If destination image is zero, lock the volume and force dismount
	if (imageNum == 0)
	{
		// Lock the volumne
		DWORD bytesReturned;
		if (!DeviceIoControl(hVolume, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL))
		{
			std::wcerr << L"Failed to lock volume." << std::endl;
			ReportError(GetLastError());
			result = false;
			goto finally;
		}

		// Force-Dismount the volume
		if (!DeviceIoControl(hVolume, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL))
		{
			std::wcerr << L"Failed to dismount volume." << std::endl;
			ReportError(GetLastError());
			result = false;
			goto finally;
		}
	}

	// Check that there's a valid floppy image at the destination
	// Image 0 has already been checked
	if (imageNum != 0 && !HasFloppyImageHeader(hVolume, imageNum))
	{
		std::wcerr << L"Destination image number (" << imageNum << ") is not a valid floppy image." << std::endl;
		result = false;
		goto finally;
	}

	// Write the image
	LARGE_INTEGER pos;
	pos.QuadPart = FLOPPY_IMAGE_INTERVAL * imageNum;
	if (!SetFilePointerEx(hVolume, pos, NULL, FILE_BEGIN))
	{
		std::wcerr << L"Failed to set position on volume." << std::endl;
		ReportError(GetLastError());
		result = false;
		goto finally;
	}

	DWORD bytesWritten;
	if (!WriteFile(hVolume, pImage, FLOPPY_IMAGE_SIZE, &bytesWritten, NULL))
	{
		std::wcerr << L"Failed to write image." << std::endl;
		ReportError(GetLastError());
		result = false;
		goto finally;
	}

	if (bytesWritten != FLOPPY_IMAGE_SIZE)
	{
		std::wcerr << L"Failed to write full floppy image to thumb drive." << std::endl;
		result = false;
		goto finally;
	}

finally:
	// If destination is image 0, unlock the volume
	if (imageNum == 0)
	{
		DWORD bytesReturned;
		if (!DeviceIoControl(hVolume, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL))
		{
			std::wcerr << L"Failed to unlock volume." << std::endl;
			ReportError(GetLastError());
		}
	}

	return result;
}

bool HasFloppyImageHeader(LPBYTE pBuffer)
{
	// Magic number at the end of the sector
	if (FLOPPY_MAGIC_NUMBER != *(WORD*)(pBuffer + 0x01FE)) return false;

	// Number of bytes per block
	if (FLOPPY_BLOCK_SIZE != *(WORD*)(pBuffer + 0x0b)) return false;

	// One block per allocation unit
	if (FLOPPY_BLOCKS_PER_AU != *(pBuffer + 0x0D)) return false;

	// 2880 blocks on the disk (1.4MB floppy)
	if (FLOPPY_BLOCKS_PER_DISK != *(WORD*)(pBuffer + 0x13)) return false;

	return true;
}

bool HasFloppyImageHeader(HANDLE hVolume, int imageNum)
{
	// Allocate a page-aligned buffer for reading and writing
	LPBYTE pBuffer = (LPBYTE)VirtualAlloc(NULL, FLOPPY_BLOCK_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (pBuffer == NULL)
	{
		std::wcerr << L"Failed to allocate buffer." << std::endl;
		return false;
	}

	LARGE_INTEGER pos;
	pos.QuadPart = FLOPPY_IMAGE_INTERVAL * imageNum;
	if (!SetFilePointerEx(hVolume, pos, NULL, FILE_BEGIN))
	{
		ReportError(GetLastError());
		VirtualFree(pBuffer, 0, MEM_RELEASE);
		return false;
	}

	DWORD bytesRead;
	if (!ReadFile(hVolume, pBuffer, FLOPPY_BLOCK_SIZE, &bytesRead, NULL))
	{
		ReportError(GetLastError());
		VirtualFree(pBuffer, 0, MEM_RELEASE);
		return false;
	}

	if (bytesRead != FLOPPY_BLOCK_SIZE)
	{
		std::wcerr << L"Failed to read full first block from floppy image." << std::endl;
		VirtualFree(pBuffer, 0, MEM_RELEASE);
		return false;
	}

	// Check whether this is a 1.44 MB floppy boot sector
	bool result = HasFloppyImageHeader(pBuffer);

	VirtualFree(pBuffer, 0, MEM_RELEASE);
	return result;
}

HANDLE OpenVolumeAndVerify(wchar_t driveLetter)
{
	// Open the designated volume
	WCHAR volname[] = L"\\\\.\\X:";
	volname[4] = driveLetter;
	HANDLE hVolume = CreateFile(volname, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
	if (hVolume == INVALID_HANDLE_VALUE)
	{
		std::wcerr << L"Failed to open source drive: " << driveLetter << L":" << std::endl;
		ReportError(GetLastError());
		return INVALID_HANDLE_VALUE;
	}

	// Allow reading and writing beyond the first virtual floppy
	{
		DWORD bytesReturned;
		if (!DeviceIoControl(hVolume, FSCTL_ALLOW_EXTENDED_DASD_IO, NULL, 0, NULL, 0, &bytesReturned, NULL))
		{
			std::wcerr << L"Failed to set extended access on drive: " << driveLetter << L":" << std::endl;
			ReportError(GetLastError());
			CloseHandle(hVolume);
			return INVALID_HANDLE_VALUE;
		}
	}

	if (!HasFloppyImageHeader(hVolume, 0))
	{
		std::wcerr << L"Drive: " << driveLetter << L": is not a set of floppy images." << std::endl;
		CloseHandle(hVolume);
		return INVALID_HANDLE_VALUE;
	}

	return hVolume;
}
