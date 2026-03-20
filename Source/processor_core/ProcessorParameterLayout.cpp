#include "../PluginProcessor.h"

//==============================================================================
// PARAMETER LAYOUT - All plugin parameters
//
// Extracted from PluginProcessor.cpp (W0-A / BL-032 extension).
// Single responsibility: APVTS parameter tree construction.
// W1-D adds stable host-visible parameter grouping without changing IDs/defaults.
//==============================================================================

juce::AudioProcessorValueTreeState::ParameterLayout LocusQAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto makeGroup = [] (const char* groupId, const char* groupName)
    {
        return std::make_unique<juce::AudioProcessorParameterGroup> (groupId, groupName, " / ");
    };

    auto addParameter = [] (auto& group, std::unique_ptr<juce::RangedAudioParameter> parameter)
    {
        group->addChild (std::move (parameter));
    };

    auto globalGroup = makeGroup ("global", "Global");
    auto calibrationGroup = makeGroup ("calibration", "Calibration");

    auto emitterGroup = makeGroup ("emitter", "Emitter");
    auto emitterPositionGroup = makeGroup ("emitter_position", "Position");
    auto emitterSizeGroup = makeGroup ("emitter_size", "Size");
    auto emitterAudioGroup = makeGroup ("emitter_audio", "Audio");
    auto emitterPhysicsGroup = makeGroup ("emitter_physics", "Physics");
    auto emitterAnimationGroup = makeGroup ("emitter_animation", "Animation");
    auto emitterChoreographyGroup = makeGroup ("emitter_choreography", "Choreography");
    auto emitterIdentityGroup = makeGroup ("emitter_identity", "Identity");

    auto rendererGroup = makeGroup ("renderer", "Renderer");
    auto rendererMasterGroup = makeGroup ("renderer_master", "Master");
    auto rendererSpatializationGroup = makeGroup ("renderer_spatialization", "Spatialization");
    auto rendererRoomGroup = makeGroup ("renderer_room", "Room");
    auto rendererPhysicsGroup = makeGroup ("renderer_physics", "Physics");
    auto rendererVisualizationGroup = makeGroup ("renderer_visualization", "Visualization");

    // ==================== GLOBAL ====================
    addParameter (globalGroup, std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "mode", 1 }, "Mode",
        juce::StringArray { "Calibrate", "Emitter", "Renderer" }, 1));

    addParameter (globalGroup, std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "bypass", 1 }, "Bypass", false));

    // ==================== CALIBRATION ====================
    addParameter (calibrationGroup, std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "cal_spk_config", 1 }, "Speaker Config",
        juce::StringArray { "4x Mono", "2x Stereo" }, 0));

    addParameter (calibrationGroup, std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "cal_topology_profile", 1 }, "Topology Profile",
        juce::StringArray {
            "Mono",
            "Stereo",
            "Quad",
            "5.1",
            "7.1",
            "7.1.2",
            "7.4.2 / Atmos-style",
            "Binaural / Headphone",
            "Ambisonic 1st Order",
            "Ambisonic 3rd Order",
            "Multichannel -> Stereo Downmix"
        }, 1));

    addParameter (calibrationGroup, std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "cal_monitoring_path", 1 }, "Monitoring Path",
        juce::StringArray {
            "Speakers",
            "Stereo Downmix",
            "Steam Binaural",
            "Virtual Binaural"
        }, 0));

    addParameter (calibrationGroup, std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "cal_device_profile", 1 }, "Device Profile",
        juce::StringArray {
            "Generic",
            "AirPods Pro 2",
            "AirPods Pro 3",
            "Sony WH-1000XM5",
            "Custom SOFA"
        }, 0));

    addParameter (calibrationGroup, std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "cal_mic_channel", 1 }, "Mic Channel", 1, 8, 1));

    addParameter (calibrationGroup, std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "cal_spk1_out", 1 }, "SPK1 Output", 1, 8, 1));
    addParameter (calibrationGroup, std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "cal_spk2_out", 1 }, "SPK2 Output", 1, 8, 2));
    addParameter (calibrationGroup, std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "cal_spk3_out", 1 }, "SPK3 Output", 1, 8, 3));
    addParameter (calibrationGroup, std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "cal_spk4_out", 1 }, "SPK4 Output", 1, 8, 4));

    addParameter (calibrationGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "cal_test_level", 1 }, "Test Level",
        juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -20.0f));

    addParameter (calibrationGroup, std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "cal_test_type", 1 }, "Test Type",
        juce::StringArray { "Sweep", "Pink", "White", "Impulse" }, 0));

    // ==================== EMITTER: POSITION ====================
    addParameter (emitterPositionGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pos_azimuth", 1 }, "Azimuth",
        juce::NormalisableRange<float> (-180.0f, 180.0f, 0.1f), 0.0f));

    addParameter (emitterPositionGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pos_elevation", 1 }, "Elevation",
        juce::NormalisableRange<float> (-90.0f, 90.0f, 0.1f), 0.0f));

    addParameter (emitterPositionGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pos_distance", 1 }, "Distance",
        juce::NormalisableRange<float> (0.0f, 50.0f, 0.01f, 0.5f), 2.0f));

    addParameter (emitterPositionGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pos_x", 1 }, "Position X",
        juce::NormalisableRange<float> (-25.0f, 25.0f, 0.01f), 0.0f));

    addParameter (emitterPositionGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pos_y", 1 }, "Position Y",
        juce::NormalisableRange<float> (-25.0f, 25.0f, 0.01f), 0.0f));

    addParameter (emitterPositionGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pos_z", 1 }, "Position Z",
        juce::NormalisableRange<float> (-10.0f, 10.0f, 0.01f), 0.0f));

    addParameter (emitterPositionGroup, std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "pos_coord_mode", 1 }, "Coord Mode",
        juce::StringArray { "Spherical", "Cartesian" }, 0));

    // ==================== EMITTER: SIZE ====================
    addParameter (emitterSizeGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "size_width", 1 }, "Width",
        juce::NormalisableRange<float> (0.01f, 20.0f, 0.01f, 0.5f), 0.5f));

    addParameter (emitterSizeGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "size_depth", 1 }, "Depth",
        juce::NormalisableRange<float> (0.01f, 20.0f, 0.01f, 0.5f), 0.5f));

    addParameter (emitterSizeGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "size_height", 1 }, "Height",
        juce::NormalisableRange<float> (0.01f, 10.0f, 0.01f, 0.5f), 0.5f));

    addParameter (emitterSizeGroup, std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "size_link", 1 }, "Link Size", true));

    addParameter (emitterSizeGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "size_uniform", 1 }, "Uniform Scale",
        juce::NormalisableRange<float> (0.01f, 20.0f, 0.01f, 0.5f), 0.5f));

    // ==================== EMITTER: AUDIO ====================
    addParameter (emitterAudioGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "emit_gain", 1 }, "Emitter Gain",
        juce::NormalisableRange<float> (-60.0f, 12.0f, 0.1f), 0.0f));

    addParameter (emitterAudioGroup, std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "emit_mute", 1 }, "Mute", false));

    addParameter (emitterAudioGroup, std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "emit_solo", 1 }, "Solo", false));

    addParameter (emitterAudioGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "emit_spread", 1 }, "Spread",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

    addParameter (emitterAudioGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "emit_directivity", 1 }, "Directivity",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

    addParameter (emitterAudioGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "emit_dir_azimuth", 1 }, "Dir Aim Azimuth",
        juce::NormalisableRange<float> (-180.0f, 180.0f, 0.1f), 0.0f));

    addParameter (emitterAudioGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "emit_dir_elevation", 1 }, "Dir Aim Elevation",
        juce::NormalisableRange<float> (-90.0f, 90.0f, 0.1f), 0.0f));

    // ==================== EMITTER: PHYSICS ====================
    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "phys_enable", 1 }, "Physics Enable", false));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_mass", 1 }, "Mass",
        juce::NormalisableRange<float> (0.01f, 100.0f, 0.01f, 0.4f), 1.0f));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_drag", 1 }, "Drag",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 0.5f));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_elasticity", 1 }, "Elasticity",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_gravity", 1 }, "Gravity",
        juce::NormalisableRange<float> (-20.0f, 20.0f, 0.1f), 0.0f));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "phys_gravity_dir", 1 }, "Gravity Direction",
        juce::StringArray { "Down", "Up", "To Center", "From Center", "Custom" }, 0));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_friction", 1 }, "Friction",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_vel_x", 1 }, "Init Vel X",
        juce::NormalisableRange<float> (-50.0f, 50.0f, 0.1f), 0.0f));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_vel_y", 1 }, "Init Vel Y",
        juce::NormalisableRange<float> (-50.0f, 50.0f, 0.1f), 0.0f));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_vel_z", 1 }, "Init Vel Z",
        juce::NormalisableRange<float> (-50.0f, 50.0f, 0.1f), 0.0f));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "phys_throw", 1 }, "Throw", false));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "phys_reset", 1 }, "Reset Position", false));

    // --- Spring oscillator (P3) ---
    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "phys_spring_enable", 1 }, "Spring Enable", false));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_spring_k", 1 }, "Spring Stiffness",
        juce::NormalisableRange<float> (0.5f, 500.0f, 0.1f, 0.35f), 10.0f));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_spring_damp", 1 }, "Spring Damping",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "phys_spring_anchor_mode", 1 }, "Spring Anchor",
        juce::StringArray { "Rest Pose", "Fixed Point" }, 0));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_spring_anchor_x", 1 }, "Spring Anchor X",
        juce::NormalisableRange<float> (-25.0f, 25.0f, 0.01f), 0.0f));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_spring_anchor_y", 1 }, "Spring Anchor Y",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 1.2f));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_spring_anchor_z", 1 }, "Spring Anchor Z",
        juce::NormalisableRange<float> (-25.0f, 25.0f, 0.01f), 0.0f));

    // --- Turbulence (P3) ---
    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_turbulence", 1 }, "Turbulence",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_turbulence_rate", 1 }, "Turbulence Rate",
        juce::NormalisableRange<float> (0.1f, 20.0f, 0.1f, 0.4f), 2.0f));

    // --- Angular physics (P4) ---
    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "phys_ang_enable", 1 }, "Angular Enable", false));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_ang_drag", 1 }, "Angular Drag",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_ang_impulse_x", 1 }, "Angular Impulse X",
        juce::NormalisableRange<float> (-20.0f, 20.0f, 0.01f), 0.0f));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_ang_impulse_y", 1 }, "Angular Impulse Y",
        juce::NormalisableRange<float> (-20.0f, 20.0f, 0.01f), 0.0f));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_ang_impulse_z", 1 }, "Angular Impulse Z",
        juce::NormalisableRange<float> (-20.0f, 20.0f, 0.01f), 0.0f));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "phys_ang_throw", 1 }, "Angular Throw", false));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "phys_ang_reset", 1 }, "Angular Reset", false));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_ang_attractor_torque", 1 }, "Attractor Torque",
        juce::NormalisableRange<float> (0.0f, 50.0f, 0.1f), 5.0f));

    // --- Boids group assignment (P5) ---
    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "phys_flock_group", 1 }, "Flock Group",
        juce::StringArray { "None", "Group 1", "Group 2", "Group 3", "Group 4" }, 0));

    // --- Collision radius and mass override (P6) ---
    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_collision_radius", 1 }, "Collision Radius",
        juce::NormalisableRange<float> (0.05f, 5.0f, 0.01f), 0.3f));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_mass_override", 1 }, "Mass Override",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 0.0f));

    // ===== Emitter Physics: DAW Output Mirrors + Freeze (per slot, 8 slots) =====
    // These are the first per-slot parameters in emitterPhysicsGroup.
    // Existing params above (phys_enable, phys_mass, etc.) are single-instance (shared).
    for (int n = 0; n < 8; ++n)
    {
        const juce::String ns (n);
        const juce::String label = "Emitter " + juce::String (n + 1);

        addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "phys_out_spread_mod_" + ns, 1 },
            label + " Physics Spread",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "phys_out_gain_mod_" + ns, 1 },
            label + " Physics Gain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "phys_out_transient_" + ns, 1 },
            label + " Physics Transient",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { "phys_frozen_" + ns, 1 },
            label + " Physics Freeze", false));
    }

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_dbg_active_slot", 1 },
        "Physics Debug Active Slot",
        juce::NormalisableRange<float> (-1.0f, 31.0f, 1.0f), -1.0f));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_dbg_active_emitters", 1 },
        "Physics Debug Active Emitters",
        juce::NormalisableRange<float> (0.0f, 32.0f, 1.0f), 0.0f));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_dbg_coordinated_worker", 1 },
        "Physics Debug Coordinated Worker",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_dbg_boids_density", 1 },
        "Physics Debug Boids Density",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_dbg_worker_slot_active", 1 },
        "Physics Debug Worker Slot Active",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_dbg_worker_boids_active", 1 },
        "Physics Debug Worker Boids Active",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_dbg_boids_group_size", 1 },
        "Physics Debug Boids Group Size",
        juce::NormalisableRange<float> (0.0f, 64.0f, 1.0f), 0.0f));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_dbg_worker_pos_x", 1 },
        "Physics Debug Worker Pos X",
        juce::NormalisableRange<float> (-25.0f, 25.0f, 0.01f), 0.0f));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_dbg_worker_pos_y", 1 },
        "Physics Debug Worker Pos Y",
        juce::NormalisableRange<float> (-25.0f, 25.0f, 0.01f), 0.0f));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_dbg_worker_pos_z", 1 },
        "Physics Debug Worker Pos Z",
        juce::NormalisableRange<float> (-25.0f, 25.0f, 0.01f), 0.0f));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_dbg_align_neighbors", 1 },
        "Physics Debug Align Neighbors",
        juce::NormalisableRange<float> (0.0f, 64.0f, 1.0f), 0.0f));

    addParameter (emitterPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_dbg_coh_neighbors", 1 },
        "Physics Debug Coh Neighbors",
        juce::NormalisableRange<float> (0.0f, 64.0f, 1.0f), 0.0f));

    // ==================== EMITTER: ANIMATION ====================
    addParameter (emitterAnimationGroup, std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "anim_enable", 1 }, "Animation Enable", false));

    addParameter (emitterAnimationGroup, std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "anim_mode", 1 }, "Animation Source",
        juce::StringArray { "DAW", "Internal" }, 0));

    addParameter (emitterAnimationGroup, std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "anim_loop", 1 }, "Loop", false));

    addParameter (emitterAnimationGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "anim_speed", 1 }, "Animation Speed",
        juce::NormalisableRange<float> (0.1f, 10.0f, 0.1f), 1.0f));

    addParameter (emitterAnimationGroup, std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "anim_sync", 1 }, "Transport Sync", true));

    // ==================== EMITTER: CHOREOGRAPHY (CL-P1) ====================
    addParameter (emitterChoreographyGroup, std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "choro_enable", 1 }, "Choreography Enable", false));

    // ==================== EMITTER: IDENTITY ====================
    addParameter (emitterIdentityGroup, std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "emit_color", 1 }, "Color", 0, 15, 0));

    // ==================== RENDERER: MASTER ====================
    addParameter (rendererMasterGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_master_gain", 1 }, "Master Gain",
        juce::NormalisableRange<float> (-60.0f, 12.0f, 0.1f), 0.0f));

    addParameter (rendererMasterGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk1_gain", 1 }, "SPK1 Trim",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));
    addParameter (rendererMasterGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk2_gain", 1 }, "SPK2 Trim",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));
    addParameter (rendererMasterGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk3_gain", 1 }, "SPK3 Trim",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));
    addParameter (rendererMasterGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk4_gain", 1 }, "SPK4 Trim",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));

    addParameter (rendererMasterGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk1_delay", 1 }, "SPK1 Delay",
        juce::NormalisableRange<float> (0.0f, 50.0f, 0.01f), 0.0f));
    addParameter (rendererMasterGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk2_delay", 1 }, "SPK2 Delay",
        juce::NormalisableRange<float> (0.0f, 50.0f, 0.01f), 0.0f));
    addParameter (rendererMasterGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk3_delay", 1 }, "SPK3 Delay",
        juce::NormalisableRange<float> (0.0f, 50.0f, 0.01f), 0.0f));
    addParameter (rendererMasterGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk4_delay", 1 }, "SPK4 Delay",
        juce::NormalisableRange<float> (0.0f, 50.0f, 0.01f), 0.0f));

    // ==================== RENDERER: SPATIALIZATION ====================
    addParameter (rendererSpatializationGroup, std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_quality", 1 }, "Quality",
        juce::StringArray { "Draft", "Final" }, 0));

    addParameter (rendererSpatializationGroup, std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_distance_model", 1 }, "Distance Model",
        juce::StringArray { "Inverse Square", "Linear", "Logarithmic", "Custom" }, 0));

    addParameter (rendererSpatializationGroup, std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_headphone_mode", 1 }, "Headphone Mode",
        juce::StringArray { "Stereo Downmix", "Steam Binaural" }, 0));

    addParameter (rendererSpatializationGroup, std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_headphone_profile", 1 }, "Headphone Profile",
        juce::StringArray { "Generic", "AirPods Pro 2", "AirPods Pro 3", "Sony WH-1000XM5", "Custom SOFA" }, 0));

    addParameter (rendererSpatializationGroup, std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_audition_enable", 1 }, "Audition Enable", false));

    addParameter (rendererSpatializationGroup, std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_audition_signal", 1 }, "Audition Signal",
        juce::StringArray {
            "Sine 440",
            "Dual Tone",
            "Pink Noise",
            "Rain",
            "Snow",
            "Bouncing Balls",
            "Wind Chimes",
            "Crickets",
            "Song Birds",
            "Karplus Plucks",
            "Membrane Drops",
            "Krell Patch",
            "Generative Arp"
        }, 0));

    addParameter (rendererSpatializationGroup, std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_audition_motion", 1 }, "Audition Motion",
        juce::StringArray {
            "Center",
            "Orbit Slow",
            "Orbit Fast",
            "Figure8 Flow",
            "Helix Rise",
            "Wall Ricochet"
        }, 1));

    addParameter (rendererSpatializationGroup, std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_audition_level", 1 }, "Audition Level",
        juce::StringArray { "-36 dBFS", "-30 dBFS", "-24 dBFS", "-18 dBFS", "-12 dBFS" }, 2));

    addParameter (rendererSpatializationGroup, std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_spatial_profile", 1 }, "Spatial Profile",
        juce::StringArray {
            "Auto",
            "Stereo 2.0",
            "Quad 4.0",
            "Surround 5.2.1",
            "Surround 7.2.1",
            "Surround 7.4.2",
            "Ambisonic FOA",
            "Ambisonic HOA",
            "Atmos Bed",
            "Virtual 3D Stereo",
            "Codec IAMF",
            "Codec ADM"
        }, 0));

    addParameter (rendererSpatializationGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_distance_ref", 1 }, "Ref Distance",
        juce::NormalisableRange<float> (0.1f, 10.0f, 0.01f), 1.0f));

    addParameter (rendererSpatializationGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_distance_max", 1 }, "Max Distance",
        juce::NormalisableRange<float> (1.0f, 100.0f, 0.1f), 50.0f));

    addParameter (rendererSpatializationGroup, std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_doppler", 1 }, "Doppler", false));

    addParameter (rendererSpatializationGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_doppler_scale", 1 }, "Doppler Scale",
        juce::NormalisableRange<float> (0.0f, 5.0f, 0.01f), 1.0f));

    addParameter (rendererSpatializationGroup, std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_air_absorb", 1 }, "Air Absorption", true));

    // ==================== RENDERER: ROOM ====================
    addParameter (rendererRoomGroup, std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_room_enable", 1 }, "Room Enable", true));

    addParameter (rendererRoomGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_room_mix", 1 }, "Room Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

    addParameter (rendererRoomGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_room_size", 1 }, "Room Size",
        juce::NormalisableRange<float> (0.5f, 5.0f, 0.01f), 1.0f));

    addParameter (rendererRoomGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_room_damping", 1 }, "Room Damping",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

    addParameter (rendererRoomGroup, std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_room_er_only", 1 }, "ER Only", false));

    // ==================== RENDERER: PHYSICS GLOBAL ====================
    addParameter (rendererPhysicsGroup, std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_phys_rate", 1 }, "Physics Rate",
        juce::StringArray { "30 Hz", "60 Hz", "120 Hz", "240 Hz" }, 1));

    addParameter (rendererPhysicsGroup, std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_phys_walls", 1 }, "Wall Collision", true));

    addParameter (rendererPhysicsGroup, std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_phys_interact", 1 }, "Object Interaction", false));

    addParameter (rendererPhysicsGroup, std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_phys_pause", 1 }, "Pause Physics", false));

    // --- Boundary mode (P2) ---
    addParameter (rendererPhysicsGroup, std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "phys_boundary_mode", 1 }, "Boundary Mode",
        juce::StringArray { "Hard", "Soft", "Passthrough" }, 0));

    addParameter (rendererPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_soft_boundary_depth", 1 }, "Soft Boundary Depth",
        juce::NormalisableRange<float> (0.1f, 5.0f, 0.01f), 0.5f));

    // --- Inter-emitter collision globals (P6) ---
    addParameter (rendererPhysicsGroup, std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "phys_collide_emitters", 1 }, "Collide Emitters", false));

    addParameter (rendererPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_collision_gain_scale", 1 }, "Collision Gain Scale",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 1.0f));

    addParameter (rendererPhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_collision_decay_ms", 1 }, "Collision Decay ms",
        juce::NormalisableRange<float> (1.0f, 500.0f, 0.5f), 50.0f));

    // ==================== RENDERER: SCENE PHYSICS (Attractors — P2) ====================
    auto scenePhysicsGroup = makeGroup ("scene_physics", "Scene Physics");

    // 4 attractor/repulsor slots
    for (int n = 0; n < 4; ++n)
    {
        const juce::String ns (n);
        const juce::String label = "Attractor " + juce::String (n + 1);

        addParameter (scenePhysicsGroup, std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { "attractor_" + ns + "_active", 1 },
            label + " Active", false));

        addParameter (scenePhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "attractor_" + ns + "_pos_x", 1 },
            label + " X",
            juce::NormalisableRange<float> (-25.0f, 25.0f, 0.01f), 0.0f));

        addParameter (scenePhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "attractor_" + ns + "_pos_y", 1 },
            label + " Y",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 1.2f));

        addParameter (scenePhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "attractor_" + ns + "_pos_z", 1 },
            label + " Z",
            juce::NormalisableRange<float> (-25.0f, 25.0f, 0.01f), 0.0f));

        addParameter (scenePhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "attractor_" + ns + "_strength", 1 },
            label + " Strength",
            juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 10.0f));

        addParameter (scenePhysicsGroup, std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "attractor_" + ns + "_falloff", 1 },
            label + " Falloff",
            juce::StringArray { "1/r", "1/r\u00B2", "Constant" }, 1));  // default 1/r²

        addParameter (scenePhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "attractor_" + ns + "_radius", 1 },
            label + " Radius",
            juce::NormalisableRange<float> (0.1f, 20.0f, 0.01f, 0.5f), 5.0f));

        addParameter (scenePhysicsGroup, std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { "attractor_" + ns + "_orbit_stabilize", 1 },
            label + " Orbit Stabilize", false));
    }

    // ==================== SCENE PHYSICS: BOIDS GROUPS (P5) ====================
    for (int n = 0; n < 4; ++n)
    {
        const juce::String ns (n);
        const juce::String label = "Flock " + juce::String (n + 1);

        addParameter (scenePhysicsGroup, std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { "phys_flock_" + ns + "_enable", 1 },
            label + " Enable", false));

        addParameter (scenePhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "phys_flock_" + ns + "_sep_weight", 1 },
            label + " Sep Weight",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f));

        addParameter (scenePhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "phys_flock_" + ns + "_align_weight", 1 },
            label + " Align Weight",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        addParameter (scenePhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "phys_flock_" + ns + "_coh_weight", 1 },
            label + " Coh Weight",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        addParameter (scenePhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "phys_flock_" + ns + "_sep_radius", 1 },
            label + " Sep Radius",
            juce::NormalisableRange<float> (0.1f, 20.0f, 0.01f, 0.5f), 1.5f));

        addParameter (scenePhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "phys_flock_" + ns + "_align_radius", 1 },
            label + " Align Radius",
            juce::NormalisableRange<float> (0.1f, 20.0f, 0.01f, 0.5f), 3.0f));

        addParameter (scenePhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "phys_flock_" + ns + "_coh_radius", 1 },
            label + " Coh Radius",
            juce::NormalisableRange<float> (0.1f, 50.0f, 0.01f, 0.5f), 5.0f));

        addParameter (scenePhysicsGroup, std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "phys_flock_" + ns + "_max_speed", 1 },
            label + " Max Speed",
            juce::NormalisableRange<float> (0.1f, 50.0f, 0.1f, 0.5f), 5.0f));
    }

    // ==================== RENDERER: VISUALIZATION ====================
    addParameter (rendererVisualizationGroup, std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_viz_mode", 1 }, "View Mode",
        juce::StringArray { "Perspective", "Top Down", "Front", "Side" }, 0));

    addParameter (rendererVisualizationGroup, std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_viz_trails", 1 }, "Show Trails", true));

    addParameter (rendererVisualizationGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_viz_trail_len", 1 }, "Trail Length",
        juce::NormalisableRange<float> (0.5f, 30.0f, 0.1f), 5.0f));

    addParameter (rendererVisualizationGroup, std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_viz_vectors", 1 }, "Show Vectors", false));

    addParameter (rendererVisualizationGroup, std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_viz_physics_lens", 1 }, "Physics Lens", false));

    addParameter (rendererVisualizationGroup, std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_viz_diag_mix", 1 }, "Diagnostic Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.55f));

    addParameter (rendererVisualizationGroup, std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_viz_grid", 1 }, "Show Grid", true));

    addParameter (rendererVisualizationGroup, std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_viz_labels", 1 }, "Show Labels", true));

    emitterGroup->addChild (std::move (emitterPositionGroup));
    emitterGroup->addChild (std::move (emitterSizeGroup));
    emitterGroup->addChild (std::move (emitterAudioGroup));
    emitterGroup->addChild (std::move (emitterPhysicsGroup));
    emitterGroup->addChild (std::move (emitterAnimationGroup));
    emitterGroup->addChild (std::move (emitterChoreographyGroup));
    emitterGroup->addChild (std::move (emitterIdentityGroup));

    rendererGroup->addChild (std::move (rendererMasterGroup));
    rendererGroup->addChild (std::move (rendererSpatializationGroup));
    rendererGroup->addChild (std::move (rendererRoomGroup));
    rendererGroup->addChild (std::move (rendererPhysicsGroup));
    rendererGroup->addChild (std::move (rendererVisualizationGroup));
    rendererGroup->addChild (std::move (scenePhysicsGroup));

    layout.add (std::move (globalGroup));
    layout.add (std::move (calibrationGroup));
    layout.add (std::move (emitterGroup));
    layout.add (std::move (rendererGroup));

    return layout;
}
