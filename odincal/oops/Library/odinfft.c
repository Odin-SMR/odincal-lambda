#include <math.h>

#include "odinfft.h"

void ODINwarning(char *fmt, ...);

/* 
   The following routines:

   fft2, fft3, fft4, fft5, fft7, fft8 and fft16

   are optimised fast Fourier transforms taken from 

   H.J.Nussbaumer: "Fast Fourier Transform and Convolution Algorithms"

   Any transform of arrays of length n, where n is a product of two of
   the numbers from above can then be split into a two dimensional FFT 
   plus some extra "twiddle factors". See fft6 (2 by 3) and 
   fft32 (4 by 8) for an example.  
*/
void fft2(double x[])
{
    static double m[4];

    m[0] = x[0]+x[2];    m[1] = x[1]+x[3];
    m[2] = x[0]-x[2];    m[3] = x[1]-x[3];

    x[0] = m[0];         x[1] = m[1];
    x[2] = m[2];         x[3] = m[3];
}

void fft3(double x[])
{
    static double c1u = -0.50000000000000;  /* cos(2*PI/3) */
    static double s1u =  0.86602540378444;  /* sin(2*PI/3) */
    static double m[6], s[2], t[2];

    t[0] = x[2]+x[4];          t[1] = x[3]+x[5];
    m[0] = x[0]+t[0];          m[1] = x[1]+t[1];
    m[2] = (c1u-1.0)*t[0];     m[3] = (c1u-1.0)*t[1];
    m[4] = -s1u*(x[5]-x[3]);   m[5] = s1u*(x[4]-x[2]);
    s[0] = m[0]+m[2];          s[1] = m[1]+m[3];

    x[0] = m[0];               x[1] = m[1];
    x[2] = s[0]+m[4];          x[3] = s[1]+m[5];
    x[4] = s[0]-m[4];          x[5] = s[1]-m[5];
}

void fft4(double x[])
{
    static double m[8], t[4];
    
    t[0] = x[0]+x[4];          t[1] = x[1]+x[5];
    t[2] = x[2]+x[6];          t[3] = x[3]+x[7];
    m[0] = t[0]+t[2];          m[1] = t[1]+t[3];
    m[2] = t[0]-t[2];          m[3] = t[1]-t[3];
    m[4] = x[0]-x[4];          m[5] = x[1]-x[5];
    m[6] = x[3]-x[7];          m[7] = x[6]-x[2];
    
    x[0] = m[0];               x[1] = m[1];
    x[2] = m[4]+m[6];          x[3] = m[5]+m[7];
    x[4] = m[2];               x[5] = m[3];
    x[6] = m[4]-m[6];          x[7] = m[5]-m[7];
}

