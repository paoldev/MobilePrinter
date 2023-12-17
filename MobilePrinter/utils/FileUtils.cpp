#include "pch.h"
#include "FileUtils.h"

#include <filesystem>

void CreateDirectoryFromFileName(const wchar_t* File)
{
	std::filesystem::path filenamepath(File);
	filenamepath.remove_filename();
	if (!filenamepath.empty() && !std::filesystem::exists(filenamepath))
	{
		std::filesystem::create_directories(filenamepath);
	}
}

void SaveFile(const wchar_t* File, const void* i_buffer, const size_t i_size)
{
	CreateDirectoryFromFileName(File);

	HANDLE hFile = ::CreateFileW(File, FILE_WRITE_DATA, 0, nullptr, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		const uint8_t* pBuffer = static_cast<const uint8_t*>(i_buffer);
		size_t offset = 0;
		size_t remaining = i_size;
		do
		{
			DWORD dwWritten = 0;
			DWORD dwChunk = static_cast<DWORD>(std::min<size_t>(INT_MAX, remaining));
			if (!WriteFile(hFile, pBuffer + offset, dwChunk, &dwWritten, nullptr) || (dwWritten == 0))
			{
				break;
			}

			remaining -= dwWritten;
			offset += dwWritten;
		} while (remaining > 0);
		CloseHandle(hFile);
	}
}

void SaveFile(const wchar_t* File, const std::vector<uint8_t>& i_buffer)
{
	SaveFile(File, i_buffer.data(), i_buffer.size());
}

void LoadFile(const wchar_t* File, std::vector<uint8_t>& o_buffer)
{
	o_buffer.clear();
	HANDLE hFile = ::CreateFileW(File, FILE_READ_DATA, 0, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER fsize{};
		if (GetFileSizeEx(hFile, &fsize) && ((sizeof(size_t) >= sizeof(LARGE_INTEGER)) || (fsize.HighPart == 0)))
		{
			o_buffer.resize(static_cast<size_t>(fsize.QuadPart));

			size_t offset = 0;
			size_t remaining = o_buffer.size();
			while (remaining > 0)
			{
				DWORD dwRead = 0;
				DWORD dwChunk = static_cast<DWORD>(std::min<size_t>(INT_MAX, remaining));
				if (!ReadFile(hFile, o_buffer.data() + offset, dwChunk, &dwRead, nullptr))
				{
					o_buffer.clear();
					break;
				}

				remaining -= dwRead;
				offset += dwRead;
			}
		}
		CloseHandle(hFile);
	}
}
