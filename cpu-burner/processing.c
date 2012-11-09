#include <math.h>

double cpu_burn(double x)
{
	int i, e = x < 0.5 ? 1000 : 2000;

	for (i = 0; i < e; i++) {
		if (i % 2)
			x += cos(x) + sin(x);
		else
			x += cos(x) + sqrt(x);
	}

	return x;
}