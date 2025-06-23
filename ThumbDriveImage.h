#pragma once

extern bool ThumbDriveRead(wchar_t driveLetter, int imageNum, LPBYTE pImage);
extern bool ThumbDriveWrite(wchar_t driveLetter, int imageNum, LPBYTE pImage);
