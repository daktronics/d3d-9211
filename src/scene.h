#pragma once

#include <memory>
#include <string>

class Scene
{
public:
	Scene() {}
	virtual ~Scene() {}

	virtual std::string gpu() const = 0;

	virtual void tick(double) = 0;
	virtual void render() = 0;
	virtual void present() = 0;

private:
	Scene(Scene const&) = delete;
	Scene& operator=(Scene const&) = delete;
};

class Producer : public Scene
{


};

class Consumer : public Scene
{

};


std::shared_ptr<Producer> create_producer(void* native_window);