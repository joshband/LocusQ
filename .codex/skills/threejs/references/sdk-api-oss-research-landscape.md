Title: Three.js Spatial Audio Ecosystem Landscape
Document Type: Skill Reference
Author: APC Codex
Created Date: 2026-02-20
Last Modified Date: 2026-02-20

# SDK/API/Open-Source/Research Landscape

## Scope
Use this reference when the task needs concrete ecosystem choices for Three.js + JUCE WebView + spatial audio.

Freshness note: links checked on 2026-02-20. Re-check maintenance status before committing to a dependency.

## Core APIs and Platform Docs
- Three.js (core 3D runtime): https://github.com/mrdoob/three.js
- Web Audio API 1.1 draft/spec: https://www.w3.org/TR/webaudio-1.1/
- Web Audio spatial panning (`PannerNode`): https://developer.mozilla.org/en-US/docs/Web/API/PannerNode
- Web Audio low-latency custom DSP (`AudioWorklet`): https://developer.mozilla.org/en-US/docs/Web/API/AudioWorklet
- JUCE `WebBrowserComponent`: https://docs.juce.com/develop/classWebBrowserComponent.html
- JUCE framework repo: https://github.com/juce-framework/JUCE
- Apple audio framework overview (AVAudioEngine, PHASE, spatial audio context): https://developer.apple.com/audio/
- Apple audio and music technology overview: https://developer.apple.com/documentation/technologyoverviews/audio-and-music

## Spatial Audio SDKs and Open-Source Toolkits
- Resonance Audio (C++ SDK): https://github.com/resonance-audio/resonance-audio
- Resonance Audio Web SDK: https://github.com/resonance-audio/resonance-audio-web-sdk
- Omnitone (ambisonic/binaural on Web Audio): https://github.com/GoogleChrome/omnitone
- Steam Audio (HRTF, propagation, occlusion): https://github.com/ValveSoftware/steam-audio
- OpenAL Soft (software OpenAL 3D audio): https://github.com/kcat/openal-soft
- Spatial Audio Framework (C/C++ algorithms): https://github.com/leomccormack/Spatial_Audio_Framework
- SPARTA (JUCE plug-ins built on SAF): https://github.com/leomccormack/SPARTA
- libmysofa (SOFA HRTF reader): https://github.com/hoene/libmysofa
- SOFA toolbox (MATLAB/Octave API): https://github.com/sofacoustics/SOFAtoolbox

## NGA/Atmos-Adjacent Tooling (Metadata and Rendering Workflows)
- EBU ADM Renderer (reference renderer): https://github.com/ebu/ebu_adm_renderer
- libadm (ADM metadata handling): https://github.com/ebu/libadm
- libbw64 (BW64 + ADM container tooling): https://github.com/ebu/libbw64

Use these for standards-based object and scene metadata pipelines even when final rendering differs by target platform.

## Research Datasets and Project Repos (GitHub)
- Apple Spatial LibriSpeech (FOA dataset): https://github.com/apple/ml-spatial-librispeech
- Real Acoustic Fields (CVPR 2024): https://github.com/facebookresearch/real-acoustic-fields
- HARP HOA RIR dataset: https://github.com/whojavumusic/HARP
- BIRD impulse-response dataset: https://github.com/FrancoisGrondin/BIRD
- dEchorate dataset/tools: https://github.com/Chutlhu/dEchorate
- pyroomacoustics (simulation + beamforming R&D): https://github.com/LCAV/pyroomacoustics
- pyfar (acoustics research tooling): https://github.com/pyfar/pyfar

## Suggested Selection Heuristic
1. Start with target runtime constraints:
- Browser/WebView only.
- Native C++ DSP in plugin.
- Offline renderer/metadata toolchain.

2. Pick one primary rendering path first:
- Web-first prototype: Three.js + Web Audio (`PannerNode`/`AudioWorklet`) + optional Omnitone/Resonance Web.
- Native plugin-first: JUCE DSP + WebView UI + optional SAF/SPARTA-derived algorithms.
- Metadata-heavy/object workflows: ADM + BW64 toolchain (`libadm`, `libbw64`, `ebu_adm_renderer`) for interchange and validation.

3. Add platform specialization second:
- Apple-focused immersive path: AVAudioEngine/PHASE capabilities where applicable.
- Game/interactive propagation path: Steam Audio.

4. Confirm project health before adoption:
- Archived status.
- Recent commits/releases.
- License compatibility with commercial plugin distribution.
- Build complexity for your target platforms.

## Integration Advice for This Skill
- Prefer APIs with deterministic behavior under plugin-host constraints over feature-rich but brittle stacks.
- Treat browser audio as monitoring/prototyping unless the production architecture explicitly runs DSP in-web.
- Keep a transport schema that can map to both speaker layouts and binaural monitoring modes.
