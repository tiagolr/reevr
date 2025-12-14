// Copyright 2025 tilr
// Port of Theo Niessink <theo@taletn.com> zdf filters
#pragma once
#include "JuceHeader.h"
#include <complex>

class SVF
{
public:
	enum Mode {
		LP,
		BP,
		HP,
		LS,
		HS,
		PK,
		Off,
	};

	enum EQType {
		PostEQ,
		DecayEQ
	};

	struct EQBand {
		Mode mode;
		float freq;
		float q;
		float gain;
	};

	SVF() {}
	~SVF() {}

	void lp(float srate, float freq, float q);
	void bp(float srate, float freq, float q);
	void hp(float srate, float freq, float q);
	void ls(float srate, float freq, float q, float gain);
	void hs(float srate, float freq, float q, float gain);
	void pk(float srate, float freq, float q, float gain);
	void clear(float input);
	float process(float input);
	void processBlock(float* buf, int nsamples, int blockoffset, int blocksize, float tfreq, float tq, float tgain = 1.f);
	float getMagnitude(float freq);

	Mode mode = LP;
	float srate = 44100.0f;
	float freq = 0.0f;
	float gain = 1.f;
	float q = 0.0f;

private:
	void setup(float srate, float freq, float q, float resfactor = 1.f);

	// states
    float s1 = 0.0f;
    float s2 = 0.0f;

    // coeffs
	float g = 0.0f;
	float r2 = 0.0f;
    float a1 = 0.0f;
    float a2 = 0.0f;
    float a3 = 0.0f;
	float a1_step = 0.0f;
	float a2_step = 0.0f;
	float a3_step = 0.0f;
	float r2_step = 0.0f;
	float cl_step = 0.0f;
	float cb_step = 0.0f;
	float ch_step = 0.0f;

    // output
    float cl = 1.0f; // low-pass
    float cb = 0.0f; // band-pass
    float ch = 0.0f; // high-pass
};