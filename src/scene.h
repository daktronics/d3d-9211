#pragma once

#include <memory>
#include <string>

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

	//virtual void abort() = 0;

	virtual void push(std::shared_ptr<ISurface> const&) = 0;
	virtual std::shared_ptr<ISurface> pop() = 0;

	// return a surface to the pool for re-use
	// (consumers should call this when done with a surface they popped)
	//
	virtual void checkin(std::shared_ptr<ISurface> const&) = 0;

	// get a free surface to the pool for writing
	// (producers should call this when they want to write to a new surface)
	//
	virtual std::shared_ptr<ISurface> checkout() = 0;
	

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

	virtual std::string gpu() const = 0;

	virtual void tick(double) = 0;
	virtual void render() = 0;
	virtual void present(int32_t) = 0;

	virtual std::shared_ptr<ISurfaceQueue> queue() const = 0;

private:
	IScene(IScene const&) = delete;
	IScene& operator=(IScene const&) = delete;
};


std::shared_ptr<ISurfaceQueue> create_surface_queue();

std::shared_ptr<IScene> create_producer(void* native_window);

std::shared_ptr<IScene> create_consumer(
	void* native_window, std::shared_ptr<IScene> const& producer);