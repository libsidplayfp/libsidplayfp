#ifndef OPAMP_H
#define OPAMP_H

#include "Spline.h"

namespace reSIDfp
{

/** @internal
 * This class solves the opamp equation when loaded by different sets of resistors.
 * Equations and first implementation were written by Dag Lem.
 * This class is a rewrite without use of fixed point integer mathematics, and
 * uses the actual voltages instead of the normalized values.
 *
 * @author alankila
 */
class OpAmp {

private:
	static const double EPSILON;

	/** Current root position (cached as guess to speed up next iteration) */
	double x;

	const double Vddt, vmin, vmax;

	Spline* opamp;

	double out[2];

public:
	/**
	 * Opamp input -> output voltage conversion
	 *
	 * @param opamp opamp mapping table as pairs of points (in -> out)
	 * @param opamplength length of the opamp array
	 * @param Vddt transistor dt parameter (in volts)
	 */
	OpAmp(const double opamp[][2], const int opamplength, const double Vddt) :
		x(0.),
		Vddt(Vddt),
		vmin(opamp[0][0]),
		vmax(opamp[opamplength - 1][0]),
		opamp(new Spline(opamp, opamplength)) {}

	~OpAmp() { delete opamp; }

	void reset() {
		x = vmin;
	}

	/**
	 * Solve the opamp equation for input vi in loading context n
	 * 
	 * @param n the ratio of input/output loading
	 * @param vi input
	 * @return vo
	 */
	double solve(const double n, const double vi);
};

} // namespace reSIDfp

#endif
