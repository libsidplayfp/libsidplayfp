#ifndef SPLINE_H
#define SPLINE_H

namespace reSIDfp
{

typedef double (*Params)[6];

/** @internal
*/
class Spline {

private:
	const int paramsLength;
	Params params;
	double* c;

public:
	Spline(const double input[][2], const int inputLength);
	~Spline() { delete [] params; }

	void evaluate(const double x, double* out);
};

} // namespace reSIDfp

#endif
