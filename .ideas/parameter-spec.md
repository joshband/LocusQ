Title: LocusQ Parameter Specification
Document Type: Parameter Specification
Author: APC Codex
Created Date: 2026-02-17
Last Modified Date: 2026-03-21

# LocusQ - Parameter Specification

**Version:** v0.1 (Ideation)
**Note:** Parameters are organized by mode. All parameters are DAW-automatable unless marked [internal].

---

## Global Parameters

| ID | Name | Type | Range | Default | Unit | Notes |
|:---|:-----|:-----|:------|:--------|:-----|:------|
| `mode` | Operating Mode | Enum | Calibrate / Emitter / Renderer | Emitter | — | Determines active parameter set |
| `room_profile` | Room Profile | String | file path | "" | — | [internal runtime state] Room profile reference; not an APVTS parameter in v1 |
| `bypass` | Bypass | Bool | On / Off | Off | — | True bypass |

---

## Calibrate Mode Parameters

| ID | Name | Type | Range | Default | Unit | Notes |
|:---|:-----|:-----|:------|:--------|:-----|:------|
| `cal_state` | Calibration State | Enum | Idle / Measuring / Complete | Idle | — | [internal runtime state] Derived from calibration engine status; not an APVTS parameter in v1 |
| `cal_mic_channel` | Mic Input Channel | Int | 1–8 | 1 | — | Input channel for measurement mic |
| `cal_spk_config` | Speaker Configuration | Enum | 4xMono / 2xStereo | 4xMono | — | How speakers are routed |
| `cal_spk1_out` | Speaker 1 Output | Int | 1–8 | 1 | — | Output channel assignment |
| `cal_spk2_out` | Speaker 2 Output | Int | 1–8 | 2 | — | Output channel assignment |
| `cal_spk3_out` | Speaker 3 Output | Int | 1–8 | 3 | — | Output channel assignment |
| `cal_spk4_out` | Speaker 4 Output | Int | 1–8 | 4 | — | Output channel assignment |
| `cal_test_level` | Test Signal Level | Float | -60.0 – 0.0 | -20.0 | dBFS | Level of calibration sweeps/noise |
| `cal_test_type` | Test Signal Type | Enum | Sweep / Pink / White / Impulse | Sweep | — | Measurement signal type |

### Calibration Outputs (stored in Room Profile, not automatable)
- Speaker distances (meters)
- Speaker angles (degrees from center)
- Speaker heights (meters)
- Per-speaker delay compensation (ms)
- Per-speaker level trim (dB)
- Room dimensions estimate (W x D x H meters)
- Basic reflection map (early reflection times per speaker)
- Per-speaker frequency response curve

---

## Emitter Mode Parameters

### Position

| ID | Name | Type | Range | Default | Unit | Notes |
|:---|:-----|:-----|:------|:--------|:-----|:------|
| `pos_azimuth` | Azimuth | Float | -180.0 – 180.0 | 0.0 | degrees | Horizontal angle (0 = front center) |
| `pos_elevation` | Elevation | Float | -90.0 – 90.0 | 0.0 | degrees | Vertical angle (0 = ear level) |
| `pos_distance` | Distance | Float | 0.0 – 50.0 | 2.0 | meters | Distance from listener position |
| `pos_x` | Position X | Float | -25.0 – 25.0 | 0.0 | meters | Cartesian X (auto-derived or manual) |
| `pos_y` | Position Y | Float | -25.0 – 25.0 | 0.0 | meters | Cartesian Y (auto-derived or manual) |
| `pos_z` | Position Z | Float | -10.0 – 10.0 | 0.0 | meters | Cartesian Z / height (auto-derived or manual) |
| `pos_coord_mode` | Coordinate Mode | Enum | Spherical / Cartesian | Spherical | — | Which coordinate set is primary |

### Size & Shape

| ID | Name | Type | Range | Default | Unit | Notes |
|:---|:-----|:-----|:------|:--------|:-----|:------|
| `size_width` | Width | Float | 0.01 – 20.0 | 0.5 | meters | Object horizontal spread |
| `size_depth` | Depth | Float | 0.01 – 20.0 | 0.5 | meters | Object front-to-back spread |
| `size_height` | Height | Float | 0.01 – 10.0 | 0.5 | meters | Object vertical spread |
| `size_link` | Link Dimensions | Bool | On / Off | On | — | Lock W/D/H to uniform scale |
| `size_uniform` | Uniform Scale | Float | 0.01 – 20.0 | 0.5 | meters | Master size when linked |

### Audio

