#pragma once

#include <string>
#include <memory>

class IFontAtlas;

class IConsole
{
public:
	IConsole() {}
	virtual ~IConsole() {}

	virtual void writeln(int32_t line, std::string const& text) = 0;
	virtual void writelnf(int32_t line, const char* msg, ...) = 0;

private:
	IConsole(IConsole const&) = delete;
	IConsole& operator=(IConsole const&) = delete;
};

std::shared_ptr<IConsole> create_console(std::shared_ptr<IFontAtlas> const& font);