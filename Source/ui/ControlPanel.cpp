#include "ControlPanel.h"
#include "Palette.h"
#include "../Parameters.h"

namespace zither::ui
{
namespace
{
    using Apvts = juce::AudioProcessorValueTreeState;

    constexpr int kTabHeight = 30;
    constexpr int kCellWidth = 92;
    constexpr int kNameHeight = 16;
    constexpr int kValueHeight = 14;

    /** One control plus its lowercase name and faint value text. */
    class Cell : public juce::Component
    {
    public:
        explicit Cell (juce::String nameToUse) : name (std::move (nameToUse)) {}

        void paint (juce::Graphics& g) override
        {
            auto area = getLocalBounds();
            g.setColour (palette::mist.withAlpha (0.85f));
            g.setFont (palette::font (11.0f, 0.22f));
            g.drawText (name, area.removeFromBottom (kNameHeight), juce::Justification::centred);

            g.setColour (palette::pearl.withAlpha (0.45f));
            g.setFont (palette::font (11.0f, 0.04f));
            g.drawText (getValueText(), area.removeFromBottom (kValueHeight), juce::Justification::centred);
        }

        void resized() override
        {
            auto area = getLocalBounds();
            area.removeFromBottom (kNameHeight + kValueHeight);
            layoutControl (area);
        }

    protected:
        virtual juce::String getValueText() { return {}; }
        virtual void layoutControl (juce::Rectangle<int>) = 0;

    private:
        juce::String name;
    };

    class KnobCell final : public Cell
    {
    public:
        KnobCell (Apvts& state, const char* id, juce::String nameToUse) : Cell (std::move (nameToUse))
        {
            slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
            slider.onValueChange = [this] { repaint(); };
            addAndMakeVisible (slider);
            attachment = std::make_unique<Apvts::SliderAttachment> (state, id, slider);
        }

    private:
        juce::String getValueText() override { return slider.getTextFromValue (slider.getValue()); }

        void layoutControl (juce::Rectangle<int> area) override
        {
            const int size = juce::jmin (area.getWidth(), area.getHeight(), 58);
            slider.setBounds (area.withSizeKeepingCentre (size, size));
        }

        juce::Slider slider;
        std::unique_ptr<Apvts::SliderAttachment> attachment;
    };

    class ChoiceCell final : public Cell
    {
    public:
        ChoiceCell (Apvts& state, const char* id, juce::String nameToUse) : Cell (std::move (nameToUse))
        {
            if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (state.getParameter (id)))
                for (int i = 0; i < choice->choices.size(); ++i)
                    combo.addItem (choice->choices[i].toLowerCase(), i + 1);

            addAndMakeVisible (combo);
            attachment = std::make_unique<Apvts::ComboBoxAttachment> (state, id, combo);
        }

    private:
        void layoutControl (juce::Rectangle<int> area) override
        {
            combo.setBounds (area.withSizeKeepingCentre (area.getWidth() - 4, 28));
        }

        juce::ComboBox combo;
        std::unique_ptr<Apvts::ComboBoxAttachment> attachment;
    };

    class ToggleCell final : public Cell
    {
    public:
        ToggleCell (Apvts& state, const char* id, juce::String nameToUse) : Cell (std::move (nameToUse))
        {
            toggle.onStateChange = [this] { repaint(); };
            addAndMakeVisible (toggle);
            attachment = std::make_unique<Apvts::ButtonAttachment> (state, id, toggle);
        }

    private:
        juce::String getValueText() override { return toggle.getToggleState() ? "on" : "off"; }

        void layoutControl (juce::Rectangle<int> area) override
        {
            toggle.setBounds (area.withSizeKeepingCentre (44, 28));
        }

        juce::ToggleButton toggle;
        std::unique_ptr<Apvts::ButtonAttachment> attachment;
    };

    enum class Kind { knob, choice, toggle };

    struct ControlSpec
    {
        Kind kind;
        const char* id;
        const char* name;
    };

    struct PageSpec
    {
        const char* title;
        std::vector<ControlSpec> controls;
    };

    std::vector<PageSpec> pageSpecs()
    {
        using namespace zither::param;
        return {
            { "tune",   { { Kind::choice, root, "root" }, { Kind::choice, mode, "mode" },
                          { Kind::knob, numStrings, "strings" }, { Kind::knob, lowestOctave, "octave" } } },
            { "strike", { { Kind::choice, exciter, "strike" }, { Kind::knob, hardness, "hardness" },
                          { Kind::knob, strikePos, "position" } } },
            { "string", { { Kind::knob, decay, "decay" }, { Kind::knob, brightness, "brightness" },
                          { Kind::knob, inharmonicity, "stiffness" }, { Kind::knob, damping, "damping" } } },
            { "strum",  { { Kind::toggle, strum, "strum" }, { Kind::knob, strumSpeed, "speed" },
                          { Kind::knob, strumSpan, "span" }, { Kind::choice, strumDir, "direction" },
                          { Kind::knob, humanize, "humanize" } } },
            { "space",  { { Kind::knob, phaserMix, "phaser" }, { Kind::knob, phaserRate, "drift" },
                          { Kind::knob, delayTime, "echo" }, { Kind::knob, delayFeedback, "feedback" },
                          { Kind::knob, delayMix, "echo mix" }, { Kind::knob, reverbSize, "depth" },
                          { Kind::knob, reverbMix, "reverb" }, { Kind::knob, output, "output" } } },
        };
    }
} // namespace

