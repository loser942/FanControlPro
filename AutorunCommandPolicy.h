#pragma once

#include <string>

inline bool IsExactAutorunCommand(const std::wstring& stored, const std::wstring& exePath)
{
    return stored == L"\"" + exePath + L"\"";
}
