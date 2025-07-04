#include "Rotary.h"
#include "../PluginProcessor.h"

Rotary::Rotary(REEVRAudioProcessor& p, juce::String paramId, juce::String name, RotaryLabel format, bool isSymmetric, unsigned int color, RotaryType type)
    : juce::SettableTooltipClient()
    , juce::Component()
    , audioProcessor(p)
    , paramId(paramId)
    , name(name)
    , format(format)
    , isSymmetric(isSymmetric)
    , isAudioKnob(isAudioKnob)
    , color(color)
    , type(type)
{
    setName(name);
    audioProcessor.params.addParameterListener(paramId, this);
}

Rotary::~Rotary()
{
    audioProcessor.params.removeParameterListener(paramId, this);
}

void Rotary::parameterChanged(const juce::String& parameterID, float newValue) 
{
    (void)parameterID;
    (void)newValue;
    juce::MessageManager::callAsync([this] { repaint(); });
}

void Rotary::paint(juce::Graphics& g) {
    auto param = audioProcessor.params.getParameter(paramId);
    auto normValue = param->getValue();
    auto value = param->convertFrom0to1(normValue);

    draw_rotary_slider(g, normValue); 
    draw_label_value(g, value);
    //g.drawRect(getLocalBounds());
}

void Rotary::draw_label_value(juce::Graphics& g, float slider_val)
{
    auto text = String(name);
    std::stringstream ss;

    if (mouse_down) {
        if (format == RotaryLabel::percx100) text = std::to_string((int)std::round((slider_val * 100))) + " %";
        if (format == RotaryLabel::intx100) text = std::to_string((int)std::round((slider_val * 100)));
        else if (format == RotaryLabel::hz) {
            if (slider_val < 1000) {
                ss << std::fixed << std::setprecision(0) << slider_val << " Hz";
            } else {
                ss << std::fixed << std::setprecision(1) << slider_val / 1000.0 << " kHz";
            }
            text = ss.str();
        }
        else if (format == RotaryLabel::hzLp) {
            if (slider_val >= 20000) {
                ss << "Off";
            }
            else if (slider_val < 1000) {
                ss << std::fixed << std::setprecision(0) << (int)slider_val << " Hz";
            }
            else {
                ss << std::fixed << std::setprecision(1) << slider_val / 1000.0 << " kHz";
            }
            text = ss.str();
        }
        else if (format == RotaryLabel::hzHp) {
            if (slider_val <= 20) {
                ss << "Off";
            }
            else if (slider_val < 1000) {
                ss << std::fixed << std::setprecision(0) << (int)slider_val << " Hz";
            }
            else {
                ss << std::fixed << std::setprecision(1) << slider_val / 1000.0 << " kHz";
            }
            text = ss.str();
        }
        else if (format == RotaryLabel::hz1f) {
            if (slider_val < 1000) {
                ss << std::fixed << std::setprecision(1) << slider_val << " Hz";
            } else {
                ss << std::fixed << std::setprecision(1) << slider_val / 1000.0 << " kHz";
            }
            text = ss.str();
        }
        else if (format == RotaryLabel::float1) {
            ss << std::fixed << std::setprecision(1) << slider_val;
            text = ss.str();
        }
        else if (format == RotaryLabel::float2) {
            ss << std::fixed << std::setprecision(2) << slider_val;
            text = ss.str();
        }
        else if (format == RotaryLabel::float2x100) {
            ss << std::fixed << std::setprecision(2) << slider_val * 100;
            text = ss.str();
        }
        else if (format == RotaryLabel::gainTodB1f) {
            if (slider_val > 0.0f) {
                ss << std::fixed << std::setprecision(1) << 20.0f * std::log10(slider_val) << " dB";
                text = ss.str();
            }
            else {
                text = "-Inf";
            }
        }
        else if (format == RotaryLabel::dBfloat1) {
            ss << std::fixed << std::setprecision(1) << slider_val << " dB";
            text = ss.str();
        }
        else if (format == RotaryLabel::audioOffset) {
            ss << std::fixed << std::setprecision(1) << slider_val * LATENCY_MILLIS << " ms";
            text = ss.str();
        } 
        else if (format == envatk) {
            auto val = ENV_MIN_ATTACK + (ENV_MAX_ATTACK - ENV_MIN_ATTACK) * slider_val;
            if (val > 1000) {
                ss << std::fixed << std::setprecision(1) << val << " s";
            }
            else if (val < 1) {
                ss << std::fixed << std::setprecision(2) << val << " ms";
            }
            else {
                ss << std::fixed << std::setprecision(0) << val << " ms";
            }
            text = ss.str();
        }
        else if (format == envrel) {
            auto val = ENV_MIN_RELEASE + (ENV_MAX_RELEASE - ENV_MIN_RELEASE) * slider_val;
            if (val > 1000) {
                ss << std::fixed << std::setprecision(1) << val / 1000.0 << " s";
            }
            else if (val < 1) {
                ss << std::fixed << std::setprecision(2) << val << " ms";
            }
            else {
                ss << std::fixed << std::setprecision(0) << val << " ms";
            }
            text = ss.str();
        }
        else if (format == envhold) {
            auto val = ENV_MIN_HOLD + (ENV_MAX_HOLD - ENV_MIN_HOLD) * slider_val;
            if (val > 100) {
                ss << std::fixed << std::setprecision(0) << val << " ms";
            }
            else {
                ss << std::fixed << std::setprecision(1) << val << " ms";
            }
            text = ss.str();
        }
        else if (format == kMillis) {
            if (slider_val > 1000) {
                text = String((int)std::round(slider_val / 10000) / 10.f) + " s";
            }
            else {
                text = std::to_string((int)std::round(slider_val)) + " ms";
            }
        }
        else if (format == exp2Range) {
            text = std::to_string((int)(std::pow(2, slider_val) * 100)) + " %";
        } 
        else if (format == dryWet) {
            float dryGain, wetGain;

            if (slider_val <= 0.5f) {
                dryGain = 1.0f;
                wetGain = slider_val * 2.0f;
            } else {
                dryGain = (1.0f - slider_val) * 2.0f;
                wetGain = 1.0f;
            }

            text = std::to_string((int)std::round(dryGain * 100)) + ":" +
                std::to_string((int)std::round(wetGain * 100));
        } 
        else if (format == kChoice) {
            text = audioProcessor.params.getParameter(paramId)->getCurrentValueAsText();
        }
    }

    g.setColour(Colours::white);
    g.setFont(16.0f);
    g.drawText(text, 0, getHeight() - 16, getWidth(), 16, juce::Justification::centred, true);
}

