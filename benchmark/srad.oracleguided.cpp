#include <taskparts/benchmark.hpp>

#include "srad.hpp"

void srad(int rows, int cols, int size_I, int size_R, float* __restrict__ I, float* __restrict__ J, float q0sqr, float * __restrict__ dN, float * __restrict__ dS, float * __restrict__ dW, float * __restrict__ dE, float* __restrict__ c, int* __restrict__ iN, int* __restrict__ iS, int* __restrict__ jE, int* __restrict__ jW, float lambda) {
  taskparts::parallel_for(0, rows, [&] (size_t i) {
    taskparts::parallel_for(0, cols, [&] (size_t j) {
		
      int k = i * cols + j;
      float Jc = J[k];
 
      // directional derivates
      dN[k] = J[iN[i] * cols + j] - Jc;
      dS[k] = J[iS[i] * cols + j] - Jc;
      dW[k] = J[i * cols + jW[j]] - Jc;
      dE[k] = J[i * cols + jE[j]] - Jc;
			
      float G2 = (dN[k]*dN[k] + dS[k]*dS[k] 
		  + dW[k]*dW[k] + dE[k]*dE[k]) / (Jc*Jc);

      float L = (dN[k] + dS[k] + dW[k] + dE[k]) / Jc;

      float num  = (0.5*G2) - ((1.0/16.0)*(L*L)) ;
      float den  = 1 + (.25*L);
      float qsqr = num/(den*den);
 
      // diffusion coefficent (equ 33)
      den = (qsqr-q0sqr) / (q0sqr * (1+q0sqr));
      c[k] = 1.0 / (1.0+den) ;
                
      // saturate diffusion coefficent
      if (c[k] < 0) {c[k] = 0;}
      else if (c[k] > 1) {c[k] = 1;}
   
    }, taskparts::dflt_parallel_for_cost_fn, taskparts::bench_scheduler());
  
  }, [&] (size_t lo, size_t hi) { return cols * (hi - lo); }, taskparts::bench_scheduler());
  taskparts::parallel_for(0, rows, [&] (size_t i) {
    taskparts::parallel_for(0, cols, [&] (size_t j) {

      // current index
      int k = i * cols + j;
                
      // diffusion coefficent
      float cN = c[k];
      float cS = c[iS[i] * cols + j];
      float cW = c[k];
      float cE = c[i * cols + jE[j]];

      // divergence (equ 58)
      float D = cN * dN[k] + cS * dS[k] + cW * dW[k] + cE * dE[k];

      // image update (equ 61)
      J[k] = J[k] + 0.25*lambda*D;
    }, taskparts::dflt_parallel_for_cost_fn, taskparts::bench_scheduler());
  
  }, [&] (size_t lo, size_t hi) { return cols * (hi - lo); },taskparts::bench_scheduler());
}

int main() {
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    srad(rows, cols, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
  }, [&] (auto sched) {
    bench_pre();
  }, [&] (auto sched) {
    print_summary();
    bench_post();
  }, [&] (auto sched) {
    init_J();
  });
  return 0;
}
