// Copyright 2025 tilr

#pragma once

#include <JuceHeader.h>
#include "Convolver.h"
#include "Impulse.h"

class StereoConvolver
{
public:
    StereoConvolver() 
        : convolverLL(new Convolver())
        , convolverRR(new Convolver()) 
        , convolverLR(new Convolver())
        , convolverRL(new Convolver())
        {}
    ~StereoConvolver() {}
    
    void loadImpulse(Impulse imp);
    void prepare(int samplesPerBlock);
    void process(const float* data0, const float* data1, size_t nsamples, bool force2Chans = false);
    void reset();
    void clear();
    bool finishedLoading();

    std::vector<float> bufferLL = {};
    std::vector<float> bufferRR = {};
    std::vector<float> bufferLR = {};
    std::vector<float> bufferRL = {};
    int size = 0;
    bool isQuad = false;

protected:
    size_t headBlockSize = 0;
    size_t tailBlockSize = 0;

private:
    std::unique_ptr<Convolver> convolverLL;
    std::unique_ptr<Convolver> convolverRR;
    std::unique_ptr<Convolver> convolverLR;
    std::unique_ptr<Convolver> convolverRL;
};