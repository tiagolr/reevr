// Copyright 2025 tilr
#pragma once

#include <JuceHeader.h>
#include <cmath>
#include "../Utils.h"

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
		[] (double ratio) {
			constexpr double kMaxRads = 0.499 * juce::MathConstants<double>::pi;
			double scaled = ratio * juce::MathConstants<double>::pi;
			return std::tan(std::min(kMaxRads, scaled));
		},
		0.0, 0.5, 2048
	);

	Filter(FilterSlope slope, FilterMode mode) : slope(slope), mode(mode) {}
	~Filter(){}

	void setSlope(FilterSlope s) { slope = s; };
	void setMode(FilterMode m) { mode = m; };
	void init(double srate, double freq, double q);
	double eval(double sample);
	void reset(double sample);

	static constexpr double kMinNyquistMult = 0.48;
	inline static double getCoeff(double freq, double srate) {
		freq = jlimit(20.0, srate * kMinNyquistMult, freq);
		double ratio = jlimit(0.0, 0.5, freq / srate);
		return coeffLUT.cubic(ratio);
	}

private:
	double ic1 = 0.0;
	double ic2 = 0.0;
	double ic3 = 0.0;
	double ic4 = 0.0;

	double drive = 1.0;
	double idrive = 1.0;

	double g = 0.0;
	double k = 0.0;
	double a1 = 0.0;
	double a2 = 0.0;
	double a3 = 0.0;

	double b0 = 0.0;
	double b1 = 0.0;
	double b2 = 0.0;
	double x1 = 0.0;
	double x2 = 0.0;
	double y1 = 0.0;
	double y2 = 0.0;

	// 6db vars
	double state = 0.0;
};