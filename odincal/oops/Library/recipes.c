#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "recipes.h"

#define M1 259200
#define IA1 7141
#define IC1 54773
#define RM1 (1.0/M1)
#define M2 134456
#define IA2 8121
#define IC2 28411
#define RM2 (1.0/M2)
#define M3 243000
#define IA3 4561
#define IC3 51349

static int idum = -1;

float ran1(int *idum)
{
  static long ix1,ix2,ix3;
  static float r[97];
  float temp;
  static int iff=0;
  int j;

  if (*idum < 0 || iff == 0) {
    iff=1;
    ix1=(IC1-(*idum)) % M1;
    ix1=(IA1*ix1+IC1) % M1;
    ix2=ix1 % M2;
    ix1=(IA1*ix1+IC1) % M1;
    ix3=ix1 % M3;
    for (j=0; j<97; j++) {
      ix1=(IA1*ix1+IC1) % M1;
      ix2=(IA2*ix2+IC2) % M2;
      r[j]=(ix1+ix2*RM2)*RM1;
    }
    *idum=1;
  }
  ix1=(IA1*ix1+IC1) % M1;
  ix2=(IA2*ix2+IC2) % M2;
  ix3=(IA3*ix3+IC3) % M3;
  j=(97*ix3)/M3;
  if (j >= 97 || j < 0) {
    fprintf(stderr, "fatal error in 'ran1'\n");
    exit(1);
  }
  temp=r[j];
  r[j]=(ix1+ix2*RM2)*RM1;
  return temp;
}

float gasdev(int *idum)
{
  static int iset=0;
  static float gset;
  float fac,r,v1,v2;
  float ran1();

  if  (iset == 0) {
    do {
      v1=2.0*ran1(idum)-1.0;
      v2=2.0*ran1(idum)-1.0;
      r=v1*v1+v2*v2;
    } while (r >= 1.0);
    fac=sqrt(-2.0*log(r)/r);
    gset=v1*fac;
    iset=1;
    return (v2*fac);
  } else {
    iset=0;
    return (gset);
  }
}

int *ivector(int n)
{
  int *i;

  i = (int *)calloc(n, sizeof(int));
  if (i == (int *)NULL) {
    fprintf(stderr, "failed to allocate index of length %d\n", n);
  }
  return i;
}

double *dvector(int n)
{
  double *v;

  v = (double *)calloc(n, sizeof(double));
  if (v == (double *)NULL) {
    fprintf(stderr, "failed to allocate vector of length %d\n", n);
  }
  return v;
}

double **dmatrix(int m, int n)
{
  double **y;
  int i, j;

  y = (double **)calloc(m, sizeof(double *));
  if (y == (double **)NULL) {
    fprintf(stderr, "failed to allocate matrix of size %d by %d\n", m, n);
  }
  if (y != (double **)NULL) {
    for (i = 0; i < m; i++) {
      y[i] = (double *)calloc(n, sizeof(double));
      if (y[i] == (double *)NULL) {
	fprintf(stderr, "failed to allocate matrix of size %d by %d\n", m, n);
	for (j = 0; j < i; j++) free(y[j]);
	free(y);
	y = (double **)NULL;
	break;
      }
    }
  }
  return y;
}

void four1(float data[], int nn, int sign)
/* 
   FOUR1 replaces data by its discrete Fourier transform, 
   if isigen is input as 1; or replaces data by nn times its
   inverse discrete Fourier transform, if sign is input as -1.
   Data is a complex array of length nn or equivalently, a real 
   array of length 2*nn. nn must be an integer power of 2.  
*/ 
{ 
  register float wr, wi, wpr, wpi;
  float wtemp, theta, tempr, tempi;
  register int i, j;
  int n, m, max, step; 
  
  n = 2*nn;
  j = 0; 
  for (i=0; i<n; i+=2) { 
    if (j>i) {
      tempr = data[j]; 
      tempi = data[j+1]; 
      data[j] = data[i]; 
      data[j+1] = data[i+1]; 
      data[i] = tempr; 
      data[i+1] = tempi; 
    } 
    m = n/2;
    while (m>=2 && j>=m) {
      j -= m;
      m /= 2;
    } 
    j += m; 
  }
  
  max = 2; 
  while (n>max) {
    step = 2*max; 
    theta = 2.0*PI/(sign*max);
    wpr = -2.0*sin(0.5*theta)*sin(0.5*theta); 
    wpi = sin(theta); 
    wr = 1.0; 
    wi = 0.0; 
    for (m=0; m<max; m+=2) {
      for (i=m; i<n; i+=step) {
	j = i+max;
	tempr = wr*data[j]-wi*data[j+1];
	tempi = wr*data[j+1]+wi*data[j];
	data[j] = data[i]-tempr;
	data[j+1] = data[i+1]-tempi;
	data[i] = data[i]+tempr;
	data[i+1] = data[i+1]+tempi;
      }
      wtemp = wr;
      wr += wr*wpr-wi*wpi; 
      wi += wi*wpr+wtemp*wpi;
    } 
    max = step; 
  }
} 
  
