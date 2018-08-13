// Copyright (c) 2018 Daktronics. All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#pragma once

#include <stdint.h>
#include <string>
#include <memory>
#include <vector>

#define PI 3.14159265358979323846

struct color {

	color() : r(0.0f), g(0.0f), b(0.0f), a(0.0f) {
	}
	
	color(float red, float green, float blue, float alpha)
		: r(red), g(green), b(blue), a(alpha) {
	}

	float r;
	float g;
	float b;
	float a;
};

uint64_t time_now();

void log_message(const char*, ...);

std::string to_utf8(const wchar_t*);
std::string to_utf8(std::wstring const&);
std::wstring to_utf16(const char*);
std::wstring to_utf16(std::string const&);

std::string trim(std::string const&);

std::string to_timecode(double);

std::vector<std::string> split(std::string const& input, char sep);

int to_int(std::string, int default_val);

color parse_color(std::string const&);

// 
// simple method to wrap a raw COM pointer in a shared_ptr
// for auto Release()
//
template<class T>
std::shared_ptr<T> to_com_ptr(T* obj)
{
	return std::shared_ptr<T>(obj, [](T* p) { if (p) p->Release(); });
}