void fft5(double x[])
{
    static double c1u =  0.30901699437495;  /* cos(2*PI/5) */
    static double s1u =  0.95105651629515;  /* sin(2*PI/5) */
    static double c2u = -0.80901699437495;  /* cos(4*PI/5) */
    static double s2u =  0.58778525229247;  /* sin(4*PI/5) */
    static double t[10], s[10], m[12];

    t[0] = x[2]+x[8];           t[1]=x[3]+x[9];
    t[2] = x[4]+x[6];           t[3]=x[5]+x[7];
    t[4] = x[2]-x[8];           t[5]=x[3]-x[9];
    t[6] = x[6]-x[4];           t[7]=x[7]-x[5];
    t[8] = t[0]+t[2];           t[9]=t[1]+t[3];
    
    m[ 0] = x[0]+t[8];           m[1]=x[1]+t[9];
    m[ 2] = ((c1u+c2u)/2.0-1.0)*t[8];
    m[ 3] = ((c1u+c2u)/2.0-1.0)*t[9];
    m[ 4] = ((c1u-c2u)/2.0)*(t[0]-t[2]);
    m[ 5] = ((c1u-c2u)/2.0)*(t[1]-t[3]);
    m[ 6] =  s1u*(t[5]+t[7]);
    m[ 7] = -s1u*(t[4]+t[6]);
    m[ 8] =  (s1u+s2u)*t[7];
    m[ 9] = -(s1u+s2u)*t[6];
    m[10] = -(s1u-s2u)*t[5];
    m[11] =  (s1u-s2u)*t[4];
    
    s[0] =  m[ 0]+m[ 2];        s[1] =  m[ 1]+m[ 3];
    s[2] =  s[ 0]+m[ 4];        s[3] =  s[ 1]+m[ 5];
    s[4] =  m[ 6]-m[ 8];        s[5] =  m[ 7]-m[ 9];
    s[6] =  s[ 0]-m[ 4];        s[7] =  s[ 1]-m[ 5];
    s[8] =  m[ 6]+m[10];        s[9] =  m[ 7]+m[11];

    x[ 0] = m[ 0];              x[ 1] = m[ 1];
    x[ 2] = s[ 2]+s[ 4];        x[ 3] = s[ 3]+s[ 5];
    x[ 4] = s[ 6]+s[ 8];        x[ 5] = s[ 7]+s[ 9];
    x[ 6] = s[ 6]-s[ 8];        x[ 7] = s[ 7]-s[ 9];
    x[ 8] = s[ 2]-s[ 4];        x[ 9] = s[ 3]-s[ 5];
}

void fft6(double x[])
{
    static double X[2][6];
    static double y[4];
    double p, cp, sp;
    int i, j, l;

    l = 0;
    for (i = 0; i < 3; i++) {
	for (j = 0; j < 2; j++) {
	    X[j][2*i  ] = x[l  ];
	    X[j][2*i+1] = x[l+1];
	    l += 2;
	}
    }
    fft3(X[0]);
    fft3(X[1]);

    p = 2.0*M_PI/6;
    for (i = 0; i < 3; i++) {
	for (j = 0; j < 2; j++) {
	    cp = cos(p*i*j);
	    sp = sin(p*i*j);
  	    y[2*j  ] =  X[j][2*i  ]*cp + X[j][2*i+1]*sp;
  	    y[2*j+1] =  X[j][2*i+1]*cp - X[j][2*i  ]*sp;
	}
	fft2(y);
	for (j = 0; j < 2; j++) {
  	    X[j][2*i  ] = y[2*j  ];
  	    X[j][2*i+1] = y[2*j+1];
	}
    }

    l = 0;
    for (j = 0; j < 2; j++) {
	for (i = 0; i < 3; i++) {
	    x[l  ] = X[j][2*i  ];
	    x[l+1] = X[j][2*i+1];
	    l += 2;
	}
    }
}