void twofft(float data1[], float data2[], float fft1[], float fft2[], int n)
/* 
   Given two real input arrays data1 and data2, each of length n, this routine
   calls four1 and returns two complex output arrays, fft1 and fft2, each of
   complex length n (i.e. real length 2*n), which contain the discrete Fourier
   transforms of the respective data's. n must be an integer power of 2.
*/
{
  int nn2, j, jj;
  float rep, rem, aip, aim;

  for (j = 0; j < n; j++) {
    jj = 2*j;
    fft1[jj] = data1[j];
    fft1[jj+1] = data2[j];
  }
  four1(fft1, n, 1);
  fft2[0] = fft1[1];
  fft1[1] = fft2[1] = 0.0;
  nn2 = 2*n;
  for (j = 1; j <= n/2; j++) {
    jj = 2*j;
    rep = 0.5*(fft1[jj]+fft1[nn2-jj]);
    rem = 0.5*(fft1[jj]-fft1[nn2-jj]);
    aip = 0.5*(fft1[jj+1]+fft1[nn2-jj+1]);
    aim = 0.5*(fft1[jj+1]-fft1[nn2-jj+1]);
    fft1[jj] = rep;
    fft1[jj+1] = aim;
    fft1[nn2-jj] = rep;
    fft1[nn2-jj+1] = -aim;
    fft2[jj] = aip;
    fft2[jj+1] = -rem;
    fft2[nn2-jj] = aip;
    fft2[nn2-jj+1] = rem;
  }
}

void realft(float data[], int n, int sign)
/* 
   Calculates the Fourier transform of a set of 2n real-valued
   data points. Replaces this data (array data) by the positive 
   frequency half of its complex Fourier transform. The real-valued 
   first and last components of the complex transform are returned
   as elements data[0] and data[1] respectively. n must be a power
   of 2. This routine also calculates the inverse transform of a
   complex data array if it is the transform of real data. The
   result in this case must be multiplied by 1/n.
*/
{ 
  float theta, c1, c2, wr, wi, wpr, wpi, h1r, h1i, h2r, h2i, wtemp;
  int i, i1, i2, i3, i4; 
  
  theta = PI/n; 
  c1 = 0.5;
  if (sign == 1) { 
    c2 = -0.5;
    four1 (data,n,sign);
  } else { 
    c2 = 0.5; 
    theta = -theta; 
  }
  
  wpr = -2.0*sin(0.5*theta)*sin(0.5*theta);
  wpi = sin(theta);
  wr = 1.0+wpr;
  wi = wpi;
  for (i=1; i<n/2+1; i++) {
    i1 = i+i; i2 = i1+1; i3 = n+n-i1; i4 = i3+1;
    h1r =  c1*(data[i1]+data[i3]);
    h1i =  c1*(data[i2]-data[i4]);
    h2r = -c2*(data[i2]+data[i4]);
    h2i =  c2*(data[i1]-data[i3]);
    data[i1] =  h1r+wr*h2r-wi*h2i;
    data[i2] =  h1i+wr*h2i+wi*h2r;
    data[i3] =  h1r-wr*h2r+wi*h2i;
    data[i4] = -h1i+wr*h2i+wi*h2r;
    wtemp = wr; 
    wr += wr*wpr-wi*wpi;
    wi += wi*wpr+wtemp*wpi; 
  }
  
  if (sign == -1) {
    h1r = data[0];
    data[0] = c1*(h1r+data[1]); 
    data[1] = c1*(h1r-data[1]); 
    four1 (data,n,sign);
  } else { 
    h1r = data[0];
    data[0] = h1r+data[1];
    data[1] = h1r-data[1];
  }
} 

