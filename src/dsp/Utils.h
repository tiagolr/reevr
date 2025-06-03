#pragma once

#include <vector>
#include <cmath>
#include <functional>
#include <algorithm>
#include "../Globals.h"

using namespace globals;

class Utils
{
public:
    inline static double normalToFreq(double norm)
    {
        return F_MIN_FREQ * std::exp(norm * std::log(F_MAX_FREQ / F_MIN_FREQ));
    }

    inline static double freqToNormal(double norm)
    {
        return std::log(norm / F_MIN_FREQ) / std::log(F_MAX_FREQ / F_MIN_FREQ);
    }

    inline static double gainTodB(double gain)
    {
        return gain == 0 ? -60.0 : 20.0 * std::log10(gain);
    }

    static float normalToFreqf(float min, float max, float norm)
    {
        return min * std::exp(norm * std::log(max / min));
    }

    static float freqToNormalf(float min, float max, float norm)
    {
        return std::log(norm / min) / std::log(max / min);
    }
};


/*
* A copy of LookupTableTransform with additional cubic interpolation
*/
class LookupTable
{
public:
    LookupTable() = default;

    template <typename Func>
    LookupTable(Func fn, float min_, float max_, size_t size_)
    {
        init(fn, min_, max_, size_);
    }

    template <typename Func>
    void init(Func fn, float min_, float max_, size_t size_)
    {
        if (max_ <= min_) throw std::invalid_argument("max must be greater than min");
        if (size_ < 2) throw std::invalid_argument("size must be at least 2");
        min = min_;
        max = max_;
        size = size_;
        values.resize(size);

        scaler = (size > 1) ? (size - 1) / (max - min) : 0.0f;
        offset = -min * scaler;

        for (size_t i = 0; i < size; ++i) {
            float x = static_cast<float>(i) / (size - 1); // Normalized [0, 1]
            float mappedX = min + x * (max - min);
            mappedX = std::clamp(mappedX, min, max);
            values[i] = fn(mappedX);
        }
    }

    inline float operator()(float input) const
    {
        input = std::clamp(input, min, max);
        float normalizedIndex = input * scaler + offset;
        size_t index = static_cast<size_t>(std::floor(normalizedIndex));

        if (index >= size - 1)
            return values.back();

        float frac = normalizedIndex - index;
        return values[index] + frac * (values[index + 1] - values[index]);
    }

    inline float cubic(float input) const
    {
        input = std::clamp(input, min, max);
        float index = input * scaler + offset;
        int i = (int)index;
        float t = index - i;

        int i0 = std::max(0, i - 1);
        int i1 = i;
        int i2 = std::min((int)size - 1, i + 1);
        int i3 = std::min((int)size - 1, i + 2);

        float y0 = values[i0];
        float y1 = values[i1];
        float y2 = values[i2];
        float y3 = values[i3];

        float a0 = y3 - y2 - y0 + y1;
        float a1 = y0 - y1 - a0;
        float a2 = y2 - y0;
        float a3 = y1;

        return (a0 * t * t * t) + (a1 * t * t) + (a2 * t) + a3;
    }

    const std::vector<float>& getValues() const { return values; }
    size_t getSize() const { return size; }
    float getMin() const { return min; }
    float getMax() const { return max; }

private:
    std::vector<float> values;
    float min = 0.0f;
    float max = 1.0f;
    float scaler = 0.0f;
    float offset = 0.0f;
    size_t size = 0;
};