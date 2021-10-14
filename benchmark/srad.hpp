#pragma once

#include <stdint.h>
#include <cmath>
#include <taskparts/hash.hpp>

auto random_matrix(float *I, int rows, int cols) {
  for( int i = 0 ; i < rows ; i++){
    for ( int j = 0 ; j < cols ; j++){
      float b = ((float)RAND_MAX) *1000000000.0;
      I[i * cols + j] = (float)taskparts::hash(i+j)/b;
    }
  }
}

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
float *I, *J, q0sqr, sum, sum2, tmp, meanROI,varROI ;
int *iN,*iS,*jE,*jW;
float *dN,*dS,*dW,*dE;
int r1, r2, c1, c2;
float *c, D;
float lambda;

auto init_J() {
  for (int k = 0;  k < size_I; k++ ) {
    J[k] = (float)exp((double)I[k]) ;
  }
}

auto bench_pre() {
  r1   = 0;
  r2   = 31;
  c1   = 0;
  c2   = 31;
  lambda = 0.5;

  size_I = cols * rows;
  size_R = (r2-r1+1)*(c2-c1+1);   

  I = (float *)malloc( size_I * sizeof(float) );
  J = (float *)malloc( size_I * sizeof(float) );
  c  = (float *)malloc(sizeof(float)* size_I) ;

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
	
  random_matrix(I, rows, cols);

  init_J();
  
  sum=0; sum2=0;     
  for (int i=r1; i<=r2; i++) {
    for (int j=c1; j<=c2; j++) {
      tmp   = J[i * cols + j];
      sum  += tmp ;
      sum2 += tmp*tmp;
    }
  }
  meanROI = sum / size_R;
  varROI  = (sum2 / size_R) - meanROI*meanROI;
  q0sqr   = varROI / (meanROI*meanROI);
}

auto bench_post() {
  free(I);
  free(J);
  free(iN); free(iS); free(jW); free(jE);
  free(dN); free(dS); free(dW); free(dE);

  free(c);
};

auto print_summary() {
  float r = 0.0;
  for (int i = 0; i < size_I; i++) {
    r += J[i];
  }
  printf("result %f\n", r);
}