void cosft(float y[], int n, int sign)
/*
   Calculates the cosine transform of a set y of n real-valued data points.
   The transformed data replace the original data in array y. n must be a
   power of 2. Set sign to +1 for a transform, and to -1 for an inverse 
   transform. For an inverse transform, the output array should be multiplied
   by 2/n.
*/
{
  float wr, wi, wpr, wpi, wtemp, theta;
  float y1, y2, sum, enfo, sumo, sume, even, odd;
  int m, j;

  theta = PI/n;
  wr = 1.0; wi = 0.0;
  wpr = -2.0*pow(sin(0.5*theta),2.0);  wpi = sin(theta);
  sum = y[0];
  m = n/2;
  for (j=1; j<m; j++) {
    wtemp = wr;
    wr += wr*wpr-wi*wpi;  wi += wi*wpr+wtemp*wpi;
    y1 = 0.5*(y[j]+y[n-j]);  y2 = (y[j]-y[n-j]);
    y[j] = y1-wi*y2;  y[n-j] = y1+wi*y2;
    sum += wr*y2;
  }
  realft (y,m,1);
  y[1] = sum;
  for (j=3; j<n; j+=2) {
    sum += y[j];
    y[j] = sum;
  }
  if (sign == -1) {
    even = y[0];  odd = y[1];
    for (j=2; j<n; j+=2) {
      even += y[j];  odd += y[j+1];
    }
    enfo = 2.0*(even-odd);
    sumo = y[0]-enfo;  sume = 2.0*odd/n-sumo;
    y[0] = 0.5*enfo;  y[1] -= sume;
    for (j=2; j<n; j+=2) {
      y[j] -= sumo;  y[j+1] -= sume;
    }
  }
}

void sinft(float y[], int n)
/* 
   Calculates the sine transform of a set y of n real-valued data points.
   The transformed data replace the original data in array y. n must be a
   power of 2. For an inverse transform, the output array should be multiplied
   by 2/n.
*/
{
  float wr, wi, wpr, wpi, wtemp, theta;
  float y1, y2, sum;
  int m, j;

  theta = PI/n;
  wr = 1.0; wi = 0.0;
  wpr = -2.0*pow(sin(0.5*theta),2.0);  wpi = sin(theta);
  y[0] = 0.0;
  m = n/2;
  for (j=1; j<m; j++) {
    wtemp = wr;
    wr += wr*wpr-wi*wpi;  wi += wi*wpr+wtemp*wpi;
    y1 = wi*(y[j]+y[n-j]);  y2 = 0.5*(y[j]-y[n-j]);
    y[j] = y1+y2;  y[n-j] = y1-y2;
  }
  realft (y,m,1);
  sum = 0.0;
  y[0] = 0.5*y[1];
  y[1] = 0.0;
  for (j=0; j<n; j+=2) {
    sum += y[j];
    y[j] = y[j+1];
    y[j+1] = sum;
  }
}

int ludcmp(double **a, int n, int indx[], double *d) 
{
  double aamax, aa, *vv, sum, dum;
  int i, j, k, imax;

  vv = dvector(n);
  if (vv == (double *)NULL) return (0);

  *d = 1.0;
  for (i=0; i<n; i++) {
    aamax = 0.0;
    for (j=0; j<n; j++) {
      if ((aa=fabs((double)a[i][j])) > aamax) aamax = aa;
    }
    if (aamax == 0.0) {
      free(vv);
      return (0);
    }
    vv[i] = 1.0/aamax;
  }
  for (j=0; j<n; j++) {
    for (i=0; i<j; i++) {
      sum = a[i][j];
      for (k=0; k<i; k++) sum -= a[i][k]*a[k][j];
      a[i][j] = sum;
    }
    aamax = 0.0;
    for (i=j; i<n; i++) {
      sum = a[i][j];
      for (k=0; k<j; k++) sum -= a[i][k]*a[k][j];
      a[i][j] = sum;
      dum = vv[i]*fabs(sum);
      if (dum >= aamax) {
	imax = i;
	aamax = dum;
      }
    }
    if (j != imax) {
      for (k=0; k<n; k++) {
	dum = a[imax][k];
	a[imax][k] = a[j][k];
	a[j][k] = dum;
      }
      *d *= -1.0;
      vv[imax] = vv[j];
    }
    indx[j] = imax;
    if (j < n-1) {
      if (a[j][j] == 0.0) a[j][j] = TINY;
      dum = 1.0/a[j][j];
      for (i=j+1; i<n; i++) a[i][j] *= dum;
    }
  }
  if (a[n-1][n-1] == 0.0) a[n-1][n-1] = TINY;
  free(vv);

  return (1);
}

