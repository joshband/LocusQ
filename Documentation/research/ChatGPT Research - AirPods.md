Title: ChatGPT Research - AirPods
Document Type: Research Notes
Author: APC Codex
Created Date: 2026-02-26
Last Modified Date: 2026-02-26

You’re building the “missing layer” Apple never shipped: explicit, developer-controlled head pose that can drive your own 3D scene + your own spatial renderer, not just Apple’s system Spatial Audio toggle. That’s the right way to think about it.

Below is a practical, engineering-first map of what you can actually get from AirPods Pro 2 head tracking, what you can’t, and how to turn the stream into something stable enough for real-time 3D visualization and a DAW spatial plugin.

⸻

What AirPods Pro 2 head tracking actually gives you (and what it doesn’t)

You get (reliably)

From CMHeadphoneMotionManager → CMDeviceMotion, Apple explicitly calls out:
	•	Attitude (orientation; effectively a quaternion / yaw-pitch-roll representation)
	•	Rotation rate (angular velocity)
	•	User acceleration
	•	Plus SensorLocation metadata (left/right bud) and connection events.  ￼

This is enough for:
	•	3DoF head pose (yaw/pitch/roll) in your 3D visualizer
	•	Head-tracked binaural / Ambisonics monitoring (rotate the scene opposite the head)
	•	Gesture features (nod/shake/tilt classification off rotation rate + acceleration)

You do not get (in any robust, long-term way)
	•	True head position (translation) in room coordinates.
	•	You can integrate acceleration, but it drifts into nonsense quickly (physics is cruel, IMUs are liars, and drift always wins without an external reference).
	•	Absolute yaw to world north (no stable magnetometer-based heading like a phone; and even phones are noisy here).

So: treat this as orientation-first, and if you want 6DoF later, fuse with optical tracking (ARKit / Vision / external tracker). For your current macOS + AirPods scope: orientation is the gold.

⸻

The CMHeadphoneMotionManager API surface you can use

Here’s the practical API inventory (names may appear slightly different in Swift vs ObjC bindings, but the capabilities are these):

Core properties
	•	AuthorizationStatus (permission state)  ￼
	•	DeviceMotionAvailable (isDeviceMotionAvailable)  ￼
	•	DeviceMotionActive (isDeviceMotionActive)  ￼
	•	DeviceMotion (latest sample; the “pull” interface)  ￼
	•	Delegate (connect/disconnect callbacks)  ￼
	•	ConnectionStatusActive (whether connection status updates are active)  ￼

Core methods
	•	StartDeviceMotionUpdates() (start updates; then read DeviceMotion)  ￼
	•	StartDeviceMotionUpdates(queue, handler) (push interface; you get samples in a callback)  ￼
	•	StopDeviceMotionUpdates()  ￼
	•	StartConnectionStatusUpdates() / StopConnectionStatusUpdates()  ￼

Delegate events

The delegate protocol gives you at least:
	•	headphoneMotionManagerDidConnect
	•	headphoneMotionManagerDidDisconnect  ￼

Permissions (don’t skip this)

You must include NSMotionUsageDescription in your Info.plist. Apple’s docs are blunt: if the key is missing, the system can crash your app when you start motion services.  ￼
WWDC also emphasizes checking authorization status to handle denied/restricted cases cleanly.  ￼

⸻

What’s in each sample (CMDeviceMotion) and how to interpret it

Apple’s Core Motion talk makes it explicit that, when streaming from AirPods-class devices, you should inspect CMDeviceMotion for:
	•	attitude
	•	userAcceleration
	•	rotationRate
…and also sensor location metadata per sample.  ￼

Attitude = your primary truth

Use attitude as a quaternion for:
	•	Driving your head model rotation in 3D
	•	Rotating your audio scene (or listener) for binaural rendering

Important: attitude is relative to a reference frame and can drift over time. Treat it as “good short-term, needs calibration to define ‘forward’.”

WWDC gives the canonical pattern:
	•	Save a startingPose
	•	Use attitude math (e.g., multiply) to compute the relative rotation from that baseline  ￼

That is exactly what you want for DAW monitoring: “I pressed Calibrate while looking forward; now that’s my zero.”

Rotation rate = prediction + gesture juice

Rotation rate helps with:
	•	Latency compensation (predict a few milliseconds ahead)
	•	Nod/shake classification (cleaner than raw attitude deltas)

User acceleration = gesture juice, not position

Use it for:
	•	Gesture detection (micro-nods, taps, quick movements)
Not for:
	•	Long-term position tracking (drift city)

SensorLocation = your “one bud at a time” gotcha

A nasty-but-manageable detail: motion data is delivered from one bud at a time, and it can switch buds based on in-ear detection / removal.  ￼

Engineering implication:
	•	You must handle discontinuities when sensorLocation flips.
	•	Either:
	•	maintain a calibration per bud, or
	•	rebase smoothly when a switch is detected (short crossfade / slerp over ~100–300ms)

⸻

