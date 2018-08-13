// Copyright (c) 2018 Daktronics. All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#include "scene.h"
#include "util.h"

#include <list>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>

using namespace std;

namespace {

	class BlockingQueue
	{
	private:
		list<shared_ptr<ISurface>> queue_;
		condition_variable signal_;
		mutex mutable lock_;

	public:

		void push(std::shared_ptr<ISurface> const& surface)
		{
			if (surface)
			{
				lock_guard<mutex> guard(lock_);
				queue_.push_back(surface);
				signal_.notify_all();
			}
		}

		shared_ptr<ISurface> pop(uint32_t timeout_ms)
		{
			for (;;)
			{
				{
					lock_guard<mutex> guard(lock_);
					if (!queue_.empty())
					{
						auto const s = queue_.front();
						queue_.pop_front();
						return s;
					}
				}

				unique_lock<mutex> lock(lock_);
				if (!signal_.wait_for(lock, timeout_ms * 1ms,
					[&]() { return (queue_.size() > 0); })) 
				{
					break;
				}
			}
			return nullptr;
		}
	};

	//
	// surface queue implementation for exchange - not specific
	// to either Direct3D 9 or 11
	//
	class SurfaceQueue : public ISurfaceQueue, 
						public enable_shared_from_this<SurfaceQueue>
	{
	private:
		BlockingQueue due_;
		BlockingQueue pool_;

	public:
		SurfaceQueue() {
		}
		
		void produce(std::shared_ptr<ISurface> const& surface) override {
			due_.push(surface);
		}

		shared_ptr<ISurface> consume(uint32_t timeout_ms) override 
		{
			auto const surf = due_.pop(timeout_ms);
			if (!surf) {
				log_message("timeout waiting for consume\n");
			}
			return surf;
		}

		// return a surface to the pool for re-use
		// (consumers should call this when done with a surface they popped)
		//
		void checkin(std::shared_ptr<ISurface> const& surface) override {
			pool_.push(surface);
		}

		// get a free surface to the pool for writing
		// (producers should call this when they want to render to a new surface)
		//
		shared_ptr<ISurface> checkout(uint32_t timeout_ms) override
		{
			auto const surf = pool_.pop(timeout_ms);
			if (!surf) {
				log_message("timeout waiting for checkout\n");
			}
			return surf;
		}
	};
}

std::shared_ptr<ISurfaceQueue> create_surface_queue()
{
	return make_shared<SurfaceQueue>();
}