#pragma once

namespace globals {
	// convolver
	inline unsigned int CONV_XFADE = 50;
	inline unsigned int CONV_LOAD_COOLDOWN = 250;
	inline unsigned int CONV_CLEAR_TAILS_COOLDOWN = 5;

	// filter consts
	inline unsigned int F_LERP_MILLIS = 50;
	inline const double F_MIN_FREQ = 20.0;
	inline const double F_MAX_FREQ = 22050.0;
	inline const double F_MAX_DRIVE = 36.0;

	inline unsigned int COLOR_BG = 0xff181818;
	inline unsigned int COLOR_ACTIVE = 0xff9ca7ff;
	inline unsigned int COLOR_NEUTRAL = 0xff666666;
	inline unsigned int COLOR_NEUTRAL_LIGHT = 0x99999999;
	inline unsigned int COLOR_SEEK = 0xff80ffff;
	inline unsigned int COLOR_KNOB = 0xff272727;
	inline unsigned int COLOR_AUDIO = 0xffffd42a;
	inline unsigned int COLOR_MIDI = 0xff00e672;
	inline unsigned int COLOR_SELECTION = 0xff50a9ff;
	inline unsigned int COLOR_SEQ_MAX = 0xffffffff;
	inline unsigned int COLOR_SEQ_MIN = 0xffffffff;
	inline unsigned int COLOR_SEQ_INVX = 0xff00ffff;
	inline unsigned int COLOR_SEQ_TEN = 0xff50ff60;
	inline unsigned int COLOR_SEQ_TENA = 0xffffee50;
	inline unsigned int COLOR_SEQ_TENR = 0xffffB950;
	inline unsigned int COLOR_SEQ_SKEW = 0xffffB950;

	inline const int LATENCY_MILLIS = 4;
	inline const int AUDIO_COOLDOWN_MILLIS = 50;
	inline const int AUDIO_DRUMSBUF_MILLIS = 20;
	inline const int AUDIO_NOTE_LENGTH_MILLIS = 100;
	inline const int MAX_UNDO = 100;

	// view consts
	inline const int PLUG_WIDTH = 690;
	inline const int PLUG_HEIGHT = 650;
	inline const int MAX_PLUG_WIDTH = 640 * 3;
	inline const int MAX_PLUG_HEIGHT = 650 * 2;
	inline const int PLUG_PADDING = 15;
	inline const int HOVER_RADIUS = 7;
	inline const int MPOINT_HOVER_RADIUS = 5;
	inline const int POINT_RADIUS = 4;
	inline const int MPOINT_RADIUS = 3;
	inline const int MSEL_PADDING = 8;

	// Envelope follower
	inline const float ENV_MIN_ATTACK = 0.01f;
	inline const float ENV_MAX_ATTACK = 200.0f;
	inline const float ENV_MIN_HOLD = 0.0f;
	inline const float ENV_MAX_HOLD = 500.0f;
	inline const float ENV_MIN_RELEASE = 10.0f;
	inline const float ENV_MAX_RELEASE = 10000.0f;

	// paint mode
	inline const int PAINT_PATS_IDX = 100; // starting index of paint patterns, audio patterns always range 0..11
	inline const int PAINT_PATS = 32;

	// sequencer
	inline const int SEQ_PAT_IDX = 1000;
};