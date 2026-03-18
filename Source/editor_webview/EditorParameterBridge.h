#pragma once

#include <array>
#include <memory>
#include <utility>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

namespace locusq::editor_webview
{

enum class ParameterScope
{
    global,
    calibrate,
    emitter,
    renderer
};

enum class RelayKind
{
    slider,
    toggle,
    combo
};

struct ParameterBridgeSpec
{
    ParameterScope scope {};
    RelayKind relayKind {};
    const char* parameterId = "";
};

// BL-039/W1-A canonical relay inventory. One spec list now drives both
// WebView relay registration and APVTS attachment construction.
inline constexpr std::array kParameterBridgeSpecs {
    ParameterBridgeSpec { ParameterScope::global, RelayKind::toggle, "bypass" },
    ParameterBridgeSpec { ParameterScope::global, RelayKind::combo, "mode" },

    ParameterBridgeSpec { ParameterScope::calibrate, RelayKind::combo, "cal_device_profile" },
    ParameterBridgeSpec { ParameterScope::calibrate, RelayKind::slider, "cal_mic_channel" },
    ParameterBridgeSpec { ParameterScope::calibrate, RelayKind::combo, "cal_monitoring_path" },
    ParameterBridgeSpec { ParameterScope::calibrate, RelayKind::slider, "cal_spk1_out" },
    ParameterBridgeSpec { ParameterScope::calibrate, RelayKind::slider, "cal_spk2_out" },
    ParameterBridgeSpec { ParameterScope::calibrate, RelayKind::slider, "cal_spk3_out" },
    ParameterBridgeSpec { ParameterScope::calibrate, RelayKind::slider, "cal_spk4_out" },
    ParameterBridgeSpec { ParameterScope::calibrate, RelayKind::combo, "cal_spk_config" },
    ParameterBridgeSpec { ParameterScope::calibrate, RelayKind::slider, "cal_test_level" },
    ParameterBridgeSpec { ParameterScope::calibrate, RelayKind::combo, "cal_test_type" },
    ParameterBridgeSpec { ParameterScope::calibrate, RelayKind::combo, "cal_topology_profile" },

    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::toggle, "anim_enable" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::toggle, "anim_loop" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::combo, "anim_mode" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "anim_speed" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::toggle, "anim_sync" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "emit_color" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "emit_dir_azimuth" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "emit_dir_elevation" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "emit_directivity" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "emit_gain" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::toggle, "emit_mute" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::toggle, "emit_solo" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "emit_spread" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_drag" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_elasticity" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::toggle, "phys_enable" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_friction" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_gravity" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::combo, "phys_gravity_dir" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_mass" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::toggle, "phys_reset" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::toggle, "phys_throw" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_vel_x" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_vel_y" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_vel_z" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "pos_azimuth" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::combo, "pos_coord_mode" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "pos_distance" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "pos_elevation" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "pos_x" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "pos_y" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "pos_z" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::toggle, "size_link" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "size_uniform" },

    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::toggle, "rend_air_absorb" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::toggle, "rend_audition_enable" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::combo, "rend_audition_level" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::combo, "rend_audition_motion" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::combo, "rend_audition_signal" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "rend_distance_max" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::combo, "rend_distance_model" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "rend_distance_ref" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::toggle, "rend_doppler" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "rend_doppler_scale" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::combo, "rend_headphone_mode" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::combo, "rend_headphone_profile" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "rend_master_gain" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::toggle, "rend_phys_interact" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::toggle, "rend_phys_pause" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::combo, "rend_phys_rate" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::toggle, "rend_phys_walls" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::combo, "rend_quality" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "rend_room_damping" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::toggle, "rend_room_enable" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::toggle, "rend_room_er_only" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "rend_room_mix" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "rend_room_size" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "rend_spk1_delay" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "rend_spk1_gain" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "rend_spk2_delay" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "rend_spk2_gain" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "rend_spk3_delay" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "rend_spk3_gain" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "rend_spk4_delay" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "rend_spk4_gain" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "rend_viz_diag_mix" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::toggle, "rend_viz_grid" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::toggle, "rend_viz_labels" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::combo, "rend_viz_mode" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::toggle, "rend_viz_physics_lens" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "rend_viz_trail_len" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::toggle, "rend_viz_trails" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::toggle, "rend_viz_vectors" },

    // ===== P3 — Spring oscillator (emitter) =====
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::toggle, "phys_spring_enable" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_spring_k" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_spring_damp" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::combo,  "phys_spring_anchor_mode" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_spring_anchor_x" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_spring_anchor_y" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_spring_anchor_z" },

    // ===== P3 — Turbulence (emitter) =====
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_turbulence" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_turbulence_rate" },

    // ===== P4 — Angular physics (emitter) =====
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::toggle, "phys_ang_enable" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_ang_drag" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_ang_impulse_x" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_ang_impulse_y" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_ang_impulse_z" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::toggle, "phys_ang_throw" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::toggle, "phys_ang_reset" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_ang_attractor_torque" },

    // ===== P5 — Boids group assignment (emitter) =====
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::combo,  "phys_flock_group" },

    // ===== P6 — Collision radius + mass override (emitter) =====
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_collision_radius" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_mass_override" },

    // ===== P2 — Boundary mode (renderer) =====
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::combo,  "phys_boundary_mode" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "phys_soft_boundary_depth" },

    // ===== P6 — Inter-emitter collision globals (renderer) =====
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::toggle, "phys_collide_emitters" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "phys_collision_gain_scale" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "phys_collision_decay_ms" },

    // ===== P2 — Attractors ×4 (renderer / scene) =====
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::toggle, "attractor_0_active" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "attractor_0_pos_x" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "attractor_0_pos_y" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "attractor_0_pos_z" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "attractor_0_strength" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::combo,  "attractor_0_falloff" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "attractor_0_radius" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::toggle, "attractor_0_orbit_stabilize" },

    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::toggle, "attractor_1_active" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "attractor_1_pos_x" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "attractor_1_pos_y" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "attractor_1_pos_z" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "attractor_1_strength" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::combo,  "attractor_1_falloff" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "attractor_1_radius" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::toggle, "attractor_1_orbit_stabilize" },

    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::toggle, "attractor_2_active" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "attractor_2_pos_x" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "attractor_2_pos_y" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "attractor_2_pos_z" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "attractor_2_strength" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::combo,  "attractor_2_falloff" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "attractor_2_radius" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::toggle, "attractor_2_orbit_stabilize" },

    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::toggle, "attractor_3_active" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "attractor_3_pos_x" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "attractor_3_pos_y" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "attractor_3_pos_z" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "attractor_3_strength" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::combo,  "attractor_3_falloff" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "attractor_3_radius" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::toggle, "attractor_3_orbit_stabilize" },

    // ===== P5 — Boids groups ×4 (renderer / scene) =====
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::toggle, "phys_flock_0_enable" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "phys_flock_0_sep_weight" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "phys_flock_0_align_weight" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "phys_flock_0_coh_weight" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "phys_flock_0_sep_radius" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "phys_flock_0_align_radius" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "phys_flock_0_coh_radius" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "phys_flock_0_max_speed" },

    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::toggle, "phys_flock_1_enable" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "phys_flock_1_sep_weight" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "phys_flock_1_align_weight" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "phys_flock_1_coh_weight" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "phys_flock_1_sep_radius" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "phys_flock_1_align_radius" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "phys_flock_1_coh_radius" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "phys_flock_1_max_speed" },

    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::toggle, "phys_flock_2_enable" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "phys_flock_2_sep_weight" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "phys_flock_2_align_weight" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "phys_flock_2_coh_weight" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "phys_flock_2_sep_radius" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "phys_flock_2_align_radius" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "phys_flock_2_coh_radius" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "phys_flock_2_max_speed" },

    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::toggle, "phys_flock_3_enable" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "phys_flock_3_sep_weight" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "phys_flock_3_align_weight" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "phys_flock_3_coh_weight" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "phys_flock_3_sep_radius" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "phys_flock_3_align_radius" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "phys_flock_3_coh_radius" },
    ParameterBridgeSpec { ParameterScope::renderer, RelayKind::slider, "phys_flock_3_max_speed" },

    // ===== Physics DAW Output Mirrors and Freeze Toggles =====
    // Physics DAW Output Mirrors (slider = read-only display relay)
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_out_spread_mod_0" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_out_spread_mod_1" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_out_spread_mod_2" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_out_spread_mod_3" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_out_spread_mod_4" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_out_spread_mod_5" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_out_spread_mod_6" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_out_spread_mod_7" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_out_gain_mod_0" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_out_gain_mod_1" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_out_gain_mod_2" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_out_gain_mod_3" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_out_gain_mod_4" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_out_gain_mod_5" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_out_gain_mod_6" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::slider, "phys_out_gain_mod_7" },
    // Physics Freeze Toggles
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::toggle, "phys_frozen_0" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::toggle, "phys_frozen_1" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::toggle, "phys_frozen_2" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::toggle, "phys_frozen_3" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::toggle, "phys_frozen_4" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::toggle, "phys_frozen_5" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::toggle, "phys_frozen_6" },
    ParameterBridgeSpec { ParameterScope::emitter, RelayKind::toggle, "phys_frozen_7" }
};

