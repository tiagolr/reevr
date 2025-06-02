#include "StereoConvolver.h"

bool StereoConvolver::finishedLoading()
{
	return convolverL->isFinished();
}

void StereoConvolver::prepare(int samplesPerBlock)
{
	size = samplesPerBlock;
	headBlockSize = 1;
	while (headBlockSize < static_cast<size_t>(samplesPerBlock)) {
		headBlockSize *= 2;
	}
	tailBlockSize = std::min(size_t(8192), 2 * headBlockSize);
	bufferL.resize(samplesPerBlock, 0.0f);
	bufferR.resize(samplesPerBlock, 0.0f);
}

void StereoConvolver::loadImpulse(Impulse imp)
{
	convolverL->init(headBlockSize, tailBlockSize, imp.bufferL.data(), imp.bufferL.size());
	convolverR->init(headBlockSize, tailBlockSize, imp.bufferR.data(), imp.bufferR.size());
}

void StereoConvolver::process(const float* dataL, const float* dataR, size_t nsamples)
{
	convolverL->process(dataL, bufferL.data(), nsamples);
	convolverR->process(dataR, bufferR.data(), nsamples);
}

void StereoConvolver::reset()
{
	convolverL->reset();
	convolverR->reset();
	bufferL.clear();
	bufferR.clear();
}

void StereoConvolver::clear()
{
	convolverL->clear();
	convolverR->clear();
}