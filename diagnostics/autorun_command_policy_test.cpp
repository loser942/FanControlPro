#include "../AutorunCommandPolicy.h"

#include <cassert>

int main()
{
    const std::wstring path = L"C:\\测试目录\\FanControlPro.exe";
    assert(IsExactAutorunCommand(L"\"" + path + L"\"", path));
    const std::wstring separator(1, static_cast<wchar_t>(92));
    const std::wstring legacyPath = L"C:" + separator + L"旧目录" + separator + L"FanControlPro.exe";
    assert(!IsExactAutorunCommand(legacyPath, path));
    assert(!IsExactAutorunCommand(L"\"" + path + L"\" --legacy", path));
    return 0;
}