| ID | Name | Type | Range | Default | Unit | Notes |
|:---|:-----|:-----|:------|:--------|:-----|:------|
| `emit_gain` | Emitter Gain | Float | -inf – +12.0 | 0.0 | dB | Output level of this emitter |
| `emit_mute` | Mute | Bool | On / Off | Off | — | Silence this emitter |
| `emit_solo` | Solo | Bool | On / Off | Off | — | Solo in Renderer context |
| `emit_spread` | Spread | Float | 0.0 – 1.0 | 0.0 | — | 0 = point source, 1 = fully diffuse |
| `emit_directivity` | Directivity | Float | 0.0 – 1.0 | 0.5 | — | 0 = omnidirectional, 1 = tight beam |
| `emit_dir_azimuth` | Directivity Aim Azimuth | Float | -180.0 – 180.0 | 0.0 | degrees | Where the beam points horizontally |
| `emit_dir_elevation` | Directivity Aim Elevation | Float | -90.0 – 90.0 | 0.0 | degrees | Where the beam points vertically |

### Physics

| ID | Name | Type | Range | Default | Unit | Notes |
|:---|:-----|:-----|:------|:--------|:-----|:------|
| `phys_enable` | Physics Enable | Bool | On / Off | Off | — | Toggle physics simulation |
| `phys_mass` | Mass | Float | 0.01 – 100.0 | 1.0 | kg | Affects inertia and momentum |
| `phys_drag` | Drag | Float | 0.0 – 10.0 | 0.5 | — | Air resistance / damping |
| `phys_elasticity` | Elasticity | Float | 0.0 – 1.0 | 0.7 | — | Bounce coefficient (0 = absorb, 1 = perfect bounce) |
| `phys_gravity` | Gravity | Float | -20.0 – 20.0 | 0.0 | m/s^2 | Downward pull (0 = zero-G, 9.8 = Earth) |
| `phys_gravity_dir` | Gravity Direction | Enum | Down / Up / ToCenter / FromCenter / Custom | Down | — | Direction of gravitational pull |
| `phys_friction` | Surface Friction | Float | 0.0 – 1.0 | 0.3 | — | Friction against room boundaries |
| `phys_vel_x` | Initial Velocity X | Float | -50.0 – 50.0 | 0.0 | m/s | Launch velocity X component |
| `phys_vel_y` | Initial Velocity Y | Float | -50.0 – 50.0 | 0.0 | m/s | Launch velocity Y component |
| `phys_vel_z` | Initial Velocity Z | Float | -50.0 – 50.0 | 0.0 | m/s | Launch velocity Z component |
| `phys_throw` | Throw Trigger | Bool | Off / Trigger | Off | — | Momentary: applies initial velocity and starts sim |
| `phys_reset` | Reset Position | Bool | Off / Trigger | Off | — | Momentary: returns object to keyframed/manual position |

### Emitter — Spring Oscillator (P3)

| ID | Name | Type | Range | Default | Unit | Notes |
|:---|:-----|:-----|:------|:--------|:-----|:------|
| `phys_spring_enable` | Spring Enable | Bool | On / Off | Off | — | Per-emitter spring tether enable |
| `phys_spring_k` | Spring Stiffness | Float | 0.5 – 500.0 | 10.0 | N/m | Stiffness; controls oscillation frequency ω = √(k/m) |
| `phys_spring_damp` | Spring Damping | Float | 0.0 – 1.0 | 0.3 | — | 0 = undamped, 1 = critically damped |
| `phys_spring_anchor_mode` | Spring Anchor Mode | Enum | Rest Pose / Fixed Point | Rest Pose | — | Anchor reference: rest_pose or fixed_point |
| `phys_spring_anchor_x` | Spring Anchor X | Float | -25.0 – 25.0 | 0.0 | meters | Fixed-point anchor X (used when mode = Fixed Point) |
| `phys_spring_anchor_y` | Spring Anchor Y | Float | 0.0 – 10.0 | 1.2 | meters | Fixed-point anchor Y |
| `phys_spring_anchor_z` | Spring Anchor Z | Float | -25.0 – 25.0 | 0.0 | meters | Fixed-point anchor Z |

### Emitter — Turbulence (P3)

| ID | Name | Type | Range | Default | Unit | Notes |
|:---|:-----|:-----|:------|:--------|:-----|:------|
| `phys_turbulence` | Turbulence | Float | 0.0 – 1.0 | 0.0 | — | Stochastic force intensity; max impulse = turbulence × mass × 9.8 |
| `phys_turbulence_rate` | Turbulence Rate | Float | 0.1 – 20.0 | 2.0 | Hz | One-pole smoother cutoff for coherent stochastic drift |

### Emitter — Angular Physics (P4)