class ParameterBridgeAttachment
{
public:
    virtual ~ParameterBridgeAttachment() = default;
};

template <typename Attachment>
class ParameterBridgeAttachmentModel final : public ParameterBridgeAttachment
{
public:
    template <typename... Args>
    explicit ParameterBridgeAttachmentModel (Args&&... args)
        : attachment (std::forward<Args> (args)...)
    {
    }

private:
    Attachment attachment;
};

class ParameterBridgeRelay
{
public:
    virtual ~ParameterBridgeRelay() = default;

    virtual const char* getParameterId() const = 0;
    virtual juce::WebBrowserComponent::Options addToOptions (juce::WebBrowserComponent::Options options) = 0;
    virtual std::unique_ptr<ParameterBridgeAttachment> makeAttachment (juce::RangedAudioParameter& parameter,
                                                                      juce::UndoManager* undoManager) = 0;
};

template <typename Relay, typename Attachment>
class ParameterBridgeRelayModel final : public ParameterBridgeRelay
{
public:
    explicit ParameterBridgeRelayModel (const char* parameterIdIn)
        : parameterId (parameterIdIn),
          relay (parameterIdIn)
    {
    }

    const char* getParameterId() const override
    {
        return parameterId;
    }

    juce::WebBrowserComponent::Options addToOptions (juce::WebBrowserComponent::Options options) override
    {
        return std::move (options).withOptionsFrom (relay);
    }

