#pragma once

const WORD FLOPPY_MAGIC_NUMBER = 0xAA55;
const size_t FLOPPY_BLOCK_SIZE = 512;
const size_t FLOPPY_BLOCKS_PER_AU = 1; // AU = Allocation Unit
const size_t FLOPPY_AU_SIZE = FLOPPY_BLOCKS_PER_AU * FLOPPY_BLOCK_SIZE;
const size_t FLOPPY_BLOCKS_PER_DISK = 2880;
const size_t FLOPPY_IMAGE_SIZE = FLOPPY_BLOCKS_PER_DISK * FLOPPY_BLOCK_SIZE;
const size_t FLOPPY_IMAGE_INTERVAL = 0x180000; // Interval, in bytes, between beginnings of images on the thumb drive

const size_t BOOTSECTOR_SERIALNUM_OFFSET = 0x27;
const size_t BOOTSECTOR_SERIALNUM_LEN = 0x04;
const size_t BOOTSECTOR_LABEL_OFFSET = 0x2B;
const size_t BOOTSECTOR_LABEL_LEN = 0x0B;

extern BYTE BootSector[512];

