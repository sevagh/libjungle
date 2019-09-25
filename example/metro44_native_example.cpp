#include "libjungle.h"
#include <iostream>

int main(int argc, char** argv)
{
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " bpm" << std::endl;
		exit(1);
	}

	int bpm = std::stoi(argv[1]);
	auto tempo = jungle::tempo::Tempo(bpm);

	std::cout << "init " << bpm << "bpm tempo ticker" << std::endl;

	auto audio_engine = jungle::audio::Engine();
	auto stream = audio_engine.new_stream();

	std::cout << "init audio engine" << std::endl;
	std::cout << "tempo.period_us: " << tempo.period_us << std::endl;
	std::cout << "tempo.period_us/2.0: " << tempo.period_us / 2.0 << std::endl;

	auto metronome_event_cycle
	    = jungle::metronome::metronome_common_time(stream);

	tempo.register_event_cycle(metronome_event_cycle);
	tempo.start();

	audio_engine.eventloop();
}