void fft7(double x[])
{
    static double c1u =  0.62348980185873;  /* cos(2*PI/7) */
    static double s1u =  0.78183148246803;  /* sin(2*PI/7) */
    static double c2u = -0.22252093395631;  /* cos(4*PI/7) */
    static double s2u =  0.97492791218182;  /* sin(4*PI/7) */
    static double c3u = -0.90096886790242;  /* cos(6*PI/7) */
    static double s3u =  0.43388373911756;  /* sin(6*PI/7) */
    static double t[28], s[22], m[18];

    t[ 0] =  x[ 2]+x[12];        t[ 1] =  x[ 3]+x[13];
    t[ 2] =  x[ 4]+x[10];        t[ 3] =  x[ 5]+x[11];
    t[ 4] =  x[ 6]+x[ 8];        t[ 5] =  x[ 7]+x[ 9];
    t[ 6] =  t[ 0]+t[ 2]+t[ 4];  t[ 7] =  t[ 1]+t[ 3]+t[ 5];
    t[ 8] =  x[ 2]-x[12];        t[ 9] =  x[ 3]-x[13];
    t[10] =  x[ 4]-x[10];        t[11] =  x[ 5]-x[11];
    t[12] =  x[ 8]-x[ 6];        t[13] =  x[ 9]-x[ 7];
    t[14] =  t[ 0]-t[ 4];        t[15] =  t[ 1]-t[ 5];
    t[16] =  t[ 4]-t[ 2];        t[17] =  t[ 5]-t[ 3];
    t[18] =  t[ 8]+t[10]+t[12];  t[19] =  t[ 9]+t[11]+t[13];
    t[20] =  t[12]-t[ 8];        t[21] =  t[13]-t[ 9];
    t[22] =  t[10]-t[12];        t[23] =  t[11]-t[13];
    t[24] = -t[14]-t[16];        t[25] = -t[15]-t[17];
    t[26] = -t[20]-t[22];        t[27] = -t[21]-t[23];

    m[ 0] =  x[ 0]+t[ 6];                  
    m[ 1] =  x[ 1]+t[ 7];
    m[ 2] =  ((c1u+c2u+c3u)/3.0-1.0)*t[ 6]; 
    m[ 3] =  ((c1u+c2u+c3u)/3.0-1.0)*t[ 7];
    m[ 4] =  ((2.0*c1u-c2u-c3u)/3.0)*t[14]; 
    m[ 5] =  ((2.0*c1u-c2u-c3u)/3.0)*t[15];
    m[ 6] =  ((c1u-2.0*c2u+c3u)/3.0)*t[16]; 
    m[ 7] =  ((c1u-2.0*c2u+c3u)/3.0)*t[17];
    m[ 8] =  ((c1u+c2u-2.0*c3u)/3.0)*t[24]; 
    m[ 9] =  ((c1u+c2u-2.0*c3u)/3.0)*t[25];
    m[10] =  ((s1u+s2u-s3u)/3.0)*t[19];    
    m[11] = -((s1u+s2u-s3u)/3.0)*t[18];
    m[12] = -((2.0*s1u-s2u+s3u)/3.0)*t[21]; 
    m[13] =  ((2.0*s1u-s2u+s3u)/3.0)*t[20];
    m[14] = -((s1u-2.0*s2u-s3u)/3.0)*t[23];
    m[15] =  ((s1u-2.0*s2u-s3u)/3.0)*t[22];
    m[16] = -((s1u+s2u+2.0*s3u)/3.0)*t[27];
    m[17] =  ((s1u+s2u+2.0*s3u)/3.0)*t[26];

    s[ 0] = -m[ 4]-m[ 6];        s[ 1] = -m[ 5]-m[ 7];
    s[ 2] = -m[ 4]-m[ 8];        s[ 3] = -m[ 5]-m[ 9];
    s[ 4] = -m[12]-m[14];        s[ 5] = -m[13]-m[15];
    s[ 6] =  m[12]+m[16];        s[ 7] =  m[13]+m[17];
    s[ 8] =  m[ 0]+m[ 2];        s[ 9] =  m[ 1]+m[ 3];
    s[10] =  s[ 8]-s[ 0];        s[11] =  s[ 9]-s[ 1];
    s[12] =  s[ 8]+s[ 2];        s[13] =  s[ 9]+s[ 3];
    s[14] =  s[ 8]+s[ 0]-s[ 2];  s[15] =  s[ 9]+s[ 1]-s[ 3];
    s[16] =  m[10]-s[ 4];        s[17] =  m[11]-s[ 5];
    s[18] =  m[10]-s[ 6];        s[19] =  m[11]-s[ 7];
    s[20] =  m[10]+s[ 4]+s[ 6];  s[21] =  m[11]+s[ 5]+s[ 7];

    x[ 0] = m[ 0];        x[ 1] = m[ 1];
    x[ 2] = s[10]+s[16];  x[ 3] = s[11]+s[17];
    x[ 4] = s[12]+s[18];  x[ 5] = s[13]+s[19];
    x[ 6] = s[14]-s[20];  x[ 7] = s[15]-s[21];
    x[ 8] = s[14]+s[20];  x[ 9] = s[15]+s[21];
    x[10] = s[12]-s[18];  x[11] = s[13]-s[19];
    x[12] = s[10]-s[16];  x[13] = s[11]-s[17];
}

