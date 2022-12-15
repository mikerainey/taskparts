#ifndef TASKPARTS_TPALRTS
#error "need to compile with tpal flags, e.g., TASKPARTS_TPALRTS"
#endif
#include <taskparts/benchmark.hpp>
#include <math.h>

/*
  128 //number of rows in the domain
  128 //number of cols in the domain
  0	//y1 position of the speckle
  31	//y2 position of the speckle
  0	//x1 position of the speckle
  31	//x2 position of the speckle
  4	//number of threads
  0.5	//Lambda value
  2	//number of iterations
*/

int rows=4000;
int cols=4000, size_I, size_R;
float *_srad_I, *_srad_J, q0sqr, sum, sum2, tmp, meanROI,varROI ;
int *iN,*iS,*jE,*jW;
float *dN,*dS,*dW,*dE;
int r1, r2, c1, c2;
float *_srad_c;
float lambda;

void srad_interrupt(int rows, int cols, int size_I, int size_R, float* I, float* J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float* c, int* iN, int* iS, int* jE, int* jW, float lambda);
void srad_interrupt_1(int rows, int cols, int rows_lo, int rows_hi, int size_I, int size_R, float* I, float* J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float* c, int* iN, int* iS, int* jE, int* jW, float lambda);
void srad_interrupt_2(int rows, int cols, int rows_lo, int rows_hi, int cols_lo, int cols_hi, int size_I, int size_R, float* I, float* J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float* c, int* iN, int* iS, int* jE, int* jW, float lambda);
void srad_interrupt_inner_1(int rows, int cols, int rows_lo, int rows_hi, int cols_lo, int cols_hi, int size_I, int size_R, float* I, float* J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float* c, int* iN, int* iS, int* jE, int* jW, float lambda);
void srad_interrupt_inner_2(int rows, int cols, int rows_lo, int rows_hi, int cols_lo, int cols_hi, int size_I, int size_R, float* I, float* J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float* c, int* iN, int* iS, int* jE, int* jW, float lambda);

/* Handler functions */
/* ================= */

