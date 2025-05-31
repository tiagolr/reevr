#include "impulse.h"

class VectorAudioSource : public AudioSource
{
public:
    VectorAudioSource(const std::vector<float>& leftIn, const std::vector<float>& rightIn)
        : left(leftIn), right(rightIn) 
    {
        length = (int) std::min(left.size(), right.size());
        position = 0;
    }

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override
    {
        ignoreUnused(samplesPerBlockExpected, sampleRate);
        position = 0;
    }

    void releaseResources() override {}

    void getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill) override
    {
        int numSamples = bufferToFill.numSamples;
        int numChannels = bufferToFill.buffer->getNumChannels();

        for (int ch = 0; ch < numChannels; ++ch) {
            float* bufferData = bufferToFill.buffer->getWritePointer(ch, bufferToFill.startSample);

            for (int i = 0; i < numSamples; ++i) {
                if (position + i < length) {
                    if (ch == 0)
                        bufferData[i] = left[position + i];
                    else if (ch == 1)
                        bufferData[i] = right[position + i];
                    else
                        bufferData[i] = 0.0f; // silence for extra channels
                }
                else {
                    bufferData[i] = 0.0f; // zero padding after data ends
                }
            }
        }

        position += numSamples;
    }

    int getTotalLength() const { return length; }

private:
    const std::vector<float>& left;
    const std::vector<float>& right;
    int length;
    int position;
};

void Impulse::loadDefault()
{
    AudioFormatManager manager;
    manager.registerBasicFormats();
    std::unique_ptr<juce::InputStream> inputStream = std::make_unique<juce::MemoryInputStream>(
        BinaryData::Hall_Stereo_wav, 
        BinaryData::Hall_Stereo_wavSize, 
        false
    );
    std::unique_ptr<juce::AudioFormatReader> reader(manager.createReaderFor(std::move(inputStream)));
    if (reader == nullptr) {
        return;
    }
    name = "Default Hall";
    path = "";

    AudioBuffer<float> buf ((int)(reader->numChannels), (int)(reader->lengthInSamples));
    reader->read (buf.getArrayOfWritePointers(), buf.getNumChannels(), 0, buf.getNumSamples());
    srate = reader->sampleRate;
    bool isStereo = buf.getNumChannels() > 1;
    int nsamps = buf.getNumSamples();

    // trim IR tail silence
    int tailStart = std::max(
        getTailStart(buf.getReadPointer(0), nsamps), 
        getTailStart(buf.getReadPointer(isStereo ? 1 : 0), nsamps)
    );

    const float* data = buf.getReadPointer(0);
    rawBufferL.assign(data, data + tailStart);

    data = buf.getReadPointer(isStereo ? 1 : 0);
    rawBufferR.assign(data, data + tailStart);

    recalcImpulse();
}

void Impulse::recalcImpulse()
{
    bufferL = rawBufferL;
    bufferR = rawBufferR;

    if (bufferL.size() == 0 || bufferR.size() == 0) {
        jassertfalse;
        return;
    }

    size_t numSamples = rawBufferL.size();
    float autoGain = calculateAutoGain(bufferL, bufferR);

    peak = 0.f;
    for (int i = 0; i < numSamples; ++i) {
        bufferL[i] *= autoGain;
        bufferR[i] *= autoGain;
        peak = std::max(std::max(peak, std::fabs(bufferL[i])), std::fabs(bufferR[i]));
    }

    applyStretch();
    applyTrim();
    applyEnvelope();

    version += 1;
}

void Impulse::applyStretch()
{
    if (stretch == 0.f || !bufferL.size()) return;
    stretchsrate = std::pow(2, stretch) * srate;

    if (std::fabs(stretchsrate-srate) < 1e-6 || stretchsrate < 1.0 || srate < 1.0) 
        return;

    double ratio = srate / stretchsrate;
    VectorAudioSource source(bufferL, bufferR);
    ResamplingAudioSource resampler(&source, false);
    resampler.setResamplingRatio(ratio);

    const int blockSize = 8192;
    int inputLength = (int)bufferL.size();
    int outputLength = (int)std::ceil(inputLength * stretchsrate / srate);

    resampler.prepareToPlay(blockSize, stretchsrate);
    AudioBuffer<float> outputBuffer(2, outputLength);
    outputBuffer.clear();
    AudioSourceChannelInfo bufferToFill(outputBuffer);

    int samplesRemaining = outputLength;
    int outputPos = 0;

    while (samplesRemaining > 0) {
        int samplesThisTime = std::min(blockSize, samplesRemaining);

        bufferToFill.startSample = outputPos;
        bufferToFill.numSamples = samplesThisTime;
        resampler.getNextAudioBlock(bufferToFill);

        samplesRemaining -= samplesThisTime;
        outputPos += samplesThisTime;
    }

    resampler.releaseResources();

    bufferL.assign(outputBuffer.getReadPointer(0), outputBuffer.getReadPointer(0) + outputLength);
    bufferR.assign(outputBuffer.getReadPointer(1), outputBuffer.getReadPointer(1) + outputLength);
}

void Impulse::applyTrim()
{
    trimLeftSamples = 0;
    trimRightSamples = 0;
    size_t totalSamples = bufferL.size();
    size_t start = static_cast<size_t>(trimLeft * totalSamples);
    size_t end = totalSamples - static_cast<size_t>(trimRight * totalSamples);

    if (start >= end || start >= totalSamples || end > totalSamples) {
        bufferL.clear();
        bufferR.clear();
        return;
    }

    trimLeftSamples = (int)start;
    trimRightSamples = (int)(totalSamples - end);

    bufferL.erase(bufferL.begin() + end, bufferL.end());
    bufferL.erase(bufferL.begin(), bufferL.begin() + start);

    bufferR.erase(bufferR.begin() + end, bufferR.end());
    bufferR.erase(bufferR.begin(), bufferR.begin() + start);
}

void Impulse::applyEnvelope()
{
    auto size = (int)bufferL.size();
    if (!size) return;

    int attackSize = int(attack * size);
    int decaySize = int(decay * size);

    for (int i = 0; i < attackSize; ++i) {
        float gain = static_cast<float>(i) / static_cast<float>(attackSize);
        bufferL[i] *= gain;
        bufferR[i] *= gain;
    }

    for (int i = 0; i < decaySize; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(decaySize);
        float gain = 1.0f - (float)std::pow(t, 0.5);  // reverse exponential
        bufferL[size - decaySize + i] *= gain;
        bufferR[size - decaySize + i] *= gain;
    }
}

int Impulse::getTailStart(const float* data, int nsamples)
{
    for (int i = nsamples - 1; i >= 0; --i) {
        if (std::abs(data[i]) >= 1e-3)
            return i + 1;
    }
    return 0;
}

float Impulse::calculateAutoGain(const std::vector<float>& dataL, const std::vector<float>& dataR)
{
    int numSamples = (int)dataL.size();

    double energy = 0.0;
    for (int i = 0; i < numSamples; ++i) {
        double valL = static_cast<double>(dataL[i]);
        double valR = static_cast<double>(dataR[i]);
        energy += valL * valL + valR * valR;
    }

    if (energy > 0.0) {
        double gain = 1.0 / std::sqrt(energy);
        gain = std::min(gain, 1.0); // only reduce gain, no boost
        return static_cast<float>(gain);
    }

    return 1.0f; // Silence or zero energy
}