void fft8(double x[])
{
    static double c1u =  0.70710678118655;  /* cos(2*PI/8) */
    static double s1u =  0.70710678118655;  /* sin(2*PI/8) */
    static double t[16], s[8], m[16];

    t[ 0] =  x[ 0]+x[ 8];        t[ 1] =  x[ 1]+x[ 9];
    t[ 2] =  x[ 4]+x[12];        t[ 3] =  x[ 5]+x[13];
    t[ 4] =  x[ 2]+x[10];        t[ 5] =  x[ 3]+x[11];
    t[ 6] =  x[ 2]-x[10];        t[ 7] =  x[ 3]-x[11];
    t[ 8] =  x[ 6]+x[14];        t[ 9] =  x[ 7]+x[15];
    t[10] =  x[ 6]-x[14];        t[11] =  x[ 7]-x[15];
    t[12] =  t[ 0]+t[ 2];        t[13] =  t[ 1]+t[ 3];
    t[14] =  t[ 4]+t[ 8];        t[15] =  t[ 5]+t[ 9];

    m[ 0] =  t[12]+t[14];        m[ 1] =  t[13]+t[15];
    m[ 2] =  t[12]-t[14];        m[ 3] =  t[13]-t[15];
    m[ 4] =  t[ 0]-t[ 2];        m[ 5] =  t[ 1]-t[ 3];
    m[ 6] =  x[ 0]-x[ 8];        m[ 7] =  x[ 1]-x[ 9];
    m[ 8] =  c1u*(t[ 6]-t[10]);  m[ 9] =  c1u*(t[ 7]-t[11]);
    m[10] =  t[ 5]-t[ 9];        m[11] =  t[ 8]-t[ 4];
    m[12] =  x[ 5]-x[13];        m[13] =  x[12]-x[ 4];
    m[14] =  s1u*(t[ 7]+t[11]);  m[15] = -s1u*(t[ 6]+t[10]);

    s[ 0] =  m[ 6]+m[ 8];        s[ 1] =  m[ 7]+m[ 9];
    s[ 2] =  m[ 6]-m[ 8];        s[ 3] =  m[ 7]-m[ 9];
    s[ 4] =  m[12]+m[14];        s[ 5] =  m[13]+m[15];
    s[ 6] =  m[12]-m[14];        s[ 7] =  m[13]-m[15];

    x[ 0] = m[ 0];               x[ 1] = m[ 1];
    x[ 2] = s[ 0]+s[ 4];         x[ 3] = s[ 1]+s[ 5];
    x[ 4] = m[ 4]+m[10];         x[ 5] = m[ 5]+m[11];
    x[ 6] = s[ 2]-s[ 6];         x[ 7] = s[ 3]-s[ 7];
    x[ 8] = m[ 2];               x[ 9] = m[ 3];
    x[10] = s[ 2]+s[ 6];         x[11] = s[ 3]+s[ 7];
    x[12] = m[ 4]-m[10];         x[13] = m[ 5]-m[11];
    x[14] = s[ 0]-s[ 4];         x[15] = s[ 1]-s[ 5];
}

