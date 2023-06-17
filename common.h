/**
 * Misc. common constants and functions.
 * 
 * Author: Joe Desmond - desmonji@bc.edu
 */
#pragma once

#include <sstream>
#include <string>
#include <vector>

#define MAX_ANSWER_CHARS 64

namespace Akk {
	std::vector<std::wstring> split_str(std::wstring& str, wchar_t delim);
}
