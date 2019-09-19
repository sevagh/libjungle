#include "libjungle.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <iostream>
#include <memory>
#include <soundio/soundio.h>
#include <stdio.h>
#include <vector>

jungle::audio::Engine::Engine()
{
	int err;

	soundio = soundio_create();
	if (!soundio)
		throw std::runtime_error("out of memory");

	if ((err = soundio_connect(soundio)))
		throw std::runtime_error(std::string("error connecting: ")
		                         + soundio_strerror(err));

	soundio_flush_events(soundio);

	int default_out_device_index = soundio_default_output_device_index(soundio);
	if (default_out_device_index < 0)
		throw std::runtime_error("no output device found");

	device = soundio_get_output_device(soundio, default_out_device_index);
	if (!device)
		throw std::runtime_error("out of memory");

	std::cout << "Using default output device: " << device->name << std::endl;
}

jungle::audio::Engine::Stream jungle::audio::Engine::new_stream()
{
	auto stream = jungle::audio::Engine::Stream(device);
	return stream;
}

jungle::audio::Engine::~Engine()
{
	soundio_device_unref(device);
	soundio_destroy(soundio);
}
