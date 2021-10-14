#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <algorithm>

#define unlikely(x)    __builtin_expect(!!(x), 0)

#define DT 64

extern volatile 
int heartbeat;

extern
int srad_handler(int rows, int cols, int rows_lo, int rows_hi, int cols_lo, int cols_hi, int size_I, int size_R, float* __restrict__ I, float* __restrict__ J, float q0sqr, float * __restrict__ dN, float * __restrict__ dS, float * __restrict__ dW, float * __restrict__ dE, float* __restrict__ c, int* __restrict__ iN, int* __restrict__ iS, int* __restrict__ jE, int* __restrict__ jW, float lambda);

extern
int srad_handler_1(int rows, int cols, int rows_lo, int rows_hi, int cols_lo, int cols_hi, int size_I, int size_R, float* __restrict__ I, float* __restrict__ J, float q0sqr, float *__restrict__ dN, float *__restrict__ dS, float *__restrict__ dW, float *__restrict__ dE, float* __restrict__ c, int* __restrict__ iN, int* __restrict__ iS, int* __restrict__ jE, int* __restrict__ jW, float lambda);

extern
int srad_handler_inner_1(int rows, int cols, int rows_lo, int rows_hi, int cols_lo, int cols_hi, int size_I, int size_R, float* __restrict__ I, float* __restrict__ J, float q0sqr, float *__restrict__ dN, float *__restrict__ dS, float *__restrict__ dW, float *__restrict__ dE, float* __restrict__ c, int* __restrict__ iN, int* __restrict__ iS, int* __restrict__ jE, int* __restrict__ jW, float lambda);

extern
int srad_handler_2(int rows, int cols, int rows_lo, int rows_hi, int cols_lo, int cols_hi, int size_I, int size_R, float* __restrict__ I, float* __restrict__ J, float q0sqr, float *__restrict__ dN, float *__restrict__ dS, float *__restrict__ dW, float *__restrict__ dE, float* __restrict__ c, int* __restrict__ iN, int* __restrict__ iS, int* __restrict__ jE, int* __restrict__ jW, float lambda);

extern
int srad_handler_inner_2(int rows, int cols, int rows_lo, int rows_hi, int cols_lo, int cols_hi, int size_I, int size_R, float* __restrict__ I, float* __restrict__ J, float q0sqr, float *__restrict__ dN, float *__restrict__ dS, float *__restrict__ dW, float *__restrict__ dE, float* __restrict__ c, int* __restrict__ iN, int* __restrict__ iS, int* __restrict__ jE, int* __restrict__ jW, float lambda);

void srad_tpal(int rows, int cols, int size_I, int size_R, float* __restrict__ I, float* __restrict__ J, float q0sqr, float * __restrict__ dN, float * __restrict__ dS, float * __restrict__ dW, float * __restrict__ dE, float* __restrict__ c, int* __restrict__ iN, int* __restrict__ iS, int* __restrict__ jE, int* __restrict__ jW, float lambda) {
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
      if (unlikely(heartbeat)) {
        if (srad_handler(rows, cols, i, rows, col_lo, col_hi, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda)) {
          return;
        }
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

void srad_tpal_1(int rows, int cols, int rows_lo, int rows_hi, int size_I, int size_R, float* __restrict__ I, float* __restrict__ J, float q0sqr, float * __restrict__ dN, float * __restrict__ dS, float * __restrict__ dW, float * __restrict__ dE, float* __restrict__ c, int* __restrict__ iN, int* __restrict__ iS, int* __restrict__ jE, int* __restrict__ jW, float lambda) {
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
      if (unlikely(heartbeat)) {
        if (srad_handler_1(rows, cols, i, rows_hi, col_lo, col_hi, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda)) {
          return;
        }
      }
    }
  }
}

void srad_tpal_inner_1(int rows, int cols, int rows_lo, int rows_hi, int cols_lo, int cols_hi, int size_I, int size_R, float* __restrict__ I, float* __restrict__ J, float q0sqr, float * __restrict__ dN, float * __restrict__ dS, float * __restrict__ dW, float * __restrict__ dE, float* __restrict__ c, int* __restrict__ iN, int* __restrict__ iS, int* __restrict__ jE, int* __restrict__ jW, float lambda) {
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
    if (unlikely(heartbeat)) {
      if (srad_handler_inner_1(rows, cols, rows_lo, rows_hi, col_lo, col_hi, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda)) {
	return;
      }
    }
  }
}

void srad_tpal_2(int rows, int cols, int rows_lo, int rows_hi, int cols_lo, int cols_hi, int size_I, int size_R, float* __restrict__ I, float* __restrict__ J, float q0sqr, float * __restrict__ dN, float * __restrict__ dS, float * __restrict__ dW, float * __restrict__ dE, float* __restrict__ c, int* __restrict__ iN, int* __restrict__ iS, int* __restrict__ jE, int* __restrict__ jW, float lambda) {
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
      if (unlikely(heartbeat)) {
	if (srad_handler_2(rows, cols, i, rows_hi, col_lo, col_hi, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda)) {
	  return;
	}
      }
    }
  }
}

void srad_tpal_inner_2(int rows, int cols, int rows_lo, int rows_hi, int cols_lo, int cols_hi, int size_I, int size_R, float* __restrict__ I, float* __restrict__ J, float q0sqr, float * __restrict__ dN, float * __restrict__ dS, float * __restrict__ dW, float * __restrict__ dE, float* __restrict__ c, int* __restrict__ iN, int* __restrict__ iS, int* __restrict__ jE, int* __restrict__ jW, float lambda) {
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
    if (unlikely(heartbeat)) {
      if (srad_handler_inner_2(rows, cols, rows_lo, rows_hi, col_lo, col_hi, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda)) {
	return;
      }
    }
  }
}
