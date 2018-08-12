#include "console.h"
#include "util.h"
#include "assets.h"

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <vector>

using namespace std;

namespace {

	class Line
	{
	private:
		string text_;
		vector<shared_ptr<Glyph const>> glyphs_;
		shared_ptr<IFontAtlas> const font_;

	public:
		Line(shared_ptr<IFontAtlas> const& font) 
			: font_(font) {
		}
		
		void write(string const& text)
		{
			if (text != text_) 
			{
				text_ = text;
				map_glyphs(text);
			}
		}

	private:

		void map_glyphs(string const& text)
		{
			glyphs_.clear();
			glyphs_.reserve(text.size());

			for (auto const& c : text)
			{
				auto glyph = font_->find(c);
				if (!glyph) 
				{
					// todo: handle a replacement character
					assert(0);
					continue;
				}			

				glyphs_.push_back(glyph);
			}
		}
	};


	class Console : public IConsole
	{
	private:
		size_t max_format_cch_ = 1024;
		string format_buff_;
		vector<shared_ptr<Line>> lines_;
		shared_ptr<IFontAtlas> const font_;

	public:
		Console(shared_ptr<IFontAtlas> const& font)
			: max_format_cch_(1024)
			, font_(font) 		
		{
			format_buff_.reserve(max_format_cch_);
		}

		void writeln(int32_t index, std::string const& text) override
		{
			auto const line = alloc_line(index);
			if (!line) {
				return;
			}

			line->write(text);
		}

		void writelnf(int32_t index, const char* msg, ...) override
		{
			// not necessarily safe ... just convenient
			
			va_list args;
			va_start(args, msg);
			auto const ret = vsnprintf(
				&format_buff_[0], max_format_cch_, msg, args);
			assert(ret >= 0 && ret < max_format_cch_);
			
			writeln(index, format_buff_);
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

shared_ptr<IConsole> create_console(shared_ptr<IFontAtlas> const& font)
{
	if (font) {
		return make_shared<Console>(font);
	}
	return nullptr;
}
