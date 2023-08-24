#ifdef CORRFREQS

/* $Id$ */

/*                                 AC1           AC2       */
static double ssbdf0[]  = {        50.03,        50.63 };
static double ssbdfdt[] = {       -0.974,       -0.920 };
static double clkf0[]   = { 224.001466e6, 223.060055e6 };
static double clkdfdt[] = {   -68.333333,   -85.945946 };
static double clkT0[]   = {         22.5,         21.2 };

#endif

#ifdef PLLFREQS

/*                           PLL A    PLL B      */
static double pllc0[]   = {  29.23,   24.69 };
static double pllc1[]   = { -0.138,  -0.109 };

#endif

#ifdef CALLOAD

/* 
   Correct temperatures reported by calibration load,
   based on measurements in Svobodny 2001-02-09 

   The true temperature reported on the A side will be
   T = T - dtcal[0];

   The true temperature reported on the B side will be
   T = T - dtcal[1];

   where T is the uncorrected temperature calulated via
   J. Lassing's formula.
*/

static double dtcal[] = { 1.8, 1.1 };

#endif

#ifdef CALIBRATION

/* main beam spill-over efficiencies for the various frontends */
/*                           555    495    572    549    119   */
static double etaML[5] = { 0.976, 0.968, 0.978, 0.975, 0.954 };
/* spill-over efficiency for cold sky beam                     */
static double etaMS  = 1.0;
/* spill-over efficiency for hot load beam                     */
static double etaMT  = 1.0;
/* hot load emissivity                                         */
static double epsr   = 1.0;
/* Rayleigh-Jeans temperature of spill-over radiation          */
static double Tspill = 290.0;

#endif
