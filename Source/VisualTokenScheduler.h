#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <limits>

struct VisualToken
{
    enum class Type : std::uint8_t
    {
        bar = 0,
        beat = 1,
        subdivision = 2,
        swing = 3
    };

    std::uint32_t sampleOffsetInBlock = 0;
    float ppq = 0.0f;
    std::uint8_t type = static_cast<std::uint8_t> (Type::beat);
};

struct VisualTokenSnapshot
{
    static constexpr std::uint32_t kMaxTokens = 32;

    std::atomic<std::uint32_t> seq { 0 };
    std::atomic<std::uint32_t> count { 0 };
    std::array<VisualToken, kMaxTokens> tokens {};
};

class VisualTokenScheduler
{
public:
    void reset() noexcept
    {
        snapshot.count.store (0, std::memory_order_release);
        snapshot.seq.store (0, std::memory_order_release);
    }

    void setSubdivisionPerBeat (int subdivision) noexcept
    {
        subdivisionPerBeat = juce::jlimit (1, 8, subdivision);
    }

    void processBlock (juce::AudioPlayHead* playHead,
                       int numSamples,
                       double sampleRate) noexcept
    {
        if (playHead == nullptr || numSamples <= 0 || ! isFinitePositive (sampleRate))
        {
            publishEmpty();
            return;
        }

        const auto position = playHead->getPosition();
        if (! position)
        {
            publishEmpty();
            return;
        }

        if (! position->getIsPlaying())
        {
            publishEmpty();
            return;
        }

        const auto bpm = position->getBpm();
        const auto blockStartPpq = position->getPpqPosition();
        if (! bpm || ! blockStartPpq || ! isFinitePositive (*bpm) || ! std::isfinite (*blockStartPpq))
        {
            publishEmpty();
            return;
        }

        auto beatLengthQuarterNotes = 1.0;
        auto beatsPerBarQuarterNotes = 4.0;

        if (const auto timeSignature = position->getTimeSignature())
        {
            const auto numerator = juce::jmax (1, timeSignature->numerator);
            const auto denominator = juce::jmax (1, timeSignature->denominator);
            beatLengthQuarterNotes = 4.0 / static_cast<double> (denominator);
            beatsPerBarQuarterNotes = static_cast<double> (numerator) * beatLengthQuarterNotes;
        }

        if (! isFinitePositive (beatLengthQuarterNotes) || ! isFinitePositive (beatsPerBarQuarterNotes))
        {
            publishEmpty();
            return;
        }

        const auto subdivisionStepQuarterNotes = beatLengthQuarterNotes / static_cast<double> (juce::jmax (1, subdivisionPerBeat));
        if (! isFinitePositive (subdivisionStepQuarterNotes))
        {
            publishEmpty();
            return;
        }

        const auto blockDurationSeconds = static_cast<double> (numSamples) / sampleRate;
        const auto blockDurationQuarterNotes = (*bpm * blockDurationSeconds) / 60.0;
        if (! isFinitePositive (blockDurationQuarterNotes))
        {
            publishEmpty();
            return;
        }

        const auto blockStartQuarterNotes = *blockStartPpq;
        const auto blockEndQuarterNotes = blockStartQuarterNotes + blockDurationQuarterNotes;

        auto barAnchorQuarterNotes = std::floor (blockStartQuarterNotes / beatsPerBarQuarterNotes) * beatsPerBarQuarterNotes;
        if (const auto hostBarStart = position->getPpqPositionOfLastBarStart(); hostBarStart && std::isfinite (*hostBarStart))
            barAnchorQuarterNotes = *hostBarStart;

        auto nextBar = firstBoundaryAtOrAfter (blockStartQuarterNotes, barAnchorQuarterNotes, beatsPerBarQuarterNotes);
        auto nextBeat = firstBoundaryAtOrAfter (blockStartQuarterNotes, 0.0, beatLengthQuarterNotes);
        auto nextSubdivision = firstBoundaryAtOrAfter (blockStartQuarterNotes, 0.0, subdivisionStepQuarterNotes);

        std::array<VisualToken, VisualTokenSnapshot::kMaxTokens> localTokens {};
        std::uint32_t localCount = 0;

        constexpr int kMaxIterations = 128;
        for (int guard = 0; guard < kMaxIterations && localCount < VisualTokenSnapshot::kMaxTokens; ++guard)
        {
            const auto nextBoundary = juce::jmin (nextBar, juce::jmin (nextBeat, nextSubdivision));
            if (! (nextBoundary < (blockEndQuarterNotes - kBoundaryEpsilon)))
                break;

            if (std::abs (nextBar - nextBoundary) <= kBoundaryEpsilon)
            {
                appendToken (localTokens, localCount, nextBar, VisualToken::Type::bar, blockStartQuarterNotes, *bpm, sampleRate, numSamples);
                nextBar += beatsPerBarQuarterNotes;
            }

            if (std::abs (nextBeat - nextBoundary) <= kBoundaryEpsilon)
            {
                appendToken (localTokens, localCount, nextBeat, VisualToken::Type::beat, blockStartQuarterNotes, *bpm, sampleRate, numSamples);
                nextBeat += beatLengthQuarterNotes;
            }

            if (std::abs (nextSubdivision - nextBoundary) <= kBoundaryEpsilon)
            {
                appendToken (localTokens, localCount, nextSubdivision, VisualToken::Type::subdivision, blockStartQuarterNotes, *bpm, sampleRate, numSamples);
                nextSubdivision += subdivisionStepQuarterNotes;
            }
        }

        publishSnapshot (localTokens, localCount);
    }