| ID | Name | Type | Range | Default | Unit | Notes |
|:---|:-----|:-----|:------|:--------|:-----|:------|
| `phys_ang_enable` | Angular Enable | Bool | On / Off | Off | — | Enable angular physics (drives directivityAim) |
| `phys_ang_drag` | Angular Drag | Float | 0.0 – 1.0 | 0.3 | — | Angular velocity decay per tick |
| `phys_ang_impulse_x` | Angular Impulse X | Float | -20.0 – 20.0 | 0.0 | — | Angular impulse X component |
| `phys_ang_impulse_y` | Angular Impulse Y | Float | -20.0 – 20.0 | 0.0 | — | Angular impulse Y component |
| `phys_ang_impulse_z` | Angular Impulse Z | Float | -20.0 – 20.0 | 0.0 | — | Angular impulse Z component |
| `phys_ang_throw` | Angular Throw | Bool | Off / Trigger | Off | — | One-shot: apply angular impulse |
| `phys_ang_reset` | Angular Reset | Bool | Off / Trigger | Off | — | One-shot: zero angular velocity and restore default aim |
| `phys_ang_attractor_torque` | Attractor Torque | Float | 0.0 – 50.0 | 5.0 | — | Torque magnitude pulling aim toward nearest active attractor |

### Emitter — Per-Emitter Collision / Mass (P5–P6)

| ID | Name | Type | Range | Default | Unit | Notes |
|:---|:-----|:-----|:------|:--------|:-----|:------|
| `phys_flock_group` | Flock Group | Enum | None / Group 1 / Group 2 / Group 3 / Group 4 | None | — | Boids group assignment for inter-emitter flocking rules |
| `phys_collision_radius` | Collision Radius | Float | 0.05 – 5.0 | 0.3 | meters | Per-emitter collision sphere radius (used when phys_collide_emitters is On) |
| `phys_mass_override` | Mass Override | Float | 0.0 – 10.0 | 0.0 | kg | Per-emitter mass override; 0 = use global phys_mass default |

### Keyframe / Animation

| ID | Name | Type | Range | Default | Unit | Notes |
|:---|:-----|:-----|:------|:--------|:-----|:------|
| `anim_enable` | Animation Enable | Bool | On / Off | Off | — | Toggle keyframe animation |
| `anim_mode` | Animation Source | Enum | DAW / Internal | DAW | — | Use DAW automation or internal timeline |
| `anim_loop` | Loop Animation | Bool | On / Off | Off | — | Loop internal keyframe sequence |
| `anim_speed` | Animation Speed | Float | 0.1 – 10.0 | 1.0 | x | Playback rate multiplier for internal timeline |
| `anim_sync` | Transport Sync | Bool | On / Off | On | — | Sync internal timeline to DAW transport |

### Choreography Lab (CL-P1 — infrastructure gate)

| ID | Name | Type | Range | Default | Unit | Notes |
|:---|:-----|:-----|:------|:--------|:-----|:------|
| `choro_enable` | Choreography Enable | Bool | On / Off | Off | — | Master enable for Choreography Lab (ADR-0020 Layer 3). When off, ChoreographyWorker produces zero offsets and behaviour is identical to the pre-choreography baseline. |

### Choreography Lab — Formation Patterns (CL-P2)

| ID | Name | Type | Range | Default | Unit | Notes |
|:---|:-----|:-----|:------|:--------|:-----|:------|
| `choro_formation_type` | Formation Type | Enum | Line / Arc / Circle / Grid / Spiral / Sphere Surface / Custom | Circle | — | Active formation geometry. Custom produces zero offsets (placeholder; per-slot data not via APVTS). |
| `choro_form_axis` | Formation Axis | Enum | X / Y / Z | X | — | Axis of propagation for Line formation. |
| `choro_form_plane` | Formation Plane | Enum | XZ / XY / YZ | XZ | — | Reference plane for Arc and Circle formations. |
| `choro_form_radius` | Formation Radius | Float | 0.1 – 20.0 | 2.0 | m | Radius for Arc, Circle, Spiral, and Sphere Surface formations. |
| `choro_form_spacing` | Formation Spacing | Float | 0.1 – 5.0 | 1.0 | m | Inter-slot spacing for Line formation; inter-turn spacing for Spiral. |
| `choro_form_arc_angle` | Arc Angle | Float | 0.0 – 360.0 | 180.0 | deg | Total sweep angle for Arc formation. |
| `choro_form_phase_offset` | Phase Offset | Float | 0.0 – 360.0 | 0.0 | deg | Angular phase offset for Circle formation slot 0. |
| `choro_form_rows` | Grid Rows | Int | 1 – 16 | 2 | — | Row count for Grid formation. |
| `choro_form_cols` | Grid Cols | Int | 1 – 16 | 2 | — | Column count for Grid formation. |
| `choro_form_spacing_x` | Grid Spacing X | Float | 0.1 – 5.0 | 1.0 | m | Column spacing for Grid formation. |
| `choro_form_spacing_z` | Grid Spacing Z | Float | 0.1 – 5.0 | 1.0 | m | Row spacing for Grid formation. |
| `choro_form_turns` | Spiral Turns | Float | 0.5 – 8.0 | 2.0 | — | Number of full turns for Spiral formation. |
| `choro_form_height_rise` | Spiral Height Rise | Float | -5.0 – 5.0 | 1.0 | m | Total Y rise across all turns for Spiral formation. |
| `choro_formation_morph_rate` | Formation Morph Rate | Float | 0.01 – 10.0 | 1.0 | Hz | Rate at which morphPhase advances (cycles/second). Controls expansion/collapse animation speed. |
| `choro_formation_morph_loop` | Formation Morph Loop | Bool | On / Off | Off | — | When On, morphPhase wraps continuously. When Off, morphPhase clamps at 1.0. |
| `choro_formation_morph_pingpong` | Formation Morph Pingpong | Bool | On / Off | Off | — | When On (requires Loop=On), morphPhase reverses at 0.0 and 1.0 boundaries creating pulse animation. |

