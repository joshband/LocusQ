#pragma once
#include "HeadphonePeqHook.h"
#include <juce_core/juce_core.h>
#include <vector>

namespace locusq::headphone_dsp
{

struct PeqBandSpec
{
    enum class Type { PK, LSC, HSC } type = Type::PK;
    float fcHz   = 1000.0f;
    float gainDb = 0.0f;
    float q      = 0.707f;
};

struct HeadphonePreset
{
    juce::String modelId;
    juce::String mode;
    float preampDb = 0.0f;
    std::vector<PeqBandSpec> bands;
    bool valid = false;
};

inline HeadphonePreset loadHeadphonePreset (const juce::File& yamlFile)
{
    HeadphonePreset result;
    if (! yamlFile.existsAsFile())
        return result;

    const auto lines = juce::StringArray::fromLines (yamlFile.loadFileAsString());
    for (const auto& line : lines)
    {
        const auto t = line.trim();
        if (t.startsWith ("hp_model_id:"))
            result.modelId = t.fromFirstOccurrenceOf (":", false, false).trim();
        else if (t.startsWith ("hp_mode:"))
            result.mode = t.fromFirstOccurrenceOf (":", false, false).trim();
        else if (t.startsWith ("preamp_db:"))
            result.preampDb = t.fromFirstOccurrenceOf (":", false, false).trim().getFloatValue();
        else if (t.startsWith ("- {"))
        {
            // Parse: - {type: PK, fc_hz: 200, gain_db: -2.1, q: 1.2}
            PeqBandSpec band;
            const auto inner = t.fromFirstOccurrenceOf ("{", false, false)
                                .upToLastOccurrenceOf ("}", false, false);
            for (const auto& tok : juce::StringArray::fromTokens (inner, ",", "\""))
            {
                const auto k = tok.upToFirstOccurrenceOf (":", false, false).trim();
                const auto v = tok.fromFirstOccurrenceOf (":", false, false).trim();
                if      (k == "type")
                {
                    if (v == "LSC") band.type = PeqBandSpec::Type::LSC;
                    else if (v == "HSC") band.type = PeqBandSpec::Type::HSC;
                    // else PK (default)
                }
                else if (k == "fc_hz")   band.fcHz   = v.getFloatValue();
                else if (k == "gain_db") band.gainDb = v.getFloatValue();
                else if (k == "q")       band.q      = v.getFloatValue();
            }
            result.bands.push_back (band);
        }
    }
    result.valid = true;
    return result;
}

} // namespace locusq::headphone_dsp
