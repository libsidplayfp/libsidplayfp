#ifndef SPLINE_H
#define SPLINE_H

namespace reSIDfp
{

typedef double (*Params)[6];

/** @internal
*/
class Spline {

private:
	double* c;
	const int paramsLength;
	Params params;

public:
	Spline(const double input[][2], const int inputLength);
	~Spline() { delete [] params; }

	void evaluate(const double x, double* out);
};

} // namespace reSIDfp

#endif
