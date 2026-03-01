#pragma once

// QuadSpeakerLayout — canonical quad speaker-layout identifier used by
// SteamAudioVirtualSurround when specifying the IPL speaker-layout type.
//
// Values are designed to map directly to IPLSpeakerLayoutType when the
// LOCUSQ_ENABLE_STEAM_AUDIO preprocessor guard is active.
enum class QuadSpeakerLayout : int
{
    // Standard ±45° / ±135° quadraphonic layout (FL, FR, RL, RR).
    // Maps to IPL_SPEAKERLAYOUTTYPE_QUADRAPHONIC.
    Quadraphonic = 0
};

// Number of input channels expected by the quad virtual-surround effect.
inline constexpr int kQuadSpeakerCount = 4;