### Choreography Lab — Procedural Paths (CL-P3)

| ID | Name | Type | Range | Default | Unit | Notes |
|:---|:-----|:-----|:------|:--------|:-----|:------|
| `choro_path_type` | Path Type | Enum | Lissajous / Orbit / Pendulum / Figure Eight / Helix / Random Walk | Orbit | — | Active procedural path type. Path position is additive on the formation offset. When `choro_enable=Off` path is bypassed. |
| `choro_path_period` | Path Period | Float | 0.1 – 60.0 | 4.0 | s | Base period shared by Lissajous (base ω = 2π/period), Orbit, Figure Eight, and Helix. Pendulum ignores this (ω = √(g/L)). |
| `choro_path_speed` | Path Speed | Float | 0.1 – 10.0 | 1.0 | x | Time multiplier applied to all path types. Speed=2 runs all paths at 2× rate. |
| `choro_path_liss_freq_a` | Lissajous Freq A | Float | 1.0 – 8.0 | 2.0 | — | Frequency ratio a (x-axis): x = Ax·sin(a·ω·t + δ). |
| `choro_path_liss_freq_b` | Lissajous Freq B | Float | 1.0 – 8.0 | 3.0 | — | Frequency ratio b (y-axis): y = Ay·sin(b·ω·t). |
| `choro_path_liss_freq_c` | Lissajous Freq C | Float | 1.0 – 8.0 | 1.0 | — | Frequency ratio c (z-axis): z = Az·sin(c·ω·t). |
| `choro_path_liss_amp_x` | Lissajous Amp X | Float | 0.0 – 10.0 | 3.0 | m | X amplitude for Lissajous. |
| `choro_path_liss_amp_y` | Lissajous Amp Y | Float | 0.0 – 10.0 | 0.0 | m | Y amplitude for Lissajous. |
| `choro_path_liss_amp_z` | Lissajous Amp Z | Float | 0.0 – 10.0 | 3.0 | m | Z amplitude for Lissajous. |
| `choro_path_liss_phase` | Lissajous Phase | Float | 0.0 – 360.0 | 90.0 | deg | Phase offset δ applied to the x term of the Lissajous formula. |
| `choro_path_orbit_rx` | Orbit Radius X | Float | 0.1 – 20.0 | 3.0 | m | Semi-axis X of elliptical orbit. |
| `choro_path_orbit_rz` | Orbit Radius Z | Float | 0.1 – 20.0 | 3.0 | m | Semi-axis Z of elliptical orbit. |
| `choro_path_orbit_height` | Orbit Height | Float | -10.0 – 10.0 | 0.0 | m | Fixed Y offset of orbit centre from rest pose. |
| `choro_path_pend_length` | Pendulum Length | Float | 0.1 – 20.0 | 3.0 | m | Pendulum arm length; controls natural frequency ω = √(9.81/L). |
| `choro_path_pend_amp` | Pendulum Amplitude | Float | 0.0 – 180.0 | 45.0 | deg | Max swing angle. Position = L·sin(ampRad)·cos(ωt). |
| `choro_path_pend_plane` | Pendulum Plane | Enum | XZ / XY / YZ | XZ | — | Plane in which the pendulum swings. |
| `choro_path_fig8_scale` | Figure Eight Scale | Float | 0.1 – 20.0 | 3.0 | m | Amplitude of figure-eight lemniscate (Lissajous 2:1 at 90° phase). |
| `choro_path_fig8_plane` | Figure Eight Plane | Enum | XZ / XY / YZ | XZ | — | Plane of the figure-eight curve. |
| `choro_path_helix_radius` | Helix Radius | Float | 0.1 – 20.0 | 2.0 | m | Circle radius of the helix. |
| `choro_path_helix_pitch` | Helix Pitch | Float | 0.01 – 5.0 | 1.0 | m | Y excursion per revolution (triangle-wave; continuous and bounded). |
| `choro_path_helix_dir` | Helix Direction | Enum | Up / Down | Up | — | Phase sign of Y triangle-wave component: Up = +pitch, Down = −pitch. |
| `choro_path_walk_step` | Walk Step Size | Float | 0.001 – 0.5 | 0.02 | m/tick | Random displacement magnitude per physics tick (per axis). |
| `choro_path_walk_bounds` | Walk Bounds | Float | 0.1 – 20.0 | 5.0 | m | Half-width of axis-aligned bounding box; walk position is clamped per axis. |
| `choro_path_walk_seed` | Walk Seed | Int | 0 – 65535 | 42 | — | PRNG seed; changing the seed resets the walk position and re-seeds the RNG. |