void fft16(double x[])
{
    static double c1u =  0.92387953251129;  /* cos(2*PI/16) */
    static double s1u =  0.38268343236509;  /* sin(2*PI/16) */
    static double c2u =  0.70710678118655;  /* cos(4*PI/16) */
    static double s2u =  0.70710678118655;  /* sin(4*PI/16) */
    static double c3u =  0.38268343236509;  /* cos(6*PI/16) */
    static double s3u =  0.92387953251129;  /* sin(6*PI/16) */
    static double t[52], s[40], m[36];

    t[ 0] =  x[ 0]+x[16];        t[ 1] =  x[ 1]+x[17];
    t[ 2] =  x[ 8]+x[24];        t[ 3] =  x[ 9]+x[25];
    t[ 4] =  x[ 4]+x[20];        t[ 5] =  x[ 5]+x[21];
    t[ 6] =  x[ 4]-x[20];        t[ 7] =  x[ 5]-x[21];
    t[ 8] =  x[12]+x[28];        t[ 9] =  x[13]+x[29];
    t[10] =  x[12]-x[28];        t[11] =  x[13]-x[29];
    t[12] =  x[ 2]+x[18];        t[13] =  x[ 3]+x[19];
    t[14] =  x[ 2]-x[18];        t[15] =  x[ 3]-x[19];
    t[16] =  x[ 6]+x[22];        t[17] =  x[ 7]+x[23];
    t[18] =  x[ 6]-x[22];        t[19] =  x[ 7]-x[23];
    t[20] =  x[10]+x[26];        t[21] =  x[11]+x[27];
    t[22] =  x[10]-x[26];        t[23] =  x[11]-x[27];
    t[24] =  x[14]+x[30];        t[25] =  x[15]+x[31];
    t[26] =  x[14]-x[30];        t[27] =  x[15]-x[31];
    t[28] =  t[ 0]+t[ 2];        t[29] =  t[ 1]+t[ 3];
    t[30] =  t[ 4]+t[ 8];        t[31] =  t[ 5]+t[ 9];
    t[32] =  t[28]+t[30];        t[33] =  t[29]+t[31];
    t[34] =  t[12]+t[20];        t[35] =  t[13]+t[21];
    t[36] =  t[12]-t[20];        t[37] =  t[13]+t[21];
    t[38] =  t[16]+t[24];        t[39] =  t[17]+t[25];
    t[40] =  t[16]-t[24];        t[41] =  t[17]+t[25];
    t[42] =  t[34]+t[38];        t[43] =  t[35]+t[39];
    t[44] =  t[14]+t[26];        t[45] =  t[15]+t[27];
    t[46] =  t[14]-t[26];        t[47] =  t[15]+t[27];
    t[48] =  t[22]+t[18];        t[49] =  t[23]+t[19];
    t[50] =  t[22]-t[18];        t[51] =  t[23]+t[19];

    m[ 0] =  t[32]+t[42];        m[ 1] =  t[33]+t[43];
    m[ 2] =  t[32]-t[42];        m[ 3] =  t[33]-t[43];
    m[ 4] =  t[28]-t[30];        m[ 5] =  t[29]-t[31];
    m[ 6] =  t[ 0]-t[ 2];        m[ 7] =  t[ 1]-t[ 3];
    m[ 8] =  x[ 0]-x[16];        m[ 9] =  x[ 1]-x[17];
    m[10] =  c2u*(t[36]-t[40]);  m[11] =  c2u*(t[37]-t[41]);
    m[12] =  c2u*(t[ 6]-t[10]);  m[13] =  c2u*(t[ 7]-t[11]);
    m[14] =  c3u*(t[46]+t[50]);  m[15] =  c2u*(t[47]+t[51]);
    m[16] =  (c1u+c3u)*t[46];    m[17] =  (c1u+c3u)*t[47];
    m[18] =  (c3u-c1u)*t[50];    m[19] =  (c3u-c1u)*t[51];
    m[20] =  t[35]-t[39];        m[21] =  t[38]-t[34];
    m[22] =  t[ 5]-t[ 9];        m[23] =  t[ 8]-t[ 4];
    m[24] =  x[ 9]-x[25];        m[25] =  x[24]-x[ 8];
    m[26] =  s2u*(t[37]+t[41]);  m[27] = -s2u*(t[36]+t[40]);
    m[28] =  s2u*(t[ 7]+t[11]);  m[29] = -s2u*(t[ 6]+t[10]);
    m[30] =  s3u*(t[45]+t[49]);  m[31] = -s3u*(t[44]+t[48]);
    m[32] =  -(s3u-s1u)*t[45];   m[33] = (s3u-s1u)*t[44];
    m[34] =  (s1u+s3u)*t[49];    m[35] = -(s1u+s3u)*t[48];

    s[ 0] = m[ 6]+m[10];         s[ 1] = m[ 7]+m[11];
    s[ 2] = m[ 6]-m[10];         s[ 3] = m[ 7]-m[11];
    s[ 4] = m[26]+m[22];         s[ 5] = m[27]+m[23];
    s[ 6] = m[26]-m[22];         s[ 7] = m[27]-m[23];
    s[ 8] = m[ 8]+m[12];         s[ 9] = m[ 9]+m[13];
    s[10] = m[ 8]-m[12];         s[11] = m[ 9]-m[13];
    s[12] = m[16]-m[14];         s[13] = m[17]-m[15];
    s[14] = m[18]-m[14];         s[15] = m[19]-m[15];
    s[16] = s[ 8]+s[12];         s[17] = s[ 9]+s[13];
    s[18] = s[ 8]-s[12];         s[19] = s[ 9]-s[13];
    s[20] = s[10]+s[14];         s[21] = s[11]+s[15];
    s[22] = s[10]-s[14];         s[23] = s[11]-s[15];
    s[24] = m[24]+m[28];         s[25] = m[25]+m[29];
    s[26] = m[24]-m[28];         s[27] = m[25]-m[29];
    s[28] = m[30]+m[32];         s[29] = m[31]+m[33];
    s[30] = m[30]-m[34];         s[31] = m[31]-m[35];
    s[32] = s[24]+s[28];         s[33] = s[25]+s[29];
    s[34] = s[24]-s[28];         s[35] = s[25]-s[29];
    s[36] = s[26]+s[30];         s[37] = s[27]+s[31];
    s[38] = s[26]-s[30];         s[39] = s[27]-s[31];

    x[ 0] = m[ 0];               x[ 1] = m[ 1];
    x[ 2] = s[16]+s[32];         x[ 3] = s[17]+s[33];
    x[ 4] = s[ 0]+s[ 4];         x[ 5] = s[ 1]+s[ 5];
    x[ 6] = s[22]-s[38];         x[ 7] = s[23]-s[39];
    x[ 8] = m[ 4]+m[20];         x[ 9] = m[ 5]+m[21];
    x[10] = s[20]+s[36];         x[11] = s[21]+s[37];
    x[12] = s[ 2]+s[ 6];         x[13] = s[ 3]+s[ 7];
    x[14] = s[18]-s[34];         x[15] = s[19]-s[35];
    x[16] = m[ 2];               x[17] = m[ 3];
    x[18] = s[18]+s[34];         x[19] = s[19]+s[35];
    x[20] = s[ 2]-s[ 6];         x[21] = s[ 3]-s[ 7];
    x[22] = s[20]-s[36];         x[23] = s[21]-s[37];
    x[24] = m[ 4]-m[20];         x[25] = m[ 5]-m[21];
    x[26] = s[22]+s[38];         x[27] = s[23]+s[39];
    x[28] = s[ 0]-s[ 4];         x[29] = s[ 1]-s[ 5];
    x[30] = s[16]-s[32];         x[31] = s[17]-s[33];
}

