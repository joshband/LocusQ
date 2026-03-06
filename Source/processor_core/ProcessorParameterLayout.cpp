#include "../PluginProcessor.h"

//==============================================================================
// PARAMETER LAYOUT - All plugin parameters
//
// Extracted from PluginProcessor.cpp (W0-A / BL-032 extension).
// Single responsibility: APVTS parameter tree construction.
//==============================================================================

juce::AudioProcessorValueTreeState::ParameterLayout LocusQAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // ==================== GLOBAL ====================
    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "mode", 1 }, "Mode",
        juce::StringArray { "Calibrate", "Emitter", "Renderer" }, 1));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "bypass", 1 }, "Bypass", false));

    // ==================== CALIBRATE ====================
    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "cal_spk_config", 1 }, "Speaker Config",
        juce::StringArray { "4x Mono", "2x Stereo" }, 0));

    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
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

    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "cal_monitoring_path", 1 }, "Monitoring Path",
        juce::StringArray {
            "Speakers",
            "Stereo Downmix",
            "Steam Binaural",
            "Virtual Binaural"
        }, 0));

    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "cal_device_profile", 1 }, "Device Profile",
        juce::StringArray {
            "Generic",
            "AirPods Pro 2",
            "AirPods Pro 3",
            "Sony WH-1000XM5",
            "Custom SOFA"
        }, 0));

    params.insert (params.end(), std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "cal_mic_channel", 1 }, "Mic Channel", 1, 8, 1));

    params.insert (params.end(), std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "cal_spk1_out", 1 }, "SPK1 Output", 1, 8, 1));
    params.insert (params.end(), std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "cal_spk2_out", 1 }, "SPK2 Output", 1, 8, 2));
    params.insert (params.end(), std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "cal_spk3_out", 1 }, "SPK3 Output", 1, 8, 3));
    params.insert (params.end(), std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "cal_spk4_out", 1 }, "SPK4 Output", 1, 8, 4));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "cal_test_level", 1 }, "Test Level",
        juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -20.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "cal_test_type", 1 }, "Test Type",
        juce::StringArray { "Sweep", "Pink", "White", "Impulse" }, 0));

    // ==================== EMITTER: POSITION ====================
    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pos_azimuth", 1 }, "Azimuth",
        juce::NormalisableRange<float> (-180.0f, 180.0f, 0.1f), 0.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pos_elevation", 1 }, "Elevation",
        juce::NormalisableRange<float> (-90.0f, 90.0f, 0.1f), 0.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pos_distance", 1 }, "Distance",
        juce::NormalisableRange<float> (0.0f, 50.0f, 0.01f, 0.5f), 2.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pos_x", 1 }, "Position X",
        juce::NormalisableRange<float> (-25.0f, 25.0f, 0.01f), 0.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pos_y", 1 }, "Position Y",
        juce::NormalisableRange<float> (-25.0f, 25.0f, 0.01f), 0.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pos_z", 1 }, "Position Z",
        juce::NormalisableRange<float> (-10.0f, 10.0f, 0.01f), 0.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "pos_coord_mode", 1 }, "Coord Mode",
        juce::StringArray { "Spherical", "Cartesian" }, 0));

    // ==================== EMITTER: SIZE ====================
    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "size_width", 1 }, "Width",
        juce::NormalisableRange<float> (0.01f, 20.0f, 0.01f, 0.5f), 0.5f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "size_depth", 1 }, "Depth",
        juce::NormalisableRange<float> (0.01f, 20.0f, 0.01f, 0.5f), 0.5f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "size_height", 1 }, "Height",
        juce::NormalisableRange<float> (0.01f, 10.0f, 0.01f, 0.5f), 0.5f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "size_link", 1 }, "Link Size", true));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "size_uniform", 1 }, "Uniform Scale",
        juce::NormalisableRange<float> (0.01f, 20.0f, 0.01f, 0.5f), 0.5f));

    // ==================== EMITTER: AUDIO ====================
    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "emit_gain", 1 }, "Emitter Gain",
        juce::NormalisableRange<float> (-60.0f, 12.0f, 0.1f), 0.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "emit_mute", 1 }, "Mute", false));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "emit_solo", 1 }, "Solo", false));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "emit_spread", 1 }, "Spread",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "emit_directivity", 1 }, "Directivity",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "emit_dir_azimuth", 1 }, "Dir Aim Azimuth",
        juce::NormalisableRange<float> (-180.0f, 180.0f, 0.1f), 0.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "emit_dir_elevation", 1 }, "Dir Aim Elevation",
        juce::NormalisableRange<float> (-90.0f, 90.0f, 0.1f), 0.0f));

    // ==================== EMITTER: PHYSICS ====================
    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "phys_enable", 1 }, "Physics Enable", false));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_mass", 1 }, "Mass",
        juce::NormalisableRange<float> (0.01f, 100.0f, 0.01f, 0.4f), 1.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_drag", 1 }, "Drag",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 0.5f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_elasticity", 1 }, "Elasticity",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_gravity", 1 }, "Gravity",
        juce::NormalisableRange<float> (-20.0f, 20.0f, 0.1f), 0.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "phys_gravity_dir", 1 }, "Gravity Direction",
        juce::StringArray { "Down", "Up", "To Center", "From Center", "Custom" }, 0));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_friction", 1 }, "Friction",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_vel_x", 1 }, "Init Vel X",
        juce::NormalisableRange<float> (-50.0f, 50.0f, 0.1f), 0.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_vel_y", 1 }, "Init Vel Y",
        juce::NormalisableRange<float> (-50.0f, 50.0f, 0.1f), 0.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "phys_vel_z", 1 }, "Init Vel Z",
        juce::NormalisableRange<float> (-50.0f, 50.0f, 0.1f), 0.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "phys_throw", 1 }, "Throw", false));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "phys_reset", 1 }, "Reset Position", false));

    // ==================== EMITTER: ANIMATION ====================
    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "anim_enable", 1 }, "Animation Enable", false));

    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "anim_mode", 1 }, "Animation Source",
        juce::StringArray { "DAW", "Internal" }, 0));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "anim_loop", 1 }, "Loop", false));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "anim_speed", 1 }, "Animation Speed",
        juce::NormalisableRange<float> (0.1f, 10.0f, 0.1f), 1.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "anim_sync", 1 }, "Transport Sync", true));

    // ==================== EMITTER: IDENTITY ====================
    params.insert (params.end(), std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "emit_color", 1 }, "Color", 0, 15, 0));

    // ==================== RENDERER: MASTER ====================
    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_master_gain", 1 }, "Master Gain",
        juce::NormalisableRange<float> (-60.0f, 12.0f, 0.1f), 0.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk1_gain", 1 }, "SPK1 Trim",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));
    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk2_gain", 1 }, "SPK2 Trim",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));
    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk3_gain", 1 }, "SPK3 Trim",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));
    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk4_gain", 1 }, "SPK4 Trim",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk1_delay", 1 }, "SPK1 Delay",
        juce::NormalisableRange<float> (0.0f, 50.0f, 0.01f), 0.0f));
    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk2_delay", 1 }, "SPK2 Delay",
        juce::NormalisableRange<float> (0.0f, 50.0f, 0.01f), 0.0f));
    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk3_delay", 1 }, "SPK3 Delay",
        juce::NormalisableRange<float> (0.0f, 50.0f, 0.01f), 0.0f));
    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_spk4_delay", 1 }, "SPK4 Delay",
        juce::NormalisableRange<float> (0.0f, 50.0f, 0.01f), 0.0f));

    // ==================== RENDERER: SPATIALIZATION ====================
    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_quality", 1 }, "Quality",
        juce::StringArray { "Draft", "Final" }, 0));

    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_distance_model", 1 }, "Distance Model",
        juce::StringArray { "Inverse Square", "Linear", "Logarithmic", "Custom" }, 0));

    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_headphone_mode", 1 }, "Headphone Mode",
        juce::StringArray { "Stereo Downmix", "Steam Binaural" }, 0));

    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_headphone_profile", 1 }, "Headphone Profile",
        juce::StringArray { "Generic", "AirPods Pro 2", "AirPods Pro 3", "Sony WH-1000XM5", "Custom SOFA" }, 0));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_audition_enable", 1 }, "Audition Enable", false));

    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
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

    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_audition_motion", 1 }, "Audition Motion",
        juce::StringArray {
            "Center",
            "Orbit Slow",
            "Orbit Fast",
            "Figure8 Flow",
            "Helix Rise",
            "Wall Ricochet"
        }, 1));

    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_audition_level", 1 }, "Audition Level",
        juce::StringArray { "-36 dBFS", "-30 dBFS", "-24 dBFS", "-18 dBFS", "-12 dBFS" }, 2));

    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
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

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_distance_ref", 1 }, "Ref Distance",
        juce::NormalisableRange<float> (0.1f, 10.0f, 0.01f), 1.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_distance_max", 1 }, "Max Distance",
        juce::NormalisableRange<float> (1.0f, 100.0f, 0.1f), 50.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_doppler", 1 }, "Doppler", false));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_doppler_scale", 1 }, "Doppler Scale",
        juce::NormalisableRange<float> (0.0f, 5.0f, 0.01f), 1.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_air_absorb", 1 }, "Air Absorption", true));

    // ==================== RENDERER: ROOM ====================
    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_room_enable", 1 }, "Room Enable", true));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_room_mix", 1 }, "Room Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_room_size", 1 }, "Room Size",
        juce::NormalisableRange<float> (0.5f, 5.0f, 0.01f), 1.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_room_damping", 1 }, "Room Damping",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_room_er_only", 1 }, "ER Only", false));

    // ==================== RENDERER: PHYSICS GLOBAL ====================
    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_phys_rate", 1 }, "Physics Rate",
        juce::StringArray { "30 Hz", "60 Hz", "120 Hz", "240 Hz" }, 1));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_phys_walls", 1 }, "Wall Collision", true));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_phys_interact", 1 }, "Object Interaction", false));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_phys_pause", 1 }, "Pause Physics", false));

    // ==================== RENDERER: VISUALIZATION ====================
    params.insert (params.end(), std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "rend_viz_mode", 1 }, "View Mode",
        juce::StringArray { "Perspective", "Top Down", "Front", "Side" }, 0));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_viz_trails", 1 }, "Show Trails", true));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_viz_trail_len", 1 }, "Trail Length",
        juce::NormalisableRange<float> (0.5f, 30.0f, 0.1f), 5.0f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_viz_vectors", 1 }, "Show Vectors", false));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_viz_physics_lens", 1 }, "Physics Lens", false));

    params.insert (params.end(), std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rend_viz_diag_mix", 1 }, "Diagnostic Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.55f));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_viz_grid", 1 }, "Show Grid", true));

    params.insert (params.end(), std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rend_viz_labels", 1 }, "Show Labels", true));

    return { params.begin(), params.end() };
}