### Choreography Lab — Beat-Sync (CL-P4)

All params pending BL-114 for WebView surface.

| Parameter ID | Name | Type | Range | Default | Unit | Notes |
|:---|:---|:---|:---|:---|:---|:---|
| `choro_beat_enable` | Beat Sync Enable | Bool | — | false | — | Master enable for beat-sync choreography sub-system. When off, BeatSyncSystem produces no beat events and no gain-dip. |
| `choro_beat_division` | Beat Division | Enum | Whole / Half / Quarter / Eighth / Sixteenth | Quarter | — | Grid quantization unit. Whole = 4 PPQ, Half = 2 PPQ, Quarter = 1 PPQ, Eighth = 0.5 PPQ, Sixteenth = 0.25 PPQ. |
| `choro_beat_mode` | Beat Mode | Enum | Snap / Glide / Teleport | Snap | — | Behavior on beat boundary. Snap: immediate formation step (no gain change). Glide: interpolate (future). Teleport: instant jump + gain-dip envelope. |
| `choro_teleport_dip_db` | Teleport Dip Depth | Float | −60.0 – 0.0 | −18.0 | dB | Gain reduction depth on Teleport beat. Only active when `choro_beat_mode = Teleport`. Negative values only; clipped to [−60, 0]. |
| `choro_teleport_decay_ms` | Teleport Decay | Float | 1.0 – 2000.0 | 100.0 | ms | One-pole recovery time for the Teleport gain-dip envelope. Gain recovers from dip toward 1.0 with τ = decayMs/1000. |

### Choreography Lab — Bake to Timeline (CL-P7)

All params pending BL-116 for WebView surface.

| Parameter ID | Name | Type | Range | Default | Unit | Notes |
|:---|:---|:---|:---|:---|:---|:---|
| `bake_start` | Bake Start | Float | 0.0 – 1000.0 | 0.0 | PPQ | Transport PPQ position where bake capture begins. Auto-armed: recording starts when DAW transport enters this position. |
| `bake_end` | Bake End | Float | 0.0 – 1000.0 | 8.0 | PPQ | Transport PPQ position where bake capture ends. Capture stops and export is triggered when PPQ reaches this position or transport stops. |
| `bake_kf_density` | Keyframe Density | Float | 0.5 – 100.0 | 10.0 | kf/s | Target keyframe rate after decimation. Actual density may be lower after linear thinning removes redundant keyframes within tolerance. |
| `bake_curve_fit_tolerance` | Curve Fit Tolerance | Float | 0.001 – 1.0 | 0.01 | m | Maximum allowed positional error during linear thinning. Keyframes are removed only if linear interpolation from neighbours stays within this distance (metres). |

### Emitter Identity

| ID | Name | Type | Range | Default | Unit | Notes |
|:---|:-----|:-----|:------|:--------|:-----|:------|
| `emit_label` | Label | String | 0–32 chars | "Emitter" | — | Display name in Renderer view |
| `emit_color` | Color | Int | 0–15 | auto | — | Color index for visualization |
| `emit_id` | Instance ID | Int | 0–255 | auto-assign | — | [internal] Unique ID in scene graph |

---

## Renderer Mode Parameters

### Master Output

| ID | Name | Type | Range | Default | Unit | Notes |
|:---|:-----|:-----|:------|:--------|:-----|:------|
| `rend_master_gain` | Master Gain | Float | -inf – +12.0 | 0.0 | dB | Final output level |
| `rend_spk1_gain` | Speaker 1 Trim | Float | -24.0 – +12.0 | 0.0 | dB | Per-speaker level adjustment |
| `rend_spk2_gain` | Speaker 2 Trim | Float | -24.0 – +12.0 | 0.0 | dB | Per-speaker level adjustment |
| `rend_spk3_gain` | Speaker 3 Trim | Float | -24.0 – +12.0 | 0.0 | dB | Per-speaker level adjustment |
| `rend_spk4_gain` | Speaker 4 Trim | Float | -24.0 – +12.0 | 0.0 | dB | Per-speaker level adjustment |
| `rend_spk1_delay` | Speaker 1 Delay | Float | 0.0 – 50.0 | 0.0 | ms | Per-speaker delay (from calibration or manual) |
| `rend_spk2_delay` | Speaker 2 Delay | Float | 0.0 – 50.0 | 0.0 | ms | Per-speaker delay |
| `rend_spk3_delay` | Speaker 3 Delay | Float | 0.0 – 50.0 | 0.0 | ms | Per-speaker delay |
| `rend_spk4_delay` | Speaker 4 Delay | Float | 0.0 – 50.0 | 0.0 | ms | Per-speaker delay |

