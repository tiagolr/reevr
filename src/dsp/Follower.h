// Copyright 2025 tilr
// Envelope Follower
#pragma once
#include "JuceHeader.h"
#include "../Globals.h"
#include "filter/RBJ.h"

using namespace globals;

class Follower
{
public:
	Follower() {};
	~Follower() {};

	void prepare(double srate, double thresh_, bool autorel_, double attack_, double hold, double release, double lowcutfreq, double highcutfreq);
	double process(double lsamp, double rsamp);
	void clear();

	double outl = 0.0;
	double outr = 0.0;

private:
	RBJ lowcutL;
	RBJ highcutL;
	RBJ lowcutR;
	RBJ highcutR;
	int rmswindow = 100;
	double thresh = 0.0;
	bool autorel = false;
	double attack = 1.0; // s
	double hold = 0.0; // s
	double release = 1.0; // s
	double attackcoeff = .1;
	double releasecoeff = .1;
	double minreleasecoeff = .1;
	double envelope = 0.0;
};