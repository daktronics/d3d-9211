#pragma once

#include <stdint.h>
#include <string>
#include <memory>
#include <vector>

#define PI 3.14159265358979323846

uint64_t time_now();

void log_message(const char*, ...);

std::string to_utf8(const wchar_t*);
std::string to_utf8(std::wstring const&);
std::wstring to_utf16(const char*);
std::wstring to_utf16(std::string const&);

std::string trim(std::string const&);

std::vector<std::string> split(std::string const& input, char sep);

int to_int(std::string, int default_val);

// 
// simple method to wrap a raw COM pointer in a shared_ptr
// for auto Release()
//
template<class T>
std::shared_ptr<T> to_com_ptr(T* obj)
{
	return std::shared_ptr<T>(obj, [](T* p) { if (p) p->Release(); });
}
