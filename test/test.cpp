
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include <sidplayfp/sidplay2.h>
#include <sidplayfp/SidTune.h>
#include <builders/residfp-builder/residfp.h>

int main(int argc, char *argv[]) {

	sidplay2	m_engine;
	ReSIDfpBuilder *rs = new ReSIDfpBuilder("Test");

	char name[0x100] = PC64_TESTSUITE;
	if (argc>1) {
		strcat (name, argv[1]);
		strcat (name, ".prg");
	} else {
        	strcat (name, " start.prg");
	}

	SidTune		*tune=new SidTune(name);

	if (!tune->getStatus()) {
		printf("Error: %s\n", tune->getInfo().statusString);
		goto error;
	}
	rs->create(2);

	sid2_config_t cfg;
	cfg=m_engine.config();
	cfg.clockForced=false;
	cfg.clockSpeed=SID2_CLOCK_CORRECT;
	cfg.frequency=48000;
	cfg.samplingMethod=SID2_INTERPOLATE;
	cfg.fastSampling=false;
	cfg.playback=sid2_stereo;
	cfg.sidEmulation=rs;
	cfg.sidModel=SID2_MODEL_CORRECT;
	cfg.sidSamples=true;
	m_engine.config(cfg);

	tune->selectSong(0);
	m_engine.load(tune);
{
	const int temp=48000;
	short buffer[temp];
	for (;;) {
		m_engine.play(buffer, temp);
	}
}
error:
	delete tune;
	delete rs;
}