void Rotary::mouseDown(const juce::MouseEvent& e) 
{
    if (type != RotaryType::NormalKnob) {
        audioProcessor.setSendEditMode(type == ResKnob);
    }
    e.source.enableUnboundedMouseMovement(true);
    mouse_down = true;
    auto param = audioProcessor.params.getParameter(paramId);
    auto cur_val = param->getValue();
    cur_normed_value = cur_val;
    last_mouse_position = e.getPosition();
    setMouseCursor(MouseCursor::NoCursor);
    start_mouse_pos = Desktop::getInstance().getMousePosition();
    repaint();
    param->beginChangeGesture();
}

void Rotary::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    if (event.mods.isLeftButtonDown() || event.mods.isRightButtonDown()) {
        return; // prevent crash, param is already mutating
    }
    auto speed = (event.mods.isShiftDown() ? 0.01f : 0.05f);
    auto slider_change = wheel.deltaY > 0 ? speed : wheel.deltaY < 0 ? -speed : 0;
    auto param = audioProcessor.params.getParameter(paramId);
    param->beginChangeGesture();
    param->setValueNotifyingHost(param->getValue() + slider_change);
    while (wheel.deltaY > 0.0f && param->getValue() == 0.0f) { // FIX wheel not working when value is zero, first step takes more than 0.05% 
        slider_change += 0.05f;
        param->setValueNotifyingHost(param->getValue() + slider_change);
    }
    param->endChangeGesture();
}

void Rotary::mouseUp(const juce::MouseEvent& e) {
    mouse_down = false;
    setMouseCursor(MouseCursor::NormalCursor);
    e.source.enableUnboundedMouseMovement(false);
    Desktop::getInstance().setMousePosition(start_mouse_pos);
    repaint();
    auto param = audioProcessor.params.getParameter(paramId);
    param->endChangeGesture();
}

void Rotary::mouseDoubleClick(const juce::MouseEvent& e) {
    (void)e;
    auto param = audioProcessor.params.getParameter(paramId);
    param->setValueNotifyingHost(param->getDefaultValue());
}

void Rotary::mouseDrag(const juce::MouseEvent& e) {
    auto change = e.getPosition() - last_mouse_position;
    last_mouse_position = e.getPosition();
    auto speed = (e.mods.isShiftDown() ? 40.0f : 4.0f) * pixels_per_percent;
    auto slider_change = float(change.getX() - change.getY()) / speed;
    cur_normed_value += slider_change;
    auto param = audioProcessor.params.getParameter(paramId);
    param->setValueNotifyingHost(cur_normed_value);
}

void Rotary::draw_rotary_slider(juce::Graphics& g, float slider_pos) {
    auto bounds = getBounds();
    const float radius = 16.0f;
    const float angle = -deg130 + slider_pos * (deg130 - -deg130);
    auto kbounds = Rectangle<float>(bounds.getWidth()/2.0f-radius, bounds.getHeight()/2.0f-radius-8.0f, radius*2.0f, radius*2.0f);

    g.setColour(Colour(COLOR_KNOB).brighter(0.f));
    g.fillEllipse(kbounds);
    g.setColour(Colour(0x33000000));  // semi-transparent black
    g.fillEllipse(kbounds.translated(0, 2.f).reduced(1.0f));

    g.setColour(Colour(COLOR_KNOB));
    juce::Path arcKnob;
    arcKnob.addCentredArc(bounds.getWidth() / 2.0f, bounds.getHeight() / 2.0f - 8.0f, radius + 5.0f, radius + 5.0f, 0,-deg130, deg130, true);
    g.strokePath(arcKnob, PathStrokeType(3.0, PathStrokeType::JointStyle::curved, PathStrokeType::rounded));

    g.setColour(Colour(color));
    if ((isSymmetric && slider_pos != 0.5f) || (!isSymmetric && slider_pos)) {
        juce::Path arcActive;
        arcActive.addCentredArc(bounds.getWidth() / 2.0f, bounds.getHeight() / 2.0f - 8.0f, radius + 5.0f, radius + 5.0f, 0, isSymmetric ? 0 : -deg130, angle, true);
        g.strokePath(arcActive, PathStrokeType(3.0, PathStrokeType::JointStyle::curved, PathStrokeType::rounded));
    }

    g.setColour(Colours::white);
    juce::Path p;
    p.addLineSegment (juce::Line<float>(0.0f, -5.0f, 0.0f, -radius + 5.0f), 0.1f);
    juce::PathStrokeType(3.0f, PathStrokeType::JointStyle::curved, PathStrokeType::rounded).createStrokedPath(p, p);
    g.fillPath (p, juce::AffineTransform::rotation (angle).translated(bounds.getWidth() / 2.0f, bounds.getHeight() / 2.0f - 8.0f));
}