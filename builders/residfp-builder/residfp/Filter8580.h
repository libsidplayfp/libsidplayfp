#ifndef FILTER8580_H
#define FILTER8580_H

#include "siddefs-fp.h"

#include "Filter.h"

namespace reSIDfp
{

/** @internal
 * Filter for 8580 chip based on simple linear approximation
 * of the FC control.
 *
 * This is like the original reSID filter except the phase
 * of BP output has been inverted. I saw samplings on the internet
 * that indicated it would genuinely happen like this.
 *
 * @author Ken Händel
 * @author Dag Lem
 * @author Antti Lankila
 * @author Leandro Nini
 */
class Filter8580 : public Filter {

private:
	double highFreq;
	float Vlp, Vbp, Vhp;
	float ve, w0, _1_div_Q;

public:
	Filter8580() :
		highFreq(12500.),
		Vlp(0.f),
		Vbp(0.f),
		Vhp(0.f),
		ve(0.f),
		w0(0.f),
		_1_div_Q(0.f) {}

	int clock(const int voice1, const int voice2, const int voice3);

	void updatedCenterFrequency();

	void updatedResonance();

	void input(const int input);

	void updatedMixing() {}

	void setFilterCurve(const double curvePosition);
};

} // namespace reSIDfp

#if RESID_INLINING || defined(FILTER8580_CPP)

#include <stdlib.h>
#include <math.h>

namespace reSIDfp
{

RESID_INLINE
int Filter8580::clock(const int v1, const int v2, const int v3) {
	const int voice1 = v1 >> 7;
	const int voice2 = v2 >> 7;
	const int voice3 = v3 >> 7;

	int Vi = 0;
	float Vo = 0.f;
	if (filt1) {
		Vi += voice1;
	} else {
		Vo += voice1;
	}
	if (filt2) {
		Vi += voice2;
	} else {
		Vo += voice2;
	}
	// NB! Voice 3 is not silenced by voice3off if it is routed
	// through the filter.
	if (filt3) {
		Vi += voice3;
	} else if (!voice3off) {
		Vo += voice3;
	}
	if (filtE) {
		Vi += (int)ve;
	} else {
		Vo += ve;
	}

	const float dVbp = w0 * Vhp;
	const float dVlp = w0 * Vbp;
	Vbp -= dVbp;
	Vlp -= dVlp;
	Vhp = (Vbp*_1_div_Q) - Vlp - Vi + float(rand())/float(RAND_MAX);

	if (lp) {
		Vo += Vlp;
	}
	if (bp) {
		Vo += Vbp;
	}
	if (hp) {
		Vo += Vhp;
	}

	return (int) Vo * vol >> 4;
}

} // namespace reSIDfp

#endif

#endif
