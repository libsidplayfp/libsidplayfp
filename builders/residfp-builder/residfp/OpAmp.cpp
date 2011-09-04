#include "OpAmp.h"

#include <math.h>

namespace reSIDfp
{

const double OpAmp::EPSILON = 1e-8;

double OpAmp::solve(const double n, const double vi) {
	// Start off with an estimate of x and a root bracket [ak, bk].
	// f is decreasing, so that f(ak) > 0 and f(bk) < 0.
	double ak = vmin;
	double bk = vmax;

	const double a = n + 1.;
	const double b = Vddt;
	const double b_vi = (b - vi);
	const double c = n * (b_vi * b_vi);

	while (true) {
		double xk = x;

		// Calculate f and df.
		opamp->evaluate(x, out);
		const double vo = out[0];
		const double dvo = out[1];

		const double b_vx = b - x;
		const double b_vo = b - vo;

		const double f = a * (b_vx * b_vx) - c - (b_vo * b_vo);
		const double df = 2. * (b_vo * dvo - a * b_vx);

		x -= f / df;
		if (fabs(x - xk) < EPSILON) {
			opamp->evaluate(x, out);
			return out[0];
		}

		// Narrow down root bracket.
		if (f < 0.) {
			// f(xk) < 0
			bk = xk;
		}
		else {
			// f(xk) > 0
			ak = xk;
		}

		if (x <= ak || x >= bk) {
			// Bisection step (ala Dekker's method).
			x = (ak + bk) * 0.5;
		}
	}
}

} // namespace reSIDfp
