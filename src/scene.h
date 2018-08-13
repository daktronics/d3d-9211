// Copyright (c) 2018 Daktronics. All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#pragma once

#include <string>
#include <memory>

class IAssets;

//
// surfaces (textures) are exchanged between producers and consumers
//
class ISurface
{
public:
	ISurface() {}
	virtual ~ISurface() {}

	virtual uint32_t width() const = 0;
	virtual uint32_t height() const = 0;
	virtual void* share_handle() const = 0;

private:
	ISurface(ISurface const&) = delete;
	ISurface& operator=(ISurface const&) = delete;
};

//
// we're using a queue to exchange work between producers and consumers
//
class ISurfaceQueue
{
public:
	ISurfaceQueue() {}
	virtual ~ISurfaceQueue() {}

	// allocate/fetch a surface for writing (caller = producer)
	virtual std::shared_ptr<ISurface> checkout(uint32_t timeout_ms) = 0;

	// mark a surface as ready for consumption (caller = producer)
	virtual void produce(std::shared_ptr<ISurface> const&) = 0;	
	
	// get next surface to be consumed (caller = consumer)
	virtual std::shared_ptr<ISurface> consume(uint32_t timeout_ms) = 0;

	// surface can be de-allocated, or returned to a pool (caller = consumer)
	virtual void checkin(std::shared_ptr<ISurface> const&) = 0;

private:
	ISurfaceQueue(ISurfaceQueue const&) = delete;
	ISurfaceQueue& operator=(ISurfaceQueue const&) = delete;
};

//
// base class for both Producers and Consumers
//
class IScene
{
public:
	IScene() {}
	virtual ~IScene() {}

	// return a friendly name for the gpu in-use
	virtual std::string gpu() const = 0;

	virtual uint32_t width() const = 0;
	virtual uint32_t height() const = 0;

	virtual void set_background(std::string const&) = 0;

	virtual void tick(double) = 0;
	virtual void render() = 0;
	virtual void present(int32_t) = 0;

	virtual std::shared_ptr<ISurfaceQueue> queue() const = 0;

private:
	IScene(IScene const&) = delete;
	IScene& operator=(IScene const&) = delete;
};


std::shared_ptr<ISurfaceQueue> create_surface_queue();

std::shared_ptr<IScene> create_producer(
	void* native_window, 
	uint32_t width, 
	uint32_t height,
	std::shared_ptr<IAssets> const& assets);

std::shared_ptr<IScene> create_consumer(
	void* native_window,
	uint32_t width,
	uint32_t height,
	std::shared_ptr<IScene> const& producer);