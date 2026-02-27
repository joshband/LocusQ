Title: Claude Research - Building a head-tracked spatial audio DAW plugin with AirPods Pro 2
Document Type: Research Notes
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-02-26

# Building a head-tracked spatial audio DAW plugin with AirPods Pro 2

**AirPods Pro 2 provide 3DOF rotational head tracking at ~25 Hz through Apple's CMHeadphoneMotionManager API, now available on macOS 14+.** Combined with JUCE's multi-channel audio architecture (supporting up to 7.1.4 Atmos layouts and 7th-order Ambisonics), Apple's PHASE framework for automatic head-tracked spatial rendering, and Three.js for WebView-based 3D visualization, every piece of the technical stack exists today to build a real-time spatial audio DAW plugin with head-tracked binaural output. The open-source ecosystem—particularly SPARTA, IEM Plugin Suite, and Spatial Audio Framework—provides production-ready reference implementations for HRTF convolution, Ambisonic encoding/decoding, and OSC-based head tracking integration. With dearVR's deprecation by Sennheiser and Meta 360's discontinuation, a meaningful market gap exists for this kind of tool.

---

## CMHeadphoneMotionManager: the complete API surface

CMHeadphoneMotionManager is a CoreMotion class that streams device motion data from AirPods Pro (1st/2nd gen), AirPods Max, AirPods 3rd gen, AirPods 4, and select Beats models. It became available on **macOS 14 (Sonoma)** at WWDC23, after debuting on iOS 14 in 2020.

The API surface is intentionally minimal. The class exposes five properties: `isDeviceMotionAvailable` and `isDeviceMotionActive` (both `Bool`), `deviceMotion` (`CMDeviceMotion?` for pull-mode access), `delegate` (`CMHeadphoneMotionManagerDelegate?`), and `authorizationStatus` (`CMAuthorizationStatus`). It offers two methods for starting updates—`startDeviceMotionUpdates()` for pull-mode polling, and `startDeviceMotionUpdates(to:withHandler:)` for push-mode callbacks—plus `stopDeviceMotionUpdates()`. The delegate protocol provides just two methods: `headphoneMotionManagerDidConnect(_:)` and `headphoneMotionManagerDidDisconnect(_:)`.

Each update delivers a **CMDeviceMotion** object containing rich orientation and motion data:

