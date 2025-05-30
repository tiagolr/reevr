#include "Follower.h"

void Follower::prepare(double srate, double thresh_, bool autorel_, double attack_, double hold_, double release_, double lowcutfreq, double highcutfreq)
{
	lowcutL.hp(srate, lowcutfreq, 0.707);
	highcutL.lp(srate, highcutfreq, 0.707);
	lowcutR.hp(srate, lowcutfreq, 0.707);
	highcutR.lp(srate, highcutfreq, 0.707);
	thresh = thresh_;
	autorel = autorel_;
	attack = (ENV_MIN_ATTACK + (ENV_MAX_ATTACK - ENV_MIN_ATTACK) * attack_) / 1000.0;
	hold = hold_;
	release = (ENV_MIN_RELEASE + (ENV_MAX_RELEASE - ENV_MIN_RELEASE) * release_) / 1000.0;

	double targetLevel = 0.2; // -14dB or something slow
	attackcoeff = std::exp(std::log(targetLevel) / (attack * srate));
	releasecoeff = std::exp(std::log(targetLevel) / (release * srate));
	double minReleaseTime = release * 0.2f; // faster release
	minreleasecoeff = std::exp(std::log(targetLevel) / (minReleaseTime * srate));
}

double Follower::process(double lsamp, double rsamp)
{
	outl = lowcutL.df1(lsamp);
	outl = highcutL.df1(outl);

	outr = lowcutR.df1(rsamp);
	outr = highcutR.df1(outr);

	double amp = std::max(std::fabs(outl), std::fabs(outr));
	double in = std::max(0.0, amp - thresh);

	if (in > envelope)
		envelope = attackcoeff * envelope + (1.0 - attackcoeff) * in;
	else if (autorel) {
		double releaseRatio = (envelope - in) / (envelope + 1e-12);
		releaseRatio = releaseRatio * releaseRatio;
		releaseRatio = std::clamp(releaseRatio, 0.0, 1.0);
		double adaptiveCoeff = releasecoeff + (minreleasecoeff - releasecoeff) * releaseRatio;
		envelope = adaptiveCoeff * envelope + (1.0 - adaptiveCoeff) * in;
	}
	else
		envelope = releasecoeff * envelope + (1.0 - releasecoeff) * in;

	return envelope;
}

void Follower::clear()
{
	outl = 0.0;
	outr = 0.0;
	envelope = 0.0;
	lowcutL.clear(0.0);
	lowcutR.clear(0.0);
	highcutL.clear(0.0);
	highcutR.clear(0.0);
}