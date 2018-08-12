#include "platform.h"
#include "util.h"

#include <stdio.h>
#include <stdarg.h>

#include <memory>
#include <sstream>

using namespace std;

LARGE_INTEGER qi_freq_ = {};

uint64_t time_now()
{
	if (!qi_freq_.HighPart && !qi_freq_.LowPart) {
		QueryPerformanceFrequency(&qi_freq_);
	}
	LARGE_INTEGER t = {};
	QueryPerformanceCounter(&t);
	return static_cast<uint64_t>(
		(t.QuadPart / double(qi_freq_.QuadPart)) * 1000000);
}

void log_message(const char* msg, ...)
{
	// old-school, printf style logging
	//
	// not safe by any means - just something simple
	if (msg) 
	{
		const size_t max_cch = 1024;
		char buff[max_cch+1];
		va_list args;
		va_start(args, msg);
		auto const ret = vsnprintf(buff, max_cch, msg, args);
		assert(ret >= 0 && ret < max_cch);
		OutputDebugStringA(buff);
	}
}

string trim(string const& input)
{
	const char* ws = " \t\n\r\f\v";

	string output(input);
	output.erase(output.find_last_not_of(ws) + 1);
	output.erase(0, output.find_first_not_of(ws));
	return output;
}

vector<string> split(string const& input, char sep) 
{
	vector<string> tokens;
	size_t start = 0, end = 0;
	while ((end = input.find(sep, start)) != string::npos) {
		tokens.push_back(input.substr(start, end - start));
		start = end + 1;
	}
	tokens.push_back(input.substr(start));
	return tokens;
}


string to_utf8(wstring const& utf16)
{
	return to_utf8(utf16.c_str());
}

//
// quick and dirty conversion from utf-16 (wide-char) string to 
// utf8 string for Windows
//
string to_utf8(const wchar_t* utf16)
{
	if (!utf16) {
		return string();
	}

	auto const cch = static_cast<int>(wcslen(utf16));
	shared_ptr<char> utf8;
	auto const cb = WideCharToMultiByte(CP_UTF8, 0, utf16, cch,
		nullptr, 0, nullptr, nullptr);
	if (cb > 0)
	{
		utf8 = shared_ptr<char>(reinterpret_cast<char*>(malloc(cb + 1)), free);
		WideCharToMultiByte(CP_UTF8, 0, utf16, cch, utf8.get(), cb, nullptr, nullptr);
		*(utf8.get() + cch) = '\0';
	}
	if (!utf8) {
		return string();
	}
	return string(utf8.get(), cb);
}

std::wstring to_utf16(string const& utf8)
{
	return to_utf16(utf8.c_str());
}

//
// quick and dirty conversion from UTF-8 to wide-char string for Windows
//
wstring to_utf16(const char* utf8)
{
	if (!utf8) {
		return wstring();
	}

	auto const cb = static_cast<int>(strlen(utf8));
	shared_ptr<WCHAR> utf16;
	auto const cch = MultiByteToWideChar(CP_UTF8, 0, utf8, cb, nullptr, 0);
	if (cch > 0)
	{
		utf16 = shared_ptr<WCHAR>(reinterpret_cast<WCHAR*>(
				malloc(sizeof(WCHAR) * (cch + 1))), free);
		MultiByteToWideChar(CP_UTF8, 0, utf8, cb, utf16.get(), cch);
		*(utf16.get() + cch) = L'\0';
	}
	if (!utf16) {
		return wstring();
	}
	return wstring(utf16.get(), cch);
}

int to_int(std::string s, int default_val)
{
	int n;
	istringstream in(s);
	in >> n;
	if (!in.fail()) {
		return n;
	}
	return default_val;
}
