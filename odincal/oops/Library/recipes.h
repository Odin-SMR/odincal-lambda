#ifndef RECIPES_H
#define RECIPES_H

#ifndef PI  
#define PI 3.14159265358979323844
#endif

#ifndef TINY
#define TINY 1.0e-20
#endif

#define min(a,b) ((a) < (b) ? (a) : (b))
#define sign(a,b) ((b) >= 0.0 ? fabs(a) : -fabs(a))

float ran1(int *idum);
float gasdev(int *idum);
int *ivector(int);
double *dvector(int);
double **dmatrix(int, int);
void four1(float data[], int nn, int sign);
void twofft(float data1[], float data2[], float fft1[], float fft2[], int n);
void realft(float data[], int n, int sign);
void cosft(float y[], int n, int sign);
void sinft(float y[], int n);
int ludcmp(double **a, int n, int indx[], double *d) ;
void lubksb(double *a[], int n, int indx[], double b[]) ;
double matinv(double **a, int n) ;
int solve(double **a, double *v, int n) ;
void savgol(double c[], int np, int nl, int nr, int ld, int m);
void covsrt(double **covar, int ma, int ia[], int mfit);
void gaussj(double **a, int n, double **b, int m);
void mrqcof(double x[], double y[], double sig[], int ndata, 
	    double a[], int ia[], int ma, 
	    double **alpha, double beta[], double *chisq,
	    double (*funcs)(double, double [], double [], int));
void mrqmin(double x[], double y[], double sig[], int ndata, 
	    double a[], int ia[], int ma, 
	    double **covar, double **alpha, double *chisq,
	    double (*funcs)(double, double [], double [], int), 
	    double *alamda);
double fgauss(double x, double a[], double dyda[], int na);
void svdcmp(double **a, int m, int n, double w[], double **v);

#endif
