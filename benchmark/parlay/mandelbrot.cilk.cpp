#include "opencilk.hpp"
#include <taskparts/benchmark.hpp>
#include <cstdint>
#include <cmath>
#include <complex>
#include <emmintrin.h>

unsigned char* output = nullptr;
double _mb_x0 = -2.5;
double _mb_y0 = -0.875;
double _mb_x1 = 1;
double _mb_y1 = 0.875;
int height = 8 * 524 * 2;
// Width should be a multiple of 8
int width = height;
int max_depth = 100;
double _mb_g = 2.0;

unsigned char* mandelbrot_cilk(double x0, double y0, double x1, double y1,
                               int width, int height, int max_depth) {
  double xstep = (x1 - x0) / width;
  double ystep = (y1 - y0) / height;
  unsigned char* output = (unsigned char*)malloc(width * height * sizeof(unsigned char));
  //unsigned char* output = static_cast<unsigned char*>(_mm_malloc(width * height * sizeof(unsigned char), 64));
  // Traverse the sample space in equally spaced steps with width * height samples
  cilk_for(int j = 0; j < height; ++j) {
    cilk_for (int i = 0; i < width; ++i) {
      double z_real = x0 + i*xstep;
      double z_imaginary = y0 + j*ystep;
      double c_real = z_real;
      double c_imaginary = z_imaginary;

      // depth should be an int, but the vectorizer will not vectorize, complaining about mixed data types
      // switching it to double is worth the small cost in performance to let the vectorizer work
      double depth = 0;
      // Figures out how many recurrences are required before divergence, up to max_depth
      while(depth < max_depth) {
        if(z_real * z_real + z_imaginary * z_imaginary > 4.0) {
          break; // Escape from a circle of radius 2
        }
        double temp_real = z_real*z_real - z_imaginary*z_imaginary;
        double temp_imaginary = 2.0*z_real*z_imaginary;
        z_real = c_real + temp_real;
        z_imaginary = c_imaginary + temp_imaginary;

        ++depth;
      }
      output[j*width + i] = static_cast<unsigned char>(static_cast<double>(depth) / max_depth * 255);
    }
  }
  return output;
}

int main() {
  max_depth = taskparts::cmdline::parse_or_default_int("max_depth", max_depth);
  width = taskparts::cmdline::parse_or_default_int("width", width);
  height = taskparts::cmdline::parse_or_default_int("height", height);
  taskparts::benchmark_cilk([&] {
    output = mandelbrot_cilk(_mb_x0, _mb_y0, _mb_x1, _mb_y1, width, height, max_depth);
  }, [&] {
    for (int i = 0; i < 100; i++) {
      _mb_g /= sin(_mb_g);
    }
  }, [&] {
    _mm_free(output);
  });
  
  return 0;
}