    std::unique_ptr<ParameterBridgeAttachment> makeAttachment (juce::RangedAudioParameter& parameter,
                                                               juce::UndoManager* undoManager) override
    {
        return std::make_unique<ParameterBridgeAttachmentModel<Attachment>> (parameter, relay, undoManager);
    }

private:
    const char* parameterId = "";
    Relay relay;
};

class ParameterBridgeRelayStore
{
public:
    ParameterBridgeRelayStore()
    {
        relays.reserve (kParameterBridgeSpecs.size());

        for (const auto& spec : kParameterBridgeSpecs)
            relays.push_back (makeRelay (spec));
    }

    juce::WebBrowserComponent::Options addToOptions (juce::WebBrowserComponent::Options options)
    {
        for (auto& relay : relays)
            options = relay->addToOptions (std::move (options));

        return options;
    }

    std::vector<std::unique_ptr<ParameterBridgeAttachment>> createAttachments (juce::AudioProcessorValueTreeState& apvts,
                                                                               juce::UndoManager* undoManager = nullptr)
    {
        std::vector<std::unique_ptr<ParameterBridgeAttachment>> attachments;
        attachments.reserve (relays.size());

        for (auto& relay : relays)
        {
            auto* parameter = apvts.getParameter (relay->getParameterId());
            jassert (parameter != nullptr);

            if (parameter != nullptr)
                attachments.push_back (relay->makeAttachment (*parameter, undoManager));
        }

        return attachments;
    }

private:
    static std::unique_ptr<ParameterBridgeRelay> makeRelay (const ParameterBridgeSpec& spec)
    {
        switch (spec.relayKind)
        {
            case RelayKind::slider:
                return std::make_unique<ParameterBridgeRelayModel<juce::WebSliderRelay,
                                                                  juce::WebSliderParameterAttachment>> (spec.parameterId);

            case RelayKind::toggle:
                return std::make_unique<ParameterBridgeRelayModel<juce::WebToggleButtonRelay,
                                                                  juce::WebToggleButtonParameterAttachment>> (spec.parameterId);

            case RelayKind::combo:
                return std::make_unique<ParameterBridgeRelayModel<juce::WebComboBoxRelay,
                                                                  juce::WebComboBoxParameterAttachment>> (spec.parameterId);
        }

        jassertfalse;
        return {};
    }

    std::vector<std::unique_ptr<ParameterBridgeRelay>> relays;
};

class ParameterBridgeAttachmentStore
{
public:
    void bindToParameters (juce::AudioProcessorValueTreeState& apvts,
                           ParameterBridgeRelayStore& relayStore,
                           juce::UndoManager* undoManager = nullptr)
    {
        attachments = relayStore.createAttachments (apvts, undoManager);
    }

private:
    std::vector<std::unique_ptr<ParameterBridgeAttachment>> attachments;
};

} // namespace locusq::editor_webview