void lubksb(double *a[], int n, int indx[], double b[]) 
{
  int i, j, ii, ll;
  double sum;

  ii = -1;
  for (i=0; i<n; i++) {
    ll = indx[i];
    sum = b[ll];
    b[ll] = b[i];
    if (ii > -1) {
      for (j=ii; j<i; j++) sum -= a[i][j]*b[j];
    } else if (sum != 0.0) ii = i;
    b[i] = sum;
  }
  for (i=n-1; i>=0; i--) {
    sum = b[i];
    for (j=i+1; j<n; j++) sum -= a[i][j]*b[j];
    b[i] = sum/a[i][i];
  }
}

double matinv(double **a, int n) 
{
  double **y, Det;
  int i, j, *indx;

  indx = ivector(n);
  if (indx == (int *)NULL) return(0.0);

  y = dmatrix(n, n);
  if (y == (double **)NULL) return(0.0);

  if (!ludcmp(a, n, indx, &Det)) return (0.0);
  for (i = 0; i < n; i++) {
    Det *= a[i][i];
    for (j = 0; j < n; j++) y[i][j] = 0.0;
    y[i][i] = 1.0;
    lubksb(a, n, indx, y[i]);
  }
  for (i = 0; i < n; i++) {
    for (j = 0; j < n; j++) a[i][j] = y[i][j];
  }

  return (Det);
}

int solve(double **a, double *v, int n) 
{
  int *indx;
  double Det;

  indx = ivector(n);
  if (indx == (int *)NULL) return(0);

  if (!ludcmp(a, n, indx, &Det)) return (0);
  else {
    lubksb(a, n, indx, v);
    return (1);
  }
}

void savgol(double c[], int np, int nl, int nr, int ld, int m)
{
  int i, imj, ipj, j, k, kk, mm, *index;
  double d, fac, sum, **A, *b;

  A = dmatrix(m+1, m+1);
  if (A == (double **)NULL) return;

  b = dvector(m+1);
  if (b == (double *)NULL) return;

  index = ivector(m+1);
  if (index == (int *)NULL) return;

  for (ipj = 0; ipj <= 2*m; ipj++) {
    sum = 0.0;
    if (ipj == 0) sum = 1.0;
    for (k = 0; k < nl; k++) sum += pow((double)( k+1),(double)ipj);
    for (k = 0; k < nr; k++) sum += pow((double)(-k-1),(double)ipj);
    mm = min(ipj, 2*m-ipj);
    for (imj = -mm; imj <= mm; imj += 2) {
      A[(ipj-imj)/2][(ipj+imj)/2] = sum;
    }
  }

  if (!ludcmp(A, m+1, index, &d)) return;

  for (j = 0; j < m+1; j++) b[j] = 0.0;
  b[ld] = 1.0;
  lubksb(A, m+1, index, b);

  for (kk = 0; kk < np; kk++) c[kk] = 0.0;
  for (k = -nl; k <= nr; k++) {
    sum = b[0];
    fac = 1.0;
    for (mm = 1; mm <= m; mm++) {
      fac *= (double)k;
      sum += b[mm]*fac;
    }
    kk = (np-k) % np;
    c[kk] = sum;
  }

  free(index);
  free(b);
  for (i = 0; i < m+1; i++) free(A[i]);
  free(A);
}

#define NRANSI

void covsrt(double **covar, int ma, int ia[], int mfit)
{
  int i, j, k;
  double swap;

  for (i = mfit; i < ma; i++)
    for (j = 0; j <= i; j++) covar[i][j] = covar[j][i] = 0.0;
  k = mfit-1;
  for (j = ma-1; j >= 0; j--) {
    if (ia[j]) {
      if (k != j) {
	for (i = 0; i < ma; i++) {
	  swap = covar[i][k];
	  covar[i][k] = covar[i][j];
	  covar[i][j] = swap;
	}
	for (i = 0; i < ma; i++) {
	  swap = covar[k][i];
	  covar[k][i] = covar[j][i];
	  covar[j][i] = swap;
	}
      }
      k--;
    }
  }
}

