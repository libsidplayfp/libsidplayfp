#ifndef FILTERMODELCONFIG_H
#define FILTERMODELCONFIG_H

#define OPAMP_SIZE 22
#define DAC_SIZE 11

#include <memory>

namespace reSIDfp
{

class Integrator;

/** @internal
*/
class FilterModelConfig {

private:
	static std::auto_ptr<FilterModelConfig> instance;
	// This allows access to the private constructor
	friend class std::auto_ptr<FilterModelConfig>;

	static const double opamp_voltage[OPAMP_SIZE][2];

	const double voice_voltage_range;
	const double voice_DC_voltage;

	// Capacitor value.
	const double C;

	// Transistor parameters.
	const double Vdd;
	const double Vth;			// Threshold voltage
	const double uCox_vcr;			// 1/2*u*Cox
	const double WL_vcr;			// W/L for VCR
	const double uCox_snake;		// 1/2*u*Cox
	const double WL_snake;			// W/L for "snake"

	// DAC parameters.
	const double dac_zero;
	const double dac_scale;
	const double dac_2R_div_R;
	const bool dac_term;

	/* Derived stuff */
	const double vmin, norm;
	double opamp_working_point;
	unsigned short* mixer[8];
	unsigned short* summer[7];
	unsigned short* gain[16];
	double dac[DAC_SIZE];
	unsigned short vcr_Vg[1 << 16];
	unsigned short vcr_n_Ids_term[1 << 16];
	int opamp_rev[1 << 16];

	double evaluateTransistor(const double Vw, const double vi, const double vx);

	FilterModelConfig();
	~FilterModelConfig();

public:
	static FilterModelConfig* getInstance();

	double getDacZero(const double adjustment) const;

	int getVO_T16() const;

	int getVoiceScaleS14() const;

	int getVoiceDC() const;

	unsigned short** getGain() { return gain; }

	unsigned short** getSummer() { return summer; }

	unsigned short** getMixer() { return mixer; }

	/**
	 * Make DAC
	 * must be deleted
	 */
	unsigned int* getDAC(const double dac_zero) const;

	Integrator* buildIntegrator();

	/**
	 * Estimate the center frequency corresponding to some FC setting.
	 *
	 * FIXME: this function is extremely sensitive to prevailing voltage offsets.
	 * They got to be right within about 0.1V, or the results will be simply wrong.
	 * This casts doubt on the feasibility of this approach. Perhaps the offsets
	 * at the integrators would need to be statically solved first for 1-voice null
	 * input.
	 *
	 * @param dac_zero
	 * @param fc
	 * @return frequency in Hz
	 */
	double estimateFrequency(const double dac_zero, const int fc);
};

} // namespace reSIDfp

#endif
