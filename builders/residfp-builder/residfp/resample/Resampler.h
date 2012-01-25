#ifndef RESAMPLER_H
#define RESAMPLER_H

namespace reSIDfp
{

/** @internal
 * Abstraction of a resampling process. Given enough input, produces output.
 * Cnstructors take additional arguments that configure these objects.
 *
 * @author Antti Lankila
 */
class Resampler {

protected:
	virtual int output() const =0;

	Resampler() {}

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
		int value = output();
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