void rollforward_handler_annotation __rf_handle_srad_0(int rows, int cols, int rows_lo, int rows_hi, int cols_lo, int cols_hi, int size_I, int size_R, float* I, float* J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float* c, int* iN, int* iS, int* jE, int* jW, float lambda, bool& promoted) {
  auto nb_rows = rows_hi - rows_lo;
  if (nb_rows <= 1) {
    promoted = false;
  } else {
    auto rf = [=] {
      srad_interrupt_inner_1(rows, cols, rows_lo, rows_hi, cols_lo, cols_hi, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
    };
    rows_lo++;
    if (nb_rows == 2) {
      tpalrts_promote_via_nativefj(rf, [=] {
	srad_interrupt_1(rows, cols, rows_lo, rows_hi, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
      }, [=] {
	srad_interrupt_2(rows, cols, rows_lo, rows_hi, 0, 0, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
      }, taskparts::bench_scheduler());
    } else {
      auto rows_mid = (rows_lo + rows_hi) / 2;
      tpalrts_promote_via_nativefj([=] {
	rf();
	srad_interrupt_1(rows, cols, rows_lo, rows_mid, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
      }, [=] {
	srad_interrupt_1(rows, cols, rows_mid, rows_hi, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
      }, [=] {
	srad_interrupt_2(rows, cols, 0, rows, 0, 0, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
      }, taskparts::bench_scheduler());
    }
    promoted = true;
  }
  rollbackward
}

void rollforward_handler_annotation __rf_handle_srad_1(int rows, int cols, int rows_lo, int rows_hi, int cols_lo, int cols_hi, int size_I, int size_R, float* I, float* J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float* c, int* iN, int* iS, int* jE, int* jW, float lambda, bool& promoted) {
  auto nb_rows = rows_hi - rows_lo;
  if (nb_rows <= 1) {
    promoted = false;
  } else {
    auto rf = [=] {
      srad_interrupt_inner_1(rows, cols, rows_lo, rows_hi, cols_lo, cols_hi, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
    };
    rows_lo++;
    if (nb_rows == 2) {
      tpalrts_promote_via_nativefj(
        rf, [=] {
	srad_interrupt_1(rows, cols, rows_lo, rows_hi, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
      }, [&] {}, taskparts::bench_scheduler());
    } else {
      auto rows_mid = (rows_lo + rows_hi) / 2;
      tpalrts_promote_via_nativefj([=] {
	rf();
	srad_interrupt_1(rows, cols, rows_lo, rows_mid, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
      }, [=] {
	srad_interrupt_1(rows, cols, rows_mid, rows_hi, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
      }, [&] {}, taskparts::bench_scheduler());
    }
    promoted = true;
  }
  rollbackward
}

void rollforward_handler_annotation __rf_handle_srad_inner_1(int rows, int cols, int rows_lo, int rows_hi, int cols_lo, int cols_hi, int size_I, int size_R, float* I, float* J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float* c, int* iN, int* iS, int* jE, int* jW, float lambda, bool& promoted) {
  if ((cols_hi - cols_lo) <= 1) {
    promoted = false;
  } else {
    auto cols_mid = (cols_lo + cols_hi) / 2;
    tpalrts_promote_via_nativefj([=] {
      srad_interrupt_inner_1(rows, cols, rows_lo, rows_hi, cols_lo, cols_mid, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
    }, [=] {
      srad_interrupt_inner_1(rows, cols, rows_lo, rows_hi, cols_mid, cols_hi, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
    }, [&] {}, taskparts::bench_scheduler());
    promoted = true;
  }
  rollbackward
}

void rollforward_handler_annotation __rf_handle_srad_2(int rows, int cols, int rows_lo, int rows_hi, int cols_lo, int cols_hi, int size_I, int size_R, float* I, float* J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float* c, int* iN, int* iS, int* jE, int* jW, float lambda, bool& promoted) {
  auto nb_rows = rows_hi - rows_lo;
  if (nb_rows <= 1) {
    promoted = false;
  } else {
    auto rf = [=] {
      srad_interrupt_inner_2(rows, cols, rows_lo, rows_hi, cols_lo, cols_hi, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
    };
    rows_lo++;
    if (nb_rows == 2) {
      tpalrts_promote_via_nativefj(
        rf,
       [=] {
	srad_interrupt_2(rows, cols, rows_lo, rows_hi, 0, 0, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
      }, [&] {}, taskparts::bench_scheduler());
    } else {
      auto rows_mid = (rows_lo + rows_hi) / 2;
      tpalrts_promote_via_nativefj([=] {
        rf();
	srad_interrupt_2(rows, cols, rows_lo, rows_mid, 0, 0, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
      }, [=] {
	srad_interrupt_2(rows, cols, rows_mid, rows_hi, 0, 0, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
      }, [&] {}, taskparts::bench_scheduler());
    }
    promoted = true;
  }
  rollbackward
}

void rollforward_handler_annotation __rf_handle_srad_inner_2(int rows, int cols, int rows_lo, int rows_hi, int cols_lo, int cols_hi, int size_I, int size_R, float* I, float* J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float* c, int* iN, int* iS, int* jE, int* jW, float lambda, bool& promoted) {
  if ((cols_hi - cols_lo) <= 1) {
    promoted = false;
  } else {
    auto cols_mid = (cols_lo + cols_hi) / 2;
    tpalrts_promote_via_nativefj([=] {
      srad_interrupt_inner_2(rows, cols, rows_lo, rows_hi, cols_lo, cols_mid, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
    }, [=] {
      srad_interrupt_inner_2(rows, cols, rows_lo, rows_hi, cols_mid, cols_hi, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
    }, [&] {}, taskparts::bench_scheduler());
    promoted = true;
  }
  rollbackward
}

/* Outlined-loop functions */
/* ======================= */

#define DT 64

void srad_interrupt(int rows, int cols, int size_I, int size_R, float* I, float* J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float* c, int* iN, int* iS, int* jE, int* jW, float lambda) {
  for (int i = 0 ; i < rows ; i++) {
    int col_lo = 0;
    int col_hi = cols;
    if (! (col_lo < col_hi)) {
      continue;
    }
    for (;;) {
      int col_lo2 = col_lo;
      int col_hi2 = std::min(col_lo + DT, col_hi);
      for (; col_lo2 < col_hi2; col_lo2++) {
	int j = col_lo2;
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
	den = (qsqr-q0sqr) / (q0sqr * (1+q0sqr)) ;
	c[k] = 1.0 / (1.0+den) ;

	// saturate diffusion coefficent
	if (c[k] < 0) {c[k] = 0;}
	else if (c[k] > 1) {c[k] = 1;}
      }
      col_lo = col_lo2;
      if (! (col_lo < col_hi)) {
        break;
      }
      bool promoted = false;
      __rf_handle_srad_0(rows, cols, i, rows, col_lo, col_hi, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda, promoted);
      if (rollforward_branch_unlikely(promoted)) {
	return;
      }

    }
  }
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {        

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
    }
  }
}

void srad_interrupt_1(int rows, int cols, int rows_lo, int rows_hi, int size_I, int size_R, float* I, float* J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float* c, int* iN, int* iS, int* jE, int* jW, float lambda) {
  for (int i = rows_lo ; i < rows_hi ; i++) {
    int col_lo = 0;
    int col_hi = cols;
    if (! (col_lo < col_hi)) {
      continue;
    }
    for (;;) {
      int col_lo2 = col_lo;
      int col_hi2 = std::min(col_lo + DT, col_hi);
      for (; col_lo2 < col_hi2; col_lo2++) {
	int j = col_lo2;
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
	den = (qsqr-q0sqr) / (q0sqr * (1+q0sqr)) ;
	c[k] = 1.0 / (1.0+den) ;

	// saturate diffusion coefficent
	if (c[k] < 0) {c[k] = 0;}
	else if (c[k] > 1) {c[k] = 1;}
      }
      col_lo = col_lo2;
      if (! (col_lo < col_hi)) {
        break;
      }
      bool promoted = false;
      __rf_handle_srad_1(rows, cols, i, rows_hi, col_lo, col_hi, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda, promoted);
      if (rollforward_branch_unlikely(promoted)) {
	return;
      }
    }
  }
}

void srad_interrupt_inner_1(int rows, int cols, int rows_lo, int rows_hi, int cols_lo, int cols_hi, int size_I, int size_R, float* I, float* J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float* c, int* iN, int* iS, int* jE, int* jW, float lambda) {
  int i = rows_lo;
  int col_lo = cols_lo;
  int col_hi = cols_hi;
  if (! (col_lo < col_hi)) {
    return;
  }
  for (;;) {
    int col_lo2 = col_lo;
    int col_hi2 = std::min(col_lo + DT, col_hi);
    for (; col_lo2 < col_hi2; col_lo2++) {
      int j = col_lo2;
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
      den = (qsqr-q0sqr) / (q0sqr * (1+q0sqr)) ;
      c[k] = 1.0 / (1.0+den) ;

      // saturate diffusion coefficent
      if (c[k] < 0) {c[k] = 0;}
      else if (c[k] > 1) {c[k] = 1;}
    }
    col_lo = col_lo2;
    if (! (col_lo < col_hi)) {
      break;
    }
    bool promoted = false;
    __rf_handle_srad_inner_1(rows, cols, rows_lo, rows_hi, col_lo, col_hi, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda, promoted);
    if (rollforward_branch_unlikely(promoted)) {
      return;
    }
  }
}

void srad_interrupt_2(int rows, int cols, int rows_lo, int rows_hi, int cols_lo, int cols_hi, int size_I, int size_R, float* I, float* J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float* c, int* iN, int* iS, int* jE, int* jW, float lambda) {
  for (int i = rows_lo; i < rows_hi; i++) {
    int col_lo = 0;
    int col_hi = cols;
    if (! (col_lo < col_hi)) {
      continue;
    }
    for (;;) {
      int col_lo2 = col_lo;
      int col_hi2 = std::min(col_lo + DT, col_hi);
      for (; col_lo2 < col_hi2; col_lo2++) {
	int j = col_lo2;

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
      }
      col_lo = col_lo2;
      if (! (col_lo < col_hi)) {
	break;
      }
      bool promoted = false;
      __rf_handle_srad_2(rows, cols, i, rows_hi, col_lo, col_hi, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda, promoted);
      if (rollforward_branch_unlikely(promoted)) {
	return;
      }
    }
  }
}

void srad_interrupt_inner_2(int rows, int cols, int rows_lo, int rows_hi, int cols_lo, int cols_hi, int size_I, int size_R, float* I, float* J, float q0sqr, float *dN, float *dS, float *dW, float *dE, float* c, int* iN, int* iS, int* jE, int* jW, float lambda) {
  int i = rows_lo;
  int col_lo = cols_lo;
  int col_hi = cols_hi;
  if (! (col_lo < col_hi)) {
    return;
  }
  for (;;) {
    int col_lo2 = col_lo;
    int col_hi2 = std::min(col_lo + DT, col_hi);
    for (; col_lo2 < col_hi2; col_lo2++) {
      int j = col_lo2;

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
    }
    col_lo = col_lo2;
    if (! (col_lo < col_hi)) {
      break;
    }
    bool promoted = false;
    __rf_handle_srad_inner_2(rows, cols, rows_lo, rows_hi, col_lo, col_hi, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda, promoted);
    if (rollforward_branch_unlikely(promoted)) {
      return;
    }
  }
}

int64_t hash64(uint64_t u) {
  uint64_t v = u * 3935559000370003845ul + 2691343689449507681ul;
  v ^= v >> 21;
  v ^= v << 37;
  v ^= v >>  4;
  v *= 4768777513237032717ul;
  v ^= v << 20;
  v ^= v >> 41;
  v ^= v <<  5;
  return v;
}

void random_matrix(float *I, int rows, int cols){
  for( int i = 0 ; i < rows ; i++){
    for ( int j = 0 ; j < cols ; j++){
      I[i * cols + j] = (float)hash64(i+j)/(float)RAND_MAX ;
    }
  }
}

int main() {
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    srad_interrupt(rows, cols, size_I, size_R, _srad_I, _srad_J, q0sqr, dN, dS, dW, dE, _srad_c, iN, iS, jE, jW, lambda);
  }, [&] (auto sched) {
    r1   = 0;
    r2   = 31;
    c1   = 0;
    c2   = 31;
    lambda = 0.5;

    size_I = cols * rows;
    size_R = (r2-r1+1)*(c2-c1+1);   

    _srad_I = (float *)malloc( size_I * sizeof(float) );
    _srad_J = (float *)malloc( size_I * sizeof(float) );
    _srad_c  = (float *)malloc(sizeof(float)* size_I) ;

    iN = (int *)malloc(sizeof(unsigned int*) * rows) ;
    iS = (int *)malloc(sizeof(unsigned int*) * rows) ;
    jW = (int *)malloc(sizeof(unsigned int*) * cols) ;
    jE = (int *)malloc(sizeof(unsigned int*) * cols) ;    


    dN = (float *)malloc(sizeof(float)* size_I) ;
    dS = (float *)malloc(sizeof(float)* size_I) ;
    dW = (float *)malloc(sizeof(float)* size_I) ;
    dE = (float *)malloc(sizeof(float)* size_I) ;    


    for (int i=0; i< rows; i++) {
      iN[i] = i-1;
      iS[i] = i+1;
    }    
    for (int j=0; j< cols; j++) {
      jW[j] = j-1;
      jE[j] = j+1;
    }
    iN[0]    = 0;
    iS[rows-1] = rows-1;
    jW[0]    = 0;
    jE[cols-1] = cols-1;

    random_matrix(_srad_I, rows, cols);

    for (int k = 0;  k < size_I; k++ ) {
      _srad_J[k] = (float)exp((double)_srad_I[k]) ;
    }

    sum=0; sum2=0;     
    for (int i=r1; i<=r2; i++) {
      for (int j=c1; j<=c2; j++) {
	tmp   = _srad_J[i * cols + j];
	sum  += tmp ;
	sum2 += tmp*tmp;
      }
    }
    meanROI = sum / size_R;
    varROI  = (sum2 / size_R) - meanROI*meanROI;
    q0sqr   = varROI / (meanROI*meanROI);

  }, [&] (auto sched) {
    free(_srad_I);
    free(_srad_J);
    free(iN); free(iS); free(jW); free(jE);
    free(dN); free(dS); free(dW); free(dE);

    free(_srad_c);
  });
  
  return 0;
}
