#pragma once

extern bool ImageFileRead(std::wstring filename, LPBYTE pImage);
extern bool ImageFileWrite(std::wstring filename, LPBYTE pImage, bool overwrite);
