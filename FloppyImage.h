#pragma once

const WORD FLOPPY_MAGIC_NUMBER = 0xAA55;
const size_t FLOPPY_BLOCK_SIZE = 512; // 0x0200
const size_t FLOPPY_BLOCKS_PER_AU = 1; // AU = Allocation Unit
const size_t FLOPPY_AU_SIZE = FLOPPY_BLOCKS_PER_AU * FLOPPY_BLOCK_SIZE;
const size_t FLOPPY_BLOCKS_PER_DISK = 2880;
const size_t FLOPPY_IMAGE_SIZE = FLOPPY_BLOCKS_PER_DISK * FLOPPY_BLOCK_SIZE;
const size_t FLOPPY_IMAGE_INTERVAL = 0x180000; // Interval, in bytes, between beginnings of images on the thumb drive
const size_t FLOPPY_FAT0_OFFSET = FLOPPY_BLOCK_SIZE;
const size_t FLOPPY_BLOCKS_PER_FAT = 9;
const size_t FLOPPY_FAT_SIZE = FLOPPY_BLOCK_SIZE * FLOPPY_BLOCKS_PER_FAT;
const size_t FLOPPY_FAT1_OFFSET = FLOPPY_FAT0_OFFSET + FLOPPY_FAT_SIZE;
const size_t FLOPPY_BLOCKS_IN_DIR = 14;
const size_t FLOPPY_FIRST_DATA_AU = 2;
const size_t FLOPPY_DATA_AU_PER_DISK = (FLOPPY_BLOCKS_PER_DISK - (1 + FLOPPY_BLOCKS_PER_FAT*2 + FLOPPY_BLOCKS_IN_DIR)) / FLOPPY_BLOCKS_PER_AU;
const size_t FLOPPY_ROOT_DIR_OFFSET = FLOPPY_FAT0_OFFSET + (FLOPPY_FAT_SIZE * 2);
const size_t FLOPPY_ROOT_DIR_ENTRIES = 224;
const size_t FLOPPY_DATA_OFFSET = FLOPPY_ROOT_DIR_OFFSET + FLOPPY_BLOCKS_IN_DIR * FLOPPY_BLOCK_SIZE;

const size_t BOOTSECTOR_SERIALNUM_OFFSET = 0x27;
const size_t BOOTSECTOR_SERIALNUM_LEN = 0x04;
const size_t BOOTSECTOR_LABEL_OFFSET = 0x2B;
const size_t BOOTSECTOR_LABEL_LEN = 0x0B;

extern BYTE BootSector[512];

#pragma pack( push, 1)
struct FloppyDateTime
{
	unsigned int twoSecond : 5;
	unsigned int minute : 6;
	unsigned int hour : 5;
	unsigned int day : 5;
	unsigned int month : 4;
	unsigned int year : 7;
};

struct FloppyDirectoryEntry
{
	char Filename[11]; // Includes 3-byte extension - dot is implied
	BYTE Attributes;
	BYTE Reserved[10];
	FloppyDateTime DateTime;
	WORD StartCluster;
	DWORD FileSize;
};
#pragma pack( pop )
