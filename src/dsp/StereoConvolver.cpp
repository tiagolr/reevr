#include "StereoConvolver.h"

bool StereoConvolver::finishedLoading()
{
	return convolverLL->isFinished();
}

void StereoConvolver::prepare(int samplesPerBlock)
{
	size = samplesPerBlock;
	headBlockSize = 1;
	while (headBlockSize < static_cast<size_t>(samplesPerBlock)) {
		headBlockSize *= 2;
	}
	tailBlockSize = std::max(size_t(8192), 2 * headBlockSize);
	bufferLL.resize(samplesPerBlock, 0.0f);
	bufferRR.resize(samplesPerBlock, 0.0f);
	bufferLR.resize(samplesPerBlock, 0.0f);
	bufferRL.resize(samplesPerBlock, 0.0f);
}

void StereoConvolver::loadImpulse(Impulse imp)
{
	convolverLL->init(headBlockSize, tailBlockSize, imp.bufferLL.data(), imp.bufferLL.size());
	convolverRR->init(headBlockSize, tailBlockSize, imp.bufferRR.data(), imp.bufferRR.size());
	isQuad = imp.isQuad;
	if (isQuad) {
		convolverLR->init(headBlockSize, tailBlockSize, imp.bufferLR.data(), imp.bufferLR.size());
		convolverRL->init(headBlockSize, tailBlockSize, imp.bufferRL.data(), imp.bufferRL.size());
	}
}

void StereoConvolver::process(const float* dataL, const float* dataR, size_t nsamples, bool force2Chans)
{
	convolverLL->process(dataL, bufferLL.data(), nsamples);
	convolverRR->process(dataR, bufferRR.data(), nsamples);

	if (isQuad && !force2Chans) {
		convolverLR->process(dataL, bufferLR.data(), nsamples);
		convolverRL->process(dataR, bufferRL.data(), nsamples);
	}
}

void StereoConvolver::reset()
{
	convolverLL->reset();
	convolverRR->reset();
	convolverLR->reset();
	convolverRL->reset();
	bufferLL.clear();
	bufferRR.clear();
	bufferLR.clear();
	bufferRL.clear();
}

void StereoConvolver::clear()
{
	convolverLL->clear();
	convolverRR->clear();
	convolverLR->clear();
	convolverRL->clear();
}