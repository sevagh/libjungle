#include "libmetro.h"
#include <iostream>

int main(int argc, char** argv)
{
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " bpm" << std::endl;
		exit(1);
	}

	int bpm = std::stoi(argv[1]);
	auto tempo = metro::Tempo(bpm);

	std::cout << "init " << bpm << "bpm tempo ticker" << std::endl;

	auto audio_engine = metro::audio::Engine();
	auto stream = audio_engine.new_outstream(tempo.period_us);

	std::cout << "init audio engine" << std::endl;
	std::cout << "Generating tones" << std::endl;

	auto hihat = metro::timbre::Drum(42, 100);
	auto snare = metro::timbre::Drum(38, 100);
	auto bass = metro::timbre::Drum(45, 100);

	metro::Measure beat22(std::vector<metro::QuarterNote>({
	    [&]() {
		    stream.play_timbres({&hihat, &snare});
	    },
	    [&]() { stream.play_timbres({&hihat}); },
	    [&]() {
		    stream.play_timbres({&hihat, &bass});
	    },
	    [&]() { stream.play_timbres({&hihat}); },
	}));

	tempo.register_measure(beat22);
	tempo.start();

	audio_engine.eventloop();
}
