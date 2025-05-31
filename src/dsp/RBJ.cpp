#include "RBJ.h"
#include <cmath>

void RBJ::lp(float srate, float freq, float q)
{
	auto w0 = juce::MathConstants<float>::twoPi * fmin(freq / srate, 0.49f);
	auto alpha = sin(w0) / (2.0f*q);

	auto a0 = 1.0f + alpha;
	auto scale = 1.0f / a0;
	a1 = cos(w0) * -2.0f * scale;
	a2 = (1.0f - alpha) * scale;

	b2 = b0 = (1.0f + a1 + a2) * 0.25f;
	b1 = b0 * 2.0f;
}

void RBJ::bp(float srate, float freq, float q)
{
	auto w0 = juce::MathConstants<float>::twoPi * fmin(freq / srate, 0.49f);
	auto alpha = sin(w0) / (2.0f*q);

	auto a0 = 1.0f + alpha;
	auto scale = 1.0f/a0;
	a1 = cos(w0) * -2.0f * scale;
	a2 = (1 - alpha) * scale;

	b2 = -(b0 = (1 - a2) * 0.5f * q);
	b1 = 0.0f;
}

void RBJ::hp(float srate, float freq, float q)
{
	auto w0 = juce::MathConstants<float>::twoPi * fmin(freq / srate, 0.49f);
	auto alpha = sin(w0) / (2.0f*q);

	auto a0 = 1.0f + alpha;
	auto scale = 1.0f / a0;
	a1 = cos(w0) * -2.0f * scale;
	a2 = (1.0f - alpha) * scale;

	b2 = b0 = (1.0f - a1  + a2) * 0.25f;
	b1 = b0 * -2.0f;
}

void RBJ::reset(float input)
{
	x0 = x1 = input;
	y0 = y1 = input / (1.0f + a1 + a2) * (b0 + b1 + b2);
}

float RBJ::df1(float sample)
{
	auto x2 = x1;
	x1 = x0;
	x0 = sample;

	auto y2 = y1;
	y1 = y0;
	y0 = b0*x0 + b1*x1 + b2*x2 - a1*y1 - a2*y2;

	return y0;
}