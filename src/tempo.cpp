#include <stdio.h>
#include <chrono>
#include <future>
#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include "libjungle.h"

jungle::tempo::Tempo::Tempo(int bpm) {
  if (!std::chrono::steady_clock::is_steady)
    throw std::runtime_error(
        "std::chrono::steady_clock is unsteady on this platform");

  period_us = 1000000.0 * 60.0 / (double)bpm;

  std::cout << std::fixed;
  std::cout << std::setprecision(2);
}

void jungle::tempo::Tempo::register_func(Func f) { funcs.push_back(f); }

void jungle::tempo::Tempo::start() {
  std::cout << "Starting periodic async executor with bpm: " << bpm
            << " period: " << period_us << " us" << std::endl;

  auto blocking_ticker = [&]() {
    while (true) {
      auto start = std::chrono::steady_clock::now();
      std::this_thread::sleep_for(std::chrono::microseconds(period_us));

      for (auto &f : funcs) auto _ = std::async(std::launch::async, f);

      auto end = std::chrono::steady_clock::now();
      std::chrono::duration<double, std::micro> diff = end - start;
      auto drift_pct =
          ((diff.count() - (double)period_us) / (double)period_us) * 100.0;

      std::cout << "clock drift " << drift_pct << "%\r";
      std::fflush(stdout);
    }
  };

  auto _ = std::async(std::launch::async, blocking_ticker);

  std::cout << "press ctrl-c to exit" << std::endl;
  getchar();
}