    const VisualTokenSnapshot& getSnapshot() const noexcept
    {
        return snapshot;
    }

    bool copySnapshot (std::array<VisualToken, VisualTokenSnapshot::kMaxTokens>& destination,
                       std::uint32_t& outCount,
                       std::uint32_t& outSeq) const noexcept
    {
        // Seqlock-style read: retry if writer is active (odd seq) or seq changes mid-copy.
        constexpr int kMaxReadAttempts = 3;
        for (int attempt = 0; attempt < kMaxReadAttempts; ++attempt)
        {
            const auto seqBefore = snapshot.seq.load (std::memory_order_acquire);
            if ((seqBefore & 1u) != 0u)
                continue;

            const auto count = juce::jmin (snapshot.count.load (std::memory_order_acquire),
                                           VisualTokenSnapshot::kMaxTokens);

            for (std::uint32_t i = 0; i < count; ++i)
                destination[i] = snapshot.tokens[i];

            const auto seqAfter = snapshot.seq.load (std::memory_order_acquire);
            if (seqBefore == seqAfter && (seqAfter & 1u) == 0u)
            {
                outCount = count;
                outSeq = seqAfter;
                return true;
            }
        }

        outCount = 0;
        outSeq = snapshot.seq.load (std::memory_order_acquire);
        return false;
    }

private:
    static constexpr double kBoundaryEpsilon = 1.0e-9;

    static bool isFinitePositive (double value) noexcept
    {
        return std::isfinite (value) && value > 0.0;
    }

    static double firstBoundaryAtOrAfter (double startQuarterNotes,
                                          double anchorQuarterNotes,
                                          double stepQuarterNotes) noexcept
    {
        if (! isFinitePositive (stepQuarterNotes) || ! std::isfinite (startQuarterNotes) || ! std::isfinite (anchorQuarterNotes))
            return std::numeric_limits<double>::infinity();

        const auto stepCount = std::ceil ((startQuarterNotes - anchorQuarterNotes - kBoundaryEpsilon) / stepQuarterNotes);
        auto boundary = anchorQuarterNotes + (stepCount * stepQuarterNotes);

        if (boundary + kBoundaryEpsilon < startQuarterNotes)
            boundary += stepQuarterNotes;

        return boundary;
    }

    static void appendToken (std::array<VisualToken, VisualTokenSnapshot::kMaxTokens>& tokens,
                             std::uint32_t& tokenCount,
                             double tokenQuarterNotes,
                             VisualToken::Type type,
                             double blockStartQuarterNotes,
                             double bpm,
                             double sampleRate,
                             int numSamples) noexcept
    {
        if (tokenCount >= VisualTokenSnapshot::kMaxTokens
            || ! std::isfinite (tokenQuarterNotes)
            || ! isFinitePositive (bpm)
            || ! isFinitePositive (sampleRate)
            || numSamples <= 0)
        {
            return;
        }

        const auto deltaQuarterNotes = tokenQuarterNotes - blockStartQuarterNotes;
        if (deltaQuarterNotes < -kBoundaryEpsilon)
            return;

        const auto deltaSamples = (deltaQuarterNotes * 60.0 * sampleRate) / bpm;
        auto offset = static_cast<long long> (std::llround (deltaSamples));
        offset = juce::jlimit (0LL, static_cast<long long> (numSamples - 1), offset);

        VisualToken token;
        token.sampleOffsetInBlock = static_cast<std::uint32_t> (offset);
        token.ppq = static_cast<float> (tokenQuarterNotes);
        token.type = static_cast<std::uint8_t> (type);
        tokens[tokenCount++] = token;
    }

    void publishSnapshot (const std::array<VisualToken, VisualTokenSnapshot::kMaxTokens>& tokens,
                          std::uint32_t count) noexcept
    {
        // Begin write (odd sequence => writer active).
        snapshot.seq.fetch_add (1, std::memory_order_acq_rel);

        for (std::uint32_t i = 0; i < count; ++i)
            snapshot.tokens[i] = tokens[i];

        snapshot.count.store (count, std::memory_order_release);

        // End write (even sequence => stable snapshot visible to consumers).
        snapshot.seq.fetch_add (1, std::memory_order_release);
    }

    void publishEmpty() noexcept
    {
        publishSnapshot (emptyTokens, 0);
    }

    int subdivisionPerBeat = 4;
    VisualTokenSnapshot snapshot;
    std::array<VisualToken, VisualTokenSnapshot::kMaxTokens> emptyTokens {};
};
