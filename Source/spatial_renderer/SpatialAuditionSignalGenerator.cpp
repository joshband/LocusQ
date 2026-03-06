#include "../SpatialRenderer.h"

float SpatialRenderer::generateAuditionSignalSample() noexcept
{
    const auto sampleRate = juce::jmax (1.0, currentSampleRate);

    switch (auditionSignalTypeIndex)
    {
        case 0: // Sine 440 Hz
            return advanceAuditionOscillator (440.0, auditionPhasePrimary);
        case 1: // Dual tone 220 + 880 Hz
            return 0.7f * advanceAuditionOscillator (220.0, auditionPhasePrimary)
                 + 0.3f * advanceAuditionOscillator (880.0, auditionPhaseSecondary);
        case 2: // Soft pink-like noise (simple filtered white)
        {
            const auto white = nextAuditionWhiteNoise();
            auditionNoiseOnePole = 0.985f * auditionNoiseOnePole + 0.015f * white;
            return auditionNoiseOnePole;
        }
        case 3: // Rain field with random droplets
        {
            const auto white = nextAuditionWhiteNoise();
            auditionRainBed = 0.9986f * auditionRainBed + 0.0014f * white;
            const auto rainHissRaw = white - auditionRainBed;
            auditionNoiseOnePole = 0.95f * auditionNoiseOnePole + 0.05f * rainHissRaw;

            const auto triggerRateHz = qualityHigh ? 42.0f : 31.0f;
            if (nextAuditionRand01() < triggerRateHz / static_cast<float> (sampleRate))
            {
                auditionRainDropEnv = juce::jmin (1.0f, auditionRainDropEnv + (0.26f + 0.50f * nextAuditionRand01()));
                auto randSquared = nextAuditionRand01();
                randSquared *= randSquared;
                auditionRainDropFreqHz = 620.0f + (3600.0f * randSquared);
            }

            auditionRainDropPhase += static_cast<double> (auditionRainDropFreqHz) / sampleRate;
            auditionRainDropPhase -= std::floor (auditionRainDropPhase);
            const auto rainPhase = static_cast<float> (juce::MathConstants<double>::twoPi * auditionRainDropPhase);
            const auto rainSine = static_cast<float> (std::sin (rainPhase));
            const auto rainSparkle = rainSine * std::abs (rainSine);
            const auto droplet = (0.72f * rainSine + 0.28f * rainSparkle) * auditionRainDropEnv;
            const auto splash = 0.11f * nextAuditionWhiteNoise() * auditionRainDropEnv;
            auditionRainDropEnv *= qualityHigh ? 0.9942f : 0.9930f;

            return 0.52f * auditionNoiseOnePole + 0.58f * droplet + splash;
        }
        case 4: // Snow drift (soft airy noise)
        {
            const auto white = nextAuditionWhiteNoise();
            auditionSnowBed = 0.99962f * auditionSnowBed + 0.00038f * white;
            const auto airyResidual = white - auditionSnowBed;
            auditionSnowShimmer = 0.9973f * auditionSnowShimmer + 0.0027f * airyResidual;

            auditionSnowFlutterPhase += 0.075 / sampleRate;
            auditionSnowFlutterPhase -= std::floor (auditionSnowFlutterPhase);
            auditionPhaseSecondary += 0.24 / sampleRate;
            auditionPhaseSecondary -= std::floor (auditionPhaseSecondary);

            const auto flutter = 0.86f + 0.14f * static_cast<float> (std::sin (juce::MathConstants<double>::twoPi * auditionSnowFlutterPhase));
            const auto shimmerMod = 0.22f + 0.78f * (0.5f + 0.5f * static_cast<float> (std::sin (juce::MathConstants<double>::twoPi * auditionPhaseSecondary)));
            const auto airy = 0.66f * auditionSnowBed + 0.34f * (0.86f * auditionSnowShimmer + 0.14f * airyResidual);
            const auto shimmer = 0.12f * auditionSnowShimmer * shimmerMod;
            const auto lowJitterBreath = 0.05f * static_cast<float> (std::sin (
                juce::MathConstants<double>::twoPi * (auditionSnowFlutterPhase + 0.17 * auditionPhaseSecondary)));

            return airy * flutter + shimmer + lowJitterBreath;
        }
        case 5: // Bouncing balls (clustered impacts)
        {
            bool triggerBounce = false;
            if (auditionBounceCountdownSamples > 0)
            {
                --auditionBounceCountdownSamples;
            }
            else if (auditionBounceClusterRemaining > 0)
            {
                triggerBounce = true;
                --auditionBounceClusterRemaining;
                if (auditionBounceClusterRemaining > 0)
                {
                    auditionBounceSpacingSamples = juce::jmax (44.0f, auditionBounceSpacingSamples * (0.58f + 0.08f * nextAuditionRand01()));
                    auditionBounceCountdownSamples = static_cast<int> (std::round (auditionBounceSpacingSamples));
                }
                else
                {
                    auditionBounceCooldownSamples = static_cast<int> (
                        std::round ((0.45f + 0.90f * nextAuditionRand01()) * static_cast<float> (sampleRate)));
                }
            }
            else
            {
                if (auditionBounceCooldownSamples > 0)
                    --auditionBounceCooldownSamples;

                if (auditionBounceCooldownSamples <= 0 && nextAuditionRand01() < static_cast<float> (1.4 / sampleRate))
                {
                    auditionBounceClusterRemaining = 3 + static_cast<int> (nextAuditionRand01() * 6.0f);
                    auditionBounceSpacingSamples = (0.16f + 0.14f * nextAuditionRand01()) * static_cast<float> (sampleRate);
                    triggerBounce = true;
                    --auditionBounceClusterRemaining;
                    if (auditionBounceClusterRemaining > 0)
                        auditionBounceCountdownSamples = static_cast<int> (std::round (auditionBounceSpacingSamples));
                }
            }

            if (triggerBounce)
            {
                const auto spacingNorm = juce::jlimit (0.0f, 1.0f, auditionBounceSpacingSamples / (0.36f * static_cast<float> (sampleRate)));
                const auto impact = 0.24f + 0.76f * spacingNorm;
                auditionBounceEnv = juce::jmax (auditionBounceEnv, impact);
                auto randSquared = nextAuditionRand01();
                randSquared *= randSquared;
                const auto targetFreq = 130.0f + (680.0f * spacingNorm) + (220.0f * randSquared);
                auditionBounceFreqHz = 0.55f * auditionBounceFreqHz + 0.45f * targetFreq;
            }

            auditionBouncePhase += static_cast<double> (auditionBounceFreqHz) / sampleRate;
            auditionBouncePhase -= std::floor (auditionBouncePhase);
            const auto bouncePhase = static_cast<float> (juce::MathConstants<double>::twoPi * auditionBouncePhase);
            const auto tonal = (0.76f * static_cast<float> (std::sin (bouncePhase))
                              + 0.18f * static_cast<float> (std::sin (bouncePhase * 2.35f))
                              + 0.06f * static_cast<float> (std::sin (bouncePhase * 3.70f)))
                * auditionBounceEnv;
            auditionNoiseOnePole = 0.90f * auditionNoiseOnePole + 0.10f * nextAuditionWhiteNoise();
            const auto thud = auditionNoiseOnePole * (0.26f * auditionBounceEnv);
            auto impactStrike = auditionBounceEnv;
            impactStrike *= impactStrike;
            impactStrike *= impactStrike;
            const auto impactClick = 0.22f * nextAuditionWhiteNoise() * impactStrike;
            auditionBounceEnv *= qualityHigh ? 0.9960f : 0.9948f;

            return 0.74f * tonal + thud + impactClick;
        }
        case 6: // Wind chimes (metallic resonant pings)
        {
            if (auditionChimeCooldownSamples > 0)
                --auditionChimeCooldownSamples;

            if (auditionChimeEnv < 1.0e-4f
                && auditionChimeCooldownSamples <= 0
                && nextAuditionRand01() < static_cast<float> ((qualityHigh ? 1.05f : 0.80f) / sampleRate))
            {
                static constexpr std::array<float, 6> kChimeNotes {
                    392.0f, 523.25f, 659.25f, 783.99f, 987.77f, 1174.66f
                };
                static constexpr std::array<float, 4> kChimeRatios {
                    1.50f, 1.6666666f, 2.0f, 2.5f
                };
                const auto noteIndex = juce::jlimit (
                    0,
                    static_cast<int> (kChimeNotes.size()) - 1,
                    static_cast<int> (nextAuditionRand01() * static_cast<float> (kChimeNotes.size())));
                const auto ratioIndex = juce::jlimit (
                    0,
                    static_cast<int> (kChimeRatios.size()) - 1,
                    static_cast<int> (nextAuditionRand01() * static_cast<float> (kChimeRatios.size())));
                auditionChimeFreqA = kChimeNotes[static_cast<size_t> (noteIndex)];
                auditionChimeFreqB = auditionChimeFreqA * kChimeRatios[static_cast<size_t> (ratioIndex)];
                auditionChimeEnv = 0.88f + 0.12f * nextAuditionRand01();
                auditionChimeCooldownSamples = static_cast<int> (
                    std::round ((0.14f + 0.44f * nextAuditionRand01()) * static_cast<float> (sampleRate)));
            }

            auditionChimePhaseA += static_cast<double> (auditionChimeFreqA) / sampleRate;
            auditionChimePhaseB += static_cast<double> (auditionChimeFreqB) / sampleRate;
            auditionChimePhaseA -= std::floor (auditionChimePhaseA);
            auditionChimePhaseB -= std::floor (auditionChimePhaseB);
            const auto chimePhaseA = static_cast<float> (juce::MathConstants<double>::twoPi * auditionChimePhaseA);
            const auto chimePhaseB = static_cast<float> (juce::MathConstants<double>::twoPi * auditionChimePhaseB);
            const auto body = (0.58f * static_cast<float> (std::sin (chimePhaseA))
                + 0.26f * static_cast<float> (std::sin (chimePhaseB))
                + 0.10f * static_cast<float> (std::sin (0.5f * (chimePhaseA + chimePhaseB)))
                + 0.06f * static_cast<float> (std::sin (1.618f * chimePhaseA + 0.37f * chimePhaseB)))
                * auditionChimeEnv;
            auto strikeEnv = auditionChimeEnv;
            strikeEnv *= strikeEnv;
            strikeEnv *= strikeEnv;
            strikeEnv *= strikeEnv;
            strikeEnv *= strikeEnv;
            const auto strike = (0.72f * static_cast<float> (std::sin (chimePhaseA * 2.75f))
                + 0.28f * static_cast<float> (std::sin (chimePhaseB * 1.90f)))
                * strikeEnv;
            auditionChimeEnv *= qualityHigh ? 0.99976f : 0.99962f;
            auditionChimeShimmer = 0.992f * auditionChimeShimmer + 0.008f * std::abs (body);
            return 0.70f * body + 0.24f * strike + 0.10f * auditionChimeShimmer;
        }
        case 7: // Crickets (narrow-band chirp swarms)
        {
            if (auditionCricketCooldownSamples > 0)
                --auditionCricketCooldownSamples;

            if (auditionCricketBurstSamples <= 0
                && auditionCricketCooldownSamples <= 0
                && nextAuditionRand01() < static_cast<float> (1.0 / sampleRate))
            {
                auditionCricketBurstSamples = static_cast<int> (
                    std::round ((0.06f + 0.12f * nextAuditionRand01()) * static_cast<float> (sampleRate)));
                auditionCricketCooldownSamples = static_cast<int> (
                    std::round ((0.20f + 0.58f * nextAuditionRand01()) * static_cast<float> (sampleRate)));
                auditionCricketFreqHz = 3200.0f + 3800.0f * nextAuditionRand01();
                auditionCricketEnv = juce::jmax (auditionCricketEnv, 0.72f + 0.22f * nextAuditionRand01());
            }

            if (auditionCricketBurstSamples > 0)
            {
                --auditionCricketBurstSamples;
                auditionCricketEnv = juce::jmin (1.0f, auditionCricketEnv + 0.016f);
            }
            else
            {
                auditionCricketEnv *= 0.9975f;
            }

            auditionCricketPhase += static_cast<double> (auditionCricketFreqHz) / sampleRate;
            auditionCricketPhase -= std::floor (auditionCricketPhase);
            auditionPhaseSecondary += 34.0 / sampleRate;
            auditionPhaseSecondary -= std::floor (auditionPhaseSecondary);

            const auto cricketCarrier = static_cast<float> (std::sin (
                juce::MathConstants<double>::twoPi * auditionCricketPhase));
            const auto pulseRaw = 0.5f + 0.5f * static_cast<float> (std::sin (
                juce::MathConstants<double>::twoPi * auditionPhaseSecondary));
            const auto pulse = pulseRaw * pulseRaw * pulseRaw;
            const auto buzz = 0.18f * nextAuditionWhiteNoise();
            return (0.82f * cricketCarrier + buzz) * auditionCricketEnv * pulse;
        }
        case 8: // Song birds (warbled chirp phrases)
        {
            if (auditionBirdCooldownSamples > 0)
                --auditionBirdCooldownSamples;

            if (auditionBirdPhraseSamples <= 0
                && auditionBirdCooldownSamples <= 0
                && nextAuditionRand01() < static_cast<float> (0.72 / sampleRate))
            {
                auditionBirdPhraseSamples = static_cast<int> (
                    std::round ((0.16f + 0.34f * nextAuditionRand01()) * static_cast<float> (sampleRate)));
                auditionBirdCooldownSamples = static_cast<int> (
                    std::round ((0.26f + 0.66f * nextAuditionRand01()) * static_cast<float> (sampleRate)));
                auditionBirdFreqA = 880.0f + 1900.0f * nextAuditionRand01();
                auditionBirdFreqB = auditionBirdFreqA * (1.42f + 0.36f * nextAuditionRand01());
                auditionBirdEnv = 1.0f;
            }

            if (auditionBirdPhraseSamples > 0)
                --auditionBirdPhraseSamples;

            auditionBirdWarblePhase += (2.2 + 3.4 * nextAuditionRand01()) / sampleRate;
            auditionBirdWarblePhase -= std::floor (auditionBirdWarblePhase);
            const auto warble = static_cast<float> (std::sin (
                juce::MathConstants<double>::twoPi * auditionBirdWarblePhase));
            const auto trill = 0.5f + 0.5f * static_cast<float> (std::sin (
                juce::MathConstants<double>::twoPi * auditionBirdWarblePhase * 7.5));

            const auto freqA = auditionBirdFreqA * (1.0f + 0.18f * warble);
            const auto freqB = auditionBirdFreqB * (1.0f + 0.12f * static_cast<float> (std::sin (
                juce::MathConstants<double>::twoPi * auditionBirdWarblePhase * 1.7)));
            auditionBirdPhaseA += static_cast<double> (freqA) / sampleRate;
            auditionBirdPhaseB += static_cast<double> (freqB) / sampleRate;
            auditionBirdPhaseA -= std::floor (auditionBirdPhaseA);
            auditionBirdPhaseB -= std::floor (auditionBirdPhaseB);

            const auto birdA = static_cast<float> (std::sin (
                juce::MathConstants<double>::twoPi * auditionBirdPhaseA));
            const auto birdB = static_cast<float> (std::sin (
                juce::MathConstants<double>::twoPi * auditionBirdPhaseB));
            const auto whistle = 0.72f * birdA + 0.28f * birdB;

            if (auditionBirdPhraseSamples > 0)
                auditionBirdEnv *= 0.99935f;
            else
                auditionBirdEnv *= 0.9958f;

            const auto ambience = 0.06f * auditionSnowShimmer;
            return whistle * auditionBirdEnv * (0.55f + 0.45f * trill) + ambience;
        }
        case 9: // Karplus plucks (physical string model)
        {
            if (auditionKarplusCooldownSamples > 0)
                --auditionKarplusCooldownSamples;

            if (auditionKarplusCooldownSamples <= 0
                && nextAuditionRand01() < static_cast<float> (0.85 / sampleRate))
            {
                static constexpr std::array<float, 10> kPluckNotes {
                    110.0f, 123.47f, 146.83f, 164.81f, 196.0f,
                    220.0f, 246.94f, 293.66f, 329.63f, 392.0f
                };
                const auto noteIndex = juce::jlimit (
                    0,
                    static_cast<int> (kPluckNotes.size()) - 1,
                    static_cast<int> (nextAuditionRand01() * static_cast<float> (kPluckNotes.size())));
                const auto noteHz = kPluckNotes[static_cast<size_t> (noteIndex)] * (0.98f + 0.05f * nextAuditionRand01());
                auditionKarplusDelaySamples = juce::jlimit (
                    24,
                    kAuditionKarplusMaxDelaySamples - 2,
                    static_cast<int> (std::round (sampleRate / juce::jmax (50.0f, noteHz))));
                auditionKarplusDamping = qualityHigh ? (0.992f + 0.004f * nextAuditionRand01())
                                                     : (0.986f + 0.004f * nextAuditionRand01());
                for (int i = 0; i < auditionKarplusDelaySamples; ++i)
                    auditionKarplusDelayLine[static_cast<size_t> (i)] = 0.78f * nextAuditionWhiteNoise();
                auditionKarplusWriteIndex = 0;
                auditionKarplusEnv = 1.0f;
                auditionKarplusCooldownSamples = static_cast<int> (
                    std::round ((0.15f + 0.30f * nextAuditionRand01()) * static_cast<float> (sampleRate)));
            }

            const auto delayLength = juce::jlimit (24, kAuditionKarplusMaxDelaySamples - 2, auditionKarplusDelaySamples);
            int readIndex = auditionKarplusWriteIndex - delayLength;
            if (readIndex < 0)
                readIndex += kAuditionKarplusMaxDelaySamples;
            const int readNextIndex = (readIndex + 1) % kAuditionKarplusMaxDelaySamples;
            const auto delayed = auditionKarplusDelayLine[static_cast<size_t> (readIndex)];
            const auto delayedNext = auditionKarplusDelayLine[static_cast<size_t> (readNextIndex)];
            const auto filtered = 0.5f * (delayed + delayedNext) * auditionKarplusDamping;
            auditionKarplusDelayLine[static_cast<size_t> (auditionKarplusWriteIndex)] = filtered;
            auditionKarplusWriteIndex = (auditionKarplusWriteIndex + 1) % kAuditionKarplusMaxDelaySamples;
            auditionKarplusEnv *= 0.99970f;
            return delayed * auditionKarplusEnv;
        }
        case 10: // Membrane drops (physical modal impacts)
        {
            if (auditionMembraneCooldownSamples > 0)
                --auditionMembraneCooldownSamples;

            if (auditionMembraneCooldownSamples <= 0
                && nextAuditionRand01() < static_cast<float> (0.95 / sampleRate))
            {
                auto randSquared = nextAuditionRand01();
                randSquared *= randSquared;
                auditionMembraneFreqA = 120.0f + 260.0f * randSquared;
                auditionMembraneFreqB = auditionMembraneFreqA * (1.55f + 0.25f * nextAuditionRand01());
                auditionMembraneEnv = 1.0f;
                auditionMembraneCooldownSamples = static_cast<int> (
                    std::round ((0.24f + 0.44f * nextAuditionRand01()) * static_cast<float> (sampleRate)));
            }

            auditionMembranePhaseA += static_cast<double> (auditionMembraneFreqA) / sampleRate;
            auditionMembranePhaseB += static_cast<double> (auditionMembraneFreqB) / sampleRate;
            auditionMembranePhaseA -= std::floor (auditionMembranePhaseA);
            auditionMembranePhaseB -= std::floor (auditionMembranePhaseB);
            const auto modeA = static_cast<float> (std::sin (
                juce::MathConstants<double>::twoPi * auditionMembranePhaseA));
            const auto modeB = static_cast<float> (std::sin (
                juce::MathConstants<double>::twoPi * auditionMembranePhaseB));
            const auto body = (0.70f * modeA + 0.30f * modeB) * auditionMembraneEnv;
            auto strikeEnv = auditionMembraneEnv;
            strikeEnv *= strikeEnv;
            strikeEnv *= strikeEnv;
            const auto strike = 0.28f * nextAuditionWhiteNoise() * strikeEnv;
            auditionMembraneEnv *= 0.99920f;
            return body + strike;
        }
        case 11: // Krell patch (generative synth glide)
        {
            if (auditionKrellStepSamples <= 0)
            {
                static constexpr std::array<float, 10> kKrellRatios {
                    1.0f, 1.122462f, 1.189207f, 1.334840f, 1.414214f,
                    1.587401f, 1.681793f, 1.887749f, 2.0f, 2.244924f
                };
                const auto ratioIndex = juce::jlimit (
                    0,
                    static_cast<int> (kKrellRatios.size()) - 1,
                    static_cast<int> (nextAuditionRand01() * static_cast<float> (kKrellRatios.size())));
                auditionKrellFreqTarget = 82.41f * kKrellRatios[static_cast<size_t> (ratioIndex)] * (1.0f + 0.45f * nextAuditionRand01());
                auditionKrellEnv = 0.45f + 0.55f * nextAuditionRand01();
                auditionKrellStepSamples = static_cast<int> (
                    std::round ((0.16f + 0.72f * nextAuditionRand01()) * static_cast<float> (sampleRate)));
            }
            else
            {
                --auditionKrellStepSamples;
            }

            auditionKrellFreqCurrent += (auditionKrellFreqTarget - auditionKrellFreqCurrent) * 0.0015f;
            auditionKrellPhase += std::max (40.0, static_cast<double> (auditionKrellFreqCurrent)) / sampleRate;
            auditionKrellPhase -= std::floor (auditionKrellPhase);
            auditionPhaseSecondary += 0.18 / sampleRate;
            auditionPhaseSecondary -= std::floor (auditionPhaseSecondary);
            const auto lfo = static_cast<float> (std::sin (juce::MathConstants<double>::twoPi * auditionPhaseSecondary));
            const auto phase = juce::MathConstants<double>::twoPi * auditionKrellPhase;
            const auto carrier = static_cast<float> (std::sin (phase + 0.45 * lfo));
            const auto sub = static_cast<float> (std::sin (phase * 0.5));
            const auto harmonics = static_cast<float> (std::sin (phase * (2.0 + 0.28 * lfo)));
            auditionKrellEnv *= 0.99980f;
            return std::tanh ((0.62f * carrier + 0.26f * sub + 0.18f * harmonics) * (0.65f + auditionKrellEnv));
        }
        case 12: // Generative arp patch
        {
            if (auditionArpGateSamples <= 0)
            {
                static constexpr std::array<int, 12> kArpSemitones {
                    0, 2, 3, 5, 7, 10, 12, 14, 15, 17, 19, 22
                };
                auditionArpStepIndex = (auditionArpStepIndex + 1 + static_cast<int> (nextAuditionRand01() * 3.0f))
                    % static_cast<int> (kArpSemitones.size());
                const auto semitone = kArpSemitones[static_cast<size_t> (auditionArpStepIndex)];
                const auto freqBase = 110.0f * std::pow (2.0f, static_cast<float> (semitone) / 12.0f);
                auditionArpFreqA = freqBase;
                auditionArpFreqB = freqBase * (1.5f + 0.08f * nextAuditionRand01());
                auditionArpEnv = 1.0f;
                auditionArpGateSamples = static_cast<int> (
                    std::round ((0.05f + 0.17f * nextAuditionRand01()) * static_cast<float> (sampleRate)));
            }
            else
            {
                --auditionArpGateSamples;
            }

            auditionArpPhaseA += static_cast<double> (auditionArpFreqA) / sampleRate;
            auditionArpPhaseB += static_cast<double> (auditionArpFreqB) / sampleRate;
            auditionArpPhaseA -= std::floor (auditionArpPhaseA);
            auditionArpPhaseB -= std::floor (auditionArpPhaseB);
            const auto toneA = static_cast<float> (std::sin (juce::MathConstants<double>::twoPi * auditionArpPhaseA));
            const auto toneB = static_cast<float> (std::sin (juce::MathConstants<double>::twoPi * auditionArpPhaseB));
            const auto sparkle = 0.15f * nextAuditionWhiteNoise();
            auditionArpEnv *= (auditionArpGateSamples > 0) ? 0.9968f : 0.9920f;
            return (0.62f * toneA + 0.28f * toneB + sparkle) * auditionArpEnv;
        }
        default:
            return advanceAuditionOscillator (440.0, auditionPhasePrimary);
    }
}
