#include "libjungle/libjungle.h"
#include "libjungle/libjungle_synthesis.h"
#include <cassert>
#include <cfloat>
#include <chrono>
#include <cstring>
#include <mutex>
#include <soundio/soundio.h>
#include <stk/Drummer.h>
#include <stk/SineWave.h>
#include <stk/Stk.h>
#include <vector>

static std::once_flag stk_init_flag;

static void stk_init()
{
	std::call_once(stk_init_flag, []() {
		stk::Stk::showWarnings(true);
		stk::Stk::setSampleRate(jungle::core::SampleRateHz);
	});
}

jungle::synthesis::timbre::Sine::Sine(float pitch_hz, float volume_pct)
    : frames(std::vector<float>(2 * jungle::core::SampleRateHz))
{
	stk_init();

	stk::SineWave sine;
	sine.setFrequency(pitch_hz);

	for (size_t i = 0; i < frames.size(); ++i)
		frames[i] = sine.tick();

	// normalize to 1.0 * volume_pct since libsoundio expects floats between
	// -1.0 and 1.0
	stk::StkFloat max_elem = -DBL_MAX;
	for (size_t i = 0; i < frames.size(); ++i)
		max_elem = std::max(frames[i], max_elem);

	for (size_t i = 0; i < frames.size(); ++i)
		frames[i] = (volume_pct / 100.0) * (1.0 / max_elem) * frames[i];
}

// reversing the freq2midi magic in the stk Drummer code
static float midi2freq(int midi)
{
	return 440.0 * pow(2.0, (float(midi) - 69.0) / 12.0);
}

jungle::synthesis::timbre::Drum::Drum(int midi_drum_instrument,
                                      float volume_pct)
    : frames(std::vector<float>(2 * jungle::core::SampleRateHz))
{
	stk_init();

	stk::Drummer drummer;
	drummer.noteOn(midi2freq(midi_drum_instrument), volume_pct / 100.0);
	for (size_t i = 0; i < frames.size(); ++i)
		frames[i] = drummer.tick();

	drummer.noteOff(0.0);
}

void jungle::synthesis::timbre::play_on_stream(
    jungle::core::audio::Engine::OutStream& stream,
    std::list<jungle::synthesis::timbre::Timbre*> timbres)
{
	std::vector<float> frames(2 * jungle::core::SampleRateHz);

	// sum input timbres in case of simultaneous sounds
	for (auto timbre : timbres) {
		auto timbre_frames = timbre->get_frames();
		assert(timbre_frames.size() == 2 * jungle::core::SampleRateHz);

		for (size_t i = 0; i < frames.size(); ++i) {
			frames[i] += timbre_frames[i];
		}
	}

	assert(frames.size() == 2 * jungle::core::SampleRateHz);

	// fill the stream.ringbuffer with 2*48,000 samples, which should finish in
	// outstream->software_latency
	// http://libsound.io/doc-1.1.0/structSoundIoOutStream.html#a20aac1422d3cc64b679616bb8447f06d
	char* buf = soundio_ring_buffer_write_ptr(stream.ringbuf);
	size_t fill_count = stream.outstream->software_latency
	                    * stream.outstream->sample_rate
	                    * stream.outstream->bytes_per_frame;
	fill_count = std::min(fill_count, frames.size() * sizeof(float));

	// in case there's stuff in the ringbuffer, we don't want to overflow
	fill_count -= soundio_ring_buffer_fill_count(stream.ringbuf);

	if (stream.is_muted())
		std::memset(buf, 0, fill_count);
	else
		std::memcpy(buf, frames.data(), fill_count);

	soundio_ring_buffer_advance_write_ptr(stream.ringbuf, fill_count);

	// wait for how long a timbre should be
	jungle::core::tempo::precise_sleep_us(
	    std::chrono::duration_cast<std::chrono::microseconds>(
	        std::chrono::duration<float, std::ratio<1, 1>>(stream.latency_s)));

	// then, stuff it with 0s to create a smooth transition to
	// silence
	fill_count = soundio_ring_buffer_capacity(stream.ringbuf)
	             - soundio_ring_buffer_fill_count(stream.ringbuf);
	std::memset(buf, 0, fill_count);
	soundio_ring_buffer_advance_write_ptr(stream.ringbuf, fill_count);
}