### Spatialization

| ID | Name | Type | Range | Default | Unit | Notes |
|:---|:-----|:-----|:------|:--------|:-----|:------|
| `rend_quality` | Quality Tier | Enum | Draft / Final | Draft | — | Real-time vs offline rendering |
| `rend_distance_model` | Distance Model | Enum | InverseSquare / Linear / Logarithmic / Custom | InverseSquare | — | How gain falls off with distance |
| `rend_distance_ref` | Reference Distance | Float | 0.1 – 10.0 | 1.0 | meters | Distance at which gain = 0dB |
| `rend_distance_max` | Max Distance | Float | 1.0 – 100.0 | 50.0 | meters | Beyond this, gain is clamped to floor |
| `rend_doppler` | Doppler Enable | Bool | On / Off | Off | — | Pitch shift from object velocity |
| `rend_doppler_scale` | Doppler Scale | Float | 0.0 – 5.0 | 1.0 | x | Exaggeration factor for doppler effect |
| `rend_air_absorb` | Air Absorption | Bool | On / Off | On | — | High-frequency rolloff with distance |

### Room Acoustics

| ID | Name | Type | Range | Default | Unit | Notes |
|:---|:-----|:-----|:------|:--------|:-----|:------|
| `rend_room_enable` | Room Simulation | Bool | On / Off | On | — | Enable room reflections from profile |
| `rend_room_mix` | Room Mix | Float | 0.0 – 1.0 | 0.3 | — | Dry/wet for room reflections |
| `rend_room_size` | Room Size Override | Float | 0.5 – 5.0 | 1.0 | x | Scale factor on calibrated room size |
| `rend_room_damping` | Room Damping | Float | 0.0 – 1.0 | 0.5 | — | High-frequency absorption of walls |
| `rend_room_er_only` | Early Reflections Only | Bool | On / Off | Off | — | Disable late reverb tail |

### Physics Engine (Global)

| ID | Name | Type | Range | Default | Unit | Notes |
|:---|:-----|:-----|:------|:--------|:-----|:------|
| `rend_phys_rate` | Physics Update Rate | Enum | 30 / 60 / 120 / 240 Hz | 60 | Hz | Simulation tick rate |
| `rend_phys_walls` | Wall Collision | Bool | On / Off | On | — | Objects bounce off room boundaries |
| `rend_phys_interact` | Object Interaction | Bool | On / Off | Off | — | Enables global soft inter-emitter interaction force for physics-enabled emitters |
| `rend_phys_pause` | Pause Physics | Bool | On / Off | Off | — | Freeze all physics simulation |

### Renderer — Attractors (×4 slots, N = 0..3)

| ID | Name | Type | Range | Default | Unit | Notes |
|:---|:-----|:-----|:------|:--------|:-----|:------|
| `attractor_N_active` | Attractor N Active | Bool | On / Off | Off | — | Per-slot active flag; N ∈ {0,1,2,3} |
| `attractor_N_pos_x` | Attractor N X | Float | -25.0 – 25.0 | 0.0 | meters | Force-source X position; N ∈ {0,1,2,3} |
| `attractor_N_pos_y` | Attractor N Y | Float | 0.0 – 10.0 | 1.2 | meters | Force-source Y position (height) |
| `attractor_N_pos_z` | Attractor N Z | Float | -25.0 – 25.0 | 0.0 | meters | Force-source Z position |
| `attractor_N_strength` | Attractor N Strength | Float | -100.0 – 100.0 | 10.0 | — | Signed force magnitude; positive = attract, negative = repel |
| `attractor_N_falloff` | Attractor N Falloff | Enum | 1/r / 1/r² / Constant | 1/r² | — | Force falloff mode |
| `attractor_N_radius` | Attractor N Radius | Float | 0.1 – 20.0 | 5.0 | meters | Hard influence cutoff radius |
| `attractor_N_orbit_stabilize` | Attractor N Orbit Stabilize | Bool | On / Off | Off | — | Injects tangential correction force to maintain constant orbital radius |

### Renderer — Boids Groups (×4 groups, G = 0..3)

