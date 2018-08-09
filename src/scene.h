#pragma once

#include <memory>
#include <string>

class IScene
{
public:
	IScene() {}
	virtual ~IScene() {}

	virtual std::string gpu() const = 0;

	virtual void tick(double) = 0;
	virtual void render() = 0;
	virtual void present(int32_t) = 0;

private:
	IScene(IScene const&) = delete;
	IScene& operator=(IScene const&) = delete;
};

class Producer : public IScene
{


};

class Consumer : public IScene
{

};


std::shared_ptr<Producer> create_producer(void* native_window);

std::shared_ptr<Consumer> create_consumer(
	void* native_window, std::shared_ptr<Producer> const& producer);