void gaussj(double **a, int n, double **b, int m)
{
  int *indxc, *indxr, *ipiv;
  int i, icol, irow, j, k, l, ll;
  double big, dum, pivinv, swap;

  indxc = ivector(n);
  if (indxc == (int *)NULL) return;

  indxr = ivector(n);
  if (indxr == (int *)NULL) return;

  ipiv = ivector(n);
  if (ipiv == (int *)NULL) return;

  for (j = 0; j < n; j++) ipiv[j]=0;
  for (i = 0; i < n; i++) {
    big=0.0;
    for (j = 0; j < n; j++) {
      if (ipiv[j] != 1) {
	for (k = 0; k < n; k++) {
	  if (ipiv[k] == 0) {
	    if (fabs(a[j][k]) >= big) {
	      big = fabs(a[j][k]);
	      irow = j;
	      icol = k;
	    }
	  } else if (ipiv[k] > 1) {
	    fprintf(stderr, "singular matrix (1)\n");
	    return;
	  }
	}
      }
    }
    ++(ipiv[icol]);
    if (irow != icol) {
      for (l = 0;l < n; l++) {
	swap = a[irow][l];
	a[irow][l] = a[icol][l];
	a[icol][l] = swap;
      }
      for (l = 0; l < m; l++) {
	swap = b[irow][l];
	b[irow][l] = b[icol][l];
	b[icol][l] = swap;
      }
    }
    indxr[i] = irow;
    indxc[i] = icol;
    if (a[icol][icol] == 0.0) {
      fprintf(stderr, "singular matrix (2)\n");
      return;
    }
    pivinv = 1.0/a[icol][icol];
    a[icol][icol] = 1.0;
    for (l = 0; l < n; l++) a[icol][l] *= pivinv;
    for (l = 0; l < m; l++) b[icol][l] *= pivinv;
    for (ll = 0; ll < n; ll++)
      if (ll != icol) {
	dum = a[ll][icol];
	a[ll][icol] = 0.0;
	for (l = 0;l < n;l++) a[ll][l] -= a[icol][l]*dum;
	for (l = 0;l < m;l++) b[ll][l] -= b[icol][l]*dum;
      }
  }
  for (l = n-1 ;l >= 0; l--) {
    if (indxr[l] != indxc[l])
      for (k = 0;k < n; k++) {
	swap = a[k][indxr[l]];
	a[k][indxr[l]] = a[k][indxc[l]];
	a[k][indxc[l]] = swap;
      }
  }
  free(ipiv);
  free(indxr);
  free(indxc);
}

void mrqcof(double x[], double y[], double sig[], int ndata, 
	    double a[], int ia[], int ma, 
	    double **alpha, double beta[], double *chisq,
	    double (*funcs)(double, double [], double [], int))
{
  int i, j, k, l, m, mfit = 0;
  double ymod, wt, sig2i, dy, *dyda;

  dyda = dvector(ma);
  if (dyda == (double *)NULL) return;

  for (j = 0; j < ma; j++) if (ia[j]) mfit++;
  for (j = 0; j < mfit; j++) {
    for (k = 0; k <= j; k++) alpha[j][k] = 0.0;
    beta[j] = 0.0;
  }

  *chisq = 0.0;
  for (i = 0; i < ndata; i++) {
    ymod = (*funcs)(x[i], a, dyda, ma);
    sig2i = 1.0/(sig[i]*sig[i]);
    dy = y[i] - ymod;
    for (j = -1, l = 0; l < ma; l++) {
      if (ia[l]) {
	wt = dyda[l]*sig2i;
	for (j++, k = 0, m = 0; m <= l; m++) {
	  if (ia[m]) {
	    alpha[j][k] += wt*dyda[m];
	    k++;
	  }
	}
	beta[j] += dy*wt;
      }
    }
    *chisq += dy*dy*sig2i;
  }
  for (j = 1; j < mfit; j++)
    for (k = 0; k < j; k++) alpha[k][j] = alpha[j][k];
  
  free(dyda);
}