| ID | Name | Type | Range | Default | Unit | Notes |
|:---|:-----|:-----|:------|:--------|:-----|:------|
| `phys_flock_G_enable` | Flock G Enable | Bool | On / Off | Off | — | Per-group active flag; G ∈ {0,1,2,3} |
| `phys_flock_G_sep_weight` | Flock G Sep Weight | Float | 0.0 – 1.0 | 1.0 | — | Separation rule weight |
| `phys_flock_G_align_weight` | Flock G Align Weight | Float | 0.0 – 1.0 | 0.5 | — | Alignment rule weight |
| `phys_flock_G_coh_weight` | Flock G Coh Weight | Float | 0.0 – 1.0 | 0.5 | — | Cohesion rule weight |
| `phys_flock_G_sep_radius` | Flock G Sep Radius | Float | 0.1 – 20.0 | 1.5 | meters | Separation influence radius |
| `phys_flock_G_align_radius` | Flock G Align Radius | Float | 0.1 – 20.0 | 3.0 | meters | Alignment influence radius |
| `phys_flock_G_coh_radius` | Flock G Coh Radius | Float | 0.1 – 50.0 | 5.0 | meters | Cohesion influence radius |
| `phys_flock_G_max_speed` | Flock G Max Speed | Float | 0.1 – 50.0 | 5.0 | m/s | Per-emitter speed cap within group |

### Renderer — Collision Globals

| ID | Name | Type | Range | Default | Unit | Notes |
|:---|:-----|:-----|:------|:--------|:-----|:------|
| `phys_collide_emitters` | Collide Emitters | Bool | On / Off | Off | — | Global enable for O(n²) inter-emitter collision pass |
| `phys_collision_gain_scale` | Collision Gain Scale | Float | 0.0 – 10.0 | 1.0 | — | Peak dB scale for collision gain transient envelope |
| `phys_collision_decay_ms` | Collision Decay ms | Float | 1.0 – 500.0 | 50.0 | ms | Exponential decay of collision gain burst |

### Renderer — Boundary

| ID | Name | Type | Range | Default | Unit | Notes |
|:---|:-----|:-----|:------|:--------|:-----|:------|
| `phys_boundary_mode` | Boundary Mode | Enum | Hard / Soft / Passthrough | Hard | — | Wall behavior: hard reflect, soft repulsion field, or passthrough |
| `phys_soft_boundary_depth` | Soft Boundary Depth | Float | 0.1 – 5.0 | 0.5 | meters | Influence depth for soft-boundary repulsive force field |

### Visualization

| ID | Name | Type | Range | Default | Unit | Notes |
|:---|:-----|:-----|:------|:--------|:-----|:------|
| `rend_viz_mode` | View Mode | Enum | Perspective / TopDown / Front / Side | Perspective | — | Camera angle for 3D view |
| `rend_viz_trails` | Show Trails | Bool | On / Off | On | — | Display object motion trails |
| `rend_viz_trail_len` | Trail Length | Float | 0.5 – 30.0 | 5.0 | seconds | How far back trails render |
| `rend_viz_vectors` | Show Velocity Vectors | Bool | On / Off | Off | — | Display motion direction arrows |
| `rend_viz_grid` | Show Grid | Bool | On / Off | On | — | Floor grid for spatial reference |
| `rend_viz_labels` | Show Labels | Bool | On / Off | On | — | Display emitter names |
| `rend_viz_cam_x` | Camera X | Float | -50.0 – 50.0 | 0.0 | meters | [internal] Camera position |
| `rend_viz_cam_y` | Camera Y | Float | -50.0 – 50.0 | -5.0 | meters | [internal] Camera position |
| `rend_viz_cam_z` | Camera Z | Float | -20.0 – 20.0 | 3.0 | meters | [internal] Camera position |
| `rend_viz_cam_zoom` | Camera Zoom | Float | 0.1 – 10.0 | 1.0 | x | [internal] Zoom level |

---

## Parameter Count Summary

| Mode | Parameter Count |
|:-----|:---------------|
| Global | 3 |
| Calibrate | 9 + profile outputs |
| Emitter | 55 (+20: Spring ×7, Turbulence ×2, Angular ×8, Flock group + Collision radius + Mass override ×3; +24: Physics DAW Automation phys_out_spread_mod_N + phys_out_gain_mod_N + phys_frozen_N × 8 slots) = 99 |
| Renderer | 98 (+69: Attractors ×32, Boids Groups ×32, Collision globals ×3, Boundary ×2) |
| **Total Unique** | **~165** (+89 Tier A physics simulation params P2–P6; +24 Physics DAW Automation output/freeze params) |

---

## As-Built Contract Delta (2026-02-20)

1. `room_profile` and `cal_state` remain documented internal states, but are runtime/native status fields in current v1 rather than APVTS parameters.
2. `rend_phys_interact` is now runtime-active in v1: renderer-mode global control toggles inter-emitter interaction force computation for physics-enabled emitters, and Stage 12 incremental UI now binds this toggle.
3. Stage 12 incremental UI binds renderer depth controls (`rend_doppler_scale`, `rend_room_mix`, `rend_room_size`, `rend_room_damping`), while these remain missing in legacy WebView traceability rows that pre-date Stage 10-12.
4. `emit_dir_azimuth`, `emit_dir_elevation`, and `phys_vel_x/y/z` are active DSP/runtime parameters but are not yet exposed in Stage 12 incremental control bindings.
5. Device compatibility contract for next phase: mono/stereo/quad output layout support is implemented in processor/runtime; laptop speakers and headphones run through stereo output paths, with advanced personalized binaural processing still deferred.

---

## Notes

1. **Automation:** All Float/Enum/Bool parameters are DAW-automatable except those marked [internal].
2. **Coordinate sync:** When `pos_coord_mode` is Spherical, Cartesian values are derived (and vice versa). Only the primary set is automatable.
3. **Authority precedence (ADR-0020):** DAW/APVTS base state is authoritative; internal timeline defines rest pose for animated tracks; Choreography Lab adds generative offset; physics applies additive offset on top of the composed three-layer rest pose. ADR-0020 supersedes ADR-0003's three-layer model with this four-layer authority chain.
4. **Inter-instance routing (ADR-0002):** Emitter metadata is canonical shared state in `SceneGraph`; v1 renderer path may consume ephemeral same-block emitter audio pointers as fast path.
5. **Room Profile dependency:** Emitter and Renderer modes will show a warning and pass audio through unprocessed if no Room Profile is loaded.
6. **AI scope gate (ADR-0004):** AI orchestration is deferred from v1 critical path and planned only for post-v1 phases.
7. **Parameter namespaces:** `phys_flock_*` (Boids/flocking) and `phys_ang_*` (angular physics) prefixes are now active in Tier A. `phys_fluid_*`, `phys_mat_*`, `phys_field_*`, `phys_preset_*` remain reserved for Tier B.
8. **Device compatibility contract (Stage 14 planning):** laptop speakers and headphones are first-class runtime targets through existing stereo output layout support; future personalized binaural/HRTF remains post-v1.

---

## Physics DAW Automation Mirror Parameters (8 slots × 4, N = 0..7)

Physics output parameters exposed as DAW-automatable APVTS floats. These mirror per-slot physics runtime state into the DAW automation lane so hosts can read, record, and replay physics-driven modulation.

| ID | Name | Type | Range | Default | Unit | Notes |
|:---|:-----|:-----|:------|:--------|:-----|:------|
| `phys_out_gain_mod_N` | Physics Gain Mod N | Float | 0.0–1.0 | 0.0 | — | Physics gain modulation mirror for slot N. Tracks real-time physics output when live; holds last-live snapshot when frozen. DAW-automatable. N ∈ {0..7} |
| `phys_out_spread_mod_N` | Physics Spread Mod N | Float | 0.0–1.0 | 0.0 | — | Physics spread modulation mirror for slot N. Same freeze/live behavior as gain mod. DAW-automatable. N ∈ {0..7} |
| `phys_out_transient_N` | Physics Transient N | Float | 0.0–1.0 | 0.0 | — | Physics one-shot transient mirror for slot N. Tracks live `gainTransient` bursts directly and intentionally bypasses freeze snapshotting so hosts can observe transient events in real time. DAW-automatable. N ∈ {0..7} |
| `phys_frozen_N` | Physics Frozen N | Bool | 0.0/1.0 | 0.0 | — | Freeze gate for physics slot N. When 1, `phys_out_gain_mod_N` and `phys_out_spread_mod_N` hold their last-live snapshot. `gainTransient` is excluded from the snapshot. DAW-automatable. N ∈ {0..7} |

Total: 32 parameters (8 gain mod + 8 spread mod + 8 transient mirrors + 8 freeze gates).

**Authority chain (ADR-0020):** These parameters sit at the DAW/APVTS layer. Physics computes the live value; the freeze gate governs whether the gain/spread mirrors advance or hold. `phys_out_transient_N` mirrors `gainTransient` directly and intentionally bypasses freeze snapshots to prevent static transient artifacts while still giving hosts a real transient-observation lane.

---

## As-Built Note (2026-02-18)

- Parameter plumbing for phases up through 2.3 is implemented in `Source/PluginProcessor.cpp` APVTS layout and active processor paths.
- Calibration workflow controls are bridged to UI runtime via native calls and status polling.
- QA scenario stimulus references have been updated to current harness canonical stimulus contracts (for example `noise/white`, `sweep/linear_sine`).
- Status/evidence tracking for these validations is now recorded in `plugins/LocusQ/status.json` and `plugins/LocusQ/TestEvidence/`.
