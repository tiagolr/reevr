#include "Impulse.h"

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

// ==============================================

void Impulse::load(String filepath)
{
    AudioFormatManager manager;
    manager.registerBasicFormats();
    std::unique_ptr<juce::InputStream> inputStream;
    bool isDefault = filepath.isEmpty();
    
    if (isDefault) {
        inputStream = std::make_unique<juce::MemoryInputStream>(
            BinaryData::Hall_Quad_flac, 
            BinaryData::Hall_Quad_flacSize, 
            false
        );
        name = "Default";
        path = "";
    }
    else {
        File audioFile(filepath);
        if (!audioFile.existsAsFile()) {
            return load("");
        }
        name = audioFile.getFileNameWithoutExtension().toStdString();
        path = filepath.toStdString();
        inputStream = audioFile.createInputStream();
    }

    std::unique_ptr<juce::AudioFormatReader> reader(manager.createReaderFor(std::move(inputStream)));
    if (reader == nullptr) {
        if (isDefault) 
            throw std::runtime_error("Failed to load default IR");
        else 
            return load("");
    }

    try {
        AudioBuffer<float> buf ((int)(reader->numChannels), (int)(reader->lengthInSamples));
        reader->read (buf.getArrayOfWritePointers(), buf.getNumChannels(), 0, buf.getNumSamples());
        srate = reader->sampleRate;
        auto nchans = buf.getNumChannels();
        int nsamps = buf.getNumSamples();
        if (nsamps == 0) throw "Load default impulse";

        // trim IR tail silence
        int tailStart = 0;
        for (int i = 0; i < nchans; ++i) {
            tailStart = std::max(tailStart, getTailStart(buf.getReadPointer(i), nsamps));
        }

        isQuad = nchans >= 4;
        const float* data = buf.getReadPointer(0);
        rawBufferLL.assign(data, data + tailStart);

        if (isQuad) {
            data = buf.getReadPointer(1);
            rawBufferLR.assign(data, data + tailStart);

            data = buf.getReadPointer(2);
            rawBufferRL.assign(data, data + tailStart);

            data = buf.getReadPointer(3);
            rawBufferRR.assign(data, data + tailStart);
        }
        else if (nchans == 2) {
            data = buf.getReadPointer(1);
            rawBufferRR.assign(data, data + tailStart);
        }
        else {
            data = buf.getReadPointer(0);
            rawBufferRR.assign(data, data + tailStart);
        }

        recalcImpulse();
    }
    catch (...) {
        isQuad = false;
        if (isDefault)
            throw std::runtime_error("Failed to load default IR");
        else 
            return load("");
    }
}

void Impulse::recalcImpulse()
{
    bufferLL = rawBufferLL;
    bufferRR = rawBufferRR;

    if (isQuad) {
        bufferLR = rawBufferLR;
        bufferRL = rawBufferRL;
    }

    if (bufferLL.size() == 0 || bufferRR.size() == 0) {
        jassertfalse;
        return;
    }

    size_t numSamples = rawBufferLL.size();
    float autoGain = calculateAutoGain(bufferLL, bufferRR);

    peak = 0.f;
    for (int i = 0; i < numSamples; ++i) {
        bufferLL[i] *= autoGain;
        bufferRR[i] *= autoGain;
        peak = std::max(std::max(peak, std::fabs(bufferLL[i])), std::fabs(bufferRR[i]));

        if (isQuad) {
            bufferLR[i] *= autoGain;
            bufferRL[i] *= autoGain;
            peak = std::max(std::max(peak, std::fabs(bufferLR[i])), std::fabs(bufferRL[i]));
        }
    }

    if (reverse) {
        std::reverse(bufferLL.begin(), bufferLL.end());
        std::reverse(bufferRR.begin(), bufferRR.end());

        if (isQuad) {
            std::reverse(bufferLR.begin(), bufferLR.end());
            std::reverse(bufferRL.begin(), bufferRL.end());
        }
    }

    applyStretch(bufferLL, bufferRR);
    if (isQuad) applyStretch(bufferLR, bufferRL);

    applyTrim();
    applyEnvelope();

    version += 1;
}

void Impulse::applyStretch(std::vector<float>& bufL, std::vector<float>& bufR)
{
    if (stretch == 0.f || !bufL.size()) return;
    stretchsrate = std::pow(2, stretch) * srate;

    if (std::fabs(stretchsrate-srate) < 1e-6 || stretchsrate < 1.0 || srate < 1.0) 
        return;

    double ratio = srate / stretchsrate;
    VectorAudioSource source(bufL, bufR);
    ResamplingAudioSource resampler(&source, false);
    resampler.setResamplingRatio(ratio);

    const int blockSize = 8192;
    int inputLength = (int)bufL.size();
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

    bufL.assign(outputBuffer.getReadPointer(0), outputBuffer.getReadPointer(0) + outputLength);
    bufR.assign(outputBuffer.getReadPointer(1), outputBuffer.getReadPointer(1) + outputLength);
}

void Impulse::applyTrim()
{
    trimLeftSamples = 0;
    trimRightSamples = 0;
    size_t totalSamples = bufferLL.size();
    size_t start = static_cast<size_t>(trimLeft * totalSamples);
    size_t end = totalSamples - static_cast<size_t>(trimRight * totalSamples);

    if (start >= end || start >= totalSamples || end > totalSamples) {
        bufferLL.clear();
        bufferRR.clear();
        if (isQuad) {
            bufferLR.clear();
            bufferRL.clear();
        }

        return;
    }

    trimLeftSamples = (int)start;
    trimRightSamples = (int)(totalSamples - end);

    bufferLL.erase(bufferLL.begin() + end, bufferLL.end());
    bufferLL.erase(bufferLL.begin(), bufferLL.begin() + start);

    bufferRR.erase(bufferRR.begin() + end, bufferRR.end());
    bufferRR.erase(bufferRR.begin(), bufferRR.begin() + start);

    if (isQuad) {
        bufferLR.erase(bufferLR.begin() + end, bufferLR.end());
        bufferLR.erase(bufferLR.begin(), bufferLR.begin() + start);

        bufferRL.erase(bufferRL.begin() + end, bufferRL.end());
        bufferRL.erase(bufferRL.begin(), bufferRL.begin() + start);
    }
}

void Impulse::applyEnvelope()
{
    auto size = (int)bufferLL.size();
    if (!size) return;

    int attackSize = int(attack * size);
    int decaySize = int(decay * size);

    for (int i = 0; i < attackSize; ++i) {
        float gain = static_cast<float>(i) / static_cast<float>(attackSize);
        bufferLL[i] *= gain;
        bufferRR[i] *= gain;
        if (isQuad) {
            bufferLR[i] *= gain;
            bufferRL[i] *= gain;
        }
    }

    for (int i = 0; i < decaySize; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(decaySize);
        float gain = 1.0f - (float)std::pow(t, 0.5);  // reverse exponential
        auto idx = size - decaySize + i;
        bufferLL[idx] *= gain;
        bufferRR[idx] *= gain;
        if (isQuad) {
            bufferLR[idx] *= gain;
            bufferRL[idx] *= gain;
        }
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