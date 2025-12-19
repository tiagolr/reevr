#include "EQWidget.h"
#include "../PluginEditor.h"

static void drawPeak(Graphics& g, Rectangle<float> bounds, Colour c, float scale = 1.f)
{
	Path p;
	p.startNewSubPath(0.0f, 11.5f);
	p.cubicTo(9.0f, 11.5f, 6.5f, 0.5f, 9.0f, 0.5f);
	p.cubicTo(11.5f, 0.5f, 9.0f, 11.5f, 18.0f, 11.5f);
	p.applyTransform(AffineTransform::scale(scale));
	p.applyTransform(AffineTransform::translation(bounds.getX(), bounds.getY()));
	g.setColour(c);
	g.strokePath(p, PathStrokeType(1.0f, PathStrokeType::curved));
}

static void drawLowShelf(Graphics& g, Rectangle<float> bounds, Colour c, float scale = 1.f)
{
	Path p;
	p.startNewSubPath(0.0f, 13.0f);
	p.cubicTo(1.0f, 13.0f, 5.0f, 13.0f, 5.0f, 13.0f);
	p.cubicTo(9.0f, 13.0f, 9.0f, 0.f, 13.0f, 0.f);
	p.cubicTo(17.0f, 0.f, 18.0f, 0.f, 18.0f, 0.f);
	p.applyTransform(AffineTransform::scale(scale));
	p.applyTransform(AffineTransform::translation(bounds.getX(), bounds.getY()));
	g.setColour(c);
	g.strokePath(p, PathStrokeType(1.0f, PathStrokeType::curved));
}

static void drawHighShelf(Graphics& g, Rectangle<float> bounds, Colour c, float scale = 1.f)
{
	Path p;
	p.startNewSubPath(18.0f, 13.0f);
	p.cubicTo(17.0f, 13.0f, 13.0f, 13.0f, 13.0f, 13.0f);
	p.cubicTo(9.0f, 13.0f, 9.0f, 0.f, 5.0f, 0.f);
	p.cubicTo(1.0f, 0.f, 0.0f, 0.f, 0.0f, 0.f);
	p.applyTransform(AffineTransform::scale(scale));
	p.applyTransform(AffineTransform::translation(bounds.getX(), bounds.getY()));
	g.setColour(c);
	g.strokePath(p, PathStrokeType(1.0f, PathStrokeType::curved));
}

static void drawHighpass(Graphics& g, Rectangle<float> bounds, Colour c, float scale = 1.f)
{
	Path p;
	p.startNewSubPath(18.f, 0.f);
	p.cubicTo(10.f, 0.f, 8.f, 0.5f, 8.f, 0.f);
	p.cubicTo(5.f, 0.f, 5.f, 1.f, 4.f, 4.0f);
	p.cubicTo(3.f, 6.f, 0.f, 11.f, 0.f, 11.f);
	p.applyTransform(AffineTransform::scale(scale));
	p.applyTransform(AffineTransform::translation(bounds.getX(), bounds.getY()));
	g.setColour(c);
	g.strokePath(p, PathStrokeType(1.0f, PathStrokeType::curved));
}

static void drawLowpass(Graphics& g, Rectangle<float> bounds, Colour c, float scale = 1.f)
{
	Path p;
	p.startNewSubPath(0.0f, 0.f);
	p.cubicTo(8.0f, 0.f, 10.f, 0.f, 10.f, 0.f);
	p.cubicTo(13.0f, 0.f, 13.f, 1.f, 14.f, 4.0f);
	p.cubicTo(15.f, 6.f, 18.0f, 11.f, 18.0f, 11.f);
	p.applyTransform(AffineTransform::scale(scale));
	p.applyTransform(AffineTransform::translation(bounds.getX(), bounds.getY()));
	g.setColour(c);
	g.strokePath(p, PathStrokeType(1.0f, PathStrokeType::curved));
}

// ==============================================================================