void fft32(double x[])
{
    static double X[4][16];
    static double y[8];
    double p, cp, sp;
    int i, j, l;

    l = 0;
    for (i = 0; i < 8; i++) {
	for (j = 0; j < 4; j++) {
	    X[j][2*i  ] = x[l  ];
	    X[j][2*i+1] = x[l+1];
	    l += 2;
	}
    }

    for (j = 0; j < 4; j++) fft8(X[j]);

    p = 2.0*M_PI/32;
    for (i = 0; i < 8; i++) {
	for (j = 0; j < 4; j++) {
	    cp = cos(p*i*j);
	    sp = sin(p*i*j);
  	    y[2*j  ] =  X[j][2*i  ]*cp + X[j][2*i+1]*sp;
  	    y[2*j+1] =  X[j][2*i+1]*cp - X[j][2*i  ]*sp;
	}
	fft4(y);
	for (j = 0; j < 4; j++) {
  	    X[j][2*i  ] = y[2*j  ];
  	    X[j][2*i+1] = y[2*j+1];
	}
    }

    l = 0;
    for (j = 0; j < 4; j++) {
	for (i = 0; i < 8; i++) {
	    x[l  ] = X[j][2*i];
	    x[l+1] = X[j][2*i+1];
	    l += 2;
	}
    }
}