class ControlPanel::Tab final : public juce::Component
{
public:
    Tab (juce::String titleToUse, std::function<void()> onClick)
        : title (std::move (titleToUse)), clicked (std::move (onClick))
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void setActive (bool shouldBeActive)
    {
        active = shouldBeActive;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();
        const float alpha = active ? 1.0f : (isMouseOver() ? 0.7f : 0.42f);
        g.setColour (palette::pearl.withAlpha (alpha));
        g.setFont (palette::font (12.0f, 0.3f));
        g.drawText (title, bounds, juce::Justification::centred);

        if (active)
        {
            const auto line = juce::Rectangle<float> (18.0f, 1.5f).withCentre ({ bounds.getCentreX(), bounds.getBottom() - 4.0f });
            g.setColour (palette::aqua.withAlpha (0.3f));
            g.fillRoundedRectangle (line.expanded (3.0f, 1.5f), 2.0f);
            g.setColour (palette::aqua);
            g.fillRoundedRectangle (line, 0.75f);
        }
    }

    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }
    void mouseUp (const juce::MouseEvent& e) override
    {
        if (e.mouseWasClicked() && clicked)
            clicked();
    }

private:
    juce::String title;
    std::function<void()> clicked;
    bool active = false;
};

class ControlPanel::Page final : public juce::Component
{
public:
    Page (Apvts& state, const std::vector<ControlSpec>& controls)
    {
        for (const auto& c : controls)
        {
            std::unique_ptr<Cell> cell;
            switch (c.kind)
            {
                case Kind::knob:   cell = std::make_unique<KnobCell> (state, c.id, c.name); break;
                case Kind::choice: cell = std::make_unique<ChoiceCell> (state, c.id, c.name); break;
                case Kind::toggle: cell = std::make_unique<ToggleCell> (state, c.id, c.name); break;
            }
            addAndMakeVisible (*cell);
            cells.push_back (std::move (cell));
        }
    }

    void resized() override
    {
        const int count = static_cast<int> (cells.size());
        const int width = juce::jmin (kCellWidth, getWidth() / juce::jmax (1, count));
        int x = (getWidth() - width * count) / 2;
        for (auto& cell : cells)
        {
            cell->setBounds (x, 0, width, getHeight());
            x += width;
        }
    }

private:
    std::vector<std::unique_ptr<Cell>> cells;
};

ControlPanel::ControlPanel (Apvts& state)
{
    const auto specs = pageSpecs();
    for (size_t i = 0; i < specs.size(); ++i)
    {
        const int index = static_cast<int> (i);
        tabs.push_back (std::make_unique<Tab> (specs[i].title, [this, index] { showPage (index); }));
        addAndMakeVisible (*tabs.back());

        pages.push_back (std::make_unique<Page> (state, specs[i].controls));
        addChildComponent (*pages.back());
    }

    showPage (0);
}

ControlPanel::~ControlPanel() = default;

void ControlPanel::showPage (int index)
{
    currentPage = index;
    for (size_t i = 0; i < pages.size(); ++i)
    {
        const bool active = static_cast<int> (i) == index;
        pages[i]->setVisible (active);
        tabs[i]->setActive (active);
    }
}

void ControlPanel::paint (juce::Graphics& g)
{
    const auto glass = getLocalBounds().toFloat().reduced (0.5f);

    // Frosted glass: a faint fill, a brighter top edge where light hits it, and a hairline border.
    g.setColour (palette::abyss.withAlpha (0.45f));
    g.fillRoundedRectangle (glass, 14.0f);
    g.setGradientFill (juce::ColourGradient::vertical (palette::pearl.withAlpha (0.07f), glass.getY(),
                                                       palette::pearl.withAlpha (0.015f), glass.getBottom()));
    g.fillRoundedRectangle (glass, 14.0f);
    g.setColour (palette::pearl.withAlpha (0.1f));
    g.drawRoundedRectangle (glass, 14.0f, 1.0f);
}

void ControlPanel::resized()
{
    auto area = getLocalBounds().reduced (12, 6);
    auto tabRow = area.removeFromTop (kTabHeight);

    const int tabWidth = juce::jmin (84, tabRow.getWidth() / juce::jmax (1, static_cast<int> (tabs.size())));
    tabRow = tabRow.withSizeKeepingCentre (tabWidth * static_cast<int> (tabs.size()), tabRow.getHeight());
    for (auto& tab : tabs)
        tab->setBounds (tabRow.removeFromLeft (tabWidth));

    area.removeFromTop (6);
    for (auto& page : pages)
        page->setBounds (area);
}
} // namespace zither::ui