EQWidget::EQWidget(REEVRAudioProcessorEditor& e, SVF::EQType _type)
	: editor(e)
	, type(_type)
	, prel(_type == SVF::ParamEQ ? "post" : "decay")
{
	eq = std::make_unique<EQDisplay>(editor, type);
	addAndMakeVisible(eq.get());
	eq->onMouseDownBand = [this](int band)
		{
			selband = band;
			freqknobs[selband]->forceLabelShowValue = true;
			gainknobs[selband]->forceLabelShowValue = true;
			freqknobs[selband]->repaint();
			gainknobs[selband]->repaint();
			toggleUIComponents();
		};
	eq->onMouseUp = [this]
		{
			freqknobs[selband]->forceLabelShowValue = false;
			gainknobs[selband]->forceLabelShowValue = false;
			freqknobs[selband]->repaint();
			gainknobs[selband]->repaint();
		};
	eq->onMouseDrag = [this]
		{
			freqknobs[selband]->repaint();
			gainknobs[selband]->repaint();
		};

	for (int i = 0; i < EQ_BANDS; ++i) {
		auto pre = prel + String("eq_band") + String(i + 1);
		if (i == 0 || i == EQ_BANDS - 1) {
			editor.audioProcessor.params.addParameterListener(pre + "_mode", this);
		}
		auto freq = std::make_unique<Rotary>(editor.audioProcessor, pre + "_freq", "Freq", RotaryLabel::hz);
		auto q = std::make_unique<Rotary>(editor.audioProcessor, pre + "_q", "Q", RotaryLabel::float1);
		auto gain = std::make_unique<Rotary>(editor.audioProcessor, pre + "_gain", "Gain", 
			type == SVF::DecayEQ ? RotaryLabel::eqDecayGain : RotaryLabel::dBfloat1, true
		);
		addChildComponent(freq.get());
		addChildComponent(q.get());
		addChildComponent(gain.get());
		freqknobs.push_back(std::move(freq));
		qknobs.push_back(std::move(q));
		gainknobs.push_back(std::move(gain));
	}

	addAndMakeVisible(bandBtn);
	bandBtn.setAlpha(0.f);
	bandBtn.onClick = [this]
		{
			showBandModeMenu();
		};

	if (type == SVF::DecayEQ) {
		addAndMakeVisible(rateSlider);
		rateSlider.setComponentID("vertical");
		rateSlider.setSliderStyle(Slider::SliderStyle::LinearVertical);
		rateSlider.setTextBoxStyle(Slider::NoTextBox, false, 80, 20);
		rateSlider.setColour(Slider::backgroundColourId, Colour(COLOR_BG).brighter(0.1f));
		rateSlider.setColour(Slider::trackColourId, Colour(COLOR_ACTIVE).darker(0.5f));
		rateSlider.setColour(Slider::thumbColourId, Colour(COLOR_ACTIVE));
		rateSliderAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(editor.audioProcessor.params, "irdecayrate", rateSlider);
		rateSlider.textFromValueFunction = [](double v)
			{
				return "Decay Rate: " + juce::String(std::round(v * 100), 0) + " %";
			};
	}
}

EQWidget::~EQWidget()
{
	for (int i = 0; i < EQ_BANDS; ++i) {
		auto pre = prel + String("eq_band") + String(i + 1);
		if (i == 0 || i == EQ_BANDS - 1) {
			editor.audioProcessor.params.removeParameterListener(pre + "_mode", this);
		}
	}
}

void EQWidget::parameterChanged(const juce::String& parameterID, float newValue)
{
	(void)parameterID;
	(void)newValue;
	MessageManager::callAsync([this] { repaint(); });
}

void EQWidget::resized()
{
	auto b = getLocalBounds();
	eq->setBounds(Rectangle<int>(b.getX(), b.getY(), 173, 103).translated(4,8));

	for (int i = 0; i < EQ_BANDS; ++i) {
		qknobs[i]->setBounds(b.getRight() - 80, b.getY(), 80, 65);
		freqknobs[i]->setBounds(qknobs[i]->getBounds().translated(-75, 75));
		gainknobs[i]->setBounds(freqknobs[i]->getBounds().translated(75,0));
	}

	bandBtn.setBounds(Rectangle<int>(30, 30)
		.withX(freqknobs[0]->getBounds().getCentreX() - 30/2)
		.withY(qknobs[0]->getY() + 10));

	if (type == SVF::DecayEQ) {
		rateSlider.setPopupDisplayEnabled(true, true, getParentComponent());
		auto eqb = eq->getBounds();
		rateSlider.setBounds(eqb.getRight() + 16, eqb.getY() - 8, 20, 120);
	}

	toggleUIComponents();
}

