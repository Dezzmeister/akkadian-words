#include "common.h"

std::vector<std::wstring> Akk::split_str(std::wstring& str, wchar_t delim) {
	std::vector<std::wstring> out;
	std::wstringstream stream(str);

	for (std::wstring token; std::getline(stream, token, delim);) {
		out.push_back(token);
	}

	return out;
}
