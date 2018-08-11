#pragma once

#include <string>
#include <memory>

class IFontAtlas;

class IConsole
{
public:
	IConsole() {}
	virtual ~IConsole() {}
	
private:
	IConsole(IConsole const&) = delete;
	IConsole& operator=(IConsole const&) = delete;
};

std::shared_ptr<IConsole> create_console(std::shared_ptr<IFontAtlas> const&);