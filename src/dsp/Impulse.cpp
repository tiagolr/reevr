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

Impulse::Impulse() : _fft()
{
    window.resize(FFT_SIZE);
    re.resize(FFT_SIZE);
    im.resize(FFT_SIZE);
    _fft.init(FFT_SIZE);

    const float w = 2.0f * MathConstants<float>::pi / FFT_SIZE;
    for (int i = 0; i < FFT_SIZE / 2; ++i)
        window[i] = 0.42f - 0.50f * std::cos(i * w) + 0.08f * std::cos(2.0f * i * w);
    for (size_t i = FFT_SIZE / 2 + 1; i < FFT_SIZE; ++i)
        window[i] = window[FFT_SIZE - 1 - i];
}

void Impulse::prepare(double _srate)
{
    srate = _srate;
}

void Impulse::setDecayEQ(std::vector<SVF::EQBand> eq)
{
    decayEQ = eq;
}

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
        bool hasMatch = false; // trueStereo matching file found
        AudioBuffer<float> buf ((int)(reader->numChannels), (int)(reader->lengthInSamples));
        AudioBuffer<float> buf2((int)(reader->numChannels), (int)(reader->lengthInSamples)); // true stereo match buffer
        reader->read (buf.getArrayOfWritePointers(), buf.getNumChannels(), 0, buf.getNumSamples());
        irsrate = reader->sampleRate;
        nfiles = 1;
        auto nchans = buf.getNumChannels();
        int nsamps = buf.getNumSamples();
        if (nsamps == 0) throw "Load default impulse";

        // find and load true stereo match file
        if (nchans == 2) {
            auto match = findTrueStereoPair(filepath, nsamps, irsrate);
            if (match.path.isNotEmpty()) {
                auto f2 = File(match.path);
                std::unique_ptr<juce::InputStream> inputStream2 = f2.createInputStream();
                std::unique_ptr<juce::AudioFormatReader> reader2(manager.createReaderFor(std::move(inputStream2)));
                if (reader2 != nullptr) {
                    reader->read(buf2.getArrayOfWritePointers(), buf2.getNumChannels(), 0, buf2.getNumSamples());
                    hasMatch = true;
                    if (!match.swapChannels) {
                        std::swap(buf, buf2);
                        float* left = buf.getWritePointer(0);
                        float* right = buf.getWritePointer(1);
                        for (int i = 0; i < nsamps; ++i) {
                            std::swap(left[i], right[i]);
                        }
                        left = buf2.getWritePointer(0);
                        right = buf2.getWritePointer(1);
                        for (int i = 0; i < nsamps; ++i) {
                            std::swap(left[i], right[i]);
                        }
                    }
                }
            }
        }

        // trim IR tail silence
        int tailStart = 0;
        for (int i = 0; i < nchans; ++i) {
            tailStart = std::max(tailStart, getTailStart(buf.getReadPointer(i), nsamps));
        }

        isQuad = nchans >= 4;
        const float* data = buf.getReadPointer(0);
        rawBufferLL.assign(data, data + tailStart);

        if (isQuad) {
            numChans = 4;
            data = buf.getReadPointer(1);
            rawBufferLR.assign(data, data + tailStart);

            data = buf.getReadPointer(2);
            rawBufferRL.assign(data, data + tailStart);

            data = buf.getReadPointer(3);
            rawBufferRR.assign(data, data + tailStart);
        }
        else if (nchans == 2 && hasMatch) {
            isQuad = true;
            nfiles = 2;
            numChans = 4;

            data = buf.getReadPointer(1);
            rawBufferLR.assign(data, data + tailStart);

            data = buf2.getReadPointer(0);
            rawBufferRL.assign(data, data + tailStart);

            data = buf2.getReadPointer(1);
            rawBufferRR.assign(data, data + tailStart);
        }
        else if (nchans == 2) {
            numChans = 2;
            data = buf.getReadPointer(1);
            rawBufferRR.assign(data, data + tailStart);
        }
        else {
            numChans = 1;
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

Impulse::TSMatch Impulse::findTrueStereoPair(String fpath, int nsamps, double _irsrate) const
{
    auto file = File(fpath);
    if (!file.existsAsFile() || file.isDirectory())
        return {};

    auto dir = file.getParentDirectory();
    auto fname = file.getFileNameWithoutExtension();
    auto ext = file.getFileExtension();

    std::pair<String, String> lr[] = {
        {"L", "R"},
        {"l", "r"},
        {"Left", "Right"},
        {"LEFT", "RIGHT"},
        {"left", "right"}
    };

    for (auto& pair : lr) {
        auto f = findPair(fname, ext, dir, pair.first, pair.second, nsamps, _irsrate);
        if (f.existsAsFile() && !f.isDirectory()) {
            TSMatch match;
            match.path = f.getFullPathName();
            match.swapChannels = true;
            return match;
        }
    }

    for (auto& pair : lr) {
        auto f = findPair(fname, ext, dir, pair.second, pair.first, nsamps, _irsrate);
        if (f.existsAsFile() && !f.isDirectory()) {
            TSMatch match;
            match.path = f.getFullPathName();
            match.swapChannels = false;
            return match;
        }
    }
    
    return {};
}

File Impulse::findPair(const juce::String& fileNameBody,
    const String& fileNameExt,
    const File& directory,
    const String& pattern,
    const String& replacement,
    const size_t sampleCount,
    const double sampleRate) const
{
    std::vector<juce::String> candidateNames;
    if (fileNameBody.startsWith(pattern))
    {
        candidateNames.push_back(replacement + fileNameBody.substring(pattern.length(), fileNameBody.length()) + fileNameExt);
    }
    if (fileNameBody.endsWith(pattern))
    {
        candidateNames.push_back(fileNameBody.substring(0, fileNameBody.length() - pattern.length()) + replacement + fileNameExt);
    }

    for (size_t i = 0; i < candidateNames.size(); ++i)
    {
        const juce::String& candidateName = candidateNames[i];
        if (directory.getChildFile(candidateName).existsAsFile())
        {
            const juce::File candidateFile = directory.getChildFile(candidateName);
            size_t candidateChannelCount = 0;
            size_t candidateSampleCount = 0;
            double candidateSampleRate = 0.0;
            bool fileInfoSuccess = false;

            juce::AudioFormatManager formatManager;
            formatManager.registerBasicFormats();
            std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(candidateFile));
            if (reader)
            {
                candidateChannelCount = static_cast<size_t>(reader->numChannels);
                candidateSampleCount = static_cast<size_t>(reader->lengthInSamples);
                candidateSampleRate = reader->sampleRate;
                fileInfoSuccess = true;
            }

            if (fileInfoSuccess &&
                candidateChannelCount == 2 &&
                candidateSampleCount == sampleCount &&
                std::fabs(candidateSampleRate - sampleRate) < 0.000001)
            {
                return candidateFile;
            }
        }
    }
    return {};
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

    resampleIRToProjectRate(bufferLL, bufferRR);
    if (isQuad) resampleIRToProjectRate(bufferLR, bufferRL);
    
    applyStretch(bufferLL, bufferRR);
    if (isQuad) applyStretch(bufferLR, bufferRL);

    applyTrim();
    applyEQ();
    applyEnvelope();

    version += 1;
}

