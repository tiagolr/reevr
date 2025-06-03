#include "Follower.h"

void Follower::prepare(float srate, float thresh_, bool autorel_, float attack_, float hold_, float release_, float lowcutfreq, float highcutfreq)
{
	lowcutL.hp(srate, lowcutfreq, 0.707f);
	highcutL.lp(srate, highcutfreq, 0.707f);
	lowcutR.hp(srate, lowcutfreq, 0.707f);
	highcutR.lp(srate, highcutfreq, 0.707f);
	thresh = thresh_;
	autorel = autorel_;
	attack = (ENV_MIN_ATTACK + (ENV_MAX_ATTACK - ENV_MIN_ATTACK) * attack_) / 1000.0f;
	hold = (ENV_MIN_HOLD + (ENV_MAX_HOLD - ENV_MIN_HOLD) * hold_) / 1000.0f * srate;
	release = (ENV_MIN_RELEASE + (ENV_MAX_RELEASE - ENV_MIN_RELEASE) * release_) / 1000.0f;

	float targetLevel = 0.2f; // -14dB or something slow
	attackcoeff = std::exp(std::log(targetLevel) / (attack * srate));
	releasecoeff = std::exp(std::log(targetLevel) / (release * srate));
	float minReleaseTime = release * 0.2f; // faster release
	minreleasecoeff = std::exp(std::log(targetLevel) / (minReleaseTime * srate));
}

float Follower::process(float lsamp, float rsamp)
{
	outl = lowcutL.df1(lsamp);
	outl = highcutL.df1(outl);

	outr = lowcutR.df1(rsamp);
	outr = highcutR.df1(outr);

	float amp = std::max(std::fabs(outl), std::fabs(outr));
	float in = std::max(0.0f, amp - thresh);

	if (in > envelope) {
		envelope = attackcoeff * envelope + (1.0f - attackcoeff) * in;
		holdCounter = hold;
	}
	else if (holdCounter > 0.0f) {
		holdCounter -= 1.0f; // Decrement hold timer
	}
	else if (autorel) {
		float releaseRatio = (envelope - in) / (envelope + 1e-12f);
		releaseRatio = releaseRatio * releaseRatio;
		releaseRatio = std::clamp(releaseRatio, 0.0f, 1.0f);
		float adaptiveCoeff = releasecoeff + (minreleasecoeff - releasecoeff) * releaseRatio;
		envelope = adaptiveCoeff * envelope + (1.0f - adaptiveCoeff) * in;
	}
	else {
		envelope = releasecoeff * envelope + (1.0f - releasecoeff) * in;
	}

	return envelope;
}

void Follower::clear()
{
	outl = 0.0f;
	outr = 0.0f;
	envelope = 0.0f;
	lowcutL.reset(0.0f);
	lowcutR.reset(0.0f);
	highcutL.reset(0.0f);
	highcutR.reset(0.0f);
}