void mrqmin(double x[], double y[], double sig[], int ndata, 
	    double a[], int ia[], int ma, 
	    double **covar, double **alpha, double *chisq,
	    double (*funcs)(double, double [], double [], int), 
	    double *alamda)
{
  int j, k, l;
  static int mfit;
  static double ochisq, *atry, *beta, *da, **oneda;

  if (*alamda < 0.0) {
    atry = dvector(ma);
    if (atry == (double *)NULL) return;

    beta = dvector(ma);
    if (beta == (double *)NULL) return;

    da = dvector(ma);
    if (da == (double *)NULL) return;

    for (mfit = 0, j = 0; j < ma; j++)
      if (ia[j]) mfit++;

    oneda = dmatrix(mfit, mfit);
    if (oneda == (double **)NULL) return;

    *alamda = 0.001;
    mrqcof(x, y, sig, ndata, a, ia, ma, alpha, beta, chisq, funcs);
    ochisq = (*chisq);

    for (j = 0; j < ma; j++) atry[j] = a[j];
  }

  for (j = 0; j < mfit; j++) {
    for (k = 0; k < mfit; k++) covar[j][k] = alpha[j][k];
    covar[j][j] = alpha[j][j]*(1.0+(*alamda));
    oneda[j][0] = beta[j];
  }
/*
  for (j = 0; j < mfit; j++) {
  for (k = 0; k < mfit; k++) printf("%lf ", covar[j][k]);
  printf(" = %lf\n", beta[j]);
  }
*/
  gaussj(covar, mfit, oneda, 1);
  for (j = 0; j < mfit; j++) da[j] = oneda[j][0];
  if (*alamda == 0.0) {
    covsrt(covar, ma, ia, mfit);
    for (j = 0; j < mfit; j++) free(oneda[j]);
    free(oneda);
    free(da);
    free(beta);
    free(atry);
    return;
  }
  for (j = 0, l = 0; l < ma; l++) if (ia[l]) atry[l] = a[l]+da[j++];
  mrqcof(x, y, sig, ndata, atry, ia, ma, covar, da, chisq, funcs);
  if (*chisq < ochisq) {
    *alamda *= 0.1;
    ochisq = (*chisq);
    for (j = 0; j < mfit; j++) {
      for (k = 0; k < mfit; k++) alpha[j][k] = covar[j][k];
      beta[j] = da[j];
    }
    for (l = 0; l < ma; l++) a[l] = atry[l];
  } else {
    *alamda *= 10.0;
    *chisq = ochisq;
  }
}

#define W 1.665109222   /* sqrt (4*ln2) */

double fgauss(double x, double a[], double dyda[], int na)
{
  int i;
  double fac, ex, arg;
  double y;

  y = 0.0;
  for (i = 0; i < na; i += 3) {
    arg = W*(x-a[i+1])/a[i+2];
    ex = exp(-arg*arg);
    fac = 2.0*a[i]*ex*arg;
    y += a[i]*ex;
    dyda[i] = ex;
    dyda[i+1] = W*fac/a[i+2];
    dyda[i+2] = fac*arg/a[i+2];
  }
  return y;
}

