#ifndef RESAMPLER_H
#define RESAMPLER_H

#include <stdlib.h>

namespace reSIDfp
{

/** @internal
 * Abstraction of a resampling process. Given enough input, produces output.
 * Cnstructors take additional arguments that configure these objects.
 *
 * @author Antti Lankila
 */
class Resampler {

private:
	int oldRandomValue;

protected:
	int triangularDithering() {
		const int prevValue = oldRandomValue;
		oldRandomValue = rand() & 0x3ff;
		return oldRandomValue - prevValue;
	}

	virtual int output() const =0;

	Resampler() :
		oldRandomValue(0) {}

public:
	virtual ~Resampler() {}

	/**
	 * Input a sample into resampler. Output "true" when resampler is ready with new sample.
	 *
	 * @param sample
	 * @return true when a sample is ready
	 */
	virtual bool input(const int sample)=0;

	/**
	 * Output a sample from resampler
	 *
	 * @return resampled sample
	 */
	short getOutput() {
		const int dither = triangularDithering();
		int value = (output() * 1024 + dither) >> 10;
		if (value > 32767) {
			value = 32767;
		}
		if (value < -32768) {
			value = -32768;
		}
		return (short)value;
	}

	virtual void reset()=0;
};

} // namespace reSIDfp

#endif
