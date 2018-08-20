// Copyright (c) 2018 Daktronics. All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#include "console.h"
#include "util.h"
#include "assets.h"

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <vector>
#include <algorithm>

using namespace std;

namespace {

	class Line
	{
	private:
		string text_;
		vector<shared_ptr<Glyph const>> glyphs_;
		shared_ptr<IFontAtlas const> const font_;

	public:
		Line(shared_ptr<IFontAtlas const> const& font) 
			: font_(font) {
		}
		
		void write(string const& text)
		{
			if (text != text_) {
				text_ = text;
				map_glyphs(text);
			}
		}

		int32_t length() const {
			return int32_t(glyphs_.size());
		}

		vector<shared_ptr<Glyph const>> glyphs() const {
			return glyphs_;
		}

	private:

		void map_glyphs(string const& text)
		{
			auto const utf16 = to_utf16(text);

			glyphs_.clear();
			glyphs_.reserve(utf16.size());
			for (auto const& c : utf16)
			{
				auto glyph = font_->find(c);
				if (glyph) {
					glyphs_.push_back(glyph);
				}
				else 
				{
					// todo: handle a replacement character
					assert(0);
					continue;
				}
			}
		}
	};


	class Console : public IConsole
	{
	private:
		size_t max_format_cch_ = 1024;
		shared_ptr<char> const format_buff_;
		vector<shared_ptr<Line>> lines_;
		shared_ptr<IFontAtlas const> const font_;

	public:
		Console(shared_ptr<IFontAtlas const> const& font)
			: max_format_cch_(1024)
			, format_buff_((char*)malloc(max_format_cch_ + 1), free)
			, font_(font) 		
		{
		}

		shared_ptr<IFontAtlas const> font() const override {
			return font_;
		}

		void writeln(int32_t index, std::string const& text) override
		{
			// ensure we have a line to write to
			auto const line = alloc_line(index);
			if (line) {
				line->write(text);
			}
		}

		void writelnf(int32_t index, const char* msg, ...) override
		{
			// not necessarily safe ... just convenient
			
			va_list args;
			va_start(args, msg);
			auto const ret = vsnprintf(
				format_buff_.get(), max_format_cch_, msg, args);

			if (ret >= 0 && ret < int32_t(max_format_cch_)) {
				writeln(index, string(format_buff_.get(), ret));
			}
			else {
				assert(0);
			}
		}

		vector<shared_ptr<Glyph const>> get_line(int32_t n) const override
		{
			if (n >= 0 && n < int32_t(lines_.size())) {
				return lines_[n]->glyphs();
			}
			return vector<shared_ptr<Glyph const>>();
		}

		int32_t line_count() const override {
			return int32_t(lines_.size());
		}

		int32_t column_count() const override {
			int32_t len = 0;
			for (auto const& l : lines_) {
				len = std::max(len, l->length());
			}
			return len;
		}

	private:

		shared_ptr<Line> alloc_line(int32_t line)
		{
			if (line < 0) {
				return nullptr;
			}			
			while (line >= int32_t(lines_.size())) {
				lines_.push_back(make_shared<Line>(font_));
			}
			return lines_[line];
		}

	};
}

shared_ptr<IConsole> create_console(shared_ptr<IFontAtlas const> const& font)
{
	if (font) {
		return make_shared<Console>(font);
	}
	return nullptr;
}