- **Attitude (CMAttitude)**: Available as quaternion (`CMQuaternion` with x/y/z/w as Double), Euler angles (pitch/roll/yaw in radians), and a 3×3 rotation matrix (`CMRotationMatrix` with m11–m33). The `multiply(byInverseOf:)` method computes relative rotation from a captured reference pose.
- **User acceleration**: `CMAcceleration` (x/y/z in g's, gravity removed)
- **Gravity vector**: `CMAcceleration` (x/y/z in g's)
- **Rotation rate**: `CMRotationRate` (x/y/z in radians/sec)
- **Magnetic field**: `CMMagneticField` (x/y/z) with accuracy enum (`.uncalibrated`, `.low`, `.medium`, `.high`)
- **Sensor location**: `CMDeviceMotion.SensorLocation` enum (`.left` or `.right`—only one AirPod streams at a time)
- **Timestamp**: `TimeInterval`

The update rate is **fixed at approximately 25 Hz** by hardware—unlike CMMotionManager's configurable `deviceMotionUpdateInterval` (up to 100 Hz). The reference frame is not user-configurable either. Practical calibration requires capturing a reference attitude at a known neutral pose, then computing relative changes via `attitude.multiply(byInverseOf: startingPose)`. **Yaw drift is a known issue** since the AirPods lack magnetometer-corrected reference frames; periodic re-centering is essential. The only required setup is an `NSMotionUsageDescription` Info.plist key—no special entitlements are needed.

### Critical differences from CMMotionManager

| Feature | CMMotionManager | CMHeadphoneMotionManager |
|---|---|---|
| Data source | Device IMU (iPhone/Watch) | AirPods IMU sensors |
| Update rate | Configurable, up to 100 Hz | **Fixed ~25 Hz** |
| DOF | 6DOF (with position inference) | **3DOF (rotation only)** |
| Update interval control | Yes (`deviceMotionUpdateInterval`) | No |
| Reference frame selection | Multiple `CMAttitudeReferenceFrame` options | Not configurable |
| Sensor location property | N/A | `.left` / `.right` bud ID |
| macOS availability | Limited | **macOS 14 Sonoma+** |

### Bridging head tracking data to external processes

The established pattern uses **OSC over UDP**, with several proven open-source implementations:

- **Headitude** (github.com/DanielRudrich/Headitude): A macOS Sonoma app that reads AirPods orientation and forwards configurable OSC messages (yaw/pitch/roll, quaternions, degree/radian variants) to any IP/port. Features a "press, nod, release" calibration gesture.
- **M1-AirPodOSC** (github.com/Mach1Studios/M1-AirPodOSC): iOS OSC transmitter designed for Mach1 Spatial System integration.
- **AirPodsPro-Motion-OSC-Forwarder** (github.com/emanuelgollob/AirPodsPro-Motion-OSC-Forwarder): Forwards yaw/pitch/roll via OSC to localhost:9999.

Alternative bridging approaches include WebSocket servers (for feeding Three.js directly), MIDI CC mapping (limited to 7-bit resolution), named pipes or TCP sockets via `InterprocessConnection`, and XPC services for macOS inter-process communication.

```swift
import CoreMotion

class HeadTracker: NSObject, CMHeadphoneMotionManagerDelegate {
    let manager = CMHeadphoneMotionManager()
    var referencePose: CMAttitude?
    
    func start() {
        guard manager.isDeviceMotionAvailable else { return }
        manager.delegate = self
        manager.startDeviceMotionUpdates(to: .main) { [weak self] motion, error in
            guard let motion = motion else { return }
            if self?.referencePose == nil { self?.referencePose = motion.attitude }
            let att = motion.attitude
            att.multiply(byInverseOf: self!.referencePose!)
            // att.quaternion, att.yaw/pitch/roll, motion.rotationRate, motion.gravity
            // Send via OSC, WebSocket, or evaluateJavaScript
        }
    }
    
    func headphoneMotionManagerDidConnect(_ m: CMHeadphoneMotionManager) { start() }
    func headphoneMotionManagerDidDisconnect(_ m: CMHeadphoneMotionManager) { }
}
```

---

## Apple's spatial audio frameworks and which to use

Apple provides three spatial audio frameworks with varying levels of abstraction and head-tracking integration. Choosing the right one depends on whether you need automatic head tracking, geometry-aware rendering, or manual control.

### PHASE: automatic head tracking and geometry-aware audio

PHASE (Physical Audio Spatialization Engine), available on **macOS 12+**, is Apple's most capable spatial audio framework. It provides geometry-aware occlusion modeling, volumetric sound sources (not just point sources), acoustic material simulation, distance-based attenuation with configurable rolloff, and built-in reverb presets. The critical feature for this project: **PHASEListener supports automatic head tracking** via `automaticHeadTrackingFlags = .orientation`, which directly integrates AirPods IMU data without any CMHeadphoneMotionManager code.

```swift
let listener = PHASEListener(engine: phaseEngine)
listener.automaticHeadTrackingFlags = .orientation  // Automatic AirPods tracking
try phaseEngine.rootObject.addChild(listener)
```

PHASE uses `PHASESpatialMixerDefinition` for 3D positioning, `PHASESource` objects attached to `PHASEShape` geometry, and an event-driven playback model with tree-based blending. The engine's `outputSpatializationMode` can be set to `.automatic`, `.alwaysUseBinaural`, or `.alwaysUseChannelBased`. However, PHASE is optimized for **playback** scenarios (games, media apps) rather than real-time DAW processing—it works with registered audio assets rather than live audio streams.

### AVAudioEngine: manual head tracking with real-time audio processing

For DAW-style real-time processing, **AVAudioEnvironmentNode** within AVAudioEngine is the more appropriate choice. It provides 3D audio positioning via `listenerPosition`, `listenerAngularOrientation`, and per-node `position` properties. The `renderingAlgorithm` property supports `.HRTFHQ`, `.HRTF`, `.sphericalHead`, and `.equalPowerPanning`. **Crucially, AVAudioEngine has no automatic head tracking**—you must manually pipe CMHeadphoneMotionManager data into `environmentNode.listenerAngularOrientation`.

```swift
let environmentNode = AVAudioEnvironmentNode()
// In head tracking callback:
environmentNode.listenerAngularOrientation = AVAudio3DAngularOrientation(
    yaw: Float(motion.attitude.yaw * 180.0 / .pi),
    pitch: Float(motion.attitude.pitch * 180.0 / .pi),
    roll: Float(motion.attitude.roll * 180.0 / .pi)
)
```

One important limitation: **only mono inputs are spatialized** by AVAudioEnvironmentNode—stereo inputs pass through without 3D processing.

### Personalized spatial audio and the rendering pipeline

Apple's Personalized Spatial Audio uses the iPhone's TrueDepth/LiDAR camera to scan ear shape, generating a custom HRTF profile that syncs across devices via iCloud (iOS 16+, macOS 13+). This profile is applied **system-wide by the OS**—there is no API for third-party apps to access or modify the personalized HRTF data. For Apple Music Atmos content, Apple applies its **own proprietary binaural renderer** (separate from Dolby's built-in binaural mode), producing a noticeably wider soundstage. RealityKit audio is primarily designed for visionOS/AR contexts and has limited spatial audio utility on macOS with AirPods.

---

## JUCE architecture for spatial audio with head tracking

JUCE provides no built-in spatial audio renderer, but its multi-channel bus architecture and OSC module form a solid foundation. The **`AudioChannelSet`** class supports every relevant spatial format through static factory methods.

### Multi-channel bus configurations

```cpp
// Dolby Atmos 7.1.4 (12 channels)
AudioChannelSet::create7point1point4()

// First-order Ambisonics (4 channels, ACN ordering, SN3D normalization)
AudioChannelSet::ambisonic(1)

// Third-order Ambisonics (16 channels)
AudioChannelSet::ambisonic(3)

// Other supported formats:
AudioChannelSet::quadraphonic()      // 4 ch
AudioChannelSet::create5point1()     // 6 ch
AudioChannelSet::create7point1()     // 8 ch
AudioChannelSet::create7point1point2() // 10 ch
AudioChannelSet::create7point1point6() // 14 ch
```

JUCE uses **ambiX convention** (ACN channel ordering + SN3D normalization) for its Ambisonic channel sets, supporting up to 7th order (64 channels). The `getAmbisonicOrder()` method returns the order of an Ambisonic layout. VST3 maps 7.1.4 to `k71_4`, and AU maps to `kAudioChannelLayoutTag_Atmos_7_1_4`. AAX has limited Atmos layout support.

### Receiving head tracking via OSC

The `juce_osc` module provides `OSCReceiver` with two callback modes: `MessageLoopCallback` (safe for UI) and **`RealtimeCallback`** (directly on network thread—optimal for low-latency head tracking):

```cpp
class SpatialProcessor : public AudioProcessor,
    private OSCReceiver::Listener<OSCReceiver::RealtimeCallback>
{
    OSCReceiver oscReceiver{"HeadTracker"};
    std::atomic<float> qw{1.f}, qx{0.f}, qy{0.f}, qz{0.f};
    
    void oscMessageReceived(const OSCMessage& msg) override {
        if (msg.getAddressPattern() == "/head/quaternion" && msg.size() >= 4) {
            qw.store(msg[0].getFloat32(), std::memory_order_relaxed);
            qx.store(msg[1].getFloat32(), std::memory_order_relaxed);
            qy.store(msg[2].getFloat32(), std::memory_order_relaxed);
            qz.store(msg[3].getFloat32(), std::memory_order_relaxed);
        }
    }
};
```

For the Swift → JUCE path, OSC over UDP is the recommended approach (**~1–5 ms latency**). The Swift app sends packets via Network.framework's `NWConnection`, and JUCE receives via `OSCReceiver`. Alternatives include shared memory (`shm_open` for lowest latency), named pipes via `InterprocessConnection`, and TCP sockets.

### Binaural rendering with HRTF convolution

JUCE's **`juce::dsp::Convolution`** class provides production-ready partitioned frequency-domain convolution. The Anaglyph binaural plugin confirms this works at production quality. Two instances (left/right ear) handle HRTF processing, with thread-safe IR loading for real-time HRTF switching during head-tracked source movement. For SOFA file loading, integrate **libmysofa** (bundled in SAF's `saf_sofa_reader` module) or the standalone library at github.com/hoene/libmysofa.

The complete rendering chain: mono source → azimuth/elevation selection → HRTF lookup with barycentric interpolation → left/right HRIR convolution → ITD application → distance attenuation → stereo binaural output.

---

## Three.js visualization and Swift-WebView bridge

Three.js provides built-in spatial audio via **AudioListener** (wrapping Web Audio API's `AudioContext.listener`), **PositionalAudio** (wrapping `PannerNode` with HRTF spatialization), and **Audio** (non-positional). Attaching an AudioListener to the camera and PositionalAudio objects to meshes creates a complete 3D audio scene where sound positioning tracks mesh positions automatically.

### Building the 3D mixing interface

For draggable audio source objects, Three.js offers two approaches. **DragControls** enables direct mesh dragging with `dragstart`/`drag`/`dragend` events—ideal for repositioning audio sources on a horizontal plane. **TransformControls** provides gizmo-based manipulation with translate/rotate/scale modes—better for precise 3D positioning. Both must coordinate with OrbitControls (disable orbit during drag) to prevent input conflicts.

Notable references include the **threejs-positional-audio-sandbox** (github.com/jbelezina/threejs-positional-audio-sandbox) with TransformControls for moving sound sources, and **Audiocube** (audiocube.app)—a web-based 3D DAW that demonstrates spatial mixing in a browser environment.

### Head tracking data flow into Three.js

The most performant pattern decouples data reception from the render loop:

```javascript
let latestTracking = null;
ws.onmessage = (e) => { latestTracking = JSON.parse(e.data); };

function animate() {
    requestAnimationFrame(animate);
    if (latestTracking) {
        camera.quaternion.set(
            latestTracking.qx, latestTracking.qy,
            latestTracking.qz, latestTracking.qw
        ).normalize();
    }
    renderer.render(scene, camera);
}
```

**Always use quaternions rather than Euler angles** for head tracking—they avoid gimbal lock and Three.js handles them natively via `Object3D.quaternion`. For smooth display when the tracker runs at 25 Hz but the display refreshes at 60 Hz, use `THREE.Quaternion.slerp()` to interpolate between tracking samples.

### WKWebView integration patterns

Hosting Three.js in a macOS WKWebView is proven (see github.com/ChediB/threejs-swift). WebGL works in WKWebView on macOS with App Sandbox's "Outgoing Connections (Client)" enabled. ES module imports from `file://` URLs can be problematic—**bundle Three.js with webpack/rollup** or use UMD builds.

For the critical **Swift → JavaScript bridge**, `evaluateJavaScript` is the primary mechanism. At head-tracking frequencies (~25–60 Hz), minimize overhead with compact function calls:

```swift
// Swift side: minimal serialization
let js = "ht(\(q.x),\(q.y),\(q.z),\(q.w))"
webView.evaluateJavaScript(js, completionHandler: nil)
```

```javascript
// JS side: minimal parsing
function ht(x, y, z, w) { camera.quaternion.set(x, y, z, w); }
```

For the **JavaScript → Swift** direction, `WKScriptMessageHandler` handles events like audio source position changes when the user drags objects in the Three.js scene. Register handlers via `WKUserContentController.add(self, name: "bridge")`, then call `window.webkit.messageHandlers.bridge.postMessage({...})` from JavaScript.

An alternative high-performance approach: run a **local WebSocket server** in the Swift app and connect from JavaScript inside the WebView. Libraries like **Sidewalk** (github.com/Danesz/Sidewalk) replace `evaluateJavaScript` with WebSocket-based JS execution for better bidirectional performance.

---

## The open-source spatial audio ecosystem

The landscape of production-ready open-source tools is remarkably rich. These are the most relevant projects organized by function.

### Complete spatial audio plugin suites

**SPARTA** (github.com/leomccormack/SPARTA) is the most comprehensive open-source collection: **20+ JUCE-based plugins** including AmbiBIN (binaural Ambisonic decoder with SOFA loader and OSC head tracking), Binauraliser (128-input binaural panner), AmbiENC (encoder up to 10th order), Rotator (with OSC head tracking), and 6DoFconv. Built on the **Spatial Audio Framework** (SAF) at github.com/leomccormack/Spatial_Audio_Framework, which provides C/C++ modules for HOA encoding/decoding, spherical harmonic transforms, VBAP, HRIR processing, and SOFA file reading via bundled libmysofa. SPARTA's plugins directly support receiving head tracking data from Headitude or any OSC source.

**IEM Plugin Suite** (github.com/tu-studio/IEMPluginSuite) from the Institute of Electronic Music and Acoustics in Graz provides 20+ free Ambisonic plugins up to 7th order: StereoEncoder, BinauralDecoder, MultiEncoder, RoomEncoder, FdnReverb, SceneRotator, EnergyVisualizer, and more. All JUCE-based, GPLv3.

**ambix** (github.com/kronihias/ambix) provides variable-order Ambisonic tools including `ambix_binaural` (decoder), `ambix_rotator`, and `ambix_converter` (FuMa↔ambiX conversion).

### Spatial audio rendering engines

- **Steam Audio** (github.com/ValveSoftware/steam-audio): **★2,700+**, fully open-sourced Feb 2024 under Apache 2.0. HRTF-based binaural rendering, physics-based sound propagation, Ambisonics rendering.
- **Google Resonance Audio** (github.com/resonance-audio/resonance-audio): Full C++ SDK, Apache 2.0. Ambisonics encoding/decoding, binaural rendering, spectral reverb. Community-maintained since 2018.
- **3D Tune-In Toolkit** (github.com/3DTune-In/3dti_AudioToolkit): C++ binaural spatialisation + hearing simulation. HRIR convolution with barycentric interpolation, ITD modeling, Ambisonic reverberation.
- **Mach1 Spatial** (github.com/Mach1Studios/m1-sdk): Cross-platform framework with Mach1Encode/Decode/Transcode libraries. Full DAW toolsuite (m1-spatialsystem) recently released free with source.
- **SpatGRIS** (github.com/GRIS-UdeM/SpatGRIS): Supports any speaker setup (2D/3D), up to 256 I/O, HRTF binaural mixdown.
- **Anaglyph** (anaglyph.dalembert.upmc.fr): Ultra-low-latency binaural spatializer (22 samples) with personalizable ITD model, SOFA support, and JUCE `dsp::Convolution` under the hood.

### HRTF databases and tools

The **SOFA Conventions master database** (sofaconventions.org) aggregates standardized HRTF datasets from MIT-KEMAR (710 positions), CIPIC (45 subjects), ARI (200+ subjects), LISTEN/IRCAM (51 subjects), SCUT, TU-Berlin, SADIE, and others—all in the AES69-2015 SOFA format. **libmysofa** (github.com/hoene/libmysofa) is the standard C library for reading SOFA files, used by FFmpeg and bundled in SAF.

### OSC libraries for Swift

- **OSCKit by orchetect** (github.com/orchetect/OSCKit): ★117, MIT license, SPM-compatible, macOS 10.13+
- **OSCKit by sammysmallman** (github.com/sammysmallman/OSCKit): TCP + UDP, OSC 1.1 compliant, AGPLv3
- **SwiftOSC** (github.com/ExistentialAudio/SwiftOSC): OSC 1.1 client/server framework

### AirPods head tracking demo repos

- **KhaosT/CMHeadphoneMotionManagerDemo**: SceneKit 3D visualization of head motion
- **tukuyo/AirPodsPro-Motion-Sampler**: Cube rotation, CSV export, comprehensive sensor readout
- **warrenm/HeadphoneMotion**: Clean API demonstration
- **kavishdevar/airpods-head-tracking**: Python-based L2CAP Bluetooth approach with gesture detection (nod/shake)
- **anastasiadevana/HeadphoneMotion**: Unity plugin wrapping CMHeadphoneMotionManager

---

## Research foundations for head-tracked binaural rendering

Academic work establishes several critical technical constraints. Stitt, Hendrickx, and Katz (Tonmeistertagung 2016) demonstrated that **head movements substantially enhance externalization** in binaural rendering, especially for frontal and rear sources, with the effect persisting even after movement stops. Their latency study found perceptibility thresholds of ~30–60 ms for simple scenes and ~40–70 ms for complex multi-source scenes—well within the achievable range for an OSC-based pipeline. Below **~10 ms total system latency** is considered transparent.

For HRTF personalization, a comprehensive 2024 review in Applied Sciences (MDPI, vol. 14, no. 23) catalogues current approaches: direct measurement, anthropometric matching, ear-image CNNs, 3D mesh-based BEM simulation (Apple's approach), and deep learning methods including autoencoders, GANs, and transformer architectures (HRTFformer, 2025). Apple's system uses TrueDepth/LiDAR scanning to generate personalized HRTFs—the gold standard for consumer-grade individualization.

The AudioMiXR paper (ACM IMWUT/UbiComp 2025) studied 6DOF spatial audio mixing on Apple Vision Pro, finding that **embodied mixing enhanced engagement** for both experts and non-experts. Users desired visual representations of audio attributes (e.g., object size as a proxy for loudness)—a direct design insight for Three.js visualization.

For Ambisonics-to-binaural rendering with head tracking, the key advantage is that **scene rotation is a simple matrix multiplication** in the spherical harmonics domain—frequency-independent and computationally efficient. Standard decoding methods include MagLS (Magnitude Least-Squares) for high-frequency accuracy and AllRAD for loudspeaker layouts. DirAC provides a parametric alternative that estimates spatial parameters directly.

---

## Dolby Atmos integration and format conversion

The Dolby Atmos renderer processes up to **128 input channels** (beds + objects with positional metadata) and renders to any monitoring configuration. **Bed channels** are fixed surround submixes (maximum 7.1.2, 10 channels). **Audio objects** (up to 118 mono) carry independent XYZ position metadata. The renderer outputs to stereo, 5.1, 7.1, 7.1.4, or binaural.

For creating Atmos-compatible output from a custom plugin, the deliverable format is **ADM BWF** (Audio Definition Model Broadcast Wave Format): an interleaved WAV file at 48 kHz containing bed audio + object audio + XML metadata (AXML chunk with position/gain automation) + CHNA chunk mapping tracks to ADM UIDs. Logic Pro exports ADM BWF natively. The **EBU ADM Renderer** (github.com/ebu/ebu_adm_renderer) is the open-source Python reference implementation supporting rendering to various layouts.

A critical limitation: the **Dolby Atmos ADM Profile only supports DirectSpeakers and Objects types—no native HOA support**. Converting Ambisonics to Atmos requires either decoding to a 7.1.2 bed or using beamforming to decompose into discrete objects. Ambisonics excels as an intermediate format because it's speaker-layout independent, rotation is trivial (essential for head tracking), and it can be decoded to any target layout. The practical workflow: encode sources to Ambisonics → rotate for head tracking → decode to binaural (headphones) or to bed channels (Atmos delivery).

Apple's consumer rendering pipeline for Atmos content applies its **own proprietary binaural renderer** rather than Dolby's built-in binaural mode, producing a noticeably wider soundstage. This means mixes monitored through Apple's Spatial Audio will sound different from Dolby's binaural monitoring—a critical consideration for engineers.

---

## Creative landscape and market positioning

The deprecation of dearVR by Sennheiser (support ends July 2025) and Meta 360 Spatial Workstation's discontinuation (May 2022) have created a significant gap. Currently active tools include **Sound Particles** ($79–$999, the closest implementation of the "audio atoms" paradigm with millions of individually positionable sound sources), **L-ISA Studio** by L-Acoustics (professional live sound), **Envelop for Live** (free, open-source, Ableton-only Ambisonics toolkit), **Audiocube** (web-based 3D DAW), and **Fiedler Audio Dolby Atmos Composer** ($449, enables Atmos mixing via headphones in stereo-only DAWs).

Several projects directly validate the proposed architecture. **Odio** (featured in Apple Developer spotlight) is an iOS spatial soundscape app built with AVAudioEnvironmentNode + CMHeadphoneMotionManager—a complete proof of the pipeline. **Headitude** demonstrates the macOS CMHeadphoneMotionManager → OSC → spatial audio plugin workflow. A creative hack documented at spaceforaudio.com shows mounting AirPods on studio headphones using silicone hooks, using them **purely as motion sensors** while professional headphones handle audio—decoupling tracking from playback.

Novel interaction patterns worth exploring include **head gestures as musical controls** (nod to trigger, shake to reject—implemented by Klipsch, Sony, and Backtracks SDK), the **"look-to-listen" paradigm** (Google's SIGGRAPH 2018 deep-learning audio-visual speech separation, adaptable to directing attention by head orientation in a 3D mix), and **accessibility applications** (MacPaw's research prototype controlling macOS by AirPods head movements via Bonjour).

---

## Recommended architecture and implementation path

The optimal architecture chains four components connected by OSC and WebView messaging:

```
┌─────────────────────────────────────────────────────────────┐
│  Swift macOS Bridge App                                      │
│  ┌──────────────────┐   OSC/UDP    ┌────────────────────┐   │
│  │ CMHeadphone       │────────────►│ JUCE Audio Plugin   │   │
│  │ MotionManager     │   (9000)    │ (VST3/AU)           │   │
│  │ (~25 Hz quaternion)│            │ • OSCReceiver        │   │
│  └────────┬─────────┘             │ • HRTF convolution   │   │
│           │                        │ • Ambisonic enc/dec  │   │
│           │ evaluateJavaScript     │ • Multi-ch output    │   │
│           │ (~60 Hz throttled)     └────────────────────┘   │
│           ▼                                                  │
│  ┌──────────────────┐  postMessage  ┌────────────────────┐  │
│  │ WKWebView         │◄────────────│ Three.js Scene       │  │
│  │ (WebGL enabled)   │             │ • DragControls       │  │
│  │                   │             │ • Audio source meshes │  │
│  │                   │             │ • Quaternion camera   │  │
│  └──────────────────┘             └────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

- **Head tracking**: CMHeadphoneMotionManager → capture reference pose → compute relative quaternion → send via OSC (orchetect/OSCKit) to JUCE plugin on localhost:9000, simultaneously forward to WKWebView via `evaluateJavaScript` for camera rotation
- **Audio processing** (JUCE): `OSCReceiver` with `RealtimeCallback` for head tracking data → HRTF lookup from SOFA database (via libmysofa/SAF) → `juce::dsp::Convolution` for binaural rendering → output as stereo binaural, or use `AudioChannelSet::create7point1point4()` for Atmos bed output, or `AudioChannelSet::ambisonic(order)` for Ambisonics intermediate
- **Visualization** (Three.js in WKWebView): DragControls for audio source positioning → position changes sent to Swift via `postMessage` → forwarded to JUCE via OSC → camera quaternion updated from head tracking for first-person perspective
- **Format flexibility**: Ambisonics as the internal representation enables rotation for head tracking, decoding to binaural for headphones, decoding to arbitrary speaker layouts, and ADM BWF export for Atmos delivery

SPARTA and IEM Plugin Suite serve as the most complete reference implementations for every component of this architecture. The Spatial Audio Framework (SAF) provides C/C++ modules that can be directly integrated into a JUCE plugin for HRTF processing, Ambisonic encoding/decoding, and SOFA file reading.

---

## Conclusion

The technical stack for building a head-tracked spatial audio DAW plugin with AirPods Pro 2 is mature. CMHeadphoneMotionManager's macOS availability since Sonoma, JUCE's comprehensive multi-channel and Ambisonic channel set support, and the wealth of open-source spatial audio processing libraries (SPARTA/SAF, IEM, Steam Audio) eliminate the need to build fundamental components from scratch. The primary engineering challenges are **managing the 25 Hz head tracking rate** (interpolation and prediction are essential for smooth binaural rendering), **HRTF selection and interpolation** for accurate spatial perception across the full sphere, and **maintaining low end-to-end latency** across the OSC → audio processing → binaural output chain. The deprecation of major commercial tools (dearVR, Meta 360) and the growing ecosystem of Atmos-enabled DAWs create a genuine opportunity for a well-designed spatial audio plugin that unifies 3D visualization, head tracking, and flexible format output in a single workflow.
