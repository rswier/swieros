// libm.h

//ulong	getfcr(void);
//void	setfsr(ulong);
//ulong	getfsr(void);
//void	setfcr(ulong);
//double	NaN(void);
//double	Inf(int);
//int isnan(double);
//int	isInf(double, int);

double M_PI = 3.1415926535; // XXX

double atan2(double x, double y) { asm(LLD,8); asm(LBLD,16); asm(ATN2); }
double fabs (double x) { asm(LLD,8); asm(FABS); }
double atan (double x) { asm(LLD,8); asm(ATAN); }
double log  (double x) { asm(LLD,8); asm(LOG); }
double log10(double x) { asm(LLD,8); asm(LOGT); }
double exp  (double x) { asm(LLD,8); asm(EXP); }
double ceil (double x) { asm(LLD,8); asm(CEIL); }
double hypot(double x, double y) { asm(LLD,8); asm(LBLD,16); asm(HYPO); }
double sin  (double x) { asm(LLD,8); asm(SIN); }
double cos  (double x) { asm(LLD,8); asm(COS); }
double tan  (double x) { asm(LLD,8); asm(TAN); }
double asin (double x) { asm(LLD,8); asm(ASIN); }
double acos (double x) { asm(LLD,8); asm(ACOS); }
double sinh (double x) { asm(LLD,8); asm(SINH); }
double cosh (double x) { asm(LLD,8); asm(COSH); }
double tanh (double x) { asm(LLD,8); asm(TANH); }
double sqrt (double x) { asm(LLD,8); asm(SQRT); }

double round(double x) { return x; } // XXX

int abs(int n) { return (n < 0) ? -n : n; } // XXX keep here?

