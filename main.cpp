#include <complex>
#include <cstddef>

#include "gif.h"
#include "shim.h"
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
  using Num = float;
  const int maxIter = 600;
  const int width = 256, height = 256;
  const Num poiX = 0.28693186889504513 - 0.0000115, poiY = 0.014286693904085048 - 0.000048;
  static_assert(sizeof(uint8_t) * 4 == sizeof(Colour)); // make sure it's RGBA8888
  GifWriter writer = {};
  GifBegin(&writer, "mandelbrot.gif", width, height, 8, 6, true);
  std::unique_ptr<Colour, decltype(shim::dealloc_raw<Colour>) *> //
      buffer(shim::alloc_raw<Colour>(width * height), shim::dealloc_raw<Colour>);
  for (int frame = 0; frame < 300; ++frame) {
    Num recp = Num(1) / interpolate<Num>(Num(frame), 0, 300, 200, 20000);
    shim::ranged<size_t> r(0, width * height);
    std::for_each(shim::par_unseq, r.begin(), r.end(), [=, buffer = buffer.get()](auto i) {
      std::array<Colour, 16> Colours{Colour{66, 30, 15}, {25, 7, 26},     {9, 1, 47},      {4, 4, 73},     //
                                     {0, 7, 100},        {12, 44, 138},   {24, 82, 177},   {57, 125, 209}, //
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
    GifWriteFrame(&writer, reinterpret_cast<uint8_t *>(buffer.get()), width, height, 8, 6, true);
  }
  GifEnd(&writer);
  return EXIT_SUCCESS;
}