#include <iostream>
#include <vector>
#include <cstring>
#include <windows.h>

#include "MidiImage.h"
#include "FloppyImage.h"

bool MidiToImage(std::vector<std::wstring> midiPaths, LPBYTE pImage)
{
	memset(pImage, 0, FLOPPY_IMAGE_SIZE);
	memcpy(pImage, BootSector, FLOPPY_BLOCK_SIZE);
	// Write a random serial number at offset 0x27
	// Write the volume label at offset 0x2b (11 bytes, pad with spaces)
	return true;
}
