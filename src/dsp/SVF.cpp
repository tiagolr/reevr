#include "SVF.h"
#include <cmath>

void SVF::setup(float _srate, float _freq, float _q, float resfactor)
{
	srate = _srate;
	freq = _freq;
	q = _q;

	g = std::tan(MathConstants<float>::pi * std::fmin(freq / srate, 0.49f));
	r2 = (1.0f / q) * resfactor;

	a1 = 1.0f / (1.0f + g * (g + r2));
	a2 = g * a1;
	a3 = g * a2;
}

void SVF::lp(float _srate, float _freq, float _q)
{
	mode = LP;
	gain = 1.f;
	setup(_srate, _freq, _q);

	cl = 1.0f;
	cb = 0.0f;
	ch = 0.0f;
}

void SVF::lp6(float _srate, float _freq)
{
	mode = LP6;
	srate = _srate;
	freq = _freq;
	g = std::tan(MathConstants<float>::pi * std::fmin(freq / srate, 0.49f));
	g = g / (1.0f + g);
}

void SVF::hp(float _srate, float _freq, float _q)
{
	mode = HP;
	gain = 1.f;
	setup(_srate, _freq, _q);

	cl = 0.0f;
	cb = 0.0f;
	ch = 1.0f;
}

void SVF::hp6(float _srate, float _freq)
{
	mode = HP6;
	srate = _srate;
	freq = _freq;
	g = std::tan(MathConstants<float>::pi * std::fmin(freq / srate, 0.49f));
	g = g / (1.0f + g);
}

void SVF::bp(float _srate, float _freq, float _q)
{
	mode = BP;
	gain = 1.f;
	setup(_srate, _freq, _q);

    cl = 0.0f;
    cb = 1.f/q;
    ch = 0.0f;
}

void SVF::bs(float _srate, float _freq, float _q)
{
	mode = BS;
	gain = 1.f;
	setup(_srate, _freq, _q);

	cl = 1.0f;
	cb = 0.0f;
	ch = 1.0f;
}

void SVF::ls(float _srate, float _freq, float _q, float _gain)
{
	mode = LS;
	gain = _gain;
	setup(_srate, _freq * std::pow(gain, -0.25f), _q);

	cl = gain;
	cb = r2 * std::sqrt(gain);
	ch = 1.f;
}

void SVF::hs(float _srate, float _freq, float _q, float _gain)
{
	mode = HS;
	gain = _gain;
	setup(_srate, _freq * std::pow(gain, 0.25f), _q);

	cl = 1.f;
	cb = r2 * std::sqrt(gain);
	ch = gain;
}

void SVF::pk(float _srate, float _freq, float _q, float _gain)
{
	mode = PK;
	gain = _gain;
	setup(_srate, _freq, _q, gain < 1.f ? 7.5f : 1.f);

	cl = 1.f;
	cb = r2 * gain;
	ch = 1.f;
}

float SVF::process(float sample)
{
	if (mode == Off) {
		return sample;
	}
	else if (mode == HP6) {
		float delta = g * (sample - s1);
		s1 += delta;
		return sample - s1;
	}
	else if (mode == LP6) {
		float delta = g * (sample - s1);
		s1 += delta;
		return s1;
	}

    float v3 = sample - s2;
    float v1 = a1 * s1 + a2 * v3; // band
    float v2 = s2 + a2 * s1 + a3 * v3; // low

    s1 = 2.0f * v1 - s1;
    s2 = 2.0f * v2 - s2;

    return cl * v2 + cb * v1 + ch * (sample - r2 * v1 - v2);
}

