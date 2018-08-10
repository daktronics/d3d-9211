#pragma once

#include <string>
#include <memory>

class IImage
{
public:
	IImage() {}
	virtual ~IImage() {}

	virtual uint32_t width() const = 0;
	virtual uint32_t height() const = 0;
	virtual void* lock(uint32_t& stride) = 0;
	virtual void unlock() = 0;

private:
	IImage(IImage const&) = delete;
	IImage& operator=(IImage const&) = delete;
};


class IAssets
{
public:
	IAssets() {}
	virtual ~IAssets() {}

	virtual void generate(uint32_t width, uint32_t height) = 0;
	virtual std::shared_ptr<std::string> locate(std::string const&) = 0;

	virtual std::shared_ptr<IImage> load_image(
		std::shared_ptr<std::string> const& filename) = 0;

private:
	IAssets(IAssets const&) = delete;
	IAssets& operator=(IAssets const&) = delete;
};



std::shared_ptr<IAssets> create_assets();
