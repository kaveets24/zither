#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>
#include <vector>

namespace zither::ui
{
/**
    One frosted-glass panel with paged controls (tune · strike · string · strum · space),
    so only a handful of controls show at a time.
*/
class ControlPanel final : public juce::Component
{
public:
    explicit ControlPanel (juce::AudioProcessorValueTreeState&);
    ~ControlPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class Tab;
    class Page;

    void showPage (int index);

    std::vector<std::unique_ptr<Tab>> tabs;
    std::vector<std::unique_ptr<Page>> pages;
    int currentPage = 0;
};
} // namespace zither::ui