void Impulse::resampleIRToProjectRate(std::vector<float>& bufL, std::vector<float>& bufR) const
{
    if (fabs(irsrate - srate) < 1e-6) return;

    double ratio = irsrate / srate;  // source / dest

    VectorAudioSource source(bufL, bufR);
    ResamplingAudioSource resampler(&source, false);
    resampler.setResamplingRatio(ratio);

    const int inputLength = (int)bufL.size();
    const int outputLength = (int)std::ceil(inputLength / ratio);

    AudioBuffer<float> out(2, outputLength);
    AudioSourceChannelInfo info(out);

    resampler.prepareToPlay(4096, srate);
    resampler.getNextAudioBlock(info);
    resampler.releaseResources();

    bufL.assign(out.getReadPointer(0), out.getReadPointer(0) + outputLength);
    bufR.assign(out.getReadPointer(1), out.getReadPointer(1) + outputLength);
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

void Impulse::applyEQ()
{
    auto size = (int)bufferLL.size();
    if (!size) return;

    std::vector<SVF> eq;
    for (auto& band : decayEQ) {
        SVF svf;
        if (band.mode == SVF::LP) svf.lp((float)srate, band.freq, band.q);
        else if (band.mode == SVF::LS) svf.ls((float)srate, band.freq, band.q, band.gain);
        else if (band.mode == SVF::HP) svf.hp((float)srate, band.freq, band.q);
        else if (band.mode == SVF::HS) svf.hs((float)srate, band.freq, band.q, band.gain);
        else svf.pk((float)srate, band.freq, band.q, band.gain);
        eq.push_back(svf);
    }

    if (eq.empty())
        return;

    const int LUTSize = 4096 / 2 + 1;
    const float minFreq = 1.f;
    const float maxFreq = (float)srate * 0.49f;
    std::vector<float> magLUT;
    magLUT.resize(LUTSize);
    std::fill(magLUT.begin(), magLUT.end(), 1.f);

    for (int i = 0; i < LUTSize; ++i) {
        auto freq = minFreq * std::pow(maxFreq / minFreq, (float)i / (LUTSize - 1));
        for (auto& svf : eq)
            magLUT[i] *= svf.getMagnitude(freq);
    }

    applyDecay(bufferLL, magLUT);
    applyDecay(bufferRR, magLUT);
    applyDecay(bufferLR, magLUT);
    applyDecay(bufferRL, magLUT);
}

void Impulse::applyDecay(std::vector<float>& buf, std::vector<float>& magLUT)
{
    (void)magLUT;
    const size_t numBlocks = (buf.size() + HOP_SIZE - 1) / HOP_SIZE;
    if (numBlocks < 1) return;

    std::vector<float> output(buf.size(), 0.f);
    std::vector<float> norm(buf.size(), 0.f);
    std::vector<float> block(FFT_SIZE, 0.0f);

    for (size_t b = 0; b < numBlocks; ++b)
    {
        std::fill(block.begin(), block.end(), 0.0f);
        size_t start = b * HOP_SIZE;
        size_t blockSize = std::min((size_t)FFT_SIZE, buf.size() - start);

        for (size_t i = 0; i < blockSize; ++i)
            block[i] = buf[start + i] * window[i];

        _fft.fft(block.data(), re.data(), im.data());

        // Zero middle frequencies
        size_t lowBin = 400;
        size_t highBin = 600;
        size_t maxBin = FFT_SIZE / 2;
        if (highBin > maxBin - 1) highBin = maxBin - 1;
        for (size_t k = lowBin; k <= highBin; ++k) {
            re[k] = 0.0f;
            im[k] = 0.0f;
        }

        _fft.ifft(block.data(), re.data(), im.data());

        for (size_t i = 0; i < blockSize; ++i) {
            size_t outPos = start + i;
            if (outPos < output.size()) {
                output[outPos] += block[i] * window[i];
                norm[outPos] += window[i];
            }
        }
    }

    for (size_t i = 0; i < buf.size(); ++i) {
        buf[i] = norm[i] > 0.f ? output[i] / norm[i] : 0.f;
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