void svdcmp(double **a, int m, int n, double w[], double **v)
{
  int flag, i, its, j, jj, k, l, nm;
  double anorm, c, f, g, h, s, scale, x, y, z, *rv1;

  rv1 = dvector(n);
  if (rv1 == (double *)NULL) return;

  g = scale = anorm = 0.0;
  for (i = 0; i < n; i++) {
    l = i+1;
    rv1[i] = scale*g;
    g = s = scale = 0.0;
    if (i < m) {
      for (k = i; k < m; k++) scale += fabs(a[k][i]);
      if (scale) {
	for (k = i; k < m; k++) {
	  a[k][i] /= scale;
	  s += a[k][i]*a[k][i];
	}
	f = a[i][i];
	g = -sign(sqrt(s),f);
	h = f*g-s;
	a[i][i] = f-g;
	for (j = l; j < n; j++) {
	  for (s = 0.0, k = i; k < m; k++) s += a[k][i]*a[k][j];
	  f = s/h;
	  for (k = i; k < m; k++) a[k][j] += f*a[k][i];
	}
	for (k = i; k < m; k++) a[k][i] *= scale;
      }
    }
    w[i] = scale *g;
    g = s = scale = 0.0;
    if (i < m && i != n-1) {
      for (k = l; k < n; k++) scale += fabs(a[i][k]);
      if (scale) {
	for (k = l; k < n; k++) {
	  a[i][k] /= scale;
	  s += a[i][k]*a[i][k];
	}
	f = a[i][l];
	g = -sign(sqrt(s),f);
	h = f*g-s;
	a[i][l] = f-g;
	for (k = l; k < n; k++) rv1[k] = a[i][k]/h;
	for (j = l; j < m; j++) {
	  for (s = 0.0, k = l; k < n; k++) s += a[j][k]*a[i][k];
	  for (k = l; k < n; k++) a[j][k] += s*rv1[k];
	}
	for (k = l; k < n; k++) a[i][k] *= scale;
      }
    }
    if (anorm < fabs(w[i])+fabs(rv1[i])) anorm = fabs(w[i])+fabs(rv1[i]);
  }
  for (i = n-1; i >= 0; i--) {
    if (i < n-1) {
      if (g) {
	for (j = l; j < n; j++)
	  v[j][i] = (a[i][j]/a[i][l])/g;
	for (j = l; j < n; j++) {
	  for (s = 0.0, k = l; k < n; k++) s += a[i][k]*v[k][j];
	  for (k = l; k < n; k++) v[k][j] += s*v[k][i];
	}
      }
      for (j = l; j < n; j++) v[i][j] = v[j][i] = 0.0;
    }
    v[i][i] = 1.0;
    g = rv1[i];
    l = i;
  }
  for (i = (m < n ? m-1 : n-1); i >= 0; i--) {
    l = i+1;
    g = w[i];
    for (j = l; j < n; j++) a[i][j] = 0.0;
    if (g) {
      g = 1.0/g;
      for (j = l; j < n; j++) {
	for (s = 0.0, k = l; k < m; k++) s += a[k][i]*a[k][j];
	f = (s/a[i][i])*g;
	for (k = i; k < m; k++) a[k][j] += f*a[k][i];
      }
      for (j = i; j < m; j++) a[j][i] *= g;
    } else for (j = i; j < m; j++) a[j][i] = 0.0;
    ++a[i][i];
  }
  for (k = n-1; k >= 0; k--) {
    for (its = 1; its <= 30; its++) {
      flag = 1;
      for (l = k; l >= 0; l--) {
	nm = l-1;
	if ((fabs(rv1[l])+anorm) == anorm) {
	  flag = 0;
	  break;
	}
	if ((fabs(w[nm])+anorm) == anorm) break;
      }
      if (flag) {
	c = 0.0;
	s = 1.0;
	for (i = l; i <= k; i++) {
	  f = s*rv1[i];
	  rv1[i] = c*rv1[i];
	  if ((fabs(f)+anorm) == anorm) break;
	  g = w[i];
	  h = sqrt(f*f+g*g);
	  w[i] = h;
	  h = 1.0/h;
	  c = g*h;
	  s = -f*h;
	  for (j = 0; j < m; j++) {
	    y = a[j][nm];
	    z = a[j][i];
	    a[j][nm] = y*c+z*s;
	    a[j][i] = z*c-y*s;
	  }
	}
      }
      z = w[k];
      if (l == k) {
	if (z < 0.0) {
	  w[k] = -z;
	  for (j = 0; j < n; j++) v[j][k] = -v[j][k];
	}
	break;
      }
      if (its == 30) fprintf(stderr, "no convergence in 30 svdcmp iterations");
      x = w[l];
      nm = k-1;
      y = w[nm];
      g = rv1[nm];
      h = rv1[k];
      f = ((y-z)*(y+z)+(g-h)*(g+h))/(2.0*h*y);
      g = sqrt(f*f+1.0);
      f = ((x-z)*(x+z)+h*((y/(f+sign(g,f)))-h))/x;
      c = s = 1.0;
      for (j = l; j <= nm; j++) {
	i = j+1;
	g = rv1[i];
	y = w[i];
	h = s*g;
	g = c*g;
	z = sqrt(f*f+h*h);
	rv1[j] = z;
	c = f/z;
	s = h/z;
	f = x*c+g*s;
	g = g*c-x*s;
	h = y*s;
	y *= c;
	for (jj = 0; jj < n; jj++) {
	  x = v[jj][j];
	  z = v[jj][i];
	  v[jj][j] = x*c+z*s;
	  v[jj][i] = z*c-x*s;
	}
	z = sqrt(f*f+h*h);
	w[j] = z;
	if (z) {
	  z = 1.0/z;
	  c = f*z;
	  s = h*z;
	}
	f = c*g+s*y;
	x = c*y-s*g;
	for (jj = 0; jj < m; jj++) {
	  y = a[jj][j];
	  z = a[jj][i];
	  a[jj][j] = y*c+z*s;
	  a[jj][i] = z*c-y*s;
	}
      }
      rv1[l] = 0.0;
      rv1[k] = f;
      w[k] = x;
    }
  }
  free(rv1);
}
