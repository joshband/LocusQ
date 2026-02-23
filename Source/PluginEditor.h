#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

//==============================================================================
/**
 * LocusQ Plugin Editor - WebView UI with Three.js 3D Viewport
 *
 * CRITICAL: Member declaration order MUST be:
 * 1. Parameter relays (destroyed last)
 * 2. WebBrowserComponent (destroyed middle)
 * 3. Parameter attachments (destroyed first)
 *
 * This order prevents DAW crashes on plugin unload.
 */
class LocusQAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    LocusQAudioProcessorEditor (LocusQAudioProcessor&);
    ~LocusQAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    //==============================================================================
    // Timer for pushing scene state to WebView
    void timerCallback() override;
    void updateStandaloneWindowTitle();

    //==============================================================================
    // CRITICAL: MEMBER DECLARATION ORDER
    // DO NOT REORDER - This prevents DAW crash on unload
    //==============================================================================

    // 1. PARAMETER RELAYS (Destroyed last)
    // Global
    juce::WebComboBoxRelay modeRelay { "mode" };
    juce::WebToggleButtonRelay bypassRelay { "bypass" };

    // Calibrate
    juce::WebComboBoxRelay calSpkConfigRelay { "cal_spk_config" };
    juce::WebSliderRelay calMicChannelRelay { "cal_mic_channel" };
    juce::WebSliderRelay calSpk1OutRelay { "cal_spk1_out" };
    juce::WebSliderRelay calSpk2OutRelay { "cal_spk2_out" };
    juce::WebSliderRelay calSpk3OutRelay { "cal_spk3_out" };
    juce::WebSliderRelay calSpk4OutRelay { "cal_spk4_out" };
    juce::WebSliderRelay calTestLevelRelay { "cal_test_level" };
    juce::WebComboBoxRelay calTestTypeRelay { "cal_test_type" };

    // Emitter Position
    juce::WebSliderRelay azimuthRelay { "pos_azimuth" };
    juce::WebSliderRelay elevationRelay { "pos_elevation" };
    juce::WebSliderRelay distanceRelay { "pos_distance" };
    juce::WebSliderRelay posXRelay { "pos_x" };
    juce::WebSliderRelay posYRelay { "pos_y" };
    juce::WebSliderRelay posZRelay { "pos_z" };
    juce::WebComboBoxRelay coordModeRelay { "pos_coord_mode" };

    // Emitter Size
    juce::WebToggleButtonRelay sizeLinkRelay { "size_link" };
    juce::WebSliderRelay sizeUniformRelay { "size_uniform" };

    // Emitter Audio
    juce::WebSliderRelay emitGainRelay { "emit_gain" };
    juce::WebSliderRelay spreadRelay { "emit_spread" };
    juce::WebSliderRelay directivityRelay { "emit_directivity" };
    juce::WebToggleButtonRelay muteRelay { "emit_mute" };
    juce::WebToggleButtonRelay soloRelay { "emit_solo" };
    juce::WebSliderRelay emitColorRelay { "emit_color" };
    juce::WebSliderRelay dirAzimuthRelay  { "emit_dir_azimuth" };
    juce::WebSliderRelay dirElevationRelay { "emit_dir_elevation" };

    // Emitter Physics
    juce::WebToggleButtonRelay physEnableRelay { "phys_enable" };
    juce::WebSliderRelay physMassRelay { "phys_mass" };
    juce::WebSliderRelay physDragRelay { "phys_drag" };
    juce::WebSliderRelay physElasticityRelay { "phys_elasticity" };
    juce::WebSliderRelay physGravityRelay { "phys_gravity" };
    juce::WebComboBoxRelay physGravityDirRelay { "phys_gravity_dir" };
    juce::WebSliderRelay physFrictionRelay { "phys_friction" };
    juce::WebToggleButtonRelay physThrowRelay { "phys_throw" };
    juce::WebToggleButtonRelay physResetRelay { "phys_reset" };
    juce::WebSliderRelay physVelXRelay { "phys_vel_x" };
    juce::WebSliderRelay physVelYRelay { "phys_vel_y" };
    juce::WebSliderRelay physVelZRelay { "phys_vel_z" };

    // Emitter Animation
    juce::WebToggleButtonRelay animEnableRelay { "anim_enable" };
    juce::WebComboBoxRelay animModeRelay { "anim_mode" };
    juce::WebToggleButtonRelay animLoopRelay { "anim_loop" };
    juce::WebSliderRelay animSpeedRelay { "anim_speed" };
    juce::WebToggleButtonRelay animSyncRelay { "anim_sync" };

    // Renderer
    juce::WebSliderRelay masterGainRelay { "rend_master_gain" };
    juce::WebSliderRelay spk1GainRelay { "rend_spk1_gain" };
    juce::WebSliderRelay spk2GainRelay { "rend_spk2_gain" };
    juce::WebSliderRelay spk3GainRelay { "rend_spk3_gain" };
    juce::WebSliderRelay spk4GainRelay { "rend_spk4_gain" };
    juce::WebSliderRelay spk1DelayRelay { "rend_spk1_delay" };
    juce::WebSliderRelay spk2DelayRelay { "rend_spk2_delay" };
    juce::WebSliderRelay spk3DelayRelay { "rend_spk3_delay" };
    juce::WebSliderRelay spk4DelayRelay { "rend_spk4_delay" };
    juce::WebComboBoxRelay qualityRelay { "rend_quality" };
    juce::WebComboBoxRelay distanceModelRelay { "rend_distance_model" };
    juce::WebComboBoxRelay headphoneModeRelay { "rend_headphone_mode" };
    juce::WebComboBoxRelay headphoneProfileRelay { "rend_headphone_profile" };
    juce::WebSliderRelay distanceRefRelay { "rend_distance_ref" };
    juce::WebSliderRelay distanceMaxRelay { "rend_distance_max" };
    juce::WebToggleButtonRelay dopplerRelay { "rend_doppler" };
    juce::WebSliderRelay dopplerScaleRelay { "rend_doppler_scale" };
    juce::WebToggleButtonRelay airAbsorbRelay { "rend_air_absorb" };
    juce::WebToggleButtonRelay roomEnableRelay { "rend_room_enable" };
    juce::WebSliderRelay roomMixRelay { "rend_room_mix" };
    juce::WebSliderRelay roomSizeRelay { "rend_room_size" };
    juce::WebSliderRelay roomDampingRelay { "rend_room_damping" };
    juce::WebToggleButtonRelay roomErOnlyRelay { "rend_room_er_only" };
    juce::WebComboBoxRelay physRateRelay { "rend_phys_rate" };
    juce::WebToggleButtonRelay physWallsRelay { "rend_phys_walls" };
    juce::WebToggleButtonRelay physInteractRelay { "rend_phys_interact" };
    juce::WebToggleButtonRelay physPauseRelay { "rend_phys_pause" };
    juce::WebComboBoxRelay vizModeRelay { "rend_viz_mode" };
    juce::WebToggleButtonRelay vizTrailsRelay { "rend_viz_trails" };
    juce::WebSliderRelay vizTrailLenRelay { "rend_viz_trail_len" };
    juce::WebToggleButtonRelay vizVectorsRelay { "rend_viz_vectors" };
    juce::WebToggleButtonRelay vizPhysicsLensRelay { "rend_viz_physics_lens" };
    juce::WebSliderRelay vizDiagMixRelay { "rend_viz_diag_mix" };
    juce::WebToggleButtonRelay vizGridRelay { "rend_viz_grid" };
    juce::WebToggleButtonRelay vizLabelsRelay { "rend_viz_labels" };

    // 2. WEBBROWSERCOMPONENT (Destroyed middle)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. PARAMETER ATTACHMENTS (Destroyed first)
    std::unique_ptr<juce::WebComboBoxParameterAttachment> modeAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> bypassAttachment;

    std::unique_ptr<juce::WebComboBoxParameterAttachment> calSpkConfigAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> calMicChannelAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> calSpk1OutAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> calSpk2OutAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> calSpk3OutAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> calSpk4OutAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> calTestLevelAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> calTestTypeAttachment;

    std::unique_ptr<juce::WebSliderParameterAttachment> azimuthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> elevationAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> distanceAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> posXAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> posYAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> posZAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> coordModeAttachment;

    std::unique_ptr<juce::WebToggleButtonParameterAttachment> sizeLinkAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> sizeUniformAttachment;

    std::unique_ptr<juce::WebSliderParameterAttachment> emitGainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> spreadAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> directivityAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> muteAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> soloAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> emitColorAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> dirAzimuthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> dirElevationAttachment;

    std::unique_ptr<juce::WebToggleButtonParameterAttachment> physEnableAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> physMassAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> physDragAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> physElasticityAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> physGravityAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> physGravityDirAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> physFrictionAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> physThrowAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> physResetAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> physVelXAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> physVelYAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> physVelZAttachment;

    std::unique_ptr<juce::WebToggleButtonParameterAttachment> animEnableAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> animModeAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> animLoopAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> animSpeedAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> animSyncAttachment;

    std::unique_ptr<juce::WebSliderParameterAttachment> masterGainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> spk1GainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> spk2GainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> spk3GainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> spk4GainAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> spk1DelayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> spk2DelayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> spk3DelayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> spk4DelayAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> qualityAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> distanceModelAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> headphoneModeAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> headphoneProfileAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> distanceRefAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> distanceMaxAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> dopplerAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> dopplerScaleAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> airAbsorbAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> roomEnableAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> roomMixAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> roomSizeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> roomDampingAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> roomErOnlyAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> physRateAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> physWallsAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> physInteractAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> physPauseAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> vizModeAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> vizTrailsAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> vizTrailLenAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> vizVectorsAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> vizPhysicsLensAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> vizDiagMixAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> vizGridAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> vizLabelsAttachment;

    bool runtimeProbeDone = false;
    int runtimeProbeTicks = 0;
    bool standaloneWindowTitleUpdated = false;
    bool uiSelfTestProbeInFlight = false;
    bool uiSelfTestResultWritten = false;
    int uiSelfTestPollTicks = 0;

    //==============================================================================
    // Resource provider for embedded web files
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    // Helper functions
    static const char* getMimeForExtension (const juce::String& extension);

    // Reference to processor
    LocusQAudioProcessor& audioProcessor;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LocusQAudioProcessorEditor)
};
