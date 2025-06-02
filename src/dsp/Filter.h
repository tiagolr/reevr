// Copyright 2025 tilr
#pragma once

#include <JuceHeader.h>
#include <cmath>
#include "Utils.h"

enum FilterSlope {
	k6dB,
	k12dB,
	k24dB
};

enum FilterMode {
	LP,
	BP,
	HP,
};

class Filter
{
public:
	FilterSlope slope;
	FilterMode mode;

	inline static LookupTable coeffLUT = LookupTable(
		[] (float ratio) {
			constexpr float kMaxRads = 0.499f * juce::MathConstants<float>::pi;
			float scaled = ratio * juce::MathConstants<float>::pi;
			return std::tan(std::min(kMaxRads, scaled));
		},
		0.0f, 0.5f, 2048
	);

	Filter(FilterSlope slope, FilterMode mode) : slope(slope), mode(mode) {}
	~Filter(){}

	void setSlope(FilterSlope s) { slope = s; };
	void setMode(FilterMode m) { mode = m; };
	void init(float srate, float freq, float q = 0.0765f, float q2 = 0.6173 );
	float eval(float sample);
	void reset(float sample);

	static constexpr float kMinNyquistMult = 0.48f;
	inline static float getCoeff(float freq, float srate) {
		freq = jlimit(20.0f, srate * kMinNyquistMult, freq);
		float ratio = jlimit(0.0f, 0.5f, freq / srate);
		return (float)coeffLUT.cubic(ratio);
	}

private:
	float ic1 = 0.0f;
	float ic2 = 0.0f;
	float ic3 = 0.0f;
	float ic4 = 0.0f;

	float drive = 1.0f;
	float idrive = 1.0f;

	float g = 0.0f;
	float k = 0.0f;
	float k2 = 0.0f;
	float a1 = 0.0f;
	float a2 = 0.0f;
	float a3 = 0.0f;
	float a12 = 0.0f;
	float a22 = 0.0f;
	float a32 = 0.0f;

	float b0 = 0.0f;
	float b1 = 0.0f;
	float b2 = 0.0f;
	float x1 = 0.0f;
	float x2 = 0.0f;
	float y1 = 0.0f;
	float y2 = 0.0f;

	// 6db vars
	float state = 0.0f;
};