void SVF::processBlock(float* buf, int nsamples, int blockoffset, int blocksize, float tfreq, float tq, float tgain)
{
	(void)blocksize;
	if (mode == Off) {
		return;
	}
	else if (mode == LP6 || mode == HP6) {
		processBlock6dB(buf, nsamples, tfreq);
		return;
	}

	// current values
	float curr_a1 = a1;
	float curr_a2 = a2;
	float curr_a3 = a3;
	float curr_r2 = r2;
	float curr_cl = cl;
	float curr_cb = cb;
	float curr_ch = ch;

	if (blockoffset == 0) {
		auto interpolate = tfreq != freq || tq != q || tgain != gain;
		if (interpolate) {
			if (mode == LP) lp(srate, tfreq, tq);
			else if (mode == BP) bp(srate, tfreq, tq);
			else if (mode == HP) hp(srate, tfreq, tq);
			else if (mode == LS) ls(srate, tfreq, tq, tgain);
			else if (mode == HS) hs(srate, tfreq, tq, tgain);
			else if (mode == PK) pk(srate, tfreq, tq, tgain);

			auto size = 1.f / blocksize;
			a1_step = (a1 - curr_a1) * size;
			a2_step = (a2 - curr_a2) * size;
			a3_step = (a3 - curr_a3) * size;
			r2_step = (r2 - curr_r2) * size;

			cl_step = (cl - curr_cl) * size;
			cb_step = (cb - curr_cb) * size;
			ch_step = (ch - curr_ch) * size;
		}
		else {
			a1_step = 0.0f; a2_step = 0.0f;
			a3_step = 0.0f; r2_step = 0.0f;
			cl_step = 0.0f; cb_step = 0.0f; ch_step = 0.0f;
		}
	}

	for (int n = 0; n < nsamples; ++n) {
		auto sample = buf[n];
		float v3 = sample - s2;
		float v1 = curr_a1 * s1 + curr_a2 * v3; // band
		float v2 = s2 + curr_a2 * s1 + curr_a3 * v3; // low

		s1 = 2.0f * v1 - s1;
		s2 = 2.0f * v2 - s2;

		buf[n] = curr_cl * v2 + curr_cb * v1 + curr_ch * (sample - curr_r2 * v1 - v2);

		curr_a1 += a1_step;
		curr_a2 += a2_step;
		curr_a3 += a3_step;
		curr_r2 += r2_step;
		curr_cl += cl_step;
		curr_cb += cb_step;
		curr_ch += ch_step;
	}

	a1 = curr_a1; a2 = curr_a2;
	a3 = curr_a3; r2 = curr_r2;
	cl = curr_cl; cb = curr_cb; ch = curr_ch;
}

void SVF::processBlock6dB(float* buf, int nsamples, float tfreq)
{
	float cur_g = g;
	float g_step = 0.f;

	if (tfreq != freq) {
		if (mode == LP6) lp6(srate, tfreq);
		else if (mode == HP6) hp6(srate, tfreq);

		auto size = 1.f / nsamples;
		g_step = (g - cur_g) * size;
	}

	if (mode == LP6) {
		for (int n = 0; n < nsamples; ++n)
		{
			float sample = buf[n];
			float delta = cur_g * (sample - s1);
			s1 += delta;
			buf[n] = s1;
			cur_g += g_step;
		}
	}
	else if (mode == HP6) {
		for (int n = 0; n < nsamples; ++n)
		{
			float sample = buf[n];
			float delta = cur_g * (sample - s1);
			s1 += delta;
			buf[n] = sample - s1;
			cur_g += g_step;
		}
	}
	g = cur_g;
}

void SVF::clear(float input)
{
	s1 = 0.f;
	s2 = input;
};

float SVF::getMagnitude(float _freq)
{
	_freq = std::min(_freq, 0.49f * srate);

	if (mode == LP6 || mode == HP6) {
		float omega = 2.0f * MathConstants<float>::pi * _freq / srate;
		float a = g;
		float b = 1.0f - a;
		float c = std::cos(omega);

		float denom = 1.0f + b * b - 2.0f * b * c;
		denom = std::max(denom, 1e-12f);

		float num = (mode == LP6)
			? (a * a)
			: (2.0f - 2.0f * c);

		return std::sqrt(num / denom);
	}

	std::complex<float> j(0.0f, 1.0f);
	float g_eval = std::tan(MathConstants<float>::pi * std::fmin(_freq / srate, 0.49f));
	float g_norm = g_eval / g;
	std::complex<float> denom = std::complex<float>(g_norm * g_norm - 1.0f, g_norm * r2);
	std::complex<float> num =
		-cl * std::complex<float>(1.0f, 0.0f)
		+ cb * std::complex<float>(0.0f, g_norm)
		+ ch * std::complex<float>(g_norm * g_norm, 0.0f);

	std::complex<float> H = num / denom;
	return std::abs(H);
}