macOS reality: AirPods motion on Mac is now feasible (macOS 14+)

WWDC23: CMHeadphoneMotionManager comes to macOS 14, enabling streaming from supported AirPods to Mac.  ￼

And the open-source Headitude app is a concrete proof point: it states that starting with macOS Sonoma 14.0 it can read AirPods orientation via CoreMotion and forward via OSC.  ￼

So your “Swift macOS bridge app” approach is not just plausible—it’s the cleanest architecture.

⸻

A practical architecture for your bridge + JUCE + Three.js

Strong opinion (practical): make the bridge the “truth oracle”

Do all of this in the Swift bridge:
	•	Permission + connection lifecycle
	•	Calibration
	•	Smoothing / prediction
	•	Coordinate conventions

Then emit a clean stream to:
	•	JUCE plugin
	•	WebView/three.js visualizer
	•	(optional) external tools via OSC

This avoids duplicating tricky quaternion logic in multiple places.

Transport choice

For DAW ecosystems, OSC is the lingua franca, because it plugs into:
	•	Dolby Atmos Renderer workflows (people literally ask about Headitude OSC formats there)  ￼
	•	Nuendo/Steinberg head-tracking via OSC server workflows  ￼

For your Three.js WebView, WebSocket is usually more convenient, but OSC-over-UDP is still fine if you control both ends.

Recommendation:
	•	OSC/UDP out of the bridge (for DAW + general interoperability)
	•	Optional WebSocket mirror for the WebView if you want easier JS plumbing

Calibration pattern (copy what works in the wild)

Headitude’s approach is instructive:
	•	It explicitly says the app “doesn’t know how you wear your AirPods”
	•	It uses a quick calibration routine and mentions using gravity + quaternion math internally  ￼

That matches real-world behavior: earbud seating differs slightly every time.

⸻

Minimal Swift bridge logic (conceptual, but accurate)

Key steps WWDC outlines:
	1.	Check isDeviceMotionAvailable
	2.	Set delegate for connection events
	3.	Start streaming with push or pull interface
	4.	Use attitude math to compute relative pose  ￼

And per Apple docs: include NSMotionUsageDescription.  ￼

In your handler, you want to publish something like:

timestamp
sensorLocation
quat (x,y,z,w)   // or w,x,y,z — pick one and never change it
rotationRate (x,y,z)
userAccel (x,y,z)
(optional) gravity (x,y,z)  // if present/meaningful for your device


⸻

Mapping to Three.js and your audio coordinate system

The biggest pitfall: coordinate conventions

Three.js defaults:
	•	right-handed
	•	camera looks down -Z
Apple’s AVAudioEnvironmentNode docs similarly describe forward as -Z by default for listener orientation.  ￼

So you can choose a unified convention:
	•	+X = right
	•	+Y = up
	•	-Z = forward

Then:
	•	your Three.js head model and your audio listener agree
	•	debugging becomes 10x less miserable

Apply the quaternion directly

In JS you’ll typically do something like:
	•	object.quaternion.set(x, y, z, w) (Three.js uses x,y,z,w order)

If your Swift sends w,x,y,z, reorder.

For spatial audio rendering

You usually want world-locked sources:
	•	when head rotates right, the world should remain stable, so the relative azimuth/elevation of each source changes accordingly.

Implementation-wise:
	•	either rotate the listener by head orientation, or
	•	rotate the entire scene by the inverse head orientation.
Both are equivalent if you’re consistent.

⸻

Apple spatial audio APIs: what’s useful to you (and what’s not)

AVAudioEngine / AVAudioEnvironmentNode

This is Apple’s basic 3D audio scene simulation layer (HRTF binauralization, 3D positions, etc.).  ￼

However, for your DAW plugin path, you’re likely not embedding AVAudioEngine inside a JUCE effect (possible in theory, miserable in practice, and you’d be fighting hosts + realtime constraints).

PHASE (Physical Audio Spatialization Engine)

PHASE is Apple’s higher-level spatial audio API oriented around sources/listeners/geometry.  ￼
Apple also has a PHASE doc about “personalizing spatial audio” via head movement + personal profile.  ￼

Again: great for native apps, less directly relevant for a cross-platform-ish JUCE plugin pipeline.

The “automatic head tracking” entitlements (read: probably not your plan A)

Apple documents a property isListenerHeadTrackingEnabled and explicitly states enabling it requires the com.apple.developer.coremotion.head-pose entitlement.  ￼

That’s a big signal:
	•	Apple can wire head pose directly into their renderer
	•	but they gate it behind an entitlement, which typically means “not generally available to arbitrary third-party apps”

So: for your project, CMHeadphoneMotionManager remains the sane, shippable route.

⸻

Open source + example projects worth cataloging

