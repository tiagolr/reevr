// Copyright 2025 tilr

#pragma once

#include <JuceHeader.h>
#include "Convolver.h"
#include "Impulse.h"

class StereoConvolver
{
public:
    StereoConvolver() : convolverL(new Convolver()), convolverR(new Convolver()) {}
    ~StereoConvolver() {}
    
    void loadImpulse(Impulse imp);
    void prepare(int samplesPerBlock);
    void process(const float* data0, const float* data1, size_t nsamples);
    void reset();
    void clear();
    bool finishedLoading();

    std::vector<float> bufferL = {};
    std::vector<float> bufferR = {};
    int size = 0;

protected:
    size_t headBlockSize = 0;
    size_t tailBlockSize = 0;

private:
    std::unique_ptr<Convolver> convolverL;
    std::unique_ptr<Convolver> convolverR;
};