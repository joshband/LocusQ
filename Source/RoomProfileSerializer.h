#pragma once

#include "SceneGraph.h"
#include <juce_core/juce_core.h>
#include <algorithm>

//==============================================================================
/**
 * RoomProfileSerializer
 *
 * Serialises and deserialises RoomProfile to/from JSON using JUCE's var system.
 *
 * JSON schema (version 1):
 * {
 *   "version": 1,
 *   "estimatedRT60": 0.42,
 *   "roomDimensions": { "w": 6, "d": 4, "h": 3 },
 *   "listenerPos":    { "x": 0, "y": 0, "z": 0 },
 *   "speakers": [
 *     {
 *       "id": 0,
 *       "position":  { "x": -2.5, "y": 0, "z": 2.0 },
 *       "distance":  2.69,
 *       "angle":    -45.0,
 *       "height":    1.2,
 *       "delayComp": 7.84,
 *       "gainTrim":  0.5,
 *       "frequencyResponse": [ ... 256 floats ... ]
 *     },
 *     ...
 *   ]
 * }
 */
class RoomProfileSerializer
{
public:
    //==========================================================================
    static bool saveToFile (const RoomProfile& profile, const juce::File& file)
    {
        return file.replaceWithText (toJSON (profile));
    }

    static bool loadFromFile (RoomProfile& profile, const juce::File& file)
    {
        if (! file.existsAsFile()) return false;
        return fromJSON (profile, file.loadFileAsString());
    }

    //==========================================================================
    static juce::String toJSON (const RoomProfile& p)
    {
        auto* root = new juce::DynamicObject();
        root->setProperty ("version",       1);
        root->setProperty ("estimatedRT60", p.estimatedRT60);

        // Room dimensions
        auto* dims = new juce::DynamicObject();
        dims->setProperty ("w", p.dimensions.x);
        dims->setProperty ("d", p.dimensions.y);
        dims->setProperty ("h", p.dimensions.z);
        root->setProperty ("roomDimensions", juce::var (dims));

        // Listener position
        root->setProperty ("listenerPos", vec3ToVar (p.listenerPos));

        // Speakers array
        juce::Array<juce::var> speakerArr;
        for (int i = 0; i < 4; ++i)
        {
            const auto& s = p.speakers[i];
            auto* spk = new juce::DynamicObject();
            spk->setProperty ("id",        i);
            spk->setProperty ("position",  vec3ToVar (s.position));
            spk->setProperty ("distance",  s.distance);
            spk->setProperty ("angle",     s.angle);
            spk->setProperty ("height",    s.height);
            spk->setProperty ("delayComp", s.delayComp);
            spk->setProperty ("gainTrim",  s.gainTrim);

            juce::Array<juce::var> fr;
            for (float f : s.frequencyResponse) fr.add (f);
            spk->setProperty ("frequencyResponse", fr);

            speakerArr.add (juce::var (spk));
        }
        root->setProperty ("speakers", speakerArr);

        return juce::JSON::toString (juce::var (root), false);
    }

    //==========================================================================
    static bool fromJSON (RoomProfile& p, const juce::String& json)
    {
        juce::var v = juce::JSON::parse (json);
        auto* root  = v.getDynamicObject();
        if (root == nullptr) return false;

        p.estimatedRT60 = static_cast<float> (root->getProperty ("estimatedRT60"));

        if (auto* dims = root->getProperty ("roomDimensions").getDynamicObject())
        {
            p.dimensions.x = static_cast<float> (dims->getProperty ("w"));
            p.dimensions.y = static_cast<float> (dims->getProperty ("d"));
            p.dimensions.z = static_cast<float> (dims->getProperty ("h"));
        }

        if (auto* lp = root->getProperty ("listenerPos").getDynamicObject())
            varToVec3 (root->getProperty ("listenerPos"), p.listenerPos);

        auto spkArr = root->getProperty ("speakers");
        if (spkArr.isArray())
        {
            for (int i = 0; i < std::min (4, spkArr.size()); ++i)
            {
                auto& s = p.speakers[i];
                if (auto* spk = spkArr[i].getDynamicObject())
                {
                    varToVec3 (spk->getProperty ("position"), s.position);
                    s.distance  = static_cast<float> (spk->getProperty ("distance"));
                    s.angle     = static_cast<float> (spk->getProperty ("angle"));
                    s.height    = static_cast<float> (spk->getProperty ("height"));
                    s.delayComp = static_cast<float> (spk->getProperty ("delayComp"));
                    s.gainTrim  = static_cast<float> (spk->getProperty ("gainTrim"));

                    auto fr = spk->getProperty ("frequencyResponse");
                    if (fr.isArray())
                    {
                        int bins = std::min (SpeakerProfile::NUM_FREQ_BINS, fr.size());
                        for (int b = 0; b < bins; ++b)
                            s.frequencyResponse[b] = static_cast<float> (fr[b]);
                    }
                }
            }
        }

        p.valid = true;
        return true;
    }

private:
    static juce::var vec3ToVar (const Vec3& v)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("x", v.x);
        obj->setProperty ("y", v.y);
        obj->setProperty ("z", v.z);
        return juce::var (obj);
    }

    static void varToVec3 (const juce::var& v, Vec3& out)
    {
        if (auto* obj = v.getDynamicObject())
        {
            out.x = static_cast<float> (obj->getProperty ("x"));
            out.y = static_cast<float> (obj->getProperty ("y"));
            out.z = static_cast<float> (obj->getProperty ("z"));
        }
    }
};
