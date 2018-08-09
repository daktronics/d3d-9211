#include "scene.h"

#include <list>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>

using namespace std;

namespace {

	//
	// surface queue implementation for exchange - not specific
	// to either Direct3D 9 or 11
	//
	class SurfaceQueue : public ISurfaceQueue
	{
	private:

		list<shared_ptr<ISurface>> pool_;
		list<shared_ptr<ISurface>> due_;
		condition_variable signal_;
		mutex mutable lock_;

	public:
		SurfaceQueue()
		{
		}

		void push(std::shared_ptr<ISurface> const& surface) override
		{
			if (surface)
			{
				lock_guard<mutex> guard(lock_);
				due_.push_back(surface);
				signal_.notify_all();
			}
		}

		shared_ptr<ISurface> pop() override
		{
			for (;;)
			{
				{
					lock_guard<mutex> guard(lock_);
					if (!due_.empty()) 
					{
						auto const s = due_.front();
						due_.pop_front();
						return s;
					}
				}

				unique_lock<mutex> lock(lock_);
				if (!signal_.wait_for(lock, 100ms, [&]() { return (due_.size() > 0);})) {
					break;
				}
			}

			return nullptr;
		}

		// return a surface to the pool for re-use
		// (consumers should call this when done with a surface they popped)
		//
		void checkin(std::shared_ptr<ISurface> const& surface) override
		{
			if (surface)
			{
				lock_guard<mutex> guard(lock_);
				pool_.push_back(surface);
				signal_.notify_all();
			}
		}

		// get a free surface to the pool for writing
		// (producers should call this when they want to write to a new surface)
		//
		shared_ptr<ISurface> checkout() override
		{
			for (;;)
			{
				{
					lock_guard<mutex> guard(lock_);
					if (!pool_.empty())
					{
						auto const s = pool_.front();
						pool_.pop_front();
						return s;
					}
				}

				unique_lock<mutex> lock(lock_);
				if (!signal_.wait_for(lock, 100ms, [&]() { return (pool_.size() > 0); })) {
					break;
				}
			}

			return nullptr;
		}
	};
}

std::shared_ptr<ISurfaceQueue> create_surface_queue()
{
	return make_shared<SurfaceQueue>();
}