Head tracking capture / streaming
	•	Headitude (macOS) — AirPods orientation → OSC sender; includes calibration concept; explicitly macOS Sonoma 14.0+.  ￼
	•	Mach1Studios/M1-AirPodOSC — OSC transmitter intended for spatial mixing workflows with Mach1 monitoring.  ￼
	•	tukuyo/AirPodsPro-Motion-Sampler (iOS) — shows raw CMDeviceMotion, cube rotation demo, CSV export (useful to inspect real sample characteristics).  ￼
	•	kulich-ua/HeadphoneMotion (iOS) — demonstrates head rotation visualization and “fix the sound source in the user environment”; uses SceneKit.  ￼
	•	ctxzz/HeadTrackerApp (iOS) — simple head orientation visualization app.  ￼

“Use AirPods as tracker hardware” hacks
	•	A practical workflow writeup: mounting AirPods on studio headphones so you get head tracking while monitoring on better cans (this is extremely relevant to mixing).  ￼

Non-Apple, but highly relevant renderers (for your JUCE DSP core)
	•	google/obr — Ambisonics → binaural renderer (HRTF-based); very relevant if your plugin supports Ambisonics monitoring.  ￼
	•	SoundScape Renderer (SSR) — real-time spatial audio reproduction engine with multiple rendering algorithms.  ￼
	•	VideoLAN libspatialaudio — modern renderer covering HOA/object rendering concepts; explicitly targets complex render pipelines.  ￼

⸻

Research papers & “physics truths” that will save you time

If you only internalize one research lesson: latency and head tracking correctness dominate perceived externalization. If your audio lags head motion, the world “swims,” and the illusion collapses.

A few solid references:
	•	Classic warning: low-latency head tracking is necessary; delay makes the virtual scene rotate with the listener and hurts externalization.  ￼
	•	Head tracking improves realism/externalization and helps when HRTFs aren’t personalized (dynamic cues help resolve ambiguities).  ￼
	•	DAFx 2025 example: real-time head-tracked binaural rendering using ambisonics + inertial head tracking for accessibility audio interfaces (good conceptual parallel to “DAW UI but auditory”).  ￼
	•	Work on binaural rendering incorporating head tracking (including individualized HRTF considerations).  ￼

Practical implication for your plugin:
	•	Treat pose as a time-stamped stream
	•	Predict slightly (even 10–20ms prediction can help if your audio buffer is large)
	•	Smooth orientation with quaternion slerp (not Euler smoothing)

⸻

“Dreaming” responsibly: what you can build that will feel futuristic (and shippable)

Here are ideas that are ambitious but mechanically feasible with exactly the tech you described:

1) A “spatial field” DAW plugin where tracks are atoms in a 3D molecule
	•	Each track = a particle with:
	•	position (x,y,z)
	•	radiation pattern / directivity
	•	“mass” = loudness/energy
	•	bonds = routing / sends
	•	Head tracking changes viewpoint; you literally mix by arranging a structure.
	•	Render outputs:
	•	binaural headphone monitor (head-tracked)
	•	selectable speaker layouts (quad/5.1/7.1.4 downmix previews)
	•	ambisonics bus export

2) A head-tracked “virtual control room” that makes stereo decisions less dumb

Use head tracking to emulate:
	•	stable phantom center
	•	speaker angles and crosstalk
	•	room early reflections (simple model, doesn’t need full ray tracing)

This is the kind of thing that makes panning and reverb decisions translate better when you leave headphones.

3) Head gestures as DAW control (small, tasteful, optional)
	•	Nod = “audition selected object”
	•	Shake = “bypass spatializer”
	•	Tilt = “scrub elevation” while holding a modifier
Done right, it’s a power tool. Done wrong, it’s a chiropractor’s retirement plan.

4) Object automation that’s authored as motion paths in 3D
	•	Draw or simulate paths in Three.js
	•	Commit them to automation lanes
	•	Preview with head tracking to validate externalization and motion comfort

5) Hybrid monitoring: AirPods as tracker, reference cans as playback

The “mount AirPods on studio headphones” workflow is already being used in the wild.  ￼
You can formalize it:
	•	bridge app = tracker
	•	plugin = renderer
	•	audio output = your real headphones / interface
This is a killer use case.

⸻

The engineering checklist that will make or break perceived quality
	1.	Calibration UX must be instant and obvious
	•	One button: “Set Forward”
	•	Optional: “Set Down” / “Reset”
	•	Persist per device
	2.	Handle earbud switching
	•	Detect via sensorLocation (and connection callbacks)
	•	Smooth transitions
	3.	Keep motion → audio latency low
	•	Predict pose forward by (audioOutputLatency + bufferDuration)
	•	Interpolate pose at audio block boundaries
	4.	Never ship Euler-only
	•	Use quaternions end-to-end
	•	Convert to yaw/pitch/roll only for UI readouts
	5.	Treat this as sensitive data
	•	Permission, clear messaging, graceful degradation  ￼

⸻

If you proceed with this architecture, you’re essentially building a mini “head-tracking middleware layer” for macOS + DAWs. That’s a real product shape, not just a prototype.

And the punchline: AirPods are a surprisingly decent IMU head tracker that millions of people already own, which means the ceiling here is way higher than typical niche audio tooling.