void EQWidget::paint(Graphics& g)
{
	g.fillAll(Colour(COLOR_BG));

	// draw band curve button
	g.setColour(Colour(COLOR_NEUTRAL));
	g.drawRoundedRectangle(bandBtn.getBounds().toFloat().reduced(0.5f), 3.f, 1.f);
	auto mode = eq->bandFilters[selband].mode;
	if (mode == SVF::PK) {
		drawPeak(g, bandBtn.getBounds().toFloat().translated(4.5f, 6.5f), Colours::white, 1.2f);
	}
	else if (mode == SVF::LP) {
		drawLowpass(g, bandBtn.getBounds().toFloat().translated(4.5f, 8.5f), Colours::white, 1.2f);
	}
	else if (mode == SVF::HP) {
		drawHighpass(g, bandBtn.getBounds().toFloat().translated(4.5f, 8.5f), Colours::white, 1.2f);
	}
	else if (mode == SVF::LS) {
		drawLowShelf(g, bandBtn.getBounds().toFloat().translated(4.5f, 6.5f), Colours::white, 1.2f);
	}
	else if (mode == SVF::HS) {
		drawHighShelf(g, bandBtn.getBounds().toFloat().translated(4.5f, 6.5f), Colours::white, 1.2f);
	}

	g.setFont(FontOptions(16.f));
	g.setColour(Colours::white);
	g.drawText("Band " + String(selband + 1), bandBtn.getBounds().withHeight(16).translated(0, 40).expanded(20, 0), Justification::centred);
}

void EQWidget::toggleUIComponents()
{
	for (int i = 0; i < EQ_BANDS; ++i) {
		freqknobs[i]->setVisible(selband == i);
		qknobs[i]->setVisible(selband == i);
		gainknobs[i]->setVisible(selband == i);
	}

	repaint();
}

void EQWidget::showBandModeMenu()
{
	auto mode = SVF::PK;

	if (selband == 0) {
		mode = (int)editor.audioProcessor.params.getRawParameterValue(prel + "eq_band1_mode")->load() == 0
			? SVF::LP : SVF::LS;
	}
	if (selband == EQ_BANDS - 1) {
		mode = (int)editor.audioProcessor.params.getRawParameterValue(prel + "eq_band" + String(EQ_BANDS) + "_mode")->load() == 0
			? SVF::HP : SVF::HS;
	}

	PopupMenu menu;
	if (selband == 0) {
		menu.addItem(1, "Low Cut", true, mode == SVF::HP);
		menu.addItem(2, "Low Shelf", true, mode == SVF::LS);
	}
	else if (selband == EQ_BANDS - 1) {
		menu.addItem(3, "High Cut", true, mode == SVF::LP);
		menu.addItem(4, "High Shelf", true, mode == SVF::HS);
	}
	else {
		menu.addItem(5, "Peak", true, true);
	}

	auto menuPos = localPointToGlobal(bandBtn.getBounds().getBottomLeft());
	menu.showMenuAsync(PopupMenu::Options()
		.withTargetComponent(*this)
		.withTargetScreenArea({ menuPos.getX(), menuPos.getY(), 1, 1 }),
		[this](int result) {
			if (result == 0) return;
			if (result == 1 || result == 2) {
				auto param = editor.audioProcessor.params.getParameter(prel + "eq_band1_mode");
				param->setValueNotifyingHost(param->convertTo0to1((float)result - 1));
				eq->updateEQCurve();
				toggleUIComponents();
			}
			if (result == 3 || result == 4) {
				auto param = editor.audioProcessor.params.getParameter(prel + "eq_band" + String(EQ_BANDS) + "_mode");
				param->setValueNotifyingHost(param->convertTo0to1((float)result - 3));
				eq->updateEQCurve();
				toggleUIComponents();
			}
		});
}