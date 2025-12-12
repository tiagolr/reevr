// Copyright C 2025 Tilr

#pragma once

#include "JuceHeader.h"

class Impulse
{
public:
	Impulse() {}
	~Impulse() {}

	void load(String path);
	void recalcImpulse();

	std::vector<float> bufferLL = {};
	std::vector<float> bufferLR = {};
	std::vector<float> bufferRR = {};
	std::vector<float> bufferRL = {};
	std::vector<float> rawBufferLL = {};
	std::vector<float> rawBufferLR = {};
	std::vector<float> rawBufferRR = {};
	std::vector<float> rawBufferRL = {};
	
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
	bool reverse = false;
	bool isQuad = false;
	unsigned long int version = 1;

private:
	float calculateAutoGain(const std::vector<float>& dataL, const std::vector<float>& dataR);
	int getTailStart(const float* data, int nsamples);
	void applyStretch(std::vector<float>& bufL, std::vector<float>& bufR);
	void applyTrim();
	void applyEnvelope();
};