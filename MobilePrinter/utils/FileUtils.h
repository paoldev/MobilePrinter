#pragma once

#include <vector>

void CreateDirectoryFromFileName(const wchar_t* File);
void SaveFile(const wchar_t* File, const void* i_buffer, const size_t i_size);
void SaveFile(const wchar_t* File, const std::vector<uint8_t>& i_buffer);
void LoadFile(const wchar_t* File, std::vector<uint8_t>& o_buffer);