/*
  Calculate a FFT of n times 32 data. Return only first positive
  frequency half of the resulting (symmetric) output. The real valued
  first and last components of the complex transform are returned as
  data[0] and data[1], respectively.
*/
void realft32xN(double data[], int n)
{
    static double x[8][64], y[16];
    double p,cp, sp;
    int i, j, k, l;

    p = 2.0*M_PI/n;

    if (n % 32) ODINwarning("length of data not a multiple of 32");
    k = n/32;
    if (k > 8)  ODINwarning("length of data exceeds limit");

    l = 0;
    for (i = 0 ; i < 32; i++) {
	for (j = 0 ; j < k; j++) {
	    x[j][2*i  ] = data[l];
	    x[j][2*i+1] = 0.0;       /* we have real data on input */
	    l++;
	}
    }

    /*for each row */
    for (j = 0; j < k; j++) fft32(x[j]);

    if (k > 1) {
	/*for each column */
	for (i = 0; i < 32; i++) {
	    for (j = 0; j < k; j++) {
		cp = cos(p*j*i);       /* twiddle factors */
		sp = sin(p*j*i);       /* twiddle factors */
		y[2*j  ] = x[j][2*i  ]*cp + x[j][2*i+1]*sp;
		y[2*j+1] = x[j][2*i+1]*cp - x[j][2*i  ]*sp;
	    }

	    switch (k) {
	      case 2:
		fft2(y);
		break;
	      case 3:
		fft3(y);
		break;
	      case 4:
		fft4(y);
		break;
	      case 5:
		fft5(y);
		break;
	      case 6:
		fft6(y);
		break;
	      case 7:
		fft7(y);
		break;
	      case 8:
		fft8(y);
		break;
	    }
	    for (j = 0; j < k; j++) {
		x[j][2*i  ] = y[2*j  ];
		x[j][2*i+1] = y[2*j+1];
	    }
	}
    }

    for (l = 0; l < n; l++) {
	j = l/64;
	i = l%64;
	data[l] = x[j][i];
    }
    /* return the Nyqvist frequency the same way as 'realft' */
    data[1] = x[n/64][n%64];

    return;
}

/*
 * Multiply spectrum with Hanning weighting factors.
 */
void hanning(double data[], int n)
{
    double w;
    int i;

    w = M_PI/n;
    for (i = 0; i < n; i++) data[i] *= 0.5+0.5*cos(w*i);
}

#define MAXCHIPS 8

