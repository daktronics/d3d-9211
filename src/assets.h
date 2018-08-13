// Copyright (c) 2018 Daktronics. All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

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


//
// represents a code point within a font atlas
//
struct Glyph 
{
	int32_t code;
	float left;
	float top;
	float width;
	float height;
};


class IFontAtlas
{
public:
	IFontAtlas() {}
	virtual ~IFontAtlas() {}

	virtual std::shared_ptr<IImage> image() const = 0;
	virtual std::shared_ptr<Glyph const> find(int32_t code) const = 0;

private:
	IFontAtlas(IFontAtlas const&) = delete;
	IFontAtlas& operator=(IFontAtlas const&) = delete;
};


class IAssets
{
public:
	IAssets() {}
	virtual ~IAssets() {}

	virtual void generate(uint32_t width, uint32_t height) = 0;
	virtual std::shared_ptr<std::string> locate(std::string const&) const = 0;

	virtual std::shared_ptr<IImage> load_image(
		std::shared_ptr<std::string> const& filename) const = 0;

	virtual std::shared_ptr<IFontAtlas const> load_font(
		std::shared_ptr<std::string> const& filename) const = 0;

private:
	IAssets(IAssets const&) = delete;
	IAssets& operator=(IAssets const&) = delete;
};


std::shared_ptr<IAssets> create_assets();
