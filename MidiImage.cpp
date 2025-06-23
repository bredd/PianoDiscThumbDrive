#include <iostream>
#include <vector>
#include <cstring>
#include <windows.h>

#include "MidiImage.h"
#include "FloppyImage.h"

const char DiskLabel[] = "Piano       "; // Pad to at least 11 bytes;

bool MidiToImage(std::vector<std::wstring> midiPaths, LPBYTE pImage)
{
	// === Format the Disk (Image) ===
	memset(pImage, 0, FLOPPY_IMAGE_SIZE);
	memcpy(pImage, BootSector, FLOPPY_BLOCK_SIZE);

	// Random "serial number"
	{
		#pragma warning( push )
		#pragma warning ( disable: 28159)
		DWORD ticks = GetTickCount();
        #pragma warning( pop )
		memcpy(pImage + BOOTSECTOR_SERIALNUM_OFFSET, &ticks, BOOTSECTOR_SERIALNUM_LEN);
	}

	// Label
	memcpy(pImage + BOOTSECTOR_LABEL_OFFSET, DiskLabel, BOOTSECTOR_LABEL_LEN);

	// Init the two FATs

	// Init the directory

	return true;
}