/*
  The Odin correlators use 1 to 8 correlator chips which, after padding 
  with zeros, will contribute 112 lags each. Before the FFT, data are 
  expanded to make them even sequences with symmetry around data[0], i.e.
  the original sequence data[0] ... data[111] is replaced by 
  data ... data[111] 0.0 data[111] ... data[1], a sequence of length 224.
  
  All required transforms are therefor of length n*224 = n*32*7.
*/
void odinfft(double data[], int n)
{
    static double matrix[7][2*MAXCHIPS*112/7];
    static double x[14], xn[7];
    static double p, cp, sp;
    int i, j, k;

    hanning(data, n);
    p = M_PI/n;

    if (n % 7) {
	ODINwarning("number of channels not a multiple of 7");
	return;
    }

    /* Store data in a 7 by x matrix, filling column by column */
    k = 0;
    for (i = 0; i < n/7; i++) {
	for (j = 0; j < 7; j++) {
	    matrix[j][i] = data[k];
	    k++;
	}
    }

    /* Fill up second half of matrix symmetrically */
    k = 1;
    for (i = 2*n/7-1; i > n/7; i--) {
	for (j = 6; j >= 0; j--) {
	    matrix[j][i] = data[k];
	    k++;
	}
    }

    /* take care of central column, the centre of symmetry */
    matrix[0][n/7] = 0.0;
    for (k = 1; k < 7; k++) {
	matrix[k][n/7] = data[n-k];
    }

    /* Perform fast Fourier transform row after row:
       Note, that 2*n/7 will be a multiple of 32.
       If you take each row of matrix as a (real) vector x, then on exit of 
       routine realft32xN each row will be replaced by the (complex) Fourier
       transform X of x, for the order in which they are stored read the 
       comments for routine realft32xN */
    for (i = 0; i < 7; i++) {
	realft32xN(matrix[i], 2*n/7);
    }

    /* 
       Save the last element of each Fourier transform in a temporary
       array to avoid overwriting 
    */
    for (k = 0; k < 7; k++) {
	xn[k] = matrix[k][1];
    }

    /* Now do a FFT on each column multiplied by exp(-j*2*PI*i*k/(2*n))
       where l (0..2) is the row index and k (0..n/7-1) is the column 
       index. */
    /* first element: i = 0 */
    for (k = 0; k < 7; k++) {
	/* twiddle factors (trivial in this case) */
	/* cp = cos(p*k*i) = 1.0; */
	/* sp = sin(p*k*i) = 0.0; */
	x[2*k  ] = matrix[k][0];
	x[2*k+1] = 0.0;
    }
    fft7(x);
    for (k = 0; k < 7; k++) matrix[k][0] = x[2*k];

    /* intermediate elements: 1..n/7-1 */
    for (i = 1; i < n/7; i++) {
	for (k = 0; k < 7; k++) {
	    /* twiddle factors */
	    cp = cos(p*k*i); 
	    sp = sin(p*k*i);
     	    x[2*k  ] =  matrix[k][2*i  ]*cp + matrix[k][2*i+1]*sp;
    	    x[2*k+1] =  matrix[k][2*i+1]*cp - matrix[k][2*i  ]*sp;
	}
	fft7(x);
	for (k = 0; k < 7; k++) matrix[k][i] = x[2*k];
    }

    /* last element: i = n/7 */
    for (k = 0; k < 7; k++) {
	/* twiddle factors */
	cp = cos(p*k*n/7);
	sp = sin(p*k*n/7);
  	x[2*k  ] =  xn[k]*cp;
  	x[2*k+1] = -xn[k]*sp;
    }
    fft7(x);
    for (k = 0; k < 7; k++) matrix[k][n/7] = x[2*k];

    /* Rearrange matrix elements into output vector data,
       overwriting the raw correlation coefficients with the
       power spectral densities */
    for (i = 0*n/7; i < 1*n/7; i++)  data[i] = matrix[0][i];
    for (i = 1*n/7; i < 2*n/7; i++)  data[i] = matrix[6][2*n/7-i];
    for (i = 2*n/7; i < 3*n/7; i++)  data[i] = matrix[1][i-2*n/7];
    for (i = 3*n/7; i < 4*n/7; i++)  data[i] = matrix[5][4*n/7-i];
    for (i = 4*n/7; i < 5*n/7; i++)  data[i] = matrix[2][i-4*n/7];
    for (i = 5*n/7; i < 6*n/7; i++)  data[i] = matrix[4][6*n/7-i];
    for (i = 6*n/7; i < 7*n/7; i++)  data[i] = matrix[3][i-6*n/7];
}
