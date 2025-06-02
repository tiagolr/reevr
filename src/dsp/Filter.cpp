#include "Filter.h"

void Filter::init(float srate, float freq, float q, float q2)
{
    g = getCoeff(freq, srate);
    k = 2 - 2*q;
    k2 = 2 - 2*q2;

    if (slope == k6dB) {
        g = g / (1.0f + g);
    }
    else {
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;

        a12 = 1.0f / (1.0f + g * (g + k2));
        a22 = g * a12;
        a32 = g * a22;
    }
}

float Filter::eval(float sample)
{

    if (slope == k6dB) {
        float delta = g * (sample - state);
        state += delta;
        float low = state;
        return mode == LP
            ? state
            : sample - low;
    }

    // 12p first stage
    auto v3 = sample - ic2;
    auto v1 = a1 * ic1 + a2 * v3; // band
    auto v2 = ic2 + a2 * ic1 + a3 * v3; // low
    ic1 = 2.0f * v1 - ic1;
    ic2 = 2.0f * v2 - ic2;

    float output = 0.0f;
    if (mode == LP) output = v2;
    else if (mode == BP) output = v1;
    else output = sample - k * v1 - v2;

    if (slope == k12dB) 
        return output;

    // 24p second stage
    v3 = output - ic4;
    v1 = a12 * ic3 + a22 * v3;
    v2 = ic4 + a22 * ic3 + a32 * v3;
    ic3 = 2.0f * v1 - ic3;
    ic4 = 2.0f * v2 - ic4;

    if (mode == LP) output = v2;
    else if (mode == BP) output = v1;
    else output = output - k2 * v1 - v2;

    return output;
}

void Filter::reset(float sample)
{
    ic1 = ic2 = ic3 = ic4 = sample;
    state = sample;
}