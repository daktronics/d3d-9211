#pragma once

#include <string>
#include <memory>

class IAssets
{
public:
	IAssets() {}
	virtual ~IAssets() {}

	virtual void generate(uint32_t width, uint32_t height) = 0;
	virtual std::shared_ptr<std::string> locate(std::string const&) = 0;

private:
	IAssets(IAssets const&) = delete;
	IAssets& operator=(IAssets const&) = delete;
};


std::shared_ptr<IAssets> create_assets();
