#include <chrono>
#include <complex>
#include <cstddef>
#include <iostream>

#include "gif.h"
#include "shim.h"

/* This version is functionally identical to main.cpp but is slightly less terse.
 * This version also saves the rendered frames to a managed (via unique_ptr) framebuffer,
 * this allows the frames to be reused later for experimentation.
 * */

struct Colour {
  uint8_t r, g, b, _ = 0xFF;
  constexpr Colour(int r, int g, int b) : r(r), g(g), b(b), _(0xFF) {}
  template <typename S> [[nodiscard]] constexpr Colour mix(Colour c, S x) const {
    auto scaleAndClamp = [x](auto f, auto t) { return std::min<int>(0xFF, std::max(S{}, (S(f) - t) * x + t)); };
    return {scaleAndClamp(r, c.r), scaleAndClamp(g, c.g), scaleAndClamp(b, c.b)};
  };
};

template <typename N> N interpolate(N input, N iMin, N iMax, N oMin, N oMax) {
  return ((oMax - oMin) * (input - iMin) / (iMax - iMin)) + oMin;
}

template <typename N> // clang-format off
std::pair<std::complex<N>, int> mandelbrot(std::complex<N> c, int imax, int bailout = 4) {
    std::complex<N> z; int i = 0;
    while (std::abs(z) <= bailout && i < imax) { z = z * z + c; i += 1; }
    return {z, i}; // clang-format on
}

int main() {

  // Parameters used for rendering
  using Num = float;
  const int width = 256;
  const int height = 256;
  const int maxIter = 600;
  const Num poiX = 0.28693186889504513 - 0.0000115;
  const Num poiY = 0.014286693904085048 - 0.000048;
  const int scaleStart = 200;
  const int scaleEnd = 20000;
  const int frames = 300;
  const auto output = "mandelbrot.gif";

  using namespace std::chrono;
  double totalFrameTimeMs = 0;
  std::vector<std::unique_ptr<Colour, decltype(shim::dealloc_raw<Colour>) *>> frameBuffer;
  for (int frame = 0; frame < frames; ++frame) {
    steady_clock::time_point begin = steady_clock::now();
    frameBuffer.emplace_back(shim::alloc_raw<Colour>(width * height), shim::dealloc_raw<Colour>);
    shim::ranged<size_t> r(0, width * height);
    Num recp = Num(1) / interpolate<Num>(Num(frame), 0, frames, scaleStart, scaleEnd);
    std::for_each( //
        shim::exec_policy, r.begin(), r.end(), [=, buffer = frameBuffer.back().get()](auto i) {
          std::array<Colour, 16> Colours{Colour{66, 30, 15}, {25, 7, 26},     {9, 1, 47},      {4, 4, 73},
                                         {0, 7, 100},        {12, 44, 138},   {24, 82, 177},   {57, 125, 209},
                                         {134, 181, 229},    {211, 236, 248}, {241, 233, 191}, {248, 201, 95},
                                         {255, 170, 0},      {204, 128, 0},   {153, 87, 0},    {106, 52, 3}};
          const auto &[t, iter] = mandelbrot( // generic Mandelbrot iterations
              std::complex<Num>{interpolate<Num>(i / width, {}, width, poiX - recp, poiX + recp),
                                interpolate<Num>(i % width, {}, height, poiY - recp, poiY + recp)},
              maxIter);
          if (iter < maxIter) { // Smoothed escape time colouring
            auto logZn = std::log(std::abs(t)) / 2;
            auto nu = std::log(logZn / std::log(2)) / std::log(2);
            auto c1 = Colours[size_t(std::floor(iter - nu)) % Colours.size()];
            auto c2 = Colours[size_t(std::floor(iter + 1 - nu)) % Colours.size()];
            buffer[i] = c2.mix(c1, std::fmod(Num(iter) + 1 - nu, 1));
          } else
            buffer[i] = {0, 0, 0};
        });
    steady_clock::time_point end = steady_clock::now();
    auto frameTimeMs = double(duration_cast<nanoseconds>(end - begin).count()) / 1e6;
    totalFrameTimeMs += frameTimeMs;
    std::cout << "Frame " << frame << " @ " << recp << "x: " << frameTimeMs << " ms\n";
  }
  std::cout << "Rendered " << frames << " frame(s) in " << totalFrameTimeMs << " ms\n";
  std::cout << "Writing to " << output << " , this will take a while..." << std::endl;
  steady_clock::time_point begin = steady_clock::now();
  GifWriter writer = {};
  static_assert(sizeof(uint8_t) * 4 == sizeof(Colour)); // make sure it's RGBA8888
  GifBegin(&writer, output, width, height, 8, 6, true);
  for (int frame = 0; frame < frames; ++frame) {
    GifWriteFrame(&writer, reinterpret_cast<uint8_t *>(frameBuffer[frame].get()), width, height, 8, 6, true);
    std::cout << (float(frame) / frames) * 100 << "%\r" << std::flush;
  }
  GifEnd(&writer);
  steady_clock::time_point end = steady_clock::now();
  std::cout << "Encoded " << frames << " frame(s) in " << (double(duration_cast<nanoseconds>(end - begin).count()) / 1e6) << " ms\n";
  return EXIT_SUCCESS;
}