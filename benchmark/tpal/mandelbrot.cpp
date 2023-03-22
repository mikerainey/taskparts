#ifndef TASKPARTS_TPALRTS
#error "need to compile with tpal flags, e.g., TASKPARTS_TPALRTS"
#endif
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


/* Handler functions */
/* ================= */

void mandelbrot_handle_col_loop(
  double x0, double y0, double x1, double y1,
  int width, int height, int max_depth,
  unsigned char* output, double xstep, double ystep,
  int col_lo, int col_hi, bool& promoted) {
  if ((col_hi - col_lo) <= 1) {
    promoted = false;
  } else {
    auto col_mid = (col_lo + col_hi) / 2;
    tpalrts_promote_via_nativefj([&] {
      mandelbrot_interrupt_col_loop(x0, y0, x1, y1, width, height, max_depth, output, xstep, ystep, col_lo, col_mid);    
    }, [&] {
      mandelbrot_interrupt_col_loop(x0, y0, x1, y1, width, height, max_depth, output, xstep, ystep, col_mid, col_hi);    
    }, [&] { }, taskparts::bench_scheduler());
    promoted = true;
  }
}

void mandelbrot_handle_row_loop(
  double x0, double y0, double x1, double y1,
  int width, int height, int max_depth,
  unsigned char* output, double xstep, double ystep,
  int col_lo, int col_hi, int row_lo, int row_hi, bool& promoted) {
  auto nb_cols = col_hi - col_lo;
  if (nb_cols == 0) {
    promoted = false;
  } else {
    auto rf = [=] {
      mandelbrot_interrupt_row_loop(x0, y0, x1, y1, width, height, max_depth, output, xstep, ystep, col_lo, col_hi, row_lo, row_hi);
    };
    if (nb_cols == 1) {
      rf();
      promoted = true;
      goto exit;
    }
    col_lo++;
    if (nb_cols == 2) {
      tpalrts_promote_via_nativefj([&] {
	rf();
      }, [&] {
	mandelbrot_interrupt_col_loop(x0, y0, x1, y1, width, height, max_depth, output, xstep, ystep, col_lo, col_hi);
      }, [&] {}, taskparts::bench_scheduler());
    } else {
      auto col_mid = (col_lo + col_hi) / 2;
      tpalrts_promote_via_nativefj([&] {
	rf();
	mandelbrot_interrupt_col_loop(x0, y0, x1, y1, width, height, max_depth, output, xstep, ystep, col_lo, col_mid);
      }, [&] {
	mandelbrot_interrupt_col_loop(x0, y0, x1, y1, width, height, max_depth, output, xstep, ystep, col_mid, col_hi);
      }, [&] {}, taskparts::bench_scheduler());
    }
    promoted = true;
  }
 exit:
  return;
}

void mandelbrot_handle_row_row_loop(
  double x0, double y0, double x1, double y1,
  int width, int height, int max_depth,
  unsigned char* output, double xstep, double ystep,
  int col_lo, int col_hi, int row_lo, int row_hi, bool& promoted) {
  if ((row_hi - row_lo) <= 1) {
    promoted = false;
  } else {
    auto row_mid = (row_lo + row_hi) / 2;
    tpalrts_promote_via_nativefj([&] {
      mandelbrot_interrupt_row_loop(x0, y0, x1, y1, width, height, max_depth, output, xstep, ystep, col_lo, col_hi, row_lo, row_mid);
    }, [&] {
      mandelbrot_interrupt_row_loop(x0, y0, x1, y1, width, height, max_depth, output, xstep, ystep, col_lo, col_hi, row_mid, row_hi);
    }, [&] {}, taskparts::bench_scheduler());
    promoted = true;
  }
}

unsigned char* mandelbrot_interrupt(double x0, double y0, double x1, double y1,
                                 int width, int height, int max_depth) {
  double xstep = (x1 - x0) / width;
  double ystep = (y1 - y0) / height;
  //unsigned char* output = static_cast<unsigned char*>(_mm_malloc(width * height * sizeof(unsigned char), 64));
  unsigned char* output = (unsigned char*)malloc(width * height * sizeof(unsigned char));
  mandelbrot_interrupt_col_loop(x0, y0, x1, y1, width, height, max_depth, output, xstep, ystep, 0, height);
  return output;
}

int main() {
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    output = mandelbrot_interrupt(_mb_x0, _mb_y0, _mb_x1, _mb_y1, width, height, max_depth);
  }, [&] (auto sched) {
    for (int i = 0; i < 100; i++) {
      _mb_g /= sin(_mb_g);
    }
  }, [&] (auto sched) {
    _mm_free(output);
  });
  
  return 0;
}
