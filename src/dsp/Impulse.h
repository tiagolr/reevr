// Copyright C 2025 Tilr

#pragma once

#include "JuceHeader.h"

class Impulse
{
public:
	Impulse() {}
	~Impulse() {}

	void loadDefault();
	void recalcImpulse();

	std::vector<float> bufferL = {};
	std::vector<float> bufferR = {};
	std::vector<float> rawBufferL = {};
	std::vector<float> rawBufferR = {};
	
	std::string name = "";
	std::string path = "";

	float peak = 0.0f; // used for drawing the impulse
	int trimLeftSamples = 0; // used for drawing
	int trimRightSamples = 0; // used for drawing

	double srate = 0.0;
	double stretchsrate = 0.0; // stretch samplerate
	float attack = 0.0f;
	float decay = 1.0;
	float trimLeft = 0.0;
	float trimRight = 0.0;
	float stretch = 0.0;
	unsigned long int version = 1;

private:
	float calculateAutoGain(const std::vector<float>& dataL, const std::vector<float>& dataR);
	int getTailStart(const float* data, int nsamples);
	void applyStretch();
	void applyTrim();
	void applyEnvelope();
};