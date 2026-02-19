Title: LocusQ Parameter Specification
Document Type: Parameter Specification
Author: APC Codex
Created Date: 2026-02-17
Last Modified Date: 2026-02-19

# LocusQ - Parameter Specification

**Version:** v0.1 (Ideation)
**Note:** Parameters are organized by mode. All parameters are DAW-automatable unless marked [internal].

---

## Global Parameters

| ID | Name | Type | Range | Default | Unit | Notes |
|:---|:-----|:-----|:------|:--------|:-----|:------|
| `mode` | Operating Mode | Enum | Calibrate / Emitter / Renderer | Emitter | — | Determines active parameter set |
| `room_profile` | Room Profile | String | file path | "" | — | [internal] Loaded Room Profile reference |
| `bypass` | Bypass | Bool | On / Off | Off | — | True bypass |

---

## Calibrate Mode Parameters

| ID | Name | Type | Range | Default | Unit | Notes |
|:---|:-----|:-----|:------|:--------|:-----|:------|
| `cal_state` | Calibration State | Enum | Idle / Measuring / Complete | Idle | — | [internal] Current calibration stage |
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

### Keyframe / Animation

| ID | Name | Type | Range | Default | Unit | Notes |
|:---|:-----|:-----|:------|:--------|:-----|:------|
| `anim_enable` | Animation Enable | Bool | On / Off | Off | — | Toggle keyframe animation |
| `anim_mode` | Animation Source | Enum | DAW / Internal | DAW | — | Use DAW automation or internal timeline |
| `anim_loop` | Loop Animation | Bool | On / Off | Off | — | Loop internal keyframe sequence |
| `anim_speed` | Animation Speed | Float | 0.1 – 10.0 | 1.0 | x | Playback rate multiplier for internal timeline |
| `anim_sync` | Transport Sync | Bool | On / Off | On | — | Sync internal timeline to DAW transport |

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
| `rend_phys_interact` | Object Interaction | Bool | On / Off | Off | — | Objects affect each other (v2 stretch) |
| `rend_phys_pause` | Pause Physics | Bool | On / Off | Off | — | Freeze all physics simulation |

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
| Emitter | 35 |
| Renderer | 29 |
| **Total Unique** | **~76** |

---

## Notes

1. **Automation:** All Float/Enum/Bool parameters are DAW-automatable except those marked [internal].
2. **Coordinate sync:** When `pos_coord_mode` is Spherical, Cartesian values are derived (and vice versa). Only the primary set is automatable.
3. **Authority precedence (ADR-0003):** DAW/APVTS base state is authoritative; when enabled, internal timeline defines rest pose for animated tracks; physics applies additive offset on top of that rest pose.
4. **Inter-instance routing (ADR-0002):** Emitter metadata is canonical shared state in `SceneGraph`; v1 renderer path may consume ephemeral same-block emitter audio pointers as fast path.
5. **Room Profile dependency:** Emitter and Renderer modes will show a warning and pass audio through unprocessed if no Room Profile is loaded.
6. **AI scope gate (ADR-0004):** AI orchestration is deferred from v1 critical path and planned only for post-v1 phases.
7. **Future expansion:** Flocking, swarm, fluid dynamics, and material properties will add parameters in v2. The parameter ID scheme leaves room for `phys_flock_*`, `phys_fluid_*`, `phys_mat_*` prefixes.

---

## As-Built Note (2026-02-18)

- Parameter plumbing for phases up through 2.3 is implemented in `Source/PluginProcessor.cpp` APVTS layout and active processor paths.
- Calibration workflow controls are bridged to UI runtime via native calls and status polling.
- QA scenario stimulus references have been updated to current harness canonical stimulus contracts (for example `noise/white`, `sweep/linear_sine`).
- Status/evidence tracking for these validations is now recorded in `plugins/LocusQ/status.json` and `plugins/LocusQ/TestEvidence/`.
