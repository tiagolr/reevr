// Copyright 2025 tilr
// Incomplete port of Theo Niessink <theo@taletn.com> rbj filters
#pragma once

#include "JuceHeader.h"

class RBJ
{
public:
	RBJ() {};
	~RBJ() {};

	void lp(float srate, float freq, float q);
	void bp(float srate, float freq, float q);
	void hp(float srate, float freq, float q);
	void reset(float input);
	float df1(float sample);

private:
	float a1 = 0.0f;
	float a2 = 0.0f;
	float b0 = 0.0f;
	float b1 = 0.0f;
	float b2 = 0.0f;
	float x0 = 0.0f;
	float x1 = 0.0f;
	float y0 = 0.0f;
	float y1 = 0.0f;
};