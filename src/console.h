// Copyright (c) 2018 Daktronics. All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#pragma once

#include <string>
#include <memory>
#include <vector>

class IFontAtlas;
struct Glyph;

class IConsole
{
public:
	IConsole() {}
	virtual ~IConsole() {}

	virtual std::shared_ptr<IFontAtlas const> font() const = 0;

	virtual void writeln(int32_t line, std::string const& text) = 0;
	virtual void writelnf(int32_t line, const char* msg, ...) = 0;

	virtual std::vector<std::shared_ptr<Glyph const>> get_line(int32_t) const = 0;

	virtual int32_t line_count() const = 0;
	virtual int32_t column_count() const = 0;

private:
	IConsole(IConsole const&) = delete;
	IConsole& operator=(IConsole const&) = delete;
};

std::shared_ptr<IConsole> create_console(std::shared_ptr<IFontAtlas const> const& font);