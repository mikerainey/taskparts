#include <cstdio>
#include <cstdint>
#include <cmath>
#include <complex>
#include <emmintrin.h>
#include "rollforward.h"

#define D_row 32

void mandelbrot_interrupt_col_loop(double x0, double y0, double x1, double y1,
                                   int width, int height, int max_depth,
                                   unsigned char *output, double xstep,
                                   double ystep, int col_lo, int col_hi);
void mandelbrot_interrupt_row_loop(double x0, double y0, double x1, double y1,
				   int width, int height, int max_depth,
				   unsigned char* output, double xstep, double ystep,
				   int col_lo, int col_hi, int row_lo, int row_hi);
void mandelbrot_interrupt_row_loop(double x0, double y0, double x1, double y1,
                                   int width, int height, int max_depth,
                                   unsigned char *output, double xstep,
                                   double ystep, int col_lo, int col_hi,
                                   int row_lo, int row_hi);

extern void mandelbrot_handle_col_loop(double x0, double y0, double x1,
                                       double y1, int width, int height,
                                       int max_depth, unsigned char *output,
                                       double xstep, double ystep, int col_lo,
                                       int col_hi, bool &promoted);
extern void mandelbrot_handle_row_loop(double x0, double y0, double x1,
                                       double y1, int width, int height,
                                       int max_depth, unsigned char *output,
                                       double xstep, double ystep, int col_lo,
                                       int col_hi, int row_lo, int row_hi,
                                       bool &promoted);
extern
void mandelbrot_handle_row_row_loop(double x0, double y0, double x1, double y1,
                                    int width, int height, int max_depth,
                                    unsigned char *output, double xstep,
                                    double ystep, int col_lo, int col_hi,
                                    int row_lo, int row_hi, bool &promoted);

void rollforward_handler_annotation __rf_handle_col_loop(
  double x0, double y0, double x1, double y1,
  int width, int height, int max_depth,
  unsigned char* output, double xstep, double ystep,
  int col_lo, int col_hi, bool& promoted) {
  mandelbrot_handle_col_loop(x0, y0, x1, y1, width, height, max_depth, output, xstep, ystep, col_lo, col_hi, promoted);
  rollbackward
}

void rollforward_handler_annotation __rf_handle_row_loop(
  double x0, double y0, double x1, double y1,
  int width, int height, int max_depth,
  unsigned char* output, double xstep, double ystep,
  int col_lo, int col_hi, int row_lo, int row_hi, bool& promoted) {
  mandelbrot_handle_row_loop(x0, y0, x1, y1, width, height, max_depth, output, xstep, ystep, col_lo, col_hi, row_lo, row_hi, promoted);
  rollbackward
}

void rollforward_handler_annotation __rf_handle_row_row_loop(
  double x0, double y0, double x1, double y1,
  int width, int height, int max_depth,
  unsigned char* output, double xstep, double ystep,
  int col_lo, int col_hi, int row_lo, int row_hi, bool& promoted) {
  mandelbrot_handle_row_row_loop(x0, y0, x1, y1, width, height, max_depth, output, xstep, ystep, col_lo, col_hi, row_lo, row_hi, promoted);
  rollbackward
}

void mandelbrot_interrupt_col_loop(double x0, double y0, double x1, double y1,
                                  int width, int height, int max_depth,
                                  unsigned char* output, double xstep, double ystep,
                                  int col_lo, int col_hi) {
  if (! (col_lo < col_hi)) {
    return;
  }
  for(; ; ) { // col loop
    int row_lo = 0;
    int row_hi = width;
    if (! (row_lo < row_hi)) {
      break;
    }
    for (; ; ) {
      int row_lo2 = row_lo;
      int row_hi2 = std::min(row_lo + D_row, row_hi);
      for (; row_lo2 < row_hi2; ++row_lo2) { // row loop
        double z_real = x0 + row_lo2*xstep;
        double z_imaginary = y0 + col_lo*ystep;
        double c_real = z_real;
        double c_imaginary = z_imaginary;
        double depth = 0;
        while(depth < max_depth) {
          if(z_real * z_real + z_imaginary * z_imaginary > 4.0) {
            break;
          }
          double temp_real = z_real*z_real - z_imaginary*z_imaginary;
          double temp_imaginary = 2.0*z_real*z_imaginary;
          z_real = c_real + temp_real;
          z_imaginary = c_imaginary + temp_imaginary;
          ++depth;
        }
        output[col_lo*width + row_lo2] = static_cast<unsigned char>(static_cast<double>(depth) / max_depth * 255);
      }
      row_lo = row_lo2;
      if (! (row_lo < row_hi)) {
        break;
      }
      bool promoted = false;
      __rf_handle_row_loop(x0, y0, x1, y1, width, height, max_depth, output, xstep, ystep, col_lo, col_hi, row_lo, row_hi, promoted);
      if (rollforward_branch_unlikely(promoted)) {
	return;
      }
    }
    col_lo++;
    if (! (col_lo < col_hi)) {
      break;
    }
    bool promoted = false;
    __rf_handle_col_loop(x0, y0, x1, y1, width, height, max_depth, output, xstep, ystep, col_lo, col_hi, promoted);
    if (rollforward_branch_unlikely(promoted)) {
      return;
    }
  }
}

void mandelbrot_interrupt_row_loop(double x0, double y0, double x1, double y1,
                                  int width, int height, int max_depth,
                                  unsigned char* output, double xstep, double ystep,
                                  int col_lo, int col_hi, int row_lo, int row_hi) {
  if (! (row_lo < row_hi)) {
    return;
  }
  for (; ; ) {
    int row_lo2 = row_lo;
    int row_hi2 = std::min(row_lo + D_row, row_hi);
    for (; row_lo2 < row_hi2; ++row_lo2) { // row loop
      double z_real = x0 + row_lo2*xstep;
      double z_imaginary = y0 + col_lo*ystep;
      double c_real = z_real;
      double c_imaginary = z_imaginary;
      double depth = 0;
      while(depth < max_depth) {
        if(z_real * z_real + z_imaginary * z_imaginary > 4.0) {
          break;
        }
        double temp_real = z_real*z_real - z_imaginary*z_imaginary;
        double temp_imaginary = 2.0*z_real*z_imaginary;
        z_real = c_real + temp_real;
        z_imaginary = c_imaginary + temp_imaginary;
        ++depth;
      }
      output[col_lo*width + row_lo2] = static_cast<unsigned char>(static_cast<double>(depth) / max_depth * 255);
    }
    row_lo = row_lo2;
    if (! (row_lo < row_hi)) {
      break;
    }
    bool promoted = false;
    __rf_handle_row_row_loop(x0, y0, x1, y1, width, height, max_depth, output, xstep, ystep, col_lo, col_hi, row_lo, row_hi, promoted);
    if (rollforward_branch_unlikely(promoted)) {
      return;
    }
  }
}

