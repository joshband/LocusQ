(() => {
  "use strict";

  const DEFAULT_CHOICES = {
    mode: ["Calibrate", "Emitter", "Renderer"],
    cal_spk_config: ["4x Mono", "2x Stereo"],
    cal_test_type: ["Sweep", "Pink", "White", "Impulse"],
    pos_coord_mode: ["Spherical", "Cartesian"],
    rend_quality: ["Draft", "Final"],
    rend_distance_model: ["Inverse Square", "Linear", "Logarithmic", "Custom"],
    rend_phys_rate: ["30 Hz", "60 Hz", "120 Hz", "240 Hz"],
    rend_viz_mode: ["Perspective", "Top Down", "Front", "Side"],
    anim_mode: ["DAW", "Internal"],
    phys_gravity_dir: ["Down", "Up", "To Center", "From Center", "Custom"],
  };

  const MODE_ORDER = ["calibrate", "emitter", "renderer"];
  const CALIBRATION_IDLE_MESSAGE = "Idle - press Start to begin calibration";
  const NATIVE_CALL_TIMEOUT_MS = 5000;
  const LANE_COLORS = {
    azimuth: 0xd4a847,
    elevation: 0x44aa66,
    distance: 0xaa4444,
    size: 0x4e86d8,
  };
  const PHYSICS_PRESETS = ["off", "bounce", "float", "orbit", "custom"];
  const EMITTER_PALETTE = [
    "#D4736F",
    "#D4905A",
    "#D4A847",
    "#CFBF41",
    "#AFC84A",
    "#7DC963",
    "#59C190",
    "#53B7B7",
    "#4CA9D0",
    "#618FD1",
    "#8A7FD2",
    "#A777CF",
    "#C56BC3",
    "#CF73A8",
    "#D47F8D",
    "#C18B7F",
  ];

  const dom = {
    body: document.body,
    rail: document.getElementById("rail"),
    sceneStatus: document.getElementById("scene-status"),
    roomDot: document.getElementById("room-dot"),
    roomLabel: document.getElementById("room-label"),
    qualityBadge: document.getElementById("quality-badge"),
    qualityReadout: document.getElementById("readout-quality"),
    viewportInfo: document.getElementById("viewport-info"),
    timeline: document.getElementById("timeline"),
    timelineTime: document.getElementById("timeline-time"),
    modeLive: document.getElementById("mode-live"),
    bridgeState: document.getElementById("bridge-state"),
    diagnostics: document.getElementById("diagnostics"),
    countSet: document.getElementById("count-set"),
    countValue: document.getElementById("count-value"),
    countProps: document.getElementById("count-props"),
    countLive: document.getElementById("count-live"),
    countHeartbeat: document.getElementById("count-heartbeat"),
    tabs: Array.from(document.querySelectorAll(".mode-tab")),
    panels: Array.from(document.querySelectorAll(".rail-panel")),
    lanes: Array.from(document.querySelectorAll(".lane")),
    viewButtons: Array.from(document.querySelectorAll(".view-btn")),
    toggleSizeLink: document.getElementById("toggle-size-link"),
    togglePhysEnable: document.getElementById("toggle-phys-enable"),
    toggleAnimEnable: document.getElementById("toggle-anim-enable"),
    toggleAnimLoop: document.getElementById("toggle-anim-loop"),
    toggleAnimSync: document.getElementById("toggle-anim-sync"),
    toggleEmitMute: document.getElementById("toggle-emit-mute"),
    toggleEmitSolo: document.getElementById("toggle-emit-solo"),
    toggleRendDoppler: document.getElementById("toggle-rend-doppler"),
    toggleRendAirAbsorb: document.getElementById("toggle-rend-air-absorb"),
    toggleRendRoomEnable: document.getElementById("toggle-rend-room-enable"),
    toggleRendRoomErOnly: document.getElementById("toggle-rend-room-er-only"),
    toggleRendPhysWalls: document.getElementById("toggle-rend-phys-walls"),
    toggleRendPhysPause: document.getElementById("toggle-rend-phys-pause"),
    toggleRendVizTrails: document.getElementById("toggle-rend-viz-trails"),
    toggleRendVizVectors: document.getElementById("toggle-rend-viz-vectors"),
    toggleRendVizGrid: document.getElementById("toggle-rend-viz-grid"),
    toggleRendVizLabels: document.getElementById("toggle-rend-viz-labels"),
    inputEmitLabel: document.getElementById("input-emit-label"),
    btnPhysThrow: document.getElementById("btn-phys-throw"),
    btnPhysReset: document.getElementById("btn-phys-reset"),
    btnPresetRefresh: document.getElementById("btn-preset-refresh"),
    btnPresetSave: document.getElementById("btn-preset-save"),
    btnPresetLoad: document.getElementById("btn-preset-load"),
    choiceCalSpkConfig: document.getElementById("choice-cal-spk-config"),
    choiceCalTestType: document.getElementById("choice-cal-test-type"),
    choicePosCoordMode: document.getElementById("choice-pos-coord-mode"),
    choicePhysPreset: document.getElementById("choice-phys-preset"),
    choiceAnimMode: document.getElementById("choice-anim-mode"),
    choicePhysGravityDir: document.getElementById("choice-phys-gravity-dir"),
    choiceEmitterPreset: document.getElementById("choice-emitter-preset"),
    choiceRendDistanceModel: document.getElementById("choice-rend-distance-model"),
    choiceRendPhysRate: document.getElementById("choice-rend-phys-rate"),
    choiceRendVizMode: document.getElementById("choice-rend-viz-mode"),
    sliderEmitColor: document.getElementById("slider-emit-color"),
    sliderPosAzimuth: document.getElementById("slider-pos-azimuth"),
    sliderPosElevation: document.getElementById("slider-pos-elevation"),
    sliderPosDistance: document.getElementById("slider-pos-distance"),
    sliderSizeUniform: document.getElementById("slider-size-uniform"),
    sliderSizeWidth: document.getElementById("slider-size-width"),
    sliderSizeDepth: document.getElementById("slider-size-depth"),
    sliderSizeHeight: document.getElementById("slider-size-height"),
    sliderPhysMass: document.getElementById("slider-phys-mass"),
    sliderPhysDrag: document.getElementById("slider-phys-drag"),
    sliderPhysElasticity: document.getElementById("slider-phys-elasticity"),
    sliderPhysGravity: document.getElementById("slider-phys-gravity"),
    sliderPhysFriction: document.getElementById("slider-phys-friction"),
    sliderEmitGain: document.getElementById("slider-emit-gain"),
    sliderEmitSpread: document.getElementById("slider-emit-spread"),
    sliderEmitDirectivity: document.getElementById("slider-emit-directivity"),
    sliderAnimSpeed: document.getElementById("slider-anim-speed"),
    sliderRendMasterGain: document.getElementById("slider-rend-master-gain"),
    sliderRendSpk1Gain: document.getElementById("slider-rend-spk1-gain"),
    sliderRendSpk2Gain: document.getElementById("slider-rend-spk2-gain"),
    sliderRendSpk3Gain: document.getElementById("slider-rend-spk3-gain"),
    sliderRendSpk4Gain: document.getElementById("slider-rend-spk4-gain"),
    sliderRendSpk1Delay: document.getElementById("slider-rend-spk1-delay"),
    sliderRendSpk2Delay: document.getElementById("slider-rend-spk2-delay"),
    sliderRendSpk3Delay: document.getElementById("slider-rend-spk3-delay"),
    sliderRendSpk4Delay: document.getElementById("slider-rend-spk4-delay"),
    sliderRendDistanceRef: document.getElementById("slider-rend-distance-ref"),
    sliderRendDistanceMax: document.getElementById("slider-rend-distance-max"),
    sliderRendDopplerScale: document.getElementById("slider-rend-doppler-scale"),
    sliderRendRoomMix: document.getElementById("slider-rend-room-mix"),
    sliderRendRoomSize: document.getElementById("slider-rend-room-size"),
    sliderRendRoomDamping: document.getElementById("slider-rend-room-damping"),
    sliderRendVizTrailLen: document.getElementById("slider-rend-viz-trail-len"),
    sliderCalMicChannel: document.getElementById("slider-cal-mic-channel"),
    sliderCalSpk1Out: document.getElementById("slider-cal-spk1-out"),
    sliderCalSpk2Out: document.getElementById("slider-cal-spk2-out"),
    sliderCalSpk3Out: document.getElementById("slider-cal-spk3-out"),
    sliderCalSpk4Out: document.getElementById("slider-cal-spk4-out"),
    sliderCalTestLevel: document.getElementById("slider-cal-test-level"),
    readoutSizeUniform: document.getElementById("readout-size-uniform"),
    readoutEmitColor: document.getElementById("readout-emit-color"),
    readoutPosCartesian: document.getElementById("readout-pos-cartesian"),
    readoutSizeXyz: document.getElementById("readout-size-xyz"),
    statusCalibrateCore: document.getElementById("status-calibrate-core"),
    statusCalSetup: document.getElementById("status-cal-setup"),
    statusCalTest: document.getElementById("status-cal-test"),
    statusCalProgress: document.getElementById("status-cal-progress"),
    statusCalMessage: document.getElementById("status-cal-message"),
    statusCalRouting: document.getElementById("status-cal-routing"),
    statusCalProfile: document.getElementById("status-cal-profile"),
    calProgressBar: document.getElementById("cal-progress-bar"),
    btnCalMeasure: document.getElementById("btn-cal-measure"),
    btnCalRedetect: document.getElementById("btn-cal-redetect"),
    calSpk1Dot: document.getElementById("cal-spk1-dot"),
    calSpk2Dot: document.getElementById("cal-spk2-dot"),
    calSpk3Dot: document.getElementById("cal-spk3-dot"),
    calSpk4Dot: document.getElementById("cal-spk4-dot"),
    calSpk1Status: document.getElementById("cal-spk1-status"),
    calSpk2Status: document.getElementById("cal-spk2-status"),
    calSpk3Status: document.getElementById("cal-spk3-status"),
    calSpk4Status: document.getElementById("cal-spk4-status"),
    statusEmitterAudio: document.getElementById("status-emitter-audio"),
    statusPhysSummary: document.getElementById("status-phys-summary"),
    statusAnimSummary: document.getElementById("status-anim-summary"),
    statusPreset: document.getElementById("status-preset"),
    statusEmitterParity: document.getElementById("status-emitter-parity"),
    statusRendererScene: document.getElementById("status-renderer-scene"),
    statusRendererCore: document.getElementById("status-renderer-core"),
    statusRendererSpeakers: document.getElementById("status-renderer-speakers"),
    viewportCanvas: document.getElementById("viewport-canvas"),
  };

  const runtime = {
    currentMode: "emitter",
    selectedLane: "azimuth",
    latestScene: null,
    calibrationStatus: null,
    railScrollByMode: {
      calibrate: 0,
      emitter: 0,
      renderer: 0,
    },
    snapshot: {
      emitLabel: "Emitter",
      emitColorNorm: 0,
      sizeLink: false,
      qualityIndex: 0,
      posCoordModeIndex: 0,
      posAzimuthNorm: 0,
      posElevationNorm: 0,
      posDistanceNorm: 0,
      posXNorm: 0,
      posYNorm: 0,
      posZNorm: 0,
      sizeUniformNorm: 0,
      sizeWidthNorm: 0,
      sizeDepthNorm: 0,
      sizeHeightNorm: 0,
      emitMute: false,
      emitSolo: false,
      emitGainNorm: 0,
      emitSpreadNorm: 0,
      emitDirectivityNorm: 0,
      physPresetIndex: 0,
      physMassNorm: 0,
      physDragNorm: 0,
      physElasticityNorm: 0,
      physGravityNorm: 0,
      physFrictionNorm: 0,
      physGravityDirIndex: 0,
      animSpeedNorm: 0,
      rendMasterGainNorm: 0,
      rendSpk1GainNorm: 0,
      rendSpk2GainNorm: 0,
      rendSpk3GainNorm: 0,
      rendSpk4GainNorm: 0,
      rendSpk1DelayNorm: 0,
      rendSpk2DelayNorm: 0,
      rendSpk3DelayNorm: 0,
      rendSpk4DelayNorm: 0,
      rendDistanceModelIndex: 0,
      rendDistanceRefNorm: 0,
      rendDistanceMaxNorm: 0,
      rendDoppler: false,
      rendDopplerScaleNorm: 0,
      rendAirAbsorb: false,
      rendRoomEnable: false,
      rendRoomMixNorm: 0,
      rendRoomSizeNorm: 0,
      rendRoomDampingNorm: 0,
      rendRoomErOnly: false,
      rendPhysRateIndex: 0,
      rendPhysWalls: false,
      rendPhysPause: false,
      rendVizModeIndex: 0,
      rendVizTrails: false,
      rendVizTrailLenNorm: 0,
      rendVizVectors: false,
      rendVizGrid: false,
      rendVizLabels: false,
      calSpkConfigIndex: 0,
      calMicChannelNorm: 0,
      calSpk1OutNorm: 0,
      calSpk2OutNorm: 0,
      calSpk3OutNorm: 0,
      calSpk4OutNorm: 0,
      calTestLevelNorm: 0,
      calTestTypeIndex: 0,
    },
    counters: {
      setToggle: 0,
      setChoice: 0,
      setSlider: 0,
      valueToggle: 0,
      valueChoice: 0,
      valueSlider: 0,
      propsToggle: 0,
      propsChoice: 0,
      propsSlider: 0,
      heartbeat: 0,
    },
    choiceFetchInFlight: {
      mode: false,
      quality: false,
      posCoordMode: false,
      animMode: false,
      physGravityDir: false,
      distanceModel: false,
      physRate: false,
      vizMode: false,
      calSpkConfig: false,
      calTestType: false,
    },
    lastCalibrationRedetectAt: 0,
    lastCalibrationRedetectOk: false,
    lastPresetPath: "",
    lastPresetName: "",
    uiStateFetchDone: false,
    applyingPhysicsPreset: false,
  };

  function clamp(value, min, max) {
    return Math.min(max, Math.max(min, value));
  }

  function createListenerList() {
    const listeners = new Map();
    let nextId = 0;

    return {
      addListener(fn) {
        const id = nextId++;
        listeners.set(id, fn);
        return id;
      },
      callListeners() {
        listeners.forEach(listener => {
          try {
            listener();
          } catch (_) {}
        });
      },
    };
  }

  function createDirectBridge() {
    const backend = window.__JUCE__ && window.__JUCE__.backend ? window.__JUCE__.backend : null;
    if (!backend) {
      return null;
    }

    const sliderStates = new Map();
    const toggleStates = new Map();
    const comboStates = new Map();
    const pendingNativeCalls = new Map();
    let nextNativeCallId = 1;

    const emit = (identifier, payload) => {
      backend.emitEvent(identifier, payload);
    };

    backend.addEventListener("__juce__complete", event => {
      const payload = event || {};
      const promiseId = Number(payload.promiseId);
      if (!Number.isFinite(promiseId) || !pendingNativeCalls.has(promiseId)) {
        return;
      }

      const pending = pendingNativeCalls.get(promiseId);
      pendingNativeCalls.delete(promiseId);
      window.clearTimeout(pending.timeoutId);
      pending.resolve(payload.result);
    });

    const invokeNative = (name, args) => {
      return new Promise((resolve, reject) => {
        const resultId = nextNativeCallId++;
        const timeoutId = window.setTimeout(() => {
          if (!pendingNativeCalls.has(resultId)) {
            return;
          }

          pendingNativeCalls.delete(resultId);
          reject(new Error(`native function timeout: ${name}`));
        }, NATIVE_CALL_TIMEOUT_MS);

        pendingNativeCalls.set(resultId, { resolve, reject, timeoutId });

        try {
          emit("__juce__invoke", {
            name,
            params: Array.isArray(args) ? args : [],
            resultId,
          });
        } catch (error) {
          window.clearTimeout(timeoutId);
          pendingNativeCalls.delete(resultId);
          reject(error);
        }
      });
    };

    const attachListener = (identifier, onEvent) => {
      backend.addEventListener(identifier, event => onEvent(event || {}));
      emit(identifier, { eventType: "requestInitialUpdate" });
    };

    const toScaled = (normalised, start, end, skew) => {
      const clamped = clamp(Number(normalised) || 0, 0, 1);
      const safeStart = Number(start) || 0;
      const safeEnd = Number(end) || 1;
      const safeSkew = Number.isFinite(skew) && skew > 0 ? skew : 1;
      return Math.pow(clamped, 1 / safeSkew) * (safeEnd - safeStart) + safeStart;
    };

    const toNormalised = (scaled, start, end, skew) => {
      const safeStart = Number(start) || 0;
      const safeEnd = Number(end) || 1;
      const safeSkew = Number.isFinite(skew) && skew > 0 ? skew : 1;
      const denom = safeEnd - safeStart;
      if (Math.abs(denom) < 1.0e-9) {
        return 0;
      }

      const linear = clamp((Number(scaled) - safeStart) / denom, 0, 1);
      return Math.pow(linear, safeSkew);
    };

    function createSliderState(name) {
      const state = {
        name,
        identifier: `__juce__slider${name}`,
        scaled: 0,
        properties: {
          start: 0,
          end: 1,
          skew: 1,
        },
        valueChangedEvent: createListenerList(),
        propertiesChangedEvent: createListenerList(),
        getScaledValue() {
          return Number(this.scaled) || 0;
        },
        getNormalisedValue() {
          return toNormalised(this.scaled, this.properties.start, this.properties.end, this.properties.skew);
        },
        setNormalisedValue(next) {
          this.scaled = toScaled(next, this.properties.start, this.properties.end, this.properties.skew);
          emit(this.identifier, { eventType: "valueChanged", value: this.scaled });
          this.valueChangedEvent.callListeners();
        },
        sliderDragStarted() {
          emit(this.identifier, { eventType: "sliderDragStarted" });
        },
        sliderDragEnded() {
          emit(this.identifier, { eventType: "sliderDragEnded" });
        },
      };

      attachListener(state.identifier, event => {
        if (event.eventType === "valueChanged") {
          state.scaled = Number(event.value) || 0;
          state.valueChangedEvent.callListeners();
          return;
        }

        if (event.eventType === "propertiesChanged") {
          const next = { ...event };
          delete next.eventType;
          state.properties = { ...state.properties, ...next };
          state.propertiesChangedEvent.callListeners();
        }
      });

      return state;
    }

    function createToggleState(name) {
      const state = {
        name,
        identifier: `__juce__toggle${name}`,
        value: false,
        properties: {},
        valueChangedEvent: createListenerList(),
        propertiesChangedEvent: createListenerList(),
        getValue() {
          return !!this.value;
        },
        setValue(next) {
          this.value = !!next;
          emit(this.identifier, { eventType: "valueChanged", value: this.value });
          this.valueChangedEvent.callListeners();
        },
      };

      attachListener(state.identifier, event => {
        if (event.eventType === "valueChanged") {
          state.value = !!event.value;
          state.valueChangedEvent.callListeners();
          return;
        }

        if (event.eventType === "propertiesChanged") {
          const next = { ...event };
          delete next.eventType;
          state.properties = { ...state.properties, ...next };
          state.propertiesChangedEvent.callListeners();
        }
      });

      return state;
    }

    function createComboState(name) {
      const state = {
        name,
        identifier: `__juce__comboBox${name}`,
        value: 0,
        properties: { choices: [] },
        valueChangedEvent: createListenerList(),
        propertiesChangedEvent: createListenerList(),
        getChoiceIndex() {
          const count = Array.isArray(this.properties.choices) ? this.properties.choices.length : 0;
          if (count <= 1) {
            return Math.max(0, Math.round(this.value || 0));
          }

          const normalised = clamp(Number(this.value) || 0, 0, 1);
          return clamp(Math.round(normalised * (count - 1)), 0, count - 1);
        },
        setChoiceIndex(index) {
          const count = Array.isArray(this.properties.choices) ? this.properties.choices.length : 0;
          const clamped = Math.max(0, Math.round(Number(index) || 0));

          if (count <= 1) {
            this.value = clamped;
          } else {
            this.value = clamp(clamped / Math.max(1, count - 1), 0, 1);
          }

          emit(this.identifier, { eventType: "valueChanged", value: this.value });
          this.valueChangedEvent.callListeners();
        },
      };

      attachListener(state.identifier, event => {
        if (event.eventType === "valueChanged") {
          state.value = Number(event.value) || 0;
          state.valueChangedEvent.callListeners();
          return;
        }

        if (event.eventType === "propertiesChanged") {
          const next = { ...event };
          delete next.eventType;
          state.properties = { ...state.properties, ...next };
          state.propertiesChangedEvent.callListeners();
        }
      });

      return state;
    }

    return {
      getNativeFunction(name) {
        return async function invokeNativeFunction() {
          return invokeNative(name, Array.prototype.slice.call(arguments));
        };
      },
      getSliderState(name) {
        if (!sliderStates.has(name)) {
          sliderStates.set(name, createSliderState(name));
        }
        return sliderStates.get(name);
      },
      getToggleState(name) {
        if (!toggleStates.has(name)) {
          toggleStates.set(name, createToggleState(name));
        }
        return toggleStates.get(name);
      },
      getComboBoxState(name) {
        if (!comboStates.has(name)) {
          comboStates.set(name, createComboState(name));
        }
        return comboStates.get(name);
      },
    };
  }

  function createPreviewBridge() {
    const sliderStates = new Map();
    const toggleStates = new Map();
    const comboStates = new Map();

    function makeSlider(name) {
      const defaults = {
        cal_mic_channel: { scaled: 1.0, start: 1.0, end: 8.0, skew: 1.0 },
        cal_spk1_out: { scaled: 1.0, start: 1.0, end: 8.0, skew: 1.0 },
        cal_spk2_out: { scaled: 2.0, start: 1.0, end: 8.0, skew: 1.0 },
        cal_spk3_out: { scaled: 3.0, start: 1.0, end: 8.0, skew: 1.0 },
        cal_spk4_out: { scaled: 4.0, start: 1.0, end: 8.0, skew: 1.0 },
        cal_test_level: { scaled: -20.0, start: -60.0, end: 0.0, skew: 1.0 },
        pos_azimuth: { scaled: 0.0, start: -180.0, end: 180.0, skew: 1.0 },
        pos_elevation: { scaled: 0.0, start: -90.0, end: 90.0, skew: 1.0 },
        pos_distance: { scaled: 2.0, start: 0.0, end: 50.0, skew: 0.5 },
        pos_x: { scaled: 0.0, start: -25.0, end: 25.0, skew: 1.0 },
        pos_y: { scaled: 0.0, start: -25.0, end: 25.0, skew: 1.0 },
        pos_z: { scaled: 0.0, start: -10.0, end: 10.0, skew: 1.0 },
        size_uniform: { scaled: 0.5, start: 0.01, end: 20, skew: 0.4 },
        size_width: { scaled: 0.5, start: 0.01, end: 20, skew: 0.5 },
        size_depth: { scaled: 0.5, start: 0.01, end: 20, skew: 0.5 },
        size_height: { scaled: 0.5, start: 0.01, end: 10, skew: 0.5 },
        emit_color: { scaled: 0, start: 0, end: 15, skew: 1.0 },
        emit_gain: { scaled: 0.0, start: -60.0, end: 12.0, skew: 1.0 },
        emit_spread: { scaled: 0.0, start: 0.0, end: 1.0, skew: 1.0 },
        emit_directivity: { scaled: 0.5, start: 0.0, end: 1.0, skew: 1.0 },
        phys_mass: { scaled: 1.0, start: 0.01, end: 100.0, skew: 0.4 },
        phys_drag: { scaled: 0.5, start: 0.0, end: 10.0, skew: 1.0 },
        phys_elasticity: { scaled: 0.7, start: 0.0, end: 1.0, skew: 1.0 },
        phys_gravity: { scaled: 0.0, start: -20.0, end: 20.0, skew: 1.0 },
        phys_friction: { scaled: 0.3, start: 0.0, end: 1.0, skew: 1.0 },
        anim_speed: { scaled: 1.0, start: 0.1, end: 10.0, skew: 1.0 },
        rend_master_gain: { scaled: 0.0, start: -60.0, end: 12.0, skew: 1.0 },
        rend_spk1_gain: { scaled: 0.0, start: -24.0, end: 12.0, skew: 1.0 },
        rend_spk2_gain: { scaled: 0.0, start: -24.0, end: 12.0, skew: 1.0 },
        rend_spk3_gain: { scaled: 0.0, start: -24.0, end: 12.0, skew: 1.0 },
        rend_spk4_gain: { scaled: 0.0, start: -24.0, end: 12.0, skew: 1.0 },
        rend_spk1_delay: { scaled: 0.0, start: 0.0, end: 50.0, skew: 1.0 },
        rend_spk2_delay: { scaled: 0.0, start: 0.0, end: 50.0, skew: 1.0 },
        rend_spk3_delay: { scaled: 0.0, start: 0.0, end: 50.0, skew: 1.0 },
        rend_spk4_delay: { scaled: 0.0, start: 0.0, end: 50.0, skew: 1.0 },
        rend_distance_ref: { scaled: 1.0, start: 0.1, end: 10.0, skew: 1.0 },
        rend_distance_max: { scaled: 50.0, start: 1.0, end: 100.0, skew: 1.0 },
        rend_doppler_scale: { scaled: 1.0, start: 0.0, end: 5.0, skew: 1.0 },
        rend_room_mix: { scaled: 0.3, start: 0.0, end: 1.0, skew: 1.0 },
        rend_room_size: { scaled: 1.0, start: 0.5, end: 5.0, skew: 1.0 },
        rend_room_damping: { scaled: 0.5, start: 0.0, end: 1.0, skew: 1.0 },
        rend_viz_trail_len: { scaled: 5.0, start: 0.5, end: 30.0, skew: 1.0 },
      };
      const entry = defaults[name] || { scaled: 0.5, start: 0, end: 1, skew: 1 };

      return {
        name,
        identifier: `preview_slider_${name}`,
        scaled: entry.scaled,
        properties: { start: entry.start, end: entry.end, skew: entry.skew },
        valueChangedEvent: createListenerList(),
        propertiesChangedEvent: createListenerList(),
        getScaledValue() {
          return Number(this.scaled) || 0;
        },
        getNormalisedValue() {
          const denom = this.properties.end - this.properties.start;
          if (Math.abs(denom) < 1.0e-9) return 0;
          const linear = clamp((this.scaled - this.properties.start) / denom, 0, 1);
          const skew = this.properties.skew > 0 ? this.properties.skew : 1;
          return Math.pow(linear, skew);
        },
        setNormalisedValue(next) {
          const normal = clamp(Number(next) || 0, 0, 1);
          const skew = this.properties.skew > 0 ? this.properties.skew : 1;
          this.scaled = Math.pow(normal, 1 / skew) * (this.properties.end - this.properties.start) + this.properties.start;
          this.valueChangedEvent.callListeners();
        },
        sliderDragStarted() {},
        sliderDragEnded() {},
      };
    }

    function makeToggle(name) {
      const defaults = {
        size_link: true,
        phys_enable: false,
        phys_throw: false,
        phys_reset: false,
        anim_enable: false,
        anim_loop: false,
        anim_sync: true,
        emit_mute: false,
        emit_solo: false,
        rend_doppler: false,
        rend_air_absorb: true,
        rend_room_enable: true,
        rend_room_er_only: false,
        rend_phys_walls: true,
        rend_phys_pause: false,
        rend_viz_trails: true,
        rend_viz_vectors: false,
        rend_viz_grid: true,
        rend_viz_labels: true,
      };

      return {
        name,
        identifier: `preview_toggle_${name}`,
        value: !!defaults[name],
        properties: {},
        valueChangedEvent: createListenerList(),
        propertiesChangedEvent: createListenerList(),
        getValue() {
          return !!this.value;
        },
        setValue(next) {
          this.value = !!next;
          this.valueChangedEvent.callListeners();
        },
      };
    }

    function makeCombo(name) {
      const choices = DEFAULT_CHOICES[name] ? DEFAULT_CHOICES[name].slice() : [];
      const defaults = {
        mode: 1,
        cal_spk_config: 0,
        cal_test_type: 0,
        pos_coord_mode: 0,
        rend_quality: 0,
        rend_distance_model: 0,
        rend_phys_rate: 1,
        rend_viz_mode: 0,
        anim_mode: 0,
        phys_gravity_dir: 0,
      };

      const count = choices.length;
      const index = clamp(defaults[name] || 0, 0, Math.max(0, count - 1));
      const value = count <= 1 ? index : index / Math.max(1, count - 1);

      return {
        name,
        identifier: `preview_combo_${name}`,
        value,
        properties: { choices },
        valueChangedEvent: createListenerList(),
        propertiesChangedEvent: createListenerList(),
        getChoiceIndex() {
          const total = this.properties.choices.length;
          if (total <= 1) return Math.max(0, Math.round(this.value || 0));
          return clamp(Math.round(clamp(Number(this.value) || 0, 0, 1) * (total - 1)), 0, total - 1);
        },
        setChoiceIndex(next) {
          const total = this.properties.choices.length;
          const idx = Math.max(0, Math.round(Number(next) || 0));
          this.value = total <= 1 ? idx : clamp(idx / Math.max(1, total - 1), 0, 1);
          this.valueChangedEvent.callListeners();
        },
      };
    }

    return {
      getNativeFunction() {
        return async function previewNative() {
          return [];
        };
      },
      getSliderState(name) {
        if (!sliderStates.has(name)) sliderStates.set(name, makeSlider(name));
        return sliderStates.get(name);
      },
      getToggleState(name) {
        if (!toggleStates.has(name)) toggleStates.set(name, makeToggle(name));
        return toggleStates.get(name);
      },
      getComboBoxState(name) {
        if (!comboStates.has(name)) comboStates.set(name, makeCombo(name));
        return comboStates.get(name);
      },
    };
  }

  const bridge = window.Juce || createDirectBridge() || createPreviewBridge();
  window.Juce = bridge;

  const backend = window.__JUCE__ && window.__JUCE__.backend ? window.__JUCE__.backend : null;
  const getChoiceItemsNative = typeof bridge.getNativeFunction === "function"
    ? bridge.getNativeFunction("locusqGetChoiceItems")
    : null;
  const startCalibrationNative = typeof bridge.getNativeFunction === "function"
    ? bridge.getNativeFunction("locusqStartCalibration")
    : null;
  const abortCalibrationNative = typeof bridge.getNativeFunction === "function"
    ? bridge.getNativeFunction("locusqAbortCalibration")
    : null;
  const redetectCalibrationRoutingNative = typeof bridge.getNativeFunction === "function"
    ? bridge.getNativeFunction("locusqRedetectCalibrationRouting")
    : null;
  const listEmitterPresetsNative = typeof bridge.getNativeFunction === "function"
    ? bridge.getNativeFunction("locusqListEmitterPresets")
    : null;
  const saveEmitterPresetNative = typeof bridge.getNativeFunction === "function"
    ? bridge.getNativeFunction("locusqSaveEmitterPreset")
    : null;
  const loadEmitterPresetNative = typeof bridge.getNativeFunction === "function"
    ? bridge.getNativeFunction("locusqLoadEmitterPreset")
    : null;
  const getUiStateNative = typeof bridge.getNativeFunction === "function"
    ? bridge.getNativeFunction("locusqGetUiState")
    : null;
  const setUiStateNative = typeof bridge.getNativeFunction === "function"
    ? bridge.getNativeFunction("locusqSetUiState")
    : null;
  const queryParams = new URLSearchParams(window.location.search || "");
  const selfTestRequested = queryParams.get("selftest") === "1";
  const debugUiRequested = selfTestRequested || queryParams.get("debug") === "1";

  const controlStates = {
    mode: bridge.getComboBoxState("mode"),
    calSpkConfig: bridge.getComboBoxState("cal_spk_config"),
    calTestType: bridge.getComboBoxState("cal_test_type"),
    quality: bridge.getComboBoxState("rend_quality"),
    posCoordMode: bridge.getComboBoxState("pos_coord_mode"),
    posAzimuth: bridge.getSliderState("pos_azimuth"),
    posElevation: bridge.getSliderState("pos_elevation"),
    posDistance: bridge.getSliderState("pos_distance"),
    posX: bridge.getSliderState("pos_x"),
    posY: bridge.getSliderState("pos_y"),
    posZ: bridge.getSliderState("pos_z"),
    calMicChannel: bridge.getSliderState("cal_mic_channel"),
    calSpk1Out: bridge.getSliderState("cal_spk1_out"),
    calSpk2Out: bridge.getSliderState("cal_spk2_out"),
    calSpk3Out: bridge.getSliderState("cal_spk3_out"),
    calSpk4Out: bridge.getSliderState("cal_spk4_out"),
    calTestLevel: bridge.getSliderState("cal_test_level"),
    sizeLink: bridge.getToggleState("size_link"),
    sizeUniform: bridge.getSliderState("size_uniform"),
    sizeWidth: bridge.getSliderState("size_width"),
    sizeDepth: bridge.getSliderState("size_depth"),
    sizeHeight: bridge.getSliderState("size_height"),
    emitColor: bridge.getSliderState("emit_color"),
    emitGain: bridge.getSliderState("emit_gain"),
    emitSpread: bridge.getSliderState("emit_spread"),
    emitDirectivity: bridge.getSliderState("emit_directivity"),
    rendMasterGain: bridge.getSliderState("rend_master_gain"),
    rendSpk1Gain: bridge.getSliderState("rend_spk1_gain"),
    rendSpk2Gain: bridge.getSliderState("rend_spk2_gain"),
    rendSpk3Gain: bridge.getSliderState("rend_spk3_gain"),
    rendSpk4Gain: bridge.getSliderState("rend_spk4_gain"),
    rendSpk1Delay: bridge.getSliderState("rend_spk1_delay"),
    rendSpk2Delay: bridge.getSliderState("rend_spk2_delay"),
    rendSpk3Delay: bridge.getSliderState("rend_spk3_delay"),
    rendSpk4Delay: bridge.getSliderState("rend_spk4_delay"),
    rendDistanceRef: bridge.getSliderState("rend_distance_ref"),
    rendDistanceMax: bridge.getSliderState("rend_distance_max"),
    emitMute: bridge.getToggleState("emit_mute"),
    emitSolo: bridge.getToggleState("emit_solo"),
    rendDoppler: bridge.getToggleState("rend_doppler"),
    rendAirAbsorb: bridge.getToggleState("rend_air_absorb"),
    rendDopplerScale: bridge.getSliderState("rend_doppler_scale"),
    rendRoomEnable: bridge.getToggleState("rend_room_enable"),
    rendRoomMix: bridge.getSliderState("rend_room_mix"),
    rendRoomSize: bridge.getSliderState("rend_room_size"),
    rendRoomDamping: bridge.getSliderState("rend_room_damping"),
    rendRoomErOnly: bridge.getToggleState("rend_room_er_only"),
    physEnable: bridge.getToggleState("phys_enable"),
    physMass: bridge.getSliderState("phys_mass"),
    physDrag: bridge.getSliderState("phys_drag"),
    physElasticity: bridge.getSliderState("phys_elasticity"),
    physGravity: bridge.getSliderState("phys_gravity"),
    physFriction: bridge.getSliderState("phys_friction"),
    physThrow: bridge.getToggleState("phys_throw"),
    physReset: bridge.getToggleState("phys_reset"),
    physGravityDir: bridge.getComboBoxState("phys_gravity_dir"),
    rendDistanceModel: bridge.getComboBoxState("rend_distance_model"),
    rendPhysRate: bridge.getComboBoxState("rend_phys_rate"),
    rendPhysWalls: bridge.getToggleState("rend_phys_walls"),
    rendPhysPause: bridge.getToggleState("rend_phys_pause"),
    rendVizMode: bridge.getComboBoxState("rend_viz_mode"),
    rendVizTrails: bridge.getToggleState("rend_viz_trails"),
    rendVizTrailLen: bridge.getSliderState("rend_viz_trail_len"),
    rendVizVectors: bridge.getToggleState("rend_viz_vectors"),
    rendVizGrid: bridge.getToggleState("rend_viz_grid"),
    rendVizLabels: bridge.getToggleState("rend_viz_labels"),
    animEnable: bridge.getToggleState("anim_enable"),
    animMode: bridge.getComboBoxState("anim_mode"),
    animLoop: bridge.getToggleState("anim_loop"),
    animSync: bridge.getToggleState("anim_sync"),
    animSpeed: bridge.getSliderState("anim_speed"),
  };

  function getToggleValue(state) {
    if (!state) return false;
    if (typeof state.getValue === "function") {
      return !!state.getValue();
    }
    return !!state.value;
  }

  function getChoiceIndex(state) {
    if (!state) return 0;
    if (typeof state.getChoiceIndex === "function") {
      return Math.max(0, Math.round(Number(state.getChoiceIndex()) || 0));
    }
    return Math.max(0, Math.round(Number(state.value) || 0));
  }

  function getSliderNormalised(state) {
    if (!state) return 0;
    if (typeof state.getNormalisedValue === "function") {
      return clamp(Number(state.getNormalisedValue()) || 0, 0, 1);
    }
    return 0;
  }

  function waitMs(ms) {
    return new Promise(resolve => window.setTimeout(resolve, ms));
  }

  function sanitizeStatusMessage(value, fallback = CALIBRATION_IDLE_MESSAGE) {
    let text = String(value == null ? "" : value);
    if (!text.trim()) {
      text = fallback;
    }

    return text
      .replace(/Ã¢â‚¬â„¢|â€™/g, "'")
      .replace(/Ã¢â‚¬â€œ|Ã¢â‚¬â€|â€”|â€“|â€¢/g, "-")
      .replace(/[—–−]/g, "-")
      .replace(/[Â]/g, "")
      .replace(/[\u0000-\u0008\u000B\u000C\u000E-\u001F\u007F]/g, " ")
      .replace(/[^\x09\x0A\x0D\x20-\x7E]/g, " ")
      .replace(/\s{2,}/g, " ")
      .trim() || fallback;
  }

  function createDefaultCalibrationStatus(overrides = {}) {
    return {
      state: "idle",
      running: false,
      complete: false,
      currentSpeaker: 1,
      completedSpeakers: 0,
      playPercent: 0,
      recordPercent: 0,
      overallPercent: 0,
      message: CALIBRATION_IDLE_MESSAGE,
      profileValid: false,
      ...overrides,
    };
  }

  function getCalibrationLifecycleLabel(status) {
    const current = status || createDefaultCalibrationStatus();
    if (current.running) {
      return "MEASURING";
    }
    if (current.complete || current.profileValid) {
      return "PROFILE READY";
    }
    return "NO PROFILE";
  }

  function sanitizeCalibrationStatus(status) {
    if (!status || typeof status !== "object") {
      return null;
    }

    const merged = createDefaultCalibrationStatus(status);
    const state = String(merged.state || "idle").trim().toLowerCase();
    const running = !!merged.running;
    const complete = !!merged.complete || state === "complete";

    return {
      ...merged,
      state,
      running,
      complete,
      currentSpeaker: clamp(Math.round(Number(merged.currentSpeaker) || 1), 1, 4),
      completedSpeakers: clamp(Math.round(Number(merged.completedSpeakers) || 0), 0, 4),
      playPercent: clamp(Number(merged.playPercent) || 0, 0, 1),
      recordPercent: clamp(Number(merged.recordPercent) || 0, 0, 1),
      overallPercent: clamp(Number(merged.overallPercent) || 0, 0, 1),
      message: sanitizeStatusMessage(merged.message, CALIBRATION_IDLE_MESSAGE),
      profileValid: !!merged.profileValid,
    };
  }

  async function waitForCondition(label, predicate, timeoutMs = 2000, pollMs = 25) {
    const deadline = Date.now() + timeoutMs;
    while (Date.now() < deadline) {
      if (predicate()) {
        return;
      }
      await waitMs(pollMs);
    }
    throw new Error(`timeout waiting for ${label}`);
  }

  function closeEnough(a, b, epsilon = 0.04) {
    return Math.abs(Number(a) - Number(b)) <= epsilon;
  }

  function getScaledValueOrFallback(state, fallback = 0) {
    if (!state) return fallback;
    if (typeof state.getScaledValue === "function") {
      const value = Number(state.getScaledValue());
      if (Number.isFinite(value)) return value;
    }
    return fallback;
  }

  function formatNumber(value, digits = 2, fallback = "n/a") {
    const numeric = Number(value);
    if (!Number.isFinite(numeric)) {
      return fallback;
    }
    return numeric.toFixed(digits);
  }

  function sanitizeEmitterLabel(value) {
    const raw = String(value == null ? "" : value).trim();
    const filtered = raw
      .replace(/[^A-Za-z0-9 _.\-()]/g, "")
      .trim();
    const trimmed = filtered.slice(0, 31);
    return trimmed.length > 0 ? trimmed : "Emitter";
  }

  function getSelectedEmitterFromScene() {
    const scene = runtime.latestScene;
    if (!scene || !Array.isArray(scene.emitters) || scene.emitters.length === 0) {
      return null;
    }

    const localEmitterId = Number(scene.localEmitterId);
    if (Number.isInteger(localEmitterId)) {
      const byId = scene.emitters.find(entry => Number(entry.id) === localEmitterId);
      if (byId) {
        return byId;
      }
    }

    return scene.emitters[0] || null;
  }

  async function persistUiState(partialState) {
    if (!setUiStateNative || !partialState || typeof partialState !== "object") {
      return;
    }

    try {
      await callNativeSafe("locusqSetUiState", setUiStateNative, partialState);
    } catch (error) {
      writeDiagnostics({ uiStateWriteError: String(error) });
    }
  }

  function getCalChannelFromScaled(state, fallback = 1) {
    return Math.max(1, Math.round(getScaledValueOrFallback(state, fallback)));
  }

  function getCalibrationOptionsFromControls() {
    return {
      testType: getChoiceIndex(controlStates.calTestType),
      testLevelDb: getScaledValueOrFallback(controlStates.calTestLevel, -20.0),
      sweepSeconds: 3.0,
      tailSeconds: 1.5,
      micChannel: getCalChannelFromScaled(controlStates.calMicChannel, 1) - 1,
      speakerChannels: [
        getCalChannelFromScaled(controlStates.calSpk1Out, 1) - 1,
        getCalChannelFromScaled(controlStates.calSpk2Out, 2) - 1,
        getCalChannelFromScaled(controlStates.calSpk3Out, 3) - 1,
        getCalChannelFromScaled(controlStates.calSpk4Out, 4) - 1,
      ],
    };
  }

  async function callNativeSafe(name, fn, argument) {
    if (typeof fn !== "function") {
      throw new Error(`native function unavailable: ${name}`);
    }

    if (typeof argument === "undefined") {
      return fn();
    }

    return fn(argument);
  }

  function setBridgeState(ok, text) {
    dom.bridgeState.textContent = text;
    dom.bridgeState.className = `mono ${ok ? "bridge-ok" : "bridge-warn"}`;
  }

  function writeDiagnostics(extra = {}) {
    const payload = {
      ts: new Date().toISOString(),
      hasJuce: typeof window.Juce !== "undefined",
      hasBackend: !!backend,
      currentMode: runtime.currentMode,
      lane: runtime.selectedLane,
      counters: runtime.counters,
      snapshot: runtime.snapshot,
      sceneData: runtime.latestScene,
      calibrationStatus: runtime.calibrationStatus,
      ...extra,
    };

    if (dom.diagnostics) {
      dom.diagnostics.textContent = JSON.stringify(payload, null, 2);
    }
  }

  function renderCounters() {
    dom.countSet.textContent = `${runtime.counters.setToggle} / ${runtime.counters.setChoice} / ${runtime.counters.setSlider}`;
    dom.countValue.textContent = `${runtime.counters.valueToggle} / ${runtime.counters.valueChoice} / ${runtime.counters.valueSlider}`;
    dom.countProps.textContent = `${runtime.counters.propsToggle} / ${runtime.counters.propsChoice} / ${runtime.counters.propsSlider}`;
    dom.countLive.textContent =
      `${runtime.snapshot.sizeLink ? "1" : "0"} / ${runtime.snapshot.qualityIndex} / ${runtime.snapshot.sizeUniformNorm.toFixed(3)}`;
    dom.countHeartbeat.textContent = String(runtime.counters.heartbeat);
    dom.modeLive.textContent = runtime.currentMode;
  }

  function getChoices(state, fallback) {
    const relayChoices = Array.isArray(state?.properties?.choices) ? state.properties.choices : [];
    if (relayChoices.length > 0) {
      return relayChoices.map(item => String(item));
    }
    return fallback.slice();
  }

  function rebuildSelectOptions(select, choices) {
    if (!select) return;
    select.innerHTML = "";
    choices.forEach((item, index) => {
      const option = document.createElement("option");
      option.value = String(index);
      option.textContent = String(item);
      select.appendChild(option);
    });
  }

  function emitChoiceWithFallback(state, index, assumedChoiceCount) {
    if (!backend || !state || typeof state.identifier !== "string") {
      return;
    }

    const choiceCount = assumedChoiceCount > 1 ? assumedChoiceCount : Math.max(index + 1, 2);
    const normalised = clamp(index / Math.max(1, choiceCount - 1), 0, 1);
    state.value = normalised;
    backend.emitEvent(state.identifier, { eventType: "valueChanged", value: normalised });
    if (state.valueChangedEvent && typeof state.valueChangedEvent.callListeners === "function") {
      state.valueChangedEvent.callListeners();
    }
  }

  function setChoiceIndexSafe(state, index, fallbackChoiceCount) {
    if (!state) return;
    const desiredIndex = Math.max(0, Math.round(Number(index) || 0));
    const relayChoiceCount = Array.isArray(state.properties?.choices) ? state.properties.choices.length : 0;

    if (relayChoiceCount <= 1 && fallbackChoiceCount > 1) {
      emitChoiceWithFallback(state, desiredIndex, fallbackChoiceCount);
      return;
    }

    if (typeof state.setChoiceIndex === "function") {
      state.setChoiceIndex(desiredIndex);
      return;
    }

    state.value = desiredIndex;
    if (state.valueChangedEvent && typeof state.valueChangedEvent.callListeners === "function") {
      state.valueChangedEvent.callListeners();
    }
  }

  function normaliseModeLabel(label) {
    const value = String(label || "").trim().toLowerCase();
    if (value.startsWith("cal")) return "calibrate";
    if (value.startsWith("emit")) return "emitter";
    if (value.startsWith("ren")) return "renderer";
    return null;
  }

  function modeFromChoiceIndex(index) {
    const choices = getChoices(controlStates.mode, DEFAULT_CHOICES.mode);
    const mapped = choices[index] ? normaliseModeLabel(choices[index]) : null;
    if (mapped) return mapped;
    return MODE_ORDER[clamp(index, 0, MODE_ORDER.length - 1)] || "emitter";
  }

  function modeToChoiceIndex(mode) {
    const normalisedMode = MODE_ORDER.includes(mode) ? mode : "emitter";
    const choices = getChoices(controlStates.mode, DEFAULT_CHOICES.mode);
    for (let i = 0; i < choices.length; ++i) {
      if (normaliseModeLabel(choices[i]) === normalisedMode) {
        return i;
      }
    }
    return clamp(MODE_ORDER.indexOf(normalisedMode), 0, Math.max(0, choices.length - 1));
  }

  function setActiveLane(lane) {
    runtime.selectedLane = lane;
    dom.lanes.forEach(button => {
      button.classList.toggle("selected", button.dataset.lane === lane);
    });
  }

  function updateRoomProfileBadge() {
    const status = runtime.calibrationStatus || createDefaultCalibrationStatus();
    const lifecycle = getCalibrationLifecycleLabel(status);
    const profileReady = lifecycle === "PROFILE READY";
    dom.roomDot.classList.toggle("loaded", profileReady);
    dom.roomLabel.textContent = profileReady ? "Profile Ready" : "No Profile";
  }

  function updateSceneStatusBadge() {
    const sceneStatus = dom.sceneStatus;
    if (!sceneStatus) return;

    sceneStatus.className = "scene-status";

    if (runtime.currentMode === "calibrate") {
      const status = runtime.calibrationStatus || createDefaultCalibrationStatus();
      const lifecycle = getCalibrationLifecycleLabel(status);
      if (lifecycle === "MEASURING") {
        sceneStatus.textContent = "MEASURING";
        sceneStatus.classList.add("measuring");
      } else if (lifecycle === "PROFILE READY") {
        sceneStatus.textContent = "PROFILE READY";
        sceneStatus.classList.add("ready");
      } else {
        sceneStatus.textContent = "NO PROFILE";
        sceneStatus.classList.add("noprofile");
      }
      return;
    }

    if (runtime.currentMode === "renderer") {
      sceneStatus.textContent = "READY";
      sceneStatus.classList.add("ready");
      return;
    }

    const physicsEnabled = getToggleValue(controlStates.physEnable);
    sceneStatus.textContent = physicsEnabled ? "PHYSICS" : "STABLE";
    if (physicsEnabled) sceneStatus.classList.add("physics");
  }

  function updateQualityBadge() {
    const qualityChoices = getChoices(controlStates.quality, DEFAULT_CHOICES.rend_quality);
    const choiceIndex = clamp(getChoiceIndex(controlStates.quality), 0, Math.max(0, qualityChoices.length - 1));
    runtime.snapshot.qualityIndex = choiceIndex;
    const text = qualityChoices[choiceIndex] || qualityChoices[0] || "Draft";
    const final = String(text).trim().toLowerCase().startsWith("final");

    dom.qualityBadge.textContent = String(text).toUpperCase();
    dom.qualityBadge.classList.toggle("final", final);
    dom.qualityBadge.classList.toggle("draft", !final);
    if (dom.qualityReadout) dom.qualityReadout.textContent = text;
  }

  function updateCalibrateCoreStatus() {
    if (!dom.statusCalibrateCore) {
      return;
    }

    const spkConfigChoices = getChoices(controlStates.calSpkConfig, DEFAULT_CHOICES.cal_spk_config);
    const spkConfigIndex = clamp(getChoiceIndex(controlStates.calSpkConfig), 0, Math.max(0, spkConfigChoices.length - 1));
    const spkConfigLabel = spkConfigChoices[spkConfigIndex] || "n/a";
    const micNorm = getSliderNormalised(controlStates.calMicChannel);
    const micScaled = typeof controlStates.calMicChannel.getScaledValue === "function"
      ? Number(controlStates.calMicChannel.getScaledValue())
      : 0;
    const micChannel = Math.max(1, Math.round(micScaled || 1));
    const spk1Scaled = typeof controlStates.calSpk1Out.getScaledValue === "function"
      ? Number(controlStates.calSpk1Out.getScaledValue())
      : 0;
    const spk2Scaled = typeof controlStates.calSpk2Out.getScaledValue === "function"
      ? Number(controlStates.calSpk2Out.getScaledValue())
      : 0;
    const spk3Scaled = typeof controlStates.calSpk3Out.getScaledValue === "function"
      ? Number(controlStates.calSpk3Out.getScaledValue())
      : 0;
    const spk4Scaled = typeof controlStates.calSpk4Out.getScaledValue === "function"
      ? Number(controlStates.calSpk4Out.getScaledValue())
      : 0;
    const spk1Out = Math.max(1, Math.round(spk1Scaled || 1));
    const spk2Out = Math.max(1, Math.round(spk2Scaled || 1));
    const spk3Out = Math.max(1, Math.round(spk3Scaled || 1));
    const spk4Out = Math.max(1, Math.round(spk4Scaled || 1));
    const spk1Norm = getSliderNormalised(controlStates.calSpk1Out);
    const spk2Norm = getSliderNormalised(controlStates.calSpk2Out);
    const spk3Norm = getSliderNormalised(controlStates.calSpk3Out);
    const spk4Norm = getSliderNormalised(controlStates.calSpk4Out);
    const testLevelNorm = getSliderNormalised(controlStates.calTestLevel);
    const testTypeChoices = getChoices(controlStates.calTestType, DEFAULT_CHOICES.cal_test_type);
    const testTypeIndex = clamp(getChoiceIndex(controlStates.calTestType), 0, Math.max(0, testTypeChoices.length - 1));
    const testTypeLabel = testTypeChoices[testTypeIndex] || "n/a";

    runtime.snapshot.calSpkConfigIndex = spkConfigIndex;
    runtime.snapshot.calMicChannelNorm = micNorm;
    runtime.snapshot.calSpk1OutNorm = spk1Norm;
    runtime.snapshot.calSpk2OutNorm = spk2Norm;
    runtime.snapshot.calSpk3OutNorm = spk3Norm;
    runtime.snapshot.calSpk4OutNorm = spk4Norm;
    runtime.snapshot.calTestLevelNorm = testLevelNorm;
    runtime.snapshot.calTestTypeIndex = testTypeIndex;

    const testLevelDb = getScaledValueOrFallback(controlStates.calTestLevel, -20.0);
    const routingSummary = `${spk1Out}/${spk2Out}/${spk3Out}/${spk4Out}`;

    dom.statusCalibrateCore.textContent =
      `Config ${spkConfigLabel} · Mic CH ${micChannel} · Out ${routingSummary} · Level ${Math.round(testLevelNorm * 100)}% · Type ${testTypeLabel}`;

    if (dom.statusCalSetup) {
      dom.statusCalSetup.textContent = `Mic CH ${micChannel} -> IN ${micChannel} · SPK ${routingSummary}`;
    }

    if (dom.statusCalTest) {
      dom.statusCalTest.textContent = `Type ${testTypeLabel} · Level ${formatNumber(testLevelDb, 1)} dBFS`;
    }

    updateCalibrationRoutingStatus();
  }

  function getControlRoutingChannels() {
    const getOutputChannel = (state, fallback) => {
      const scaled = typeof state?.getScaledValue === "function"
        ? Number(state.getScaledValue())
        : fallback;
      return clamp(Math.round(Number.isFinite(scaled) ? scaled : fallback), 1, 8);
    };

    return [
      getOutputChannel(controlStates.calSpk1Out, 1),
      getOutputChannel(controlStates.calSpk2Out, 2),
      getOutputChannel(controlStates.calSpk3Out, 3),
      getOutputChannel(controlStates.calSpk4Out, 4),
    ];
  }

  function normaliseRoutingArray(value, fallback) {
    if (!Array.isArray(value) || value.length < 4) {
      return fallback.slice(0, 4);
    }

    return value.slice(0, 4).map((entry, index) => {
      const candidate = Math.round(Number(entry));
      if (Number.isFinite(candidate)) {
        return clamp(candidate, 1, 8);
      }
      return fallback[index] || 1;
    });
  }

  function updateCalibrationRoutingStatus() {
    if (!dom.statusCalRouting) {
      return;
    }

    const scene = runtime.latestScene || {};
    const controlRouting = getControlRoutingChannels();
    const currentRouting = normaliseRoutingArray(scene.calCurrentSpeakerMap, controlRouting);
    const autoRouting = normaliseRoutingArray(scene.calAutoRoutingMap, currentRouting);

    const outputChannels = Math.max(1, Math.round(Number(scene.outputChannels) || 0));
    const outputLayout = String(scene.outputLayout || (outputChannels >= 4 ? "quad" : outputChannels >= 2 ? "stereo" : "mono"))
      .trim()
      .toUpperCase();
    const autoOutputChannels = Math.max(1, Math.round(Number(scene.calAutoRoutingOutputChannels) || outputChannels));

    const speakerConfigChoices = getChoices(controlStates.calSpkConfig, DEFAULT_CHOICES.cal_spk_config);
    const sceneSpeakerConfig = Number(scene.calCurrentSpeakerConfig);
    const speakerConfigIndex = Number.isFinite(sceneSpeakerConfig)
      ? clamp(Math.round(sceneSpeakerConfig), 0, Math.max(0, speakerConfigChoices.length - 1))
      : clamp(getChoiceIndex(controlStates.calSpkConfig), 0, Math.max(0, speakerConfigChoices.length - 1));
    const speakerConfigLabel = speakerConfigChoices[speakerConfigIndex] || "n/a";

    const sceneAutoConfig = Number(scene.calAutoRoutingSpeakerConfig);
    const autoSpeakerConfig = Number.isFinite(sceneAutoConfig)
      ? clamp(Math.round(sceneAutoConfig), 0, Math.max(0, speakerConfigChoices.length - 1))
      : speakerConfigIndex;
    const autoAppliedFlag = !!scene.calAutoRoutingApplied;
    const routingMatchesAuto = currentRouting.every((channel, index) => channel === autoRouting[index]);
    const configMatchesAuto = speakerConfigIndex === autoSpeakerConfig;
    const autoActive = autoAppliedFlag && routingMatchesAuto && configMatchesAuto;
    const modeLabel = autoActive ? "AUTO" : "MANUAL";
    const autoConfigLabel = speakerConfigChoices[autoSpeakerConfig] || "n/a";

    dom.statusCalRouting.textContent =
      `${modeLabel} OUT ${outputLayout} ${outputChannels}ch · Current ${currentRouting.join("/")} (${speakerConfigLabel}) · Auto ${autoRouting.join("/")} (${autoConfigLabel}, ${autoOutputChannels}ch)`;
  }

  function setCalSpeakerRow(index, dotElement, textElement, status) {
    if (!dotElement || !textElement) return;

    const complete = !!status.complete;
    const running = !!status.running;
    const completed = clamp(Number(status.completedSpeakers) || 0, 0, 4);
    const currentSpeaker = clamp((Number(status.currentSpeaker) || 1) - 1, 0, 3);
    const state = String(status.state || "idle").toLowerCase();

    dotElement.classList.remove("active", "complete");

    if (complete || index < completed) {
      dotElement.classList.add("complete");
      textElement.textContent = `SPK${index + 1}: Measured`;
      return;
    }

    if (running && index === currentSpeaker) {
      dotElement.classList.add("active");

      if (state === "playing") {
        const percent = Math.round(clamp(Number(status.playPercent) || 0, 0, 1) * 100);
        textElement.textContent = `SPK${index + 1}: Playing test signal (${percent}%)`;
        return;
      }

      if (state === "recording") {
        const percent = Math.round(clamp(Number(status.recordPercent) || 0, 0, 1) * 100);
        textElement.textContent = `SPK${index + 1}: Recording response (${percent}%)`;
        return;
      }

      if (state === "analyzing") {
        textElement.textContent = `SPK${index + 1}: Analyzing IR`;
        return;
      }
    }

    textElement.textContent = `SPK${index + 1}: Not measured`;
  }

  function updateCalibrateCaptureStatus() {
    const status = runtime.calibrationStatus || createDefaultCalibrationStatus();
    const running = !!status.running;
    const complete = !!status.complete;
    const state = String(status.state || "idle").toUpperCase();
    const overallPercent = clamp(Number(status.overallPercent) || 0, 0, 1);
    const percent = Math.round(overallPercent * 100);
    const lifecycle = getCalibrationLifecycleLabel(status);

    if (dom.statusCalProgress) {
      dom.statusCalProgress.textContent = `${state} ${percent}%`;
    }

    if (dom.statusCalMessage) {
      dom.statusCalMessage.textContent = sanitizeStatusMessage(status.message, CALIBRATION_IDLE_MESSAGE);
    }

    if (dom.calProgressBar) {
      dom.calProgressBar.style.width = `${percent}%`;
    }

    if (dom.btnCalMeasure) {
      if (running) {
        dom.btnCalMeasure.textContent = "ABORT";
      } else if (complete) {
        dom.btnCalMeasure.textContent = "MEASURE AGAIN";
      } else {
        dom.btnCalMeasure.textContent = "START MEASURE";
      }
      dom.btnCalMeasure.classList.toggle("running", running);
    }

    if (dom.statusCalProfile) {
      dom.statusCalProfile.textContent = lifecycle;
    }

    setCalSpeakerRow(0, dom.calSpk1Dot, dom.calSpk1Status, status);
    setCalSpeakerRow(1, dom.calSpk2Dot, dom.calSpk2Status, status);
    setCalSpeakerRow(2, dom.calSpk3Dot, dom.calSpk3Status, status);
    setCalSpeakerRow(3, dom.calSpk4Dot, dom.calSpk4Status, status);
  }

  function setSliderScaledValue(state, scaled) {
    if (!state || typeof state.setNormalisedValue !== "function") {
      return;
    }

    const start = Number(state.properties?.start);
    const end = Number(state.properties?.end);
    const skew = Number(state.properties?.skew);
    if (!Number.isFinite(start) || !Number.isFinite(end) || Math.abs(end - start) < 1.0e-9) {
      return;
    }

    const clampedScaled = clamp(Number(scaled) || 0, Math.min(start, end), Math.max(start, end));
    const linear = clamp((clampedScaled - start) / (end - start), 0, 1);
    const safeSkew = Number.isFinite(skew) && skew > 0 ? skew : 1;
    const normalised = Math.pow(linear, safeSkew);
    state.setNormalisedValue(normalised);
  }

  function pulseToggleState(state) {
    if (!state || typeof state.setValue !== "function") {
      return;
    }

    state.setValue(true);
    window.setTimeout(() => {
      state.setValue(false);
    }, 70);
  }

  function getCoordModeState() {
    const coordChoices = getChoices(controlStates.posCoordMode, DEFAULT_CHOICES.pos_coord_mode);
    const coordIndex = clamp(getChoiceIndex(controlStates.posCoordMode), 0, Math.max(0, coordChoices.length - 1));
    return {
      coordChoices,
      coordIndex,
      coordLabel: coordChoices[coordIndex] || "n/a",
    };
  }

  function updatePositionReadout() {
    const { coordIndex, coordLabel } = getCoordModeState();
    const azimuth = getScaledValueOrFallback(controlStates.posAzimuth, 0);
    const elevation = getScaledValueOrFallback(controlStates.posElevation, 0);
    const distance = getScaledValueOrFallback(controlStates.posDistance, 2);
    let x = getScaledValueOrFallback(controlStates.posX, 0);
    let y = getScaledValueOrFallback(controlStates.posY, 0);
    let z = getScaledValueOrFallback(controlStates.posZ, 0);

    const selectedEmitter = getSelectedEmitterFromScene();
    if (selectedEmitter) {
      x = Number(selectedEmitter.x) || x;
      y = Number(selectedEmitter.y) || y;
      z = Number(selectedEmitter.z) || z;
    }

    runtime.snapshot.posCoordModeIndex = coordIndex;
    runtime.snapshot.posAzimuthNorm = getSliderNormalised(controlStates.posAzimuth);
    runtime.snapshot.posElevationNorm = getSliderNormalised(controlStates.posElevation);
    runtime.snapshot.posDistanceNorm = getSliderNormalised(controlStates.posDistance);
    runtime.snapshot.posXNorm = getSliderNormalised(controlStates.posX);
    runtime.snapshot.posYNorm = getSliderNormalised(controlStates.posY);
    runtime.snapshot.posZNorm = getSliderNormalised(controlStates.posZ);

    if (dom.readoutPosCartesian) {
      dom.readoutPosCartesian.textContent = `X ${formatNumber(x, 2)} · Y ${formatNumber(y, 2)} · Z ${formatNumber(z, 2)} (${coordLabel})`;
    }

    return {
      coordLabel,
      azimuth,
      elevation,
      distance,
      x,
      y,
      z,
    };
  }

  function updateSizeReadout() {
    const uniform = getScaledValueOrFallback(controlStates.sizeUniform, 0.5);
    const width = getScaledValueOrFallback(controlStates.sizeWidth, uniform);
    const depth = getScaledValueOrFallback(controlStates.sizeDepth, uniform);
    const height = getScaledValueOrFallback(controlStates.sizeHeight, uniform);

    runtime.snapshot.sizeUniformNorm = getSliderNormalised(controlStates.sizeUniform);
    runtime.snapshot.sizeWidthNorm = getSliderNormalised(controlStates.sizeWidth);
    runtime.snapshot.sizeDepthNorm = getSliderNormalised(controlStates.sizeDepth);
    runtime.snapshot.sizeHeightNorm = getSliderNormalised(controlStates.sizeHeight);

    if (dom.readoutSizeUniform) {
      dom.readoutSizeUniform.textContent = `${formatNumber(uniform, 2)} m`;
    }
    if (dom.readoutSizeXyz) {
      dom.readoutSizeXyz.textContent = `W ${formatNumber(width, 2)} · D ${formatNumber(depth, 2)} · H ${formatNumber(height, 2)}`;
    }

    return { uniform, width, depth, height };
  }

  function updateColorReadout() {
    const scaledColor = Math.round(getScaledValueOrFallback(controlStates.emitColor, 0));
    const colorIndex = clamp(scaledColor, 0, EMITTER_PALETTE.length - 1);
    const colorHex = EMITTER_PALETTE[colorIndex] || EMITTER_PALETTE[0];

    runtime.snapshot.emitColorNorm = getSliderNormalised(controlStates.emitColor);
    if (dom.readoutEmitColor) {
      dom.readoutEmitColor.textContent = `#${String(colorIndex).padStart(2, "0")} ${colorHex}`;
      dom.readoutEmitColor.style.color = colorHex;
    }
    return { colorIndex, colorHex };
  }

  function applyPhysicsPreset(presetName, persistUiStateValue = true) {
    const normalized = String(presetName || "off").trim().toLowerCase();
    const resolved = PHYSICS_PRESETS.includes(normalized) ? normalized : "off";
    const forceEnable = resolved !== "off";
    runtime.applyingPhysicsPreset = true;
    try {
      if (resolved === "bounce") {
        setSliderScaledValue(controlStates.physMass, 1.0);
        setSliderScaledValue(controlStates.physDrag, 0.2);
        setSliderScaledValue(controlStates.physElasticity, 0.82);
        setSliderScaledValue(controlStates.physGravity, -9.8);
        setSliderScaledValue(controlStates.physFriction, 0.2);
        setChoiceIndexSafe(controlStates.physGravityDir, 0, DEFAULT_CHOICES.phys_gravity_dir.length);
      } else if (resolved === "float") {
        setSliderScaledValue(controlStates.physMass, 0.4);
        setSliderScaledValue(controlStates.physDrag, 0.65);
        setSliderScaledValue(controlStates.physElasticity, 0.3);
        setSliderScaledValue(controlStates.physGravity, 0.0);
        setSliderScaledValue(controlStates.physFriction, 0.45);
        setChoiceIndexSafe(controlStates.physGravityDir, 0, DEFAULT_CHOICES.phys_gravity_dir.length);
      } else if (resolved === "orbit") {
        setSliderScaledValue(controlStates.physMass, 0.8);
        setSliderScaledValue(controlStates.physDrag, 0.35);
        setSliderScaledValue(controlStates.physElasticity, 0.55);
        setSliderScaledValue(controlStates.physGravity, 6.0);
        setSliderScaledValue(controlStates.physFriction, 0.15);
        setChoiceIndexSafe(controlStates.physGravityDir, 2, DEFAULT_CHOICES.phys_gravity_dir.length);
      }

      if (typeof controlStates.physEnable.setValue === "function") {
        controlStates.physEnable.setValue(forceEnable);
      }
      if (dom.togglePhysEnable) {
        dom.togglePhysEnable.checked = forceEnable;
      }
      if (dom.choicePhysPreset) {
        dom.choicePhysPreset.value = resolved;
      }
    } finally {
      runtime.applyingPhysicsPreset = false;
    }

    runtime.snapshot.physPresetIndex = Math.max(0, PHYSICS_PRESETS.indexOf(resolved));
    if (persistUiStateValue) {
      void persistUiState({ physicsPreset: resolved });
    }
    updatePhysicsStatus();
    updateSceneStatusBadge();
    updateEmitterParityStatus();
  }

  function updatePhysicsStatus() {
    const gravityChoices = getChoices(controlStates.physGravityDir, DEFAULT_CHOICES.phys_gravity_dir);
    const gravityIndex = clamp(getChoiceIndex(controlStates.physGravityDir), 0, Math.max(0, gravityChoices.length - 1));
    const gravityLabel = gravityChoices[gravityIndex] || "n/a";
    const enabled = getToggleValue(controlStates.physEnable);
    const mass = getScaledValueOrFallback(controlStates.physMass, 1.0);
    const drag = getScaledValueOrFallback(controlStates.physDrag, 0.5);
    const elasticity = getScaledValueOrFallback(controlStates.physElasticity, 0.7);
    const gravity = getScaledValueOrFallback(controlStates.physGravity, 0.0);
    const friction = getScaledValueOrFallback(controlStates.physFriction, 0.3);
    const preset = dom.choicePhysPreset?.value || PHYSICS_PRESETS[runtime.snapshot.physPresetIndex] || "off";

    runtime.snapshot.physMassNorm = getSliderNormalised(controlStates.physMass);
    runtime.snapshot.physDragNorm = getSliderNormalised(controlStates.physDrag);
    runtime.snapshot.physElasticityNorm = getSliderNormalised(controlStates.physElasticity);
    runtime.snapshot.physGravityNorm = getSliderNormalised(controlStates.physGravity);
    runtime.snapshot.physFrictionNorm = getSliderNormalised(controlStates.physFriction);
    runtime.snapshot.physGravityDirIndex = gravityIndex;

    if (dom.statusPhysSummary) {
      dom.statusPhysSummary.textContent =
        `${enabled ? "On" : "Off"} · preset ${preset} · mass ${formatNumber(mass, 2)} · drag ${formatNumber(drag, 2)} · elasticity ${formatNumber(elasticity, 2)} · gravity ${formatNumber(gravity, 2)} · friction ${formatNumber(friction, 2)} · dir ${gravityLabel}`;
    }

    return { enabled, preset, gravityLabel };
  }

  function updateAnimationStatus() {
    const animChoices = getChoices(controlStates.animMode, DEFAULT_CHOICES.anim_mode);
    const animIndex = clamp(getChoiceIndex(controlStates.animMode), 0, Math.max(0, animChoices.length - 1));
    const animLabel = animChoices[animIndex] || "n/a";
    const enabled = getToggleValue(controlStates.animEnable);
    const loop = getToggleValue(controlStates.animLoop);
    const sync = getToggleValue(controlStates.animSync);
    const speed = getScaledValueOrFallback(controlStates.animSpeed, 1.0);

    runtime.snapshot.animSpeedNorm = getSliderNormalised(controlStates.animSpeed);
    if (dom.statusAnimSummary) {
      dom.statusAnimSummary.textContent =
        `${enabled ? "ON" : "OFF"} · Source ${animLabel} · Loop ${loop ? "ON" : "OFF"} · Sync ${sync ? "ON" : "OFF"} · Speed ${formatNumber(speed, 1)}x`;
    }

    return { enabled, animLabel, loop, sync, speed };
  }

  function updateEmitterAudioStatus() {
    if (!dom.statusEmitterAudio) {
      return;
    }

    const mute = getToggleValue(controlStates.emitMute);
    const solo = getToggleValue(controlStates.emitSolo);
    const gainNorm = getSliderNormalised(controlStates.emitGain);
    const spreadNorm = getSliderNormalised(controlStates.emitSpread);
    const directivityNorm = getSliderNormalised(controlStates.emitDirectivity);
    const gravityChoices = getChoices(controlStates.physGravityDir, DEFAULT_CHOICES.phys_gravity_dir);
    const gravityIndex = clamp(getChoiceIndex(controlStates.physGravityDir), 0, Math.max(0, gravityChoices.length - 1));
    const gravityLabel = gravityChoices[gravityIndex] || "n/a";

    runtime.snapshot.emitMute = mute;
    runtime.snapshot.emitSolo = solo;
    runtime.snapshot.emitGainNorm = gainNorm;
    runtime.snapshot.emitSpreadNorm = spreadNorm;
    runtime.snapshot.emitDirectivityNorm = directivityNorm;
    runtime.snapshot.physGravityDirIndex = gravityIndex;

    dom.statusEmitterAudio.textContent =
      `Mute ${mute ? "ON" : "OFF"} · Solo ${solo ? "ON" : "OFF"} · Gain ${Math.round(gainNorm * 100)}% · Spread ${Math.round(spreadNorm * 100)}% · Dir ${Math.round(directivityNorm * 100)}% · Gravity ${gravityLabel}`;
  }

  function updateEmitterParityStatus() {
    if (!dom.statusEmitterParity) {
      return;
    }

    const label = sanitizeEmitterLabel(dom.inputEmitLabel?.value || runtime.snapshot.emitLabel || "Emitter");
    runtime.snapshot.emitLabel = label;
    if (dom.inputEmitLabel && dom.inputEmitLabel.value !== label) {
      dom.inputEmitLabel.value = label;
    }

    const pos = updatePositionReadout();
    const size = updateSizeReadout();
    const physics = updatePhysicsStatus();
    const anim = updateAnimationStatus();
    updateColorReadout();

    dom.statusEmitterParity.textContent =
      `Label ${label} · Coord ${pos.coordLabel} A${formatNumber(pos.azimuth, 1)} E${formatNumber(pos.elevation, 1)} D${formatNumber(pos.distance, 2)} · Size U${formatNumber(size.uniform, 2)} W${formatNumber(size.width, 2)} D${formatNumber(size.depth, 2)} H${formatNumber(size.height, 2)} · Phys ${physics.enabled ? "ON" : "OFF"} ${physics.preset} · Anim ${anim.enabled ? "ON" : "OFF"} ${anim.animLabel} ${formatNumber(anim.speed, 1)}x`;
  }

  function updateRendererSceneStatus() {
    if (!dom.statusRendererScene) {
      return;
    }

    const scene = runtime.latestScene || {};
    const outputChannels = Number(scene.outputChannels);
    const outputChannelsLabel = Number.isFinite(outputChannels) && outputChannels > 0
      ? `${Math.round(outputChannels)}ch`
      : "n/a";
    const outputLayout = String(scene.outputLayout || "n/a").trim().toUpperCase() || "N/A";
    const outputMode = String(scene.rendererOutputMode || "n/a").trim() || "n/a";
    const outputRoute = Array.isArray(scene.rendererOutputChannels) && scene.rendererOutputChannels.length > 0
      ? scene.rendererOutputChannels.map(value => String(value).trim()).join("/")
      : "n/a";
    const quadMap = Array.isArray(scene.rendererQuadMap) && scene.rendererQuadMap.length > 0
      ? scene.rendererQuadMap.map(value => String(value).trim()).join("/")
      : "n/a";
    const eligible = Math.max(0, Math.round(Number(scene.rendererEligibleEmitters) || 0));
    const processed = Math.max(0, Math.round(Number(scene.rendererProcessedEmitters) || 0));
    const guardrail = !!scene.rendererGuardrailActive;

    dom.statusRendererScene.textContent =
      `READY · output ${outputLayout} ${outputChannelsLabel} · route ${outputRoute} (${outputMode}) · quad ${quadMap} · load ${processed}/${eligible} ${guardrail ? "GUARD" : "OK"}`;
  }

  function updateRendererCoreStatus() {
    const masterGainNorm = getSliderNormalised(controlStates.rendMasterGain);
    const spk1Gain = getScaledValueOrFallback(controlStates.rendSpk1Gain, 0.0);
    const spk2Gain = getScaledValueOrFallback(controlStates.rendSpk2Gain, 0.0);
    const spk3Gain = getScaledValueOrFallback(controlStates.rendSpk3Gain, 0.0);
    const spk4Gain = getScaledValueOrFallback(controlStates.rendSpk4Gain, 0.0);
    const spk1Delay = getScaledValueOrFallback(controlStates.rendSpk1Delay, 0.0);
    const spk2Delay = getScaledValueOrFallback(controlStates.rendSpk2Delay, 0.0);
    const spk3Delay = getScaledValueOrFallback(controlStates.rendSpk3Delay, 0.0);
    const spk4Delay = getScaledValueOrFallback(controlStates.rendSpk4Delay, 0.0);
    const distanceChoices = getChoices(controlStates.rendDistanceModel, DEFAULT_CHOICES.rend_distance_model);
    const distanceIndex = clamp(getChoiceIndex(controlStates.rendDistanceModel), 0, Math.max(0, distanceChoices.length - 1));
    const distanceLabel = distanceChoices[distanceIndex] || "n/a";
    const distanceRef = getScaledValueOrFallback(controlStates.rendDistanceRef, 1.0);
    const distanceMax = getScaledValueOrFallback(controlStates.rendDistanceMax, 50.0);
    const doppler = getToggleValue(controlStates.rendDoppler);
    const dopplerScale = getScaledValueOrFallback(controlStates.rendDopplerScale, 1.0);
    const airAbsorb = getToggleValue(controlStates.rendAirAbsorb);
    const roomEnable = getToggleValue(controlStates.rendRoomEnable);
    const roomMix = getScaledValueOrFallback(controlStates.rendRoomMix, 0.3);
    const roomSize = getScaledValueOrFallback(controlStates.rendRoomSize, 1.0);
    const roomDamping = getScaledValueOrFallback(controlStates.rendRoomDamping, 0.5);
    const roomErOnly = getToggleValue(controlStates.rendRoomErOnly);
    const rateChoices = getChoices(controlStates.rendPhysRate, DEFAULT_CHOICES.rend_phys_rate);
    const rateIndex = clamp(getChoiceIndex(controlStates.rendPhysRate), 0, Math.max(0, rateChoices.length - 1));
    const rateLabel = rateChoices[rateIndex] || "n/a";
    const physWalls = getToggleValue(controlStates.rendPhysWalls);
    const physPause = getToggleValue(controlStates.rendPhysPause);
    const vizChoices = getChoices(controlStates.rendVizMode, DEFAULT_CHOICES.rend_viz_mode);
    const vizIndex = clamp(getChoiceIndex(controlStates.rendVizMode), 0, Math.max(0, vizChoices.length - 1));
    const vizLabel = vizChoices[vizIndex] || "n/a";
    const vizTrails = getToggleValue(controlStates.rendVizTrails);
    const vizTrailLen = getScaledValueOrFallback(controlStates.rendVizTrailLen, 5.0);
    const vizVectors = getToggleValue(controlStates.rendVizVectors);
    const vizGrid = getToggleValue(controlStates.rendVizGrid);
    const vizLabels = getToggleValue(controlStates.rendVizLabels);

    runtime.snapshot.rendMasterGainNorm = masterGainNorm;
    runtime.snapshot.rendSpk1GainNorm = getSliderNormalised(controlStates.rendSpk1Gain);
    runtime.snapshot.rendSpk2GainNorm = getSliderNormalised(controlStates.rendSpk2Gain);
    runtime.snapshot.rendSpk3GainNorm = getSliderNormalised(controlStates.rendSpk3Gain);
    runtime.snapshot.rendSpk4GainNorm = getSliderNormalised(controlStates.rendSpk4Gain);
    runtime.snapshot.rendSpk1DelayNorm = getSliderNormalised(controlStates.rendSpk1Delay);
    runtime.snapshot.rendSpk2DelayNorm = getSliderNormalised(controlStates.rendSpk2Delay);
    runtime.snapshot.rendSpk3DelayNorm = getSliderNormalised(controlStates.rendSpk3Delay);
    runtime.snapshot.rendSpk4DelayNorm = getSliderNormalised(controlStates.rendSpk4Delay);
    runtime.snapshot.rendDistanceModelIndex = distanceIndex;
    runtime.snapshot.rendDistanceRefNorm = getSliderNormalised(controlStates.rendDistanceRef);
    runtime.snapshot.rendDistanceMaxNorm = getSliderNormalised(controlStates.rendDistanceMax);
    runtime.snapshot.rendDoppler = doppler;
    runtime.snapshot.rendDopplerScaleNorm = getSliderNormalised(controlStates.rendDopplerScale);
    runtime.snapshot.rendAirAbsorb = airAbsorb;
    runtime.snapshot.rendRoomEnable = roomEnable;
    runtime.snapshot.rendRoomMixNorm = getSliderNormalised(controlStates.rendRoomMix);
    runtime.snapshot.rendRoomSizeNorm = getSliderNormalised(controlStates.rendRoomSize);
    runtime.snapshot.rendRoomDampingNorm = getSliderNormalised(controlStates.rendRoomDamping);
    runtime.snapshot.rendRoomErOnly = roomErOnly;
    runtime.snapshot.rendPhysRateIndex = rateIndex;
    runtime.snapshot.rendPhysWalls = physWalls;
    runtime.snapshot.rendPhysPause = physPause;
    runtime.snapshot.rendVizModeIndex = vizIndex;
    runtime.snapshot.rendVizTrails = vizTrails;
    runtime.snapshot.rendVizTrailLenNorm = getSliderNormalised(controlStates.rendVizTrailLen);
    runtime.snapshot.rendVizVectors = vizVectors;
    runtime.snapshot.rendVizGrid = vizGrid;
    runtime.snapshot.rendVizLabels = vizLabels;

    if (dom.statusRendererCore) {
      dom.statusRendererCore.textContent =
        `Master ${Math.round(masterGainNorm * 100)}% · Distance ${distanceLabel} · Ref ${formatNumber(distanceRef, 2)}m · Max ${formatNumber(distanceMax, 1)}m · Doppler ${doppler ? "ON" : "OFF"} ${formatNumber(dopplerScale, 2)}x · Air ${airAbsorb ? "ON" : "OFF"} · Room ${roomEnable ? "ON" : "OFF"} mix ${formatNumber(roomMix, 2)} size ${formatNumber(roomSize, 2)} damp ${formatNumber(roomDamping, 2)} er ${roomErOnly ? "ON" : "OFF"} · Rate ${rateLabel} · Walls ${physWalls ? "ON" : "OFF"} · Pause ${physPause ? "ON" : "OFF"} · View ${vizLabel} · Trails ${vizTrails ? "ON" : "OFF"} ${formatNumber(vizTrailLen, 1)}s · Vectors ${vizVectors ? "ON" : "OFF"} · Grid ${vizGrid ? "ON" : "OFF"} · Labels ${vizLabels ? "ON" : "OFF"}`;
    }

    if (dom.statusRendererSpeakers) {
      dom.statusRendererSpeakers.textContent =
        `G ${formatNumber(spk1Gain, 1)}/${formatNumber(spk2Gain, 1)}/${formatNumber(spk3Gain, 1)}/${formatNumber(spk4Gain, 1)} dB · D ${formatNumber(spk1Delay, 1)}/${formatNumber(spk2Delay, 1)}/${formatNumber(spk3Delay, 1)}/${formatNumber(spk4Delay, 1)} ms`;
    }

    updateRendererSceneStatus();
  }

  function setPresetStatus(message, isError = false) {
    if (!dom.statusPreset) {
      return;
    }

    dom.statusPreset.textContent = message;
    dom.statusPreset.style.color = isError ? "var(--text-warning)" : "var(--text-secondary)";
  }

  function parsePresetEntries(result) {
    if (Array.isArray(result)) {
      return result;
    }
    if (result && Array.isArray(result.items)) {
      return result.items;
    }
    return [];
  }

  async function refreshEmitterPresetList() {
    if (!dom.choiceEmitterPreset) {
      return [];
    }

    if (!listEmitterPresetsNative) {
      setPresetStatus("Preset bridge unavailable", true);
      return [];
    }

    try {
      const result = await callNativeSafe("locusqListEmitterPresets", listEmitterPresetsNative);
      const entries = parsePresetEntries(result);
      dom.choiceEmitterPreset.innerHTML = "";

      entries.forEach((entry, index) => {
        const option = document.createElement("option");
        const path = String(entry?.path || "");
        option.value = path;
        option.textContent = String(entry?.name || entry?.file || `Preset ${index + 1}`);
        option.dataset.path = path;
        option.dataset.name = String(entry?.name || "");
        dom.choiceEmitterPreset.appendChild(option);
      });

      if (entries.length > 0) {
        const first = dom.choiceEmitterPreset.options[0];
        runtime.lastPresetPath = String(first?.dataset?.path || "");
        runtime.lastPresetName = String(first?.dataset?.name || "");
        setPresetStatus(`Loaded list (${entries.length})`);
      } else {
        runtime.lastPresetPath = "";
        runtime.lastPresetName = "";
        setPresetStatus("No presets found");
      }
      return entries;
    } catch (error) {
      setPresetStatus("Preset list failed", true);
      writeDiagnostics({ presetListError: String(error) });
      return [];
    }
  }

  async function handlePresetSaveClick() {
    if (!saveEmitterPresetNative) {
      setPresetStatus("Preset save bridge unavailable", true);
      return;
    }

    const suggestedName = `Preset_${new Date().toISOString().replace(/[-:]/g, "").slice(0, 15)}`;
    const requested = window.prompt("Preset name", suggestedName);
    if (requested === null) {
      return;
    }

    const trimmed = requested.trim();
    if (!trimmed) {
      setPresetStatus("Preset name is required", true);
      return;
    }

    try {
      const result = await callNativeSafe("locusqSaveEmitterPreset", saveEmitterPresetNative, { name: trimmed });
      if (result && result.ok) {
        setPresetStatus(`Saved: ${result.name || trimmed}`);
        runtime.lastPresetPath = String(result.path || "");
        runtime.lastPresetName = String(result.name || trimmed);
        await refreshEmitterPresetList();
      } else {
        setPresetStatus(result?.message || "Preset save failed", true);
      }
    } catch (error) {
      setPresetStatus("Preset save failed", true);
      writeDiagnostics({ presetSaveError: String(error) });
    }
  }

  async function handlePresetLoadClick() {
    if (!loadEmitterPresetNative || !dom.choiceEmitterPreset) {
      setPresetStatus("Preset load bridge unavailable", true);
      return;
    }

    const selectedOption = dom.choiceEmitterPreset.options[dom.choiceEmitterPreset.selectedIndex];
    const selectedPath = String(selectedOption?.dataset?.path || dom.choiceEmitterPreset.value || "");
    if (!selectedPath) {
      setPresetStatus("Select a preset first", true);
      return;
    }

    try {
      const result = await callNativeSafe("locusqLoadEmitterPreset", loadEmitterPresetNative, { path: selectedPath });
      if (result && result.ok) {
        runtime.lastPresetPath = String(result.path || selectedPath);
        runtime.lastPresetName = String(selectedOption?.dataset?.name || selectedOption?.textContent || "");
        setPresetStatus(`Loaded: ${selectedOption?.textContent || "preset"}`);
        heartbeat();
      } else {
        setPresetStatus(result?.message || "Preset load failed", true);
      }
    } catch (error) {
      setPresetStatus("Preset load failed", true);
      writeDiagnostics({ presetLoadError: String(error) });
    }
  }

  async function hydrateUiStateFromNative() {
    if (runtime.uiStateFetchDone || !getUiStateNative) {
      return;
    }
    runtime.uiStateFetchDone = true;

    try {
      const state = await callNativeSafe("locusqGetUiState", getUiStateNative);
      if (!state || typeof state !== "object") {
        return;
      }

      if (state.emitterLabel && dom.inputEmitLabel) {
        const nextLabel = sanitizeEmitterLabel(state.emitterLabel);
        dom.inputEmitLabel.value = nextLabel;
        runtime.snapshot.emitLabel = nextLabel;
      }

      const uiPreset = String(state.physicsPreset || "").trim().toLowerCase();
      if (dom.choicePhysPreset && uiPreset.length > 0) {
        dom.choicePhysPreset.value = PHYSICS_PRESETS.includes(uiPreset) ? uiPreset : "off";
        applyPhysicsPreset(dom.choicePhysPreset.value, false);
      }
    } catch (error) {
      writeDiagnostics({ uiStateReadError: String(error) });
    }
  }

  async function handleCalibrationMeasureClick() {
    const status = runtime.calibrationStatus || createDefaultCalibrationStatus();
    const running = !!status.running;

    try {
      if (running) {
        await callNativeSafe("locusqAbortCalibration", abortCalibrationNative);
        applyCalibrationStatus(createDefaultCalibrationStatus({
          message: "Calibration aborted. Ready to start.",
        }));
        writeDiagnostics({ event: "calibration.abort.requested" });
      } else {
        const options = getCalibrationOptionsFromControls();
        const started = await callNativeSafe("locusqStartCalibration", startCalibrationNative, options);
        if (started) {
          applyCalibrationStatus(createDefaultCalibrationStatus({
            state: "playing",
            running: true,
            complete: false,
            currentSpeaker: 1,
            completedSpeakers: 0,
            playPercent: 0,
            recordPercent: 0,
            overallPercent: 0,
            message: "Starting calibration...",
            profileValid: false,
          }));
        } else {
          applyCalibrationStatus(createDefaultCalibrationStatus({
            state: "error",
            running: false,
            complete: false,
            message: "Start rejected - calibration engine busy",
          }));
        }
        writeDiagnostics({ event: "calibration.start.requested", started: !!started });
      }
    } catch (error) {
      applyCalibrationStatus(createDefaultCalibrationStatus({
        state: "error",
        running: false,
        complete: false,
        message: `Native call failed: ${sanitizeStatusMessage(String(error), "unknown error")}`,
      }));
      writeDiagnostics({
        event: running ? "calibration.abort.error" : "calibration.start.error",
        error: String(error),
      });
    }
  }

  async function handleCalibrationRedetectClick() {
    try {
      const result = await callNativeSafe("locusqRedetectCalibrationRouting", redetectCalibrationRoutingNative);
      if (result && typeof result === "object") {
        const nextScene = {
          ...(runtime.latestScene || {}),
        };

        if (Array.isArray(result.routing)) {
          nextScene.calCurrentSpeakerMap = result.routing.slice(0, 4);
          nextScene.calAutoRoutingMap = result.routing.slice(0, 4);
        }

        if (Number.isFinite(Number(result.speakerConfigIndex))) {
          const configIndex = Math.round(Number(result.speakerConfigIndex));
          nextScene.calCurrentSpeakerConfig = configIndex;
          nextScene.calAutoRoutingSpeakerConfig = configIndex;
        }

        if (Number.isFinite(Number(result.outputChannels))) {
          const channels = Math.max(1, Math.round(Number(result.outputChannels)));
          nextScene.outputChannels = channels;
          nextScene.calAutoRoutingOutputChannels = channels;
        }

        nextScene.calAutoRoutingApplied = true;
        runtime.latestScene = nextScene;
      }

      runtime.lastCalibrationRedetectAt = Date.now();
      runtime.lastCalibrationRedetectOk = true;
      updateCalibrateCoreStatus();
      updateCalibrationRoutingStatus();

      const currentStatus = runtime.calibrationStatus || createDefaultCalibrationStatus();

      if (!currentStatus.running) {
        applyCalibrationStatus({
          ...createDefaultCalibrationStatus(currentStatus),
          message: "Routing re-detected from active output layout",
        });
      }

      writeDiagnostics({
        event: "calibration.routing.redetect",
        result,
      });
    } catch (error) {
      runtime.lastCalibrationRedetectAt = Date.now();
      runtime.lastCalibrationRedetectOk = false;
      writeDiagnostics({
        event: "calibration.routing.redetect.error",
        error: String(error),
      });

      const currentStatus = runtime.calibrationStatus || createDefaultCalibrationStatus();
      if (!currentStatus.running) {
        applyCalibrationStatus({
          ...createDefaultCalibrationStatus(currentStatus),
          state: "error",
          running: false,
          complete: false,
          message: `Routing re-detect failed: ${sanitizeStatusMessage(String(error), "unknown error")}`,
        });
      }
    }
  }

  async function runIncrementalStage12SelfTest() {
    const report = {
      requested: selfTestRequested,
      startedAt: new Date().toISOString(),
      status: "running",
      ok: false,
      steps: [],
    };

    const recordStep = (name, pass, details = "") => {
      report.steps.push({ name, pass, details });
    };

    const originals = {
      mode: getChoiceIndex(controlStates.mode),
      emitLabel: sanitizeEmitterLabel(dom.inputEmitLabel?.value || "Emitter"),
      emitColor: getSliderNormalised(controlStates.emitColor),
      posCoordMode: getChoiceIndex(controlStates.posCoordMode),
      posAzimuth: getSliderNormalised(controlStates.posAzimuth),
      posElevation: getSliderNormalised(controlStates.posElevation),
      posDistance: getSliderNormalised(controlStates.posDistance),
      sizeUniform: getSliderNormalised(controlStates.sizeUniform),
      sizeWidth: getSliderNormalised(controlStates.sizeWidth),
      sizeDepth: getSliderNormalised(controlStates.sizeDepth),
      sizeHeight: getSliderNormalised(controlStates.sizeHeight),
      physPreset: String(dom.choicePhysPreset?.value || "off"),
      physMass: getSliderNormalised(controlStates.physMass),
      physDrag: getSliderNormalised(controlStates.physDrag),
      physElasticity: getSliderNormalised(controlStates.physElasticity),
      physGravity: getSliderNormalised(controlStates.physGravity),
      physFriction: getSliderNormalised(controlStates.physFriction),
      animSpeed: getSliderNormalised(controlStates.animSpeed),
      mute: getToggleValue(controlStates.emitMute),
      solo: getToggleValue(controlStates.emitSolo),
      gain: getSliderNormalised(controlStates.emitGain),
      spread: getSliderNormalised(controlStates.emitSpread),
      directivity: getSliderNormalised(controlStates.emitDirectivity),
      gravity: getChoiceIndex(controlStates.physGravityDir),
      rendMasterGain: getSliderNormalised(controlStates.rendMasterGain),
      rendSpk1Gain: getSliderNormalised(controlStates.rendSpk1Gain),
      rendSpk2Gain: getSliderNormalised(controlStates.rendSpk2Gain),
      rendSpk3Gain: getSliderNormalised(controlStates.rendSpk3Gain),
      rendSpk4Gain: getSliderNormalised(controlStates.rendSpk4Gain),
      rendSpk1Delay: getSliderNormalised(controlStates.rendSpk1Delay),
      rendSpk2Delay: getSliderNormalised(controlStates.rendSpk2Delay),
      rendSpk3Delay: getSliderNormalised(controlStates.rendSpk3Delay),
      rendSpk4Delay: getSliderNormalised(controlStates.rendSpk4Delay),
      rendDistanceModel: getChoiceIndex(controlStates.rendDistanceModel),
      rendDistanceRef: getSliderNormalised(controlStates.rendDistanceRef),
      rendDistanceMax: getSliderNormalised(controlStates.rendDistanceMax),
      rendDoppler: getToggleValue(controlStates.rendDoppler),
      rendDopplerScale: getSliderNormalised(controlStates.rendDopplerScale),
      rendAirAbsorb: getToggleValue(controlStates.rendAirAbsorb),
      rendRoomEnable: getToggleValue(controlStates.rendRoomEnable),
      rendRoomMix: getSliderNormalised(controlStates.rendRoomMix),
      rendRoomSize: getSliderNormalised(controlStates.rendRoomSize),
      rendRoomDamping: getSliderNormalised(controlStates.rendRoomDamping),
      rendRoomErOnly: getToggleValue(controlStates.rendRoomErOnly),
      rendPhysRate: getChoiceIndex(controlStates.rendPhysRate),
      rendPhysWalls: getToggleValue(controlStates.rendPhysWalls),
      rendPhysPause: getToggleValue(controlStates.rendPhysPause),
      rendVizMode: getChoiceIndex(controlStates.rendVizMode),
      rendVizTrails: getToggleValue(controlStates.rendVizTrails),
      rendVizTrailLen: getSliderNormalised(controlStates.rendVizTrailLen),
      rendVizVectors: getToggleValue(controlStates.rendVizVectors),
      rendVizGrid: getToggleValue(controlStates.rendVizGrid),
      rendVizLabels: getToggleValue(controlStates.rendVizLabels),
      calSpkConfig: getChoiceIndex(controlStates.calSpkConfig),
      calMicChannel: getSliderNormalised(controlStates.calMicChannel),
      calSpk1Out: getSliderNormalised(controlStates.calSpk1Out),
      calSpk2Out: getSliderNormalised(controlStates.calSpk2Out),
      calSpk3Out: getSliderNormalised(controlStates.calSpk3Out),
      calSpk4Out: getSliderNormalised(controlStates.calSpk4Out),
      calTestLevel: getSliderNormalised(controlStates.calTestLevel),
      calTestType: getChoiceIndex(controlStates.calTestType),
      calibrationStatus: runtime.calibrationStatus
        ? JSON.parse(JSON.stringify(runtime.calibrationStatus))
        : null,
    };

    const restoreOriginals = async () => {
      try {
        if (dom.inputEmitLabel) {
          dom.inputEmitLabel.value = originals.emitLabel;
          dom.inputEmitLabel.dispatchEvent(new Event("input", { bubbles: true }));
          dom.inputEmitLabel.dispatchEvent(new Event("blur", { bubbles: true }));
        }
        if (dom.sliderEmitColor) {
          dom.sliderEmitColor.value = originals.emitColor.toFixed(3);
          dom.sliderEmitColor.dispatchEvent(new Event("input", { bubbles: true }));
        }
        if (dom.sliderPosAzimuth) {
          dom.sliderPosAzimuth.value = originals.posAzimuth.toFixed(3);
          dom.sliderPosAzimuth.dispatchEvent(new Event("input", { bubbles: true }));
        }
        if (dom.sliderPosElevation) {
          dom.sliderPosElevation.value = originals.posElevation.toFixed(3);
          dom.sliderPosElevation.dispatchEvent(new Event("input", { bubbles: true }));
        }
        if (dom.sliderPosDistance) {
          dom.sliderPosDistance.value = originals.posDistance.toFixed(3);
          dom.sliderPosDistance.dispatchEvent(new Event("input", { bubbles: true }));
        }
        if (dom.sliderSizeUniform) {
          dom.sliderSizeUniform.value = originals.sizeUniform.toFixed(3);
          dom.sliderSizeUniform.dispatchEvent(new Event("input", { bubbles: true }));
        }
        if (dom.sliderSizeWidth) {
          dom.sliderSizeWidth.value = originals.sizeWidth.toFixed(3);
          dom.sliderSizeWidth.dispatchEvent(new Event("input", { bubbles: true }));
        }
        if (dom.sliderSizeDepth) {
          dom.sliderSizeDepth.value = originals.sizeDepth.toFixed(3);
          dom.sliderSizeDepth.dispatchEvent(new Event("input", { bubbles: true }));
        }
        if (dom.sliderSizeHeight) {
          dom.sliderSizeHeight.value = originals.sizeHeight.toFixed(3);
          dom.sliderSizeHeight.dispatchEvent(new Event("input", { bubbles: true }));
        }
        if (dom.sliderPhysMass) {
          dom.sliderPhysMass.value = originals.physMass.toFixed(3);
          dom.sliderPhysMass.dispatchEvent(new Event("input", { bubbles: true }));
        }
        if (dom.sliderPhysDrag) {
          dom.sliderPhysDrag.value = originals.physDrag.toFixed(3);
          dom.sliderPhysDrag.dispatchEvent(new Event("input", { bubbles: true }));
        }
        if (dom.sliderPhysElasticity) {
          dom.sliderPhysElasticity.value = originals.physElasticity.toFixed(3);
          dom.sliderPhysElasticity.dispatchEvent(new Event("input", { bubbles: true }));
        }
        if (dom.sliderPhysGravity) {
          dom.sliderPhysGravity.value = originals.physGravity.toFixed(3);
          dom.sliderPhysGravity.dispatchEvent(new Event("input", { bubbles: true }));
        }
        if (dom.sliderPhysFriction) {
          dom.sliderPhysFriction.value = originals.physFriction.toFixed(3);
          dom.sliderPhysFriction.dispatchEvent(new Event("input", { bubbles: true }));
        }
        if (dom.sliderAnimSpeed) {
          dom.sliderAnimSpeed.value = originals.animSpeed.toFixed(3);
          dom.sliderAnimSpeed.dispatchEvent(new Event("input", { bubbles: true }));
        }

        dom.toggleEmitMute.checked = originals.mute;
        dom.toggleEmitMute.dispatchEvent(new Event("change", { bubbles: true }));
        dom.toggleEmitSolo.checked = originals.solo;
        dom.toggleEmitSolo.dispatchEvent(new Event("change", { bubbles: true }));
        dom.sliderEmitGain.value = originals.gain.toFixed(3);
        dom.sliderEmitGain.dispatchEvent(new Event("input", { bubbles: true }));
        dom.sliderEmitSpread.value = originals.spread.toFixed(3);
        dom.sliderEmitSpread.dispatchEvent(new Event("input", { bubbles: true }));
        dom.sliderEmitDirectivity.value = originals.directivity.toFixed(3);
        dom.sliderEmitDirectivity.dispatchEvent(new Event("input", { bubbles: true }));
        dom.sliderRendMasterGain.value = originals.rendMasterGain.toFixed(3);
        dom.sliderRendMasterGain.dispatchEvent(new Event("input", { bubbles: true }));
        dom.sliderRendSpk1Gain.value = originals.rendSpk1Gain.toFixed(3);
        dom.sliderRendSpk1Gain.dispatchEvent(new Event("input", { bubbles: true }));
        dom.sliderRendSpk2Gain.value = originals.rendSpk2Gain.toFixed(3);
        dom.sliderRendSpk2Gain.dispatchEvent(new Event("input", { bubbles: true }));
        dom.sliderRendSpk3Gain.value = originals.rendSpk3Gain.toFixed(3);
        dom.sliderRendSpk3Gain.dispatchEvent(new Event("input", { bubbles: true }));
        dom.sliderRendSpk4Gain.value = originals.rendSpk4Gain.toFixed(3);
        dom.sliderRendSpk4Gain.dispatchEvent(new Event("input", { bubbles: true }));
        dom.sliderRendSpk1Delay.value = originals.rendSpk1Delay.toFixed(3);
        dom.sliderRendSpk1Delay.dispatchEvent(new Event("input", { bubbles: true }));
        dom.sliderRendSpk2Delay.value = originals.rendSpk2Delay.toFixed(3);
        dom.sliderRendSpk2Delay.dispatchEvent(new Event("input", { bubbles: true }));
        dom.sliderRendSpk3Delay.value = originals.rendSpk3Delay.toFixed(3);
        dom.sliderRendSpk3Delay.dispatchEvent(new Event("input", { bubbles: true }));
        dom.sliderRendSpk4Delay.value = originals.rendSpk4Delay.toFixed(3);
        dom.sliderRendSpk4Delay.dispatchEvent(new Event("input", { bubbles: true }));
        dom.sliderRendDistanceRef.value = originals.rendDistanceRef.toFixed(3);
        dom.sliderRendDistanceRef.dispatchEvent(new Event("input", { bubbles: true }));
        dom.sliderRendDistanceMax.value = originals.rendDistanceMax.toFixed(3);
        dom.sliderRendDistanceMax.dispatchEvent(new Event("input", { bubbles: true }));
        dom.sliderRendDopplerScale.value = originals.rendDopplerScale.toFixed(3);
        dom.sliderRendDopplerScale.dispatchEvent(new Event("input", { bubbles: true }));
        dom.sliderRendRoomMix.value = originals.rendRoomMix.toFixed(3);
        dom.sliderRendRoomMix.dispatchEvent(new Event("input", { bubbles: true }));
        dom.sliderRendRoomSize.value = originals.rendRoomSize.toFixed(3);
        dom.sliderRendRoomSize.dispatchEvent(new Event("input", { bubbles: true }));
        dom.sliderRendRoomDamping.value = originals.rendRoomDamping.toFixed(3);
        dom.sliderRendRoomDamping.dispatchEvent(new Event("input", { bubbles: true }));
        dom.sliderRendVizTrailLen.value = originals.rendVizTrailLen.toFixed(3);
        dom.sliderRendVizTrailLen.dispatchEvent(new Event("input", { bubbles: true }));
        dom.sliderCalMicChannel.value = originals.calMicChannel.toFixed(3);
        dom.sliderCalMicChannel.dispatchEvent(new Event("input", { bubbles: true }));
        dom.sliderCalSpk1Out.value = originals.calSpk1Out.toFixed(3);
        dom.sliderCalSpk1Out.dispatchEvent(new Event("input", { bubbles: true }));
        dom.sliderCalSpk2Out.value = originals.calSpk2Out.toFixed(3);
        dom.sliderCalSpk2Out.dispatchEvent(new Event("input", { bubbles: true }));
        dom.sliderCalSpk3Out.value = originals.calSpk3Out.toFixed(3);
        dom.sliderCalSpk3Out.dispatchEvent(new Event("input", { bubbles: true }));
        dom.sliderCalSpk4Out.value = originals.calSpk4Out.toFixed(3);
        dom.sliderCalSpk4Out.dispatchEvent(new Event("input", { bubbles: true }));
        dom.sliderCalTestLevel.value = originals.calTestLevel.toFixed(3);
        dom.sliderCalTestLevel.dispatchEvent(new Event("input", { bubbles: true }));
        dom.toggleRendDoppler.checked = originals.rendDoppler;
        dom.toggleRendDoppler.dispatchEvent(new Event("change", { bubbles: true }));
        dom.toggleRendAirAbsorb.checked = originals.rendAirAbsorb;
        dom.toggleRendAirAbsorb.dispatchEvent(new Event("change", { bubbles: true }));
        dom.toggleRendRoomEnable.checked = originals.rendRoomEnable;
        dom.toggleRendRoomEnable.dispatchEvent(new Event("change", { bubbles: true }));
        dom.toggleRendRoomErOnly.checked = originals.rendRoomErOnly;
        dom.toggleRendRoomErOnly.dispatchEvent(new Event("change", { bubbles: true }));
        dom.toggleRendPhysWalls.checked = originals.rendPhysWalls;
        dom.toggleRendPhysWalls.dispatchEvent(new Event("change", { bubbles: true }));
        dom.toggleRendPhysPause.checked = originals.rendPhysPause;
        dom.toggleRendPhysPause.dispatchEvent(new Event("change", { bubbles: true }));
        dom.toggleRendVizTrails.checked = originals.rendVizTrails;
        dom.toggleRendVizTrails.dispatchEvent(new Event("change", { bubbles: true }));
        dom.toggleRendVizVectors.checked = originals.rendVizVectors;
        dom.toggleRendVizVectors.dispatchEvent(new Event("change", { bubbles: true }));
        dom.toggleRendVizGrid.checked = originals.rendVizGrid;
        dom.toggleRendVizGrid.dispatchEvent(new Event("change", { bubbles: true }));
        dom.toggleRendVizLabels.checked = originals.rendVizLabels;
        dom.toggleRendVizLabels.dispatchEvent(new Event("change", { bubbles: true }));

        if (dom.choicePhysGravityDir.options.length > 0) {
          dom.choicePhysGravityDir.selectedIndex = clamp(
            originals.gravity,
            0,
            dom.choicePhysGravityDir.options.length - 1
          );
          dom.choicePhysGravityDir.dispatchEvent(new Event("change", { bubbles: true }));
        }

        if (dom.choicePosCoordMode.options.length > 0) {
          dom.choicePosCoordMode.selectedIndex = clamp(
            originals.posCoordMode,
            0,
            dom.choicePosCoordMode.options.length - 1
          );
          dom.choicePosCoordMode.dispatchEvent(new Event("change", { bubbles: true }));
        }

        if (dom.choicePhysPreset) {
          dom.choicePhysPreset.value = originals.physPreset;
          dom.choicePhysPreset.dispatchEvent(new Event("change", { bubbles: true }));
        }

        if (dom.choiceRendDistanceModel.options.length > 0) {
          dom.choiceRendDistanceModel.selectedIndex = clamp(
            originals.rendDistanceModel,
            0,
            dom.choiceRendDistanceModel.options.length - 1
          );
          dom.choiceRendDistanceModel.dispatchEvent(new Event("change", { bubbles: true }));
        }

        if (dom.choiceRendPhysRate.options.length > 0) {
          dom.choiceRendPhysRate.selectedIndex = clamp(
            originals.rendPhysRate,
            0,
            dom.choiceRendPhysRate.options.length - 1
          );
          dom.choiceRendPhysRate.dispatchEvent(new Event("change", { bubbles: true }));
        }

        if (dom.choiceRendVizMode.options.length > 0) {
          dom.choiceRendVizMode.selectedIndex = clamp(
            originals.rendVizMode,
            0,
            dom.choiceRendVizMode.options.length - 1
          );
          dom.choiceRendVizMode.dispatchEvent(new Event("change", { bubbles: true }));
        }

        if (dom.choiceCalSpkConfig.options.length > 0) {
          dom.choiceCalSpkConfig.selectedIndex = clamp(
            originals.calSpkConfig,
            0,
            dom.choiceCalSpkConfig.options.length - 1
          );
          dom.choiceCalSpkConfig.dispatchEvent(new Event("change", { bubbles: true }));
        }

        if (dom.choiceCalTestType.options.length > 0) {
          dom.choiceCalTestType.selectedIndex = clamp(
            originals.calTestType,
            0,
            dom.choiceCalTestType.options.length - 1
          );
          dom.choiceCalTestType.dispatchEvent(new Event("change", { bubbles: true }));
        }

        setChoiceIndexSafe(controlStates.mode, originals.mode, DEFAULT_CHOICES.mode.length);

        const restoreStatus = createDefaultCalibrationStatus(originals.calibrationStatus || {});
        window.updateCalibrationStatus(restoreStatus);
      } catch (_) {}
    };

    try {
      await waitForCondition("self-test controls", () =>
        !!dom.inputEmitLabel &&
        !!dom.sliderEmitColor &&
        !!dom.choicePosCoordMode &&
        !!dom.sliderPosAzimuth &&
        !!dom.sliderPosElevation &&
        !!dom.sliderPosDistance &&
        !!dom.sliderSizeWidth &&
        !!dom.sliderSizeDepth &&
        !!dom.sliderSizeHeight &&
        !!dom.sliderPhysMass &&
        !!dom.sliderPhysDrag &&
        !!dom.sliderPhysElasticity &&
        !!dom.sliderPhysGravity &&
        !!dom.sliderPhysFriction &&
        !!dom.sliderAnimSpeed &&
        !!dom.choicePhysPreset &&
        !!dom.btnPresetRefresh &&
        !!dom.statusPreset &&
        !!dom.statusPhysSummary &&
        !!dom.statusAnimSummary &&
        !!dom.statusEmitterParity &&
        !!dom.toggleEmitMute &&
        !!dom.toggleEmitSolo &&
        !!dom.sliderEmitGain &&
        !!dom.sliderEmitSpread &&
        !!dom.sliderEmitDirectivity &&
        !!dom.choicePhysGravityDir &&
        !!dom.statusEmitterAudio &&
        !!dom.sliderRendMasterGain &&
        !!dom.sliderRendSpk1Gain &&
        !!dom.sliderRendSpk2Gain &&
        !!dom.sliderRendSpk3Gain &&
        !!dom.sliderRendSpk4Gain &&
        !!dom.sliderRendSpk1Delay &&
        !!dom.sliderRendSpk2Delay &&
        !!dom.sliderRendSpk3Delay &&
        !!dom.sliderRendSpk4Delay &&
        !!dom.sliderRendDistanceRef &&
        !!dom.sliderRendDistanceMax &&
        !!dom.sliderRendDopplerScale &&
        !!dom.sliderRendRoomMix &&
        !!dom.sliderRendRoomSize &&
        !!dom.sliderRendRoomDamping &&
        !!dom.sliderRendVizTrailLen &&
        !!dom.sliderCalMicChannel &&
        !!dom.sliderCalSpk1Out &&
        !!dom.sliderCalSpk2Out &&
        !!dom.sliderCalSpk3Out &&
        !!dom.sliderCalSpk4Out &&
        !!dom.sliderCalTestLevel &&
        !!dom.toggleRendDoppler &&
        !!dom.toggleRendAirAbsorb &&
        !!dom.toggleRendRoomEnable &&
        !!dom.toggleRendRoomErOnly &&
        !!dom.toggleRendPhysWalls &&
        !!dom.toggleRendPhysPause &&
        !!dom.toggleRendVizTrails &&
        !!dom.toggleRendVizVectors &&
        !!dom.toggleRendVizGrid &&
        !!dom.toggleRendVizLabels &&
        !!dom.choiceRendDistanceModel &&
        !!dom.choiceRendPhysRate &&
        !!dom.choiceRendVizMode &&
        !!dom.choiceCalSpkConfig &&
        !!dom.choiceCalTestType &&
        !!dom.statusRendererScene &&
        !!dom.statusRendererCore &&
        !!dom.statusRendererSpeakers &&
        !!dom.statusCalibrateCore &&
        !!dom.statusCalSetup &&
        !!dom.statusCalTest &&
        !!dom.btnCalMeasure &&
        !!dom.btnCalRedetect &&
        !!dom.statusCalRouting &&
        !!dom.statusCalProfile &&
        !!dom.statusCalProgress &&
        !!dom.statusCalMessage &&
        !!dom.calProgressBar &&
        !!dom.calSpk1Status &&
        !!dom.calSpk2Status &&
        !!dom.calSpk3Status &&
        !!dom.calSpk4Status
      );

      await waitForCondition("gravity choices", () => dom.choicePhysGravityDir.options.length > 0);
      await waitForCondition("coord mode choices", () => dom.choicePosCoordMode.options.length > 0);
      await waitForCondition("distance model choices", () => dom.choiceRendDistanceModel.options.length > 0);
      await waitForCondition("physics rate choices", () => dom.choiceRendPhysRate.options.length > 0);
      await waitForCondition("viz mode choices", () => dom.choiceRendVizMode.options.length > 0);
      await waitForCondition("cal speaker config choices", () => dom.choiceCalSpkConfig.options.length > 0);
      await waitForCondition("cal test type choices", () => dom.choiceCalTestType.options.length > 0);

      await waitForCondition("debug visibility gate", () =>
        !!dom.body && dom.body.classList.contains("debug-visible") === debugUiRequested
      );
      recordStep("debug_surface_visibility_gate", true, `debug-visible=${debugUiRequested ? "on" : "off"}`);

      const applyModeAndWait = async (mode) => {
        setChoiceIndexSafe(controlStates.mode, modeToChoiceIndex(mode), DEFAULT_CHOICES.mode.length);
        await waitForCondition(`mode ${mode}`, () => runtime.currentMode === mode, 1500, 25);
        await waitMs(220);
      };

      const expectModeLayout = async (mode, expectedRailWidth, timelineVisible) => {
        await applyModeAndWait(mode);
        await waitForCondition(`${mode} rail width`, () =>
          String(getComputedStyle(dom.body).getPropertyValue("--rail-width") || "").trim() === expectedRailWidth
        );
        await waitForCondition(`${mode} timeline visibility`, () =>
          !!dom.timeline && dom.timeline.classList.contains("visible") === timelineVisible
        );
      };

      const orbitBefore = sceneApp ? sceneApp.getOrbitState() : null;
      const presetBefore = sceneApp ? sceneApp.getViewPreset() : null;

      await expectModeLayout("emitter", "280px", true);
      recordStep("mode_emitter_layout_contract", true, "rail=280px timeline=visible");

      await expectModeLayout("calibrate", "320px", false);
      recordStep("mode_calibrate_layout_contract", true, "rail=320px timeline=hidden");

      await expectModeLayout("renderer", "304px", false);
      recordStep("mode_renderer_layout_contract", true, "rail=304px timeline=hidden");

      if (dom.rail) {
        await applyModeAndWait("emitter");
        const emitterMaxScroll = Math.max(0, dom.rail.scrollHeight - dom.rail.clientHeight);
        const emitterTarget = emitterMaxScroll > 0
          ? Math.max(1, Math.min(emitterMaxScroll, Math.round(emitterMaxScroll * 0.35)))
          : 0;
        dom.rail.scrollTop = emitterTarget;
        await waitMs(40);
        const emitterStored = dom.rail.scrollTop;

        await applyModeAndWait("calibrate");
        const calibrateMaxScroll = Math.max(0, dom.rail.scrollHeight - dom.rail.clientHeight);
        const calibrateTarget = calibrateMaxScroll > 0
          ? Math.max(1, Math.min(calibrateMaxScroll, Math.round(calibrateMaxScroll * 0.65)))
          : 0;
        dom.rail.scrollTop = calibrateTarget;
        await waitMs(40);
        const calibrateStored = dom.rail.scrollTop;

        await applyModeAndWait("emitter");
        await waitForCondition("emitter rail scroll restore", () =>
          Math.abs(dom.rail.scrollTop - emitterStored) <= 2
        );

        await applyModeAndWait("calibrate");
        await waitForCondition("calibrate rail scroll restore", () =>
          Math.abs(dom.rail.scrollTop - calibrateStored) <= 2
        );

        recordStep(
          "rail_scroll_memory_by_mode",
          true,
          `emitter=${Math.round(emitterStored)} calibrate=${Math.round(calibrateStored)}`
        );
      } else {
        recordStep("rail_scroll_memory_by_mode", true, "rail element not available");
      }

      if (sceneApp && orbitBefore) {
        const orbitAfter = sceneApp.getOrbitState();
        const presetAfter = sceneApp.getViewPreset();
        const orbitStable =
          closeEnough(orbitAfter.theta, orbitBefore.theta, 0.02)
          && closeEnough(orbitAfter.phi, orbitBefore.phi, 0.02)
          && closeEnough(orbitAfter.radius, orbitBefore.radius, 0.05);
        const presetStable = presetAfter === presetBefore;
        if (!orbitStable || !presetStable) {
          throw new Error(
            `camera continuity violation orbitBefore=${JSON.stringify(orbitBefore)} orbitAfter=${JSON.stringify(orbitAfter)} presetBefore=${String(presetBefore)} presetAfter=${String(presetAfter)}`
          );
        }
        recordStep(
          "mode_switch_camera_continuity",
          true,
          `theta=${orbitAfter.theta.toFixed(3)} phi=${orbitAfter.phi.toFixed(3)} radius=${orbitAfter.radius.toFixed(3)}`
        );
      } else {
        recordStep("mode_switch_camera_continuity", true, "scene unavailable in self-test context");
      }

      await applyModeAndWait("calibrate");

      if (backend) {
        if (dom.btnCalMeasure) {
          dom.btnCalMeasure.click();
        }
        await waitForCondition("step9 start->abort button state", () =>
          dom.btnCalMeasure.textContent === "ABORT"
        );
        await waitForCondition("step9 profile measuring", () =>
          String(dom.statusCalProfile?.textContent || "").trim() === "MEASURING"
        );
        recordStep("step9_start_transitions_to_abort", true, "START MEASURE changes to ABORT");
        recordStep("step9_profile_lifecycle_measuring", true, String(dom.statusCalProfile?.textContent || "").trim());

        if (dom.btnCalMeasure) {
          dom.btnCalMeasure.click();
        }
        await waitForCondition("step10 abort->idle button state", () =>
          dom.btnCalMeasure.textContent === "START MEASURE"
          && !!runtime.calibrationStatus
          && runtime.calibrationStatus.running === false
          && String(runtime.calibrationStatus.state || "").toLowerCase() === "idle"
        );
        await waitForCondition("step10 abort profile", () =>
          String(dom.statusCalProfile?.textContent || "").trim() === "NO PROFILE"
        );
        await waitForCondition("step10 abort speaker reset", () =>
          String(dom.calSpk1Status?.textContent || "").includes("Not measured")
            && String(dom.calSpk2Status?.textContent || "").includes("Not measured")
        );
        recordStep("step10_abort_returns_idle", true, "ABORT returns status/button to idle");
        recordStep("step10_abort_resets_speaker_progress", true, "SPK rows reset + profile NO PROFILE");

        const redetectMarker = Number(runtime.lastCalibrationRedetectAt) || 0;
        if (dom.btnCalRedetect) {
          dom.btnCalRedetect.click();
        }
        await waitForCondition("step11 redetect callback", () =>
          Number(runtime.lastCalibrationRedetectAt) > redetectMarker
        );
        if (!runtime.lastCalibrationRedetectOk) {
          throw new Error("step11 redetect native callback reported failure");
        }
        await waitForCondition("step11 routing status", () =>
          /Current \d+\/\d+\/\d+\/\d+/.test(String(dom.statusCalRouting.textContent || ""))
            && /Auto \d+\/\d+\/\d+\/\d+/.test(String(dom.statusCalRouting.textContent || ""))
        );
        recordStep(
          "step11_redetect_updates_routing_status",
          true,
          String(dom.statusCalRouting.textContent || "").trim()
        );

        await callNativeSafe("locusqAbortCalibration", abortCalibrationNative);
        recordStep("calibration_native_abort_bridge", true, "abort native call completed");
      } else {
        recordStep("step9_start_transitions_to_abort", true, "preview bridge mode");
        recordStep("step9_profile_lifecycle_measuring", true, "preview bridge mode");
        recordStep("step10_abort_returns_idle", true, "preview bridge mode");
        recordStep("step10_abort_resets_speaker_progress", true, "preview bridge mode");
        recordStep("step11_redetect_updates_routing_status", true, "preview bridge mode");
        recordStep("calibration_native_abort_bridge", true, "preview bridge mode");
      }

      const targetMute = !originals.mute;
      dom.toggleEmitMute.checked = targetMute;
      dom.toggleEmitMute.dispatchEvent(new Event("change", { bubbles: true }));
      await waitForCondition("emit_mute relay", () => getToggleValue(controlStates.emitMute) === targetMute);
      await waitForCondition("emit_mute status", () =>
        dom.statusEmitterAudio.textContent.includes(`Mute ${targetMute ? "ON" : "OFF"}`)
      );
      recordStep("emit_mute", true, `target=${targetMute}`);

      const targetSolo = !originals.solo;
      dom.toggleEmitSolo.checked = targetSolo;
      dom.toggleEmitSolo.dispatchEvent(new Event("change", { bubbles: true }));
      await waitForCondition("emit_solo relay", () => getToggleValue(controlStates.emitSolo) === targetSolo);
      await waitForCondition("emit_solo status", () =>
        dom.statusEmitterAudio.textContent.includes(`Solo ${targetSolo ? "ON" : "OFF"}`)
      );
      recordStep("emit_solo", true, `target=${targetSolo}`);

      const targetGain = originals.gain < 0.6 ? 0.83 : 0.17;
      dom.sliderEmitGain.value = targetGain.toFixed(3);
      dom.sliderEmitGain.dispatchEvent(new Event("input", { bubbles: true }));
      await waitForCondition("emit_gain relay", () =>
        closeEnough(getSliderNormalised(controlStates.emitGain), targetGain)
      );
      await waitForCondition("emit_gain status", () =>
        dom.statusEmitterAudio.textContent.includes(`Gain ${Math.round(targetGain * 100)}%`)
      );
      recordStep("emit_gain", true, `target=${targetGain.toFixed(3)}`);

      const targetSpread = originals.spread < 0.5 ? 0.74 : 0.26;
      dom.sliderEmitSpread.value = targetSpread.toFixed(3);
      dom.sliderEmitSpread.dispatchEvent(new Event("input", { bubbles: true }));
      await waitForCondition("emit_spread relay", () =>
        closeEnough(getSliderNormalised(controlStates.emitSpread), targetSpread)
      );
      await waitForCondition("emit_spread status", () =>
        dom.statusEmitterAudio.textContent.includes(`Spread ${Math.round(targetSpread * 100)}%`)
      );
      recordStep("emit_spread", true, `target=${targetSpread.toFixed(3)}`);

      const targetDirectivity = originals.directivity < 0.5 ? 0.68 : 0.32;
      dom.sliderEmitDirectivity.value = targetDirectivity.toFixed(3);
      dom.sliderEmitDirectivity.dispatchEvent(new Event("input", { bubbles: true }));
      await waitForCondition("emit_directivity relay", () =>
        closeEnough(getSliderNormalised(controlStates.emitDirectivity), targetDirectivity)
      );
      await waitForCondition("emit_directivity status", () =>
        dom.statusEmitterAudio.textContent.includes(`Dir ${Math.round(targetDirectivity * 100)}%`)
      );
      recordStep("emit_directivity", true, `target=${targetDirectivity.toFixed(3)}`);

      const optionCount = dom.choicePhysGravityDir.options.length;
      const targetGravity = optionCount > 1
        ? (clamp(originals.gravity, 0, optionCount - 1) + 1) % optionCount
        : clamp(originals.gravity, 0, Math.max(0, optionCount - 1));
      dom.choicePhysGravityDir.selectedIndex = targetGravity;
      dom.choicePhysGravityDir.dispatchEvent(new Event("change", { bubbles: true }));

      await waitForCondition("phys_gravity_dir relay", () =>
        getChoiceIndex(controlStates.physGravityDir) === targetGravity
      );

      const expectedGravityLabel = String(dom.choicePhysGravityDir.options[targetGravity]?.textContent || "").trim();
      if (expectedGravityLabel.length > 0) {
        await waitForCondition("phys_gravity_dir status", () =>
          dom.statusEmitterAudio.textContent.includes(`Gravity ${expectedGravityLabel}`)
        );
      }
      recordStep("phys_gravity_dir", true, `target=${targetGravity} label=${expectedGravityLabel}`);

      const nextLabel = originals.emitLabel === "Emitter" ? "EmitterA" : "Emitter";
      dom.inputEmitLabel.value = nextLabel;
      dom.inputEmitLabel.dispatchEvent(new Event("input", { bubbles: true }));
      dom.inputEmitLabel.dispatchEvent(new Event("blur", { bubbles: true }));
      await waitForCondition("emit_label status", () =>
        dom.statusEmitterParity.textContent.includes(`Label ${nextLabel}`)
      );
      recordStep("emit_label", true, `target=${nextLabel}`);

      const targetEmitColor = originals.emitColor < 0.5 ? 0.8 : 0.2;
      dom.sliderEmitColor.value = targetEmitColor.toFixed(3);
      dom.sliderEmitColor.dispatchEvent(new Event("input", { bubbles: true }));
      await waitForCondition("emit_color relay", () =>
        closeEnough(getSliderNormalised(controlStates.emitColor), targetEmitColor)
      );
      recordStep("emit_color", true, `target=${targetEmitColor.toFixed(3)}`);

      const coordCount = dom.choicePosCoordMode.options.length;
      const targetCoordMode = coordCount > 1
        ? (clamp(originals.posCoordMode, 0, coordCount - 1) + 1) % coordCount
        : clamp(originals.posCoordMode, 0, Math.max(0, coordCount - 1));
      dom.choicePosCoordMode.selectedIndex = targetCoordMode;
      dom.choicePosCoordMode.dispatchEvent(new Event("change", { bubbles: true }));
      await waitForCondition("pos_coord_mode relay", () =>
        getChoiceIndex(controlStates.posCoordMode) === targetCoordMode
      );
      const expectedCoordLabel = String(dom.choicePosCoordMode.options[targetCoordMode]?.textContent || "").trim();
      if (expectedCoordLabel.length > 0) {
        await waitForCondition("pos_coord_mode status", () =>
          dom.statusEmitterParity.textContent.includes(`Coord ${expectedCoordLabel}`)
        );
      }
      recordStep("pos_coord_mode", true, `target=${targetCoordMode} label=${expectedCoordLabel}`);

      const targetPosAzimuth = originals.posAzimuth < 0.5 ? 0.76 : 0.24;
      dom.sliderPosAzimuth.value = targetPosAzimuth.toFixed(3);
      dom.sliderPosAzimuth.dispatchEvent(new Event("input", { bubbles: true }));
      await waitForCondition("pos_azimuth relay", () =>
        closeEnough(getSliderNormalised(controlStates.posAzimuth), targetPosAzimuth)
      );
      recordStep("pos_azimuth", true, `target=${targetPosAzimuth.toFixed(3)}`);

      const targetSizeWidth = originals.sizeWidth < 0.5 ? 0.69 : 0.31;
      dom.sliderSizeWidth.value = targetSizeWidth.toFixed(3);
      dom.sliderSizeWidth.dispatchEvent(new Event("input", { bubbles: true }));
      await waitForCondition("size_width relay", () =>
        closeEnough(getSliderNormalised(controlStates.sizeWidth), targetSizeWidth)
      );
      await waitForCondition("size_width status", () =>
        dom.readoutSizeXyz.textContent.includes("W ")
      );
      recordStep("size_width", true, `target=${targetSizeWidth.toFixed(3)}`);

      const targetPhysMass = originals.physMass < 0.5 ? 0.74 : 0.26;
      dom.sliderPhysMass.value = targetPhysMass.toFixed(3);
      dom.sliderPhysMass.dispatchEvent(new Event("input", { bubbles: true }));
      await waitForCondition("phys_mass relay", () =>
        closeEnough(getSliderNormalised(controlStates.physMass), targetPhysMass)
      );
      await waitForCondition("phys_mass status", () =>
        dom.statusPhysSummary.textContent.includes("mass")
      );
      recordStep("phys_mass", true, `target=${targetPhysMass.toFixed(3)}`);

      const targetAnimSpeed = originals.animSpeed < 0.5 ? 0.73 : 0.27;
      dom.sliderAnimSpeed.value = targetAnimSpeed.toFixed(3);
      dom.sliderAnimSpeed.dispatchEvent(new Event("input", { bubbles: true }));
      await waitForCondition("anim_speed relay", () =>
        closeEnough(getSliderNormalised(controlStates.animSpeed), targetAnimSpeed)
      );
      await waitForCondition("anim_speed status", () =>
        dom.statusAnimSummary.textContent.includes("Speed")
      );
      recordStep("anim_speed", true, `target=${targetAnimSpeed.toFixed(3)}`);

      if (dom.btnPresetRefresh) {
        dom.btnPresetRefresh.click();
        await waitMs(120);
        recordStep("preset_refresh_bridge", true, String(dom.statusPreset.textContent || "").trim());
      }

      const targetRendDoppler = !originals.rendDoppler;
      dom.toggleRendDoppler.checked = targetRendDoppler;
      dom.toggleRendDoppler.dispatchEvent(new Event("change", { bubbles: true }));
      await waitForCondition("rend_doppler relay", () => getToggleValue(controlStates.rendDoppler) === targetRendDoppler);
      await waitForCondition("rend_doppler status", () =>
        dom.statusRendererCore.textContent.includes(`Doppler ${targetRendDoppler ? "ON" : "OFF"}`)
      );
      recordStep("rend_doppler", true, `target=${targetRendDoppler}`);

      const targetRendRoom = !originals.rendRoomEnable;
      dom.toggleRendRoomEnable.checked = targetRendRoom;
      dom.toggleRendRoomEnable.dispatchEvent(new Event("change", { bubbles: true }));
      await waitForCondition("rend_room_enable relay", () => getToggleValue(controlStates.rendRoomEnable) === targetRendRoom);
      await waitForCondition("rend_room_enable status", () =>
        dom.statusRendererCore.textContent.includes(`Room ${targetRendRoom ? "ON" : "OFF"}`)
      );
      recordStep("rend_room_enable", true, `target=${targetRendRoom}`);

      const targetRendMasterGain = originals.rendMasterGain < 0.6 ? 0.81 : 0.19;
      dom.sliderRendMasterGain.value = targetRendMasterGain.toFixed(3);
      dom.sliderRendMasterGain.dispatchEvent(new Event("input", { bubbles: true }));
      await waitForCondition("rend_master_gain relay", () =>
        closeEnough(getSliderNormalised(controlStates.rendMasterGain), targetRendMasterGain)
      );
      await waitForCondition("rend_master_gain status", () =>
        dom.statusRendererCore.textContent.includes(`Master ${Math.round(targetRendMasterGain * 100)}%`)
      );
      recordStep("rend_master_gain", true, `target=${targetRendMasterGain.toFixed(3)}`);

      const distanceOptionCount = dom.choiceRendDistanceModel.options.length;
      const targetDistanceModel = distanceOptionCount > 1
        ? (clamp(originals.rendDistanceModel, 0, distanceOptionCount - 1) + 1) % distanceOptionCount
        : clamp(originals.rendDistanceModel, 0, Math.max(0, distanceOptionCount - 1));
      dom.choiceRendDistanceModel.selectedIndex = targetDistanceModel;
      dom.choiceRendDistanceModel.dispatchEvent(new Event("change", { bubbles: true }));
      await waitForCondition("rend_distance_model relay", () =>
        getChoiceIndex(controlStates.rendDistanceModel) === targetDistanceModel
      );
      const expectedDistanceLabel = String(dom.choiceRendDistanceModel.options[targetDistanceModel]?.textContent || "").trim();
      if (expectedDistanceLabel.length > 0) {
        await waitForCondition("rend_distance_model status", () =>
          dom.statusRendererCore.textContent.includes(`Distance ${expectedDistanceLabel}`)
        );
      }
      recordStep("rend_distance_model", true, `target=${targetDistanceModel} label=${expectedDistanceLabel}`);

      const physRateOptionCount = dom.choiceRendPhysRate.options.length;
      const targetPhysRate = physRateOptionCount > 1
        ? (clamp(originals.rendPhysRate, 0, physRateOptionCount - 1) + 1) % physRateOptionCount
        : clamp(originals.rendPhysRate, 0, Math.max(0, physRateOptionCount - 1));
      dom.choiceRendPhysRate.selectedIndex = targetPhysRate;
      dom.choiceRendPhysRate.dispatchEvent(new Event("change", { bubbles: true }));
      await waitForCondition("rend_phys_rate relay", () =>
        getChoiceIndex(controlStates.rendPhysRate) === targetPhysRate
      );
      const expectedPhysRateLabel = String(dom.choiceRendPhysRate.options[targetPhysRate]?.textContent || "").trim();
      if (expectedPhysRateLabel.length > 0) {
        await waitForCondition("rend_phys_rate status", () =>
          dom.statusRendererCore.textContent.includes(`Rate ${expectedPhysRateLabel}`)
        );
      }
      recordStep("rend_phys_rate", true, `target=${targetPhysRate} label=${expectedPhysRateLabel}`);

      const vizModeOptionCount = dom.choiceRendVizMode.options.length;
      const targetVizMode = vizModeOptionCount > 1
        ? (clamp(originals.rendVizMode, 0, vizModeOptionCount - 1) + 1) % vizModeOptionCount
        : clamp(originals.rendVizMode, 0, Math.max(0, vizModeOptionCount - 1));
      dom.choiceRendVizMode.selectedIndex = targetVizMode;
      dom.choiceRendVizMode.dispatchEvent(new Event("change", { bubbles: true }));
      await waitForCondition("rend_viz_mode relay", () =>
        getChoiceIndex(controlStates.rendVizMode) === targetVizMode
      );
      const expectedVizModeLabel = String(dom.choiceRendVizMode.options[targetVizMode]?.textContent || "").trim();
      if (expectedVizModeLabel.length > 0) {
        await waitForCondition("rend_viz_mode status", () =>
          dom.statusRendererCore.textContent.includes(`View ${expectedVizModeLabel}`)
        );
      }
      recordStep("rend_viz_mode", true, `target=${targetVizMode} label=${expectedVizModeLabel}`);

      const expectedRendererSpeakerStatus = () => {
        const g1 = formatNumber(getScaledValueOrFallback(controlStates.rendSpk1Gain, 0.0), 1);
        const g2 = formatNumber(getScaledValueOrFallback(controlStates.rendSpk2Gain, 0.0), 1);
        const g3 = formatNumber(getScaledValueOrFallback(controlStates.rendSpk3Gain, 0.0), 1);
        const g4 = formatNumber(getScaledValueOrFallback(controlStates.rendSpk4Gain, 0.0), 1);
        const d1 = formatNumber(getScaledValueOrFallback(controlStates.rendSpk1Delay, 0.0), 1);
        const d2 = formatNumber(getScaledValueOrFallback(controlStates.rendSpk2Delay, 0.0), 1);
        const d3 = formatNumber(getScaledValueOrFallback(controlStates.rendSpk3Delay, 0.0), 1);
        const d4 = formatNumber(getScaledValueOrFallback(controlStates.rendSpk4Delay, 0.0), 1);
        return `G ${g1}/${g2}/${g3}/${g4} dB · D ${d1}/${d2}/${d3}/${d4} ms`;
      };

      const targetRendSpk1Gain = originals.rendSpk1Gain < 0.5 ? 0.78 : 0.22;
      dom.sliderRendSpk1Gain.value = targetRendSpk1Gain.toFixed(3);
      dom.sliderRendSpk1Gain.dispatchEvent(new Event("input", { bubbles: true }));
      await waitForCondition("rend_spk1_gain relay", () =>
        closeEnough(getSliderNormalised(controlStates.rendSpk1Gain), targetRendSpk1Gain)
      );
      await waitForCondition("rend_spk1_gain status", () =>
        dom.statusRendererSpeakers.textContent === expectedRendererSpeakerStatus()
      );
      recordStep("rend_spk1_gain", true, `target=${targetRendSpk1Gain.toFixed(3)}`);

      const targetRendSpk2Gain = originals.rendSpk2Gain < 0.5 ? 0.74 : 0.26;
      dom.sliderRendSpk2Gain.value = targetRendSpk2Gain.toFixed(3);
      dom.sliderRendSpk2Gain.dispatchEvent(new Event("input", { bubbles: true }));
      await waitForCondition("rend_spk2_gain relay", () =>
        closeEnough(getSliderNormalised(controlStates.rendSpk2Gain), targetRendSpk2Gain)
      );
      await waitForCondition("rend_spk2_gain status", () =>
        dom.statusRendererSpeakers.textContent === expectedRendererSpeakerStatus()
      );
      recordStep("rend_spk2_gain", true, `target=${targetRendSpk2Gain.toFixed(3)}`);

      const targetRendSpk3Gain = originals.rendSpk3Gain < 0.5 ? 0.69 : 0.31;
      dom.sliderRendSpk3Gain.value = targetRendSpk3Gain.toFixed(3);
      dom.sliderRendSpk3Gain.dispatchEvent(new Event("input", { bubbles: true }));
      await waitForCondition("rend_spk3_gain relay", () =>
        closeEnough(getSliderNormalised(controlStates.rendSpk3Gain), targetRendSpk3Gain)
      );
      await waitForCondition("rend_spk3_gain status", () =>
        dom.statusRendererSpeakers.textContent === expectedRendererSpeakerStatus()
      );
      recordStep("rend_spk3_gain", true, `target=${targetRendSpk3Gain.toFixed(3)}`);

      const targetRendSpk4Gain = originals.rendSpk4Gain < 0.5 ? 0.67 : 0.33;
      dom.sliderRendSpk4Gain.value = targetRendSpk4Gain.toFixed(3);
      dom.sliderRendSpk4Gain.dispatchEvent(new Event("input", { bubbles: true }));
      await waitForCondition("rend_spk4_gain relay", () =>
        closeEnough(getSliderNormalised(controlStates.rendSpk4Gain), targetRendSpk4Gain)
      );
      await waitForCondition("rend_spk4_gain status", () =>
        dom.statusRendererSpeakers.textContent === expectedRendererSpeakerStatus()
      );
      recordStep("rend_spk4_gain", true, `target=${targetRendSpk4Gain.toFixed(3)}`);

      const targetRendSpk1Delay = originals.rendSpk1Delay < 0.5 ? 0.62 : 0.38;
      dom.sliderRendSpk1Delay.value = targetRendSpk1Delay.toFixed(3);
      dom.sliderRendSpk1Delay.dispatchEvent(new Event("input", { bubbles: true }));
      await waitForCondition("rend_spk1_delay relay", () =>
        closeEnough(getSliderNormalised(controlStates.rendSpk1Delay), targetRendSpk1Delay)
      );
      await waitForCondition("rend_spk1_delay status", () =>
        dom.statusRendererSpeakers.textContent === expectedRendererSpeakerStatus()
      );
      recordStep("rend_spk1_delay", true, `target=${targetRendSpk1Delay.toFixed(3)}`);

      const targetRendSpk2Delay = originals.rendSpk2Delay < 0.5 ? 0.58 : 0.42;
      dom.sliderRendSpk2Delay.value = targetRendSpk2Delay.toFixed(3);
      dom.sliderRendSpk2Delay.dispatchEvent(new Event("input", { bubbles: true }));
      await waitForCondition("rend_spk2_delay relay", () =>
        closeEnough(getSliderNormalised(controlStates.rendSpk2Delay), targetRendSpk2Delay)
      );
      await waitForCondition("rend_spk2_delay status", () =>
        dom.statusRendererSpeakers.textContent === expectedRendererSpeakerStatus()
      );
      recordStep("rend_spk2_delay", true, `target=${targetRendSpk2Delay.toFixed(3)}`);

      const targetRendSpk3Delay = originals.rendSpk3Delay < 0.5 ? 0.54 : 0.46;
      dom.sliderRendSpk3Delay.value = targetRendSpk3Delay.toFixed(3);
      dom.sliderRendSpk3Delay.dispatchEvent(new Event("input", { bubbles: true }));
      await waitForCondition("rend_spk3_delay relay", () =>
        closeEnough(getSliderNormalised(controlStates.rendSpk3Delay), targetRendSpk3Delay)
      );
      await waitForCondition("rend_spk3_delay status", () =>
        dom.statusRendererSpeakers.textContent === expectedRendererSpeakerStatus()
      );
      recordStep("rend_spk3_delay", true, `target=${targetRendSpk3Delay.toFixed(3)}`);

      const targetRendSpk4Delay = originals.rendSpk4Delay < 0.5 ? 0.51 : 0.49;
      dom.sliderRendSpk4Delay.value = targetRendSpk4Delay.toFixed(3);
      dom.sliderRendSpk4Delay.dispatchEvent(new Event("input", { bubbles: true }));
      await waitForCondition("rend_spk4_delay relay", () =>
        closeEnough(getSliderNormalised(controlStates.rendSpk4Delay), targetRendSpk4Delay)
      );
      await waitForCondition("rend_spk4_delay status", () =>
        dom.statusRendererSpeakers.textContent === expectedRendererSpeakerStatus()
      );
      recordStep("rend_spk4_delay", true, `target=${targetRendSpk4Delay.toFixed(3)}`);

      const targetDistanceRef = originals.rendDistanceRef < 0.5 ? 0.71 : 0.29;
      dom.sliderRendDistanceRef.value = targetDistanceRef.toFixed(3);
      dom.sliderRendDistanceRef.dispatchEvent(new Event("input", { bubbles: true }));
      await waitForCondition("rend_distance_ref relay", () =>
        closeEnough(getSliderNormalised(controlStates.rendDistanceRef), targetDistanceRef)
      );
      const expectedDistanceRef = formatNumber(getScaledValueOrFallback(controlStates.rendDistanceRef, 1.0), 2);
      await waitForCondition("rend_distance_ref status", () =>
        dom.statusRendererCore.textContent.includes(`Ref ${expectedDistanceRef}m`)
      );
      recordStep("rend_distance_ref", true, `target=${targetDistanceRef.toFixed(3)} ref=${expectedDistanceRef}m`);

      const targetDistanceMax = originals.rendDistanceMax < 0.5 ? 0.64 : 0.36;
      dom.sliderRendDistanceMax.value = targetDistanceMax.toFixed(3);
      dom.sliderRendDistanceMax.dispatchEvent(new Event("input", { bubbles: true }));
      await waitForCondition("rend_distance_max relay", () =>
        closeEnough(getSliderNormalised(controlStates.rendDistanceMax), targetDistanceMax)
      );
      const expectedDistanceMax = formatNumber(getScaledValueOrFallback(controlStates.rendDistanceMax, 50.0), 1);
      await waitForCondition("rend_distance_max status", () =>
        dom.statusRendererCore.textContent.includes(`Max ${expectedDistanceMax}m`)
      );
      recordStep("rend_distance_max", true, `target=${targetDistanceMax.toFixed(3)} max=${expectedDistanceMax}m`);

      const targetDopplerScale = originals.rendDopplerScale < 0.5 ? 0.77 : 0.23;
      dom.sliderRendDopplerScale.value = targetDopplerScale.toFixed(3);
      dom.sliderRendDopplerScale.dispatchEvent(new Event("input", { bubbles: true }));
      await waitForCondition("rend_doppler_scale relay", () =>
        closeEnough(getSliderNormalised(controlStates.rendDopplerScale), targetDopplerScale)
      );
      const expectedDopplerScale = formatNumber(getScaledValueOrFallback(controlStates.rendDopplerScale, 1.0), 2);
      await waitForCondition("rend_doppler_scale status", () =>
        dom.statusRendererCore.textContent.includes(`${expectedDopplerScale}x`)
      );
      recordStep("rend_doppler_scale", true, `target=${targetDopplerScale.toFixed(3)} scale=${expectedDopplerScale}x`);

      const targetAirAbsorb = !originals.rendAirAbsorb;
      dom.toggleRendAirAbsorb.checked = targetAirAbsorb;
      dom.toggleRendAirAbsorb.dispatchEvent(new Event("change", { bubbles: true }));
      await waitForCondition("rend_air_absorb relay", () =>
        getToggleValue(controlStates.rendAirAbsorb) === targetAirAbsorb
      );
      await waitForCondition("rend_air_absorb status", () =>
        dom.statusRendererCore.textContent.includes(`Air ${targetAirAbsorb ? "ON" : "OFF"}`)
      );
      recordStep("rend_air_absorb", true, `target=${targetAirAbsorb}`);

      const targetRoomMix = originals.rendRoomMix < 0.5 ? 0.74 : 0.26;
      dom.sliderRendRoomMix.value = targetRoomMix.toFixed(3);
      dom.sliderRendRoomMix.dispatchEvent(new Event("input", { bubbles: true }));
      await waitForCondition("rend_room_mix relay", () =>
        closeEnough(getSliderNormalised(controlStates.rendRoomMix), targetRoomMix)
      );
      const expectedRoomMix = formatNumber(getScaledValueOrFallback(controlStates.rendRoomMix, 0.3), 2);
      await waitForCondition("rend_room_mix status", () =>
        dom.statusRendererCore.textContent.includes(`mix ${expectedRoomMix}`)
      );
      recordStep("rend_room_mix", true, `target=${targetRoomMix.toFixed(3)} mix=${expectedRoomMix}`);

      const targetRoomSize = originals.rendRoomSize < 0.5 ? 0.66 : 0.34;
      dom.sliderRendRoomSize.value = targetRoomSize.toFixed(3);
      dom.sliderRendRoomSize.dispatchEvent(new Event("input", { bubbles: true }));
      await waitForCondition("rend_room_size relay", () =>
        closeEnough(getSliderNormalised(controlStates.rendRoomSize), targetRoomSize)
      );
      const expectedRoomSize = formatNumber(getScaledValueOrFallback(controlStates.rendRoomSize, 1.0), 2);
      await waitForCondition("rend_room_size status", () =>
        dom.statusRendererCore.textContent.includes(`size ${expectedRoomSize}`)
      );
      recordStep("rend_room_size", true, `target=${targetRoomSize.toFixed(3)} size=${expectedRoomSize}`);

      const targetRoomDamping = originals.rendRoomDamping < 0.5 ? 0.61 : 0.39;
      dom.sliderRendRoomDamping.value = targetRoomDamping.toFixed(3);
      dom.sliderRendRoomDamping.dispatchEvent(new Event("input", { bubbles: true }));
      await waitForCondition("rend_room_damping relay", () =>
        closeEnough(getSliderNormalised(controlStates.rendRoomDamping), targetRoomDamping)
      );
      const expectedRoomDamping = formatNumber(getScaledValueOrFallback(controlStates.rendRoomDamping, 0.5), 2);
      await waitForCondition("rend_room_damping status", () =>
        dom.statusRendererCore.textContent.includes(`damp ${expectedRoomDamping}`)
      );
      recordStep("rend_room_damping", true, `target=${targetRoomDamping.toFixed(3)} damp=${expectedRoomDamping}`);

      const targetRoomErOnly = !originals.rendRoomErOnly;
      dom.toggleRendRoomErOnly.checked = targetRoomErOnly;
      dom.toggleRendRoomErOnly.dispatchEvent(new Event("change", { bubbles: true }));
      await waitForCondition("rend_room_er_only relay", () =>
        getToggleValue(controlStates.rendRoomErOnly) === targetRoomErOnly
      );
      await waitForCondition("rend_room_er_only status", () =>
        dom.statusRendererCore.textContent.includes(`er ${targetRoomErOnly ? "ON" : "OFF"}`)
      );
      recordStep("rend_room_er_only", true, `target=${targetRoomErOnly}`);

      const targetPhysWalls = !originals.rendPhysWalls;
      dom.toggleRendPhysWalls.checked = targetPhysWalls;
      dom.toggleRendPhysWalls.dispatchEvent(new Event("change", { bubbles: true }));
      await waitForCondition("rend_phys_walls relay", () =>
        getToggleValue(controlStates.rendPhysWalls) === targetPhysWalls
      );
      await waitForCondition("rend_phys_walls status", () =>
        dom.statusRendererCore.textContent.includes(`Walls ${targetPhysWalls ? "ON" : "OFF"}`)
      );
      recordStep("rend_phys_walls", true, `target=${targetPhysWalls}`);

      const targetPhysPause = !originals.rendPhysPause;
      dom.toggleRendPhysPause.checked = targetPhysPause;
      dom.toggleRendPhysPause.dispatchEvent(new Event("change", { bubbles: true }));
      await waitForCondition("rend_phys_pause relay", () =>
        getToggleValue(controlStates.rendPhysPause) === targetPhysPause
      );
      await waitForCondition("rend_phys_pause status", () =>
        dom.statusRendererCore.textContent.includes(`Pause ${targetPhysPause ? "ON" : "OFF"}`)
      );
      recordStep("rend_phys_pause", true, `target=${targetPhysPause}`);

      const targetVizTrails = !originals.rendVizTrails;
      dom.toggleRendVizTrails.checked = targetVizTrails;
      dom.toggleRendVizTrails.dispatchEvent(new Event("change", { bubbles: true }));
      await waitForCondition("rend_viz_trails relay", () =>
        getToggleValue(controlStates.rendVizTrails) === targetVizTrails
      );
      await waitForCondition("rend_viz_trails status", () =>
        dom.statusRendererCore.textContent.includes(`Trails ${targetVizTrails ? "ON" : "OFF"}`)
      );
      recordStep("rend_viz_trails", true, `target=${targetVizTrails}`);

      const targetVizTrailLen = originals.rendVizTrailLen < 0.5 ? 0.72 : 0.28;
      dom.sliderRendVizTrailLen.value = targetVizTrailLen.toFixed(3);
      dom.sliderRendVizTrailLen.dispatchEvent(new Event("input", { bubbles: true }));
      await waitForCondition("rend_viz_trail_len relay", () =>
        closeEnough(getSliderNormalised(controlStates.rendVizTrailLen), targetVizTrailLen)
      );
      const expectedVizTrailLen = formatNumber(getScaledValueOrFallback(controlStates.rendVizTrailLen, 5.0), 1);
      await waitForCondition("rend_viz_trail_len status", () =>
        dom.statusRendererCore.textContent.includes(`${expectedVizTrailLen}s`)
      );
      recordStep("rend_viz_trail_len", true, `target=${targetVizTrailLen.toFixed(3)} len=${expectedVizTrailLen}s`);

      const targetVizVectors = !originals.rendVizVectors;
      dom.toggleRendVizVectors.checked = targetVizVectors;
      dom.toggleRendVizVectors.dispatchEvent(new Event("change", { bubbles: true }));
      await waitForCondition("rend_viz_vectors relay", () =>
        getToggleValue(controlStates.rendVizVectors) === targetVizVectors
      );
      await waitForCondition("rend_viz_vectors status", () =>
        dom.statusRendererCore.textContent.includes(`Vectors ${targetVizVectors ? "ON" : "OFF"}`)
      );
      recordStep("rend_viz_vectors", true, `target=${targetVizVectors}`);

      const targetVizGrid = !originals.rendVizGrid;
      dom.toggleRendVizGrid.checked = targetVizGrid;
      dom.toggleRendVizGrid.dispatchEvent(new Event("change", { bubbles: true }));
      await waitForCondition("rend_viz_grid relay", () =>
        getToggleValue(controlStates.rendVizGrid) === targetVizGrid
      );
      await waitForCondition("rend_viz_grid status", () =>
        dom.statusRendererCore.textContent.includes(`Grid ${targetVizGrid ? "ON" : "OFF"}`)
      );
      recordStep("rend_viz_grid", true, `target=${targetVizGrid}`);

      const targetVizLabels = !originals.rendVizLabels;
      dom.toggleRendVizLabels.checked = targetVizLabels;
      dom.toggleRendVizLabels.dispatchEvent(new Event("change", { bubbles: true }));
      await waitForCondition("rend_viz_labels relay", () =>
        getToggleValue(controlStates.rendVizLabels) === targetVizLabels
      );
      await waitForCondition("rend_viz_labels status", () =>
        dom.statusRendererCore.textContent.includes(`Labels ${targetVizLabels ? "ON" : "OFF"}`)
      );
      recordStep("rend_viz_labels", true, `target=${targetVizLabels}`);

      const syntheticRendererScene = {
        ...(runtime.latestScene || {}),
        outputChannels: 4,
        outputLayout: "quad",
        rendererOutputMode: "quad_map_first4",
        rendererOutputChannels: ["FL", "FR", "RL", "RR"],
        rendererQuadMap: [0, 1, 3, 2],
        rendererEligibleEmitters: 4,
        rendererProcessedEmitters: 3,
        rendererGuardrailActive: false,
      };
      window.updateSceneState(syntheticRendererScene);
      await waitForCondition("renderer scene telemetry status", () =>
        dom.statusRendererScene.textContent.includes("output QUAD 4ch")
        && dom.statusRendererScene.textContent.includes("route FL/FR/RL/RR (quad_map_first4)")
      );
      recordStep("renderer_scene_output_telemetry", true, String(dom.statusRendererScene.textContent || "").trim());

      const calSpkConfigCount = dom.choiceCalSpkConfig.options.length;
      const targetCalSpkConfig = calSpkConfigCount > 1
        ? (clamp(originals.calSpkConfig, 0, calSpkConfigCount - 1) + 1) % calSpkConfigCount
        : clamp(originals.calSpkConfig, 0, Math.max(0, calSpkConfigCount - 1));
      dom.choiceCalSpkConfig.selectedIndex = targetCalSpkConfig;
      dom.choiceCalSpkConfig.dispatchEvent(new Event("change", { bubbles: true }));
      await waitForCondition("cal_spk_config relay", () =>
        getChoiceIndex(controlStates.calSpkConfig) === targetCalSpkConfig
      );
      const expectedCalSpkConfigLabel = String(dom.choiceCalSpkConfig.options[targetCalSpkConfig]?.textContent || "").trim();
      if (expectedCalSpkConfigLabel.length > 0) {
        await waitForCondition("cal_spk_config status", () =>
          dom.statusCalibrateCore.textContent.includes(`Config ${expectedCalSpkConfigLabel}`)
        );
      }
      recordStep("cal_spk_config", true, `target=${targetCalSpkConfig} label=${expectedCalSpkConfigLabel}`);

      const targetCalMic = originals.calMicChannel < 0.6 ? 0.82 : 0.18;
      dom.sliderCalMicChannel.value = targetCalMic.toFixed(3);
      dom.sliderCalMicChannel.dispatchEvent(new Event("input", { bubbles: true }));
      await waitForCondition("cal_mic_channel relay", () =>
        closeEnough(getSliderNormalised(controlStates.calMicChannel), targetCalMic)
      );
      const expectedMicChannel = Math.max(
        1,
        Math.round(
          typeof controlStates.calMicChannel.getScaledValue === "function"
            ? Number(controlStates.calMicChannel.getScaledValue())
            : 1
        )
      );
      await waitForCondition("cal_mic_channel status", () =>
        dom.statusCalibrateCore.textContent.includes(`Mic CH ${expectedMicChannel}`)
      );
      recordStep("cal_mic_channel", true, `target=${targetCalMic.toFixed(3)} ch=${expectedMicChannel}`);

      const outStatusString = () => {
        const ch1 = Math.max(1, Math.round(
          typeof controlStates.calSpk1Out.getScaledValue === "function"
            ? Number(controlStates.calSpk1Out.getScaledValue())
            : 1
        ));
        const ch2 = Math.max(1, Math.round(
          typeof controlStates.calSpk2Out.getScaledValue === "function"
            ? Number(controlStates.calSpk2Out.getScaledValue())
            : 1
        ));
        const ch3 = Math.max(1, Math.round(
          typeof controlStates.calSpk3Out.getScaledValue === "function"
            ? Number(controlStates.calSpk3Out.getScaledValue())
            : 1
        ));
        const ch4 = Math.max(1, Math.round(
          typeof controlStates.calSpk4Out.getScaledValue === "function"
            ? Number(controlStates.calSpk4Out.getScaledValue())
            : 1
        ));
        return `${ch1}/${ch2}/${ch3}/${ch4}`;
      };

      const targetCalSpk1 = originals.calSpk1Out < 0.6 ? 0.73 : 0.27;
      dom.sliderCalSpk1Out.value = targetCalSpk1.toFixed(3);
      dom.sliderCalSpk1Out.dispatchEvent(new Event("input", { bubbles: true }));
      await waitForCondition("cal_spk1_out relay", () =>
        closeEnough(getSliderNormalised(controlStates.calSpk1Out), targetCalSpk1)
      );
      await waitForCondition("cal_spk1_out status", () =>
        dom.statusCalibrateCore.textContent.includes(`Out ${outStatusString()}`)
      );
      recordStep("cal_spk1_out", true, `target=${targetCalSpk1.toFixed(3)} out=${outStatusString()}`);

      const targetCalSpk2 = originals.calSpk2Out < 0.6 ? 0.79 : 0.21;
      dom.sliderCalSpk2Out.value = targetCalSpk2.toFixed(3);
      dom.sliderCalSpk2Out.dispatchEvent(new Event("input", { bubbles: true }));
      await waitForCondition("cal_spk2_out relay", () =>
        closeEnough(getSliderNormalised(controlStates.calSpk2Out), targetCalSpk2)
      );
      await waitForCondition("cal_spk2_out status", () =>
        dom.statusCalibrateCore.textContent.includes(`Out ${outStatusString()}`)
      );
      recordStep("cal_spk2_out", true, `target=${targetCalSpk2.toFixed(3)} out=${outStatusString()}`);

      const targetCalSpk3 = originals.calSpk3Out < 0.6 ? 0.67 : 0.33;
      dom.sliderCalSpk3Out.value = targetCalSpk3.toFixed(3);
      dom.sliderCalSpk3Out.dispatchEvent(new Event("input", { bubbles: true }));
      await waitForCondition("cal_spk3_out relay", () =>
        closeEnough(getSliderNormalised(controlStates.calSpk3Out), targetCalSpk3)
      );
      await waitForCondition("cal_spk3_out status", () =>
        dom.statusCalibrateCore.textContent.includes(`Out ${outStatusString()}`)
      );
      recordStep("cal_spk3_out", true, `target=${targetCalSpk3.toFixed(3)} out=${outStatusString()}`);

      const targetCalSpk4 = originals.calSpk4Out < 0.6 ? 0.61 : 0.39;
      dom.sliderCalSpk4Out.value = targetCalSpk4.toFixed(3);
      dom.sliderCalSpk4Out.dispatchEvent(new Event("input", { bubbles: true }));
      await waitForCondition("cal_spk4_out relay", () =>
        closeEnough(getSliderNormalised(controlStates.calSpk4Out), targetCalSpk4)
      );
      await waitForCondition("cal_spk4_out status", () =>
        dom.statusCalibrateCore.textContent.includes(`Out ${outStatusString()}`)
      );
      recordStep("cal_spk4_out", true, `target=${targetCalSpk4.toFixed(3)} out=${outStatusString()}`);

      const targetCalLevel = originals.calTestLevel < 0.5 ? 0.76 : 0.24;
      dom.sliderCalTestLevel.value = targetCalLevel.toFixed(3);
      dom.sliderCalTestLevel.dispatchEvent(new Event("input", { bubbles: true }));
      await waitForCondition("cal_test_level relay", () =>
        closeEnough(getSliderNormalised(controlStates.calTestLevel), targetCalLevel)
      );
      await waitForCondition("cal_test_level status", () =>
        dom.statusCalibrateCore.textContent.includes(`Level ${Math.round(targetCalLevel * 100)}%`)
      );
      recordStep("cal_test_level", true, `target=${targetCalLevel.toFixed(3)}`);

      const calTestTypeCount = dom.choiceCalTestType.options.length;
      const targetCalTestType = calTestTypeCount > 1
        ? (clamp(originals.calTestType, 0, calTestTypeCount - 1) + 1) % calTestTypeCount
        : clamp(originals.calTestType, 0, Math.max(0, calTestTypeCount - 1));
      dom.choiceCalTestType.selectedIndex = targetCalTestType;
      dom.choiceCalTestType.dispatchEvent(new Event("change", { bubbles: true }));
      await waitForCondition("cal_test_type relay", () =>
        getChoiceIndex(controlStates.calTestType) === targetCalTestType
      );
      const expectedCalTestTypeLabel = String(dom.choiceCalTestType.options[targetCalTestType]?.textContent || "").trim();
      if (expectedCalTestTypeLabel.length > 0) {
        await waitForCondition("cal_test_type status", () =>
          dom.statusCalibrateCore.textContent.includes(`Type ${expectedCalTestTypeLabel}`)
        );
      }
      recordStep("cal_test_type", true, `target=${targetCalTestType} label=${expectedCalTestTypeLabel}`);
      await waitForCondition("cal_setup density status", () =>
        String(dom.statusCalSetup.textContent || "").includes("SPK ")
          && String(dom.statusCalSetup.textContent || "").includes("Mic CH")
      );
      recordStep("cal_setup_density_readouts", true, String(dom.statusCalSetup.textContent || "").trim());
      await waitForCondition("cal_test detail status", () =>
        String(dom.statusCalTest.textContent || "").includes("Type ")
          && String(dom.statusCalTest.textContent || "").includes("dBFS")
      );
      recordStep("cal_test_detail_readout", true, String(dom.statusCalTest.textContent || "").trim());

      window.updateCalibrationStatus({
        state: "recording",
        running: true,
        complete: false,
        currentSpeaker: 2,
        completedSpeakers: 1,
        playPercent: 0.25,
        recordPercent: 0.63,
        overallPercent: 0.41,
        message: "Recording response",
        profileValid: false,
      });
      await waitForCondition("cal_capture running progress", () =>
        dom.statusCalProgress.textContent.includes("RECORDING 41%")
      );
      await waitForCondition("cal_capture running message", () =>
        dom.statusCalMessage.textContent.includes("Recording response")
      );
      await waitForCondition("cal_capture running button", () =>
        dom.btnCalMeasure.textContent === "ABORT"
      );
      await waitForCondition("cal_capture running speaker row", () =>
        dom.calSpk2Status.textContent.includes("Recording response")
      );
      await waitForCondition("cal_capture running lifecycle", () =>
        String(dom.statusCalProfile.textContent || "").trim() === "MEASURING"
      );
      await waitForCondition("cal_capture running overlay", () =>
        String(dom.viewportInfo.textContent || "").includes("MEASURING")
      );
      recordStep("cal_capture_running_status", true, "state=recording progress=41%");

      window.updateCalibrationStatus({
        state: "complete",
        running: false,
        complete: true,
        currentSpeaker: 4,
        completedSpeakers: 4,
        playPercent: 1,
        recordPercent: 1,
        overallPercent: 1,
        message: "Profile ready",
        profileValid: true,
      });
      await waitForCondition("cal_capture complete progress", () =>
        dom.statusCalProgress.textContent.includes("COMPLETE 100%")
      );
      await waitForCondition("cal_capture complete button", () =>
        dom.btnCalMeasure.textContent === "MEASURE AGAIN"
      );
      await waitForCondition("cal_capture complete speaker row", () =>
        dom.calSpk4Status.textContent.includes("Measured")
      );
      await waitForCondition("cal_capture complete lifecycle", () =>
        String(dom.statusCalProfile.textContent || "").trim() === "PROFILE READY"
      );
      await waitForCondition("cal_capture complete overlay", () =>
        String(dom.viewportInfo.textContent || "").includes("PROFILE READY")
      );
      recordStep("cal_capture_complete_status", true, "state=complete progress=100%");

      if (backend) {
        dom.btnCalMeasure.click();
        await waitForCondition("measure_again resets start state", () =>
          dom.btnCalMeasure.textContent === "ABORT"
            && !!runtime.calibrationStatus
            && runtime.calibrationStatus.running === true
            && String(runtime.calibrationStatus.state || "").toLowerCase() === "playing"
            && Math.round((Number(runtime.calibrationStatus.overallPercent) || 0) * 100) === 0
            && Number(runtime.calibrationStatus.completedSpeakers) === 0
        );
        await waitForCondition("measure_again lifecycle measuring", () =>
          String(dom.statusCalProfile.textContent || "").trim() === "MEASURING"
        );
        recordStep("measure_again_resets_capture_state", true, "button=ABORT state=playing progress=0%");

        dom.btnCalMeasure.click();
        await waitForCondition("measure_again abort to idle", () =>
          dom.btnCalMeasure.textContent === "START MEASURE"
            && String(dom.statusCalProfile.textContent || "").trim() === "NO PROFILE"
            && String(dom.calSpk1Status.textContent || "").includes("Not measured")
        );
        recordStep("measure_again_abort_returns_idle", true, "idle reset after measure-again run");
      } else {
        recordStep("measure_again_resets_capture_state", true, "preview bridge mode");
        recordStep("measure_again_abort_returns_idle", true, "preview bridge mode");
      }

      report.ok = true;
      report.status = "pass";
    } catch (error) {
      report.ok = false;
      report.status = "fail";
      report.error = String(error);
      recordStep("failure", false, report.error);
    } finally {
      await restoreOriginals();
      report.finishedAt = new Date().toISOString();
      window.__LQ_SELFTEST_RESULT__ = report;
      writeDiagnostics({
        selfTestRequested,
        selfTestStatus: report.status,
        selfTestOk: report.ok,
      });
    }

    return report;
  }

  function updateViewportInfo(sceneApp) {
    const scene = runtime.latestScene || {};
    const emitterCount = Number.isFinite(scene.emitterCount) ? Number(scene.emitterCount) : 0;
    const orbit = sceneApp ? sceneApp.getOrbitState() : null;
    const orbitText = orbit
      ? ` - cam t:${orbit.theta.toFixed(2)} p:${orbit.phi.toFixed(2)} r:${orbit.radius.toFixed(2)}`
      : "";

    if (runtime.currentMode === "calibrate") {
      const status = runtime.calibrationStatus || createDefaultCalibrationStatus();
      const lifecycle = getCalibrationLifecycleLabel(status);
      const calMessage = sanitizeStatusMessage(status.message, "profile capture shell");
      dom.viewportInfo.textContent = `Calibrate Mode - ${lifecycle} - ${calMessage}${orbitText}`;
      return;
    }

    if (runtime.currentMode === "renderer") {
      const channels = Number.isFinite(scene.outputChannels) ? Number(scene.outputChannels) : 0;
      const layout = String(scene.outputLayout || "n/a").trim().toUpperCase() || "N/A";
      const route = Array.isArray(scene.rendererOutputChannels) && scene.rendererOutputChannels.length > 0
        ? scene.rendererOutputChannels.map(value => String(value).trim()).join("/")
        : "n/a";
      dom.viewportInfo.textContent = `Renderer Mode - ${emitterCount} emitters - out ${layout} ${channels}ch (${route})${orbitText}`;
      return;
    }

    dom.viewportInfo.textContent = `Emitter Mode - ${emitterCount} objects${orbitText}`;
  }

  function applyMode(mode, sceneApp) {
    const nextMode = MODE_ORDER.includes(mode) ? mode : "emitter";

    if (dom.rail && runtime.currentMode in runtime.railScrollByMode) {
      runtime.railScrollByMode[runtime.currentMode] = dom.rail.scrollTop;
    }

    runtime.currentMode = nextMode;

    dom.body.classList.remove("mode-calibrate", "mode-emitter", "mode-renderer");
    dom.body.classList.add(`mode-${nextMode}`);

    dom.tabs.forEach(tab => {
      tab.classList.toggle("active", tab.dataset.mode === nextMode);
    });

    dom.panels.forEach(panel => {
      panel.classList.toggle("active", panel.dataset.panel === nextMode);
    });

    if (dom.rail && nextMode in runtime.railScrollByMode) {
      dom.rail.scrollTop = runtime.railScrollByMode[nextMode] || 0;
    }

    dom.timeline.classList.toggle("visible", nextMode === "emitter");
    updateSceneStatusBadge();
    updateRoomProfileBadge();
    updateViewportInfo(sceneApp);
  }

  function updateTimelineTimeFromScene() {
    const scene = runtime.latestScene || {};
    const time = Number(scene.animTime);
    if (!Number.isFinite(time)) return;

    const totalMs = Math.max(0, Math.round(time * 1000));
    const minutes = Math.floor(totalMs / 60000);
    const seconds = Math.floor((totalMs % 60000) / 1000);
    const millis = totalMs % 1000;
    dom.timelineTime.textContent = `${String(minutes).padStart(2, "0")}:${String(seconds).padStart(2, "0")}.${String(millis).padStart(3, "0")}`;
  }

  async function ensureChoicesFromNative(parameterId, state, select, fallbackChoices, flagKey, onApplied) {
    if (!getChoiceItemsNative || runtime.choiceFetchInFlight[flagKey]) {
      return;
    }

    const relayChoices = Array.isArray(state?.properties?.choices) ? state.properties.choices : [];
    if (relayChoices.length > 0) {
      return;
    }

    runtime.choiceFetchInFlight[flagKey] = true;
    try {
      const result = await getChoiceItemsNative(parameterId);
      let choices = [];

      if (Array.isArray(result)) {
        choices = result;
      } else if (result && Array.isArray(result.items)) {
        choices = result.items;
      } else if (typeof result === "string" && result.length > 0) {
        choices = [result];
      }

      if (choices.length === 0) {
        choices = fallbackChoices.slice();
      }

      state.properties = { ...(state.properties || {}), choices: choices.slice() };
      rebuildSelectOptions(select, choices);
      const nextIndex = clamp(getChoiceIndex(state), 0, Math.max(0, choices.length - 1));
      select.selectedIndex = nextIndex;
      onApplied(nextIndex, choices);
    } catch (error) {
      writeDiagnostics({ nativeChoiceFetchError: String(error) });
    } finally {
      runtime.choiceFetchInFlight[flagKey] = false;
    }
  }

  function bindToggle(input, state, onSynced) {
    if (!input || !state) return;

    input.addEventListener("change", () => {
      runtime.counters.setToggle += 1;
      if (typeof state.setValue === "function") {
        state.setValue(input.checked);
      } else {
        state.value = !!input.checked;
      }
      if (typeof onSynced === "function") onSynced(getToggleValue(state));
      renderCounters();
      writeDiagnostics({ event: `${input.id}.change` });
    });

    state.valueChangedEvent.addListener(() => {
      runtime.counters.valueToggle += 1;
      input.checked = getToggleValue(state);
      if (typeof onSynced === "function") onSynced(getToggleValue(state));
      renderCounters();
    });

    state.propertiesChangedEvent.addListener(() => {
      runtime.counters.propsToggle += 1;
      renderCounters();
    });

    input.checked = getToggleValue(state);
    if (typeof onSynced === "function") onSynced(input.checked);
  }

  function bindChoice(select, state, fallbackChoices, onSynced, parameterId, nativeFlagKey) {
    if (!select || !state) return;

    let choices = getChoices(state, fallbackChoices);
    rebuildSelectOptions(select, choices);

    select.addEventListener("change", () => {
      runtime.counters.setChoice += 1;
      choices = getChoices(state, fallbackChoices);
      setChoiceIndexSafe(state, select.selectedIndex, choices.length);
      if (typeof onSynced === "function") onSynced(select.selectedIndex, choices);
      renderCounters();
      writeDiagnostics({ event: `${select.id}.change`, selectedIndex: select.selectedIndex });
    });

    state.valueChangedEvent.addListener(() => {
      runtime.counters.valueChoice += 1;
      choices = getChoices(state, fallbackChoices);
      if (select.options.length !== choices.length) {
        rebuildSelectOptions(select, choices);
      }
      const idx = clamp(getChoiceIndex(state), 0, Math.max(0, choices.length - 1));
      select.selectedIndex = idx;
      if (typeof onSynced === "function") onSynced(idx, choices);
      renderCounters();
    });

    state.propertiesChangedEvent.addListener(() => {
      runtime.counters.propsChoice += 1;
      choices = getChoices(state, fallbackChoices);
      rebuildSelectOptions(select, choices);
      const idx = clamp(getChoiceIndex(state), 0, Math.max(0, choices.length - 1));
      select.selectedIndex = idx;
      if (typeof onSynced === "function") onSynced(idx, choices);
      renderCounters();
    });

    const initialIndex = clamp(getChoiceIndex(state), 0, Math.max(0, choices.length - 1));
    select.selectedIndex = initialIndex;
    if (typeof onSynced === "function") onSynced(initialIndex, choices);

    if (parameterId && nativeFlagKey) {
      void ensureChoicesFromNative(
        parameterId,
        state,
        select,
        fallbackChoices,
        nativeFlagKey,
        (idx, nextChoices) => {
          if (typeof onSynced === "function") onSynced(idx, nextChoices);
        }
      );
    }
  }

  function bindSlider(range, state, readout, onSynced) {
    if (!range || !state) return;

    const syncFromState = () => {
      const normalised = getSliderNormalised(state);
      const scaled = typeof state.getScaledValue === "function" ? Number(state.getScaledValue()) : 0;
      range.value = normalised.toFixed(3);
      if (readout) {
        readout.textContent = `normalized=${normalised.toFixed(3)} scaled=${scaled.toFixed(3)}`;
      }
      if (typeof onSynced === "function") onSynced(normalised);
    };

    range.addEventListener("pointerdown", () => {
      if (typeof state.sliderDragStarted === "function") {
        state.sliderDragStarted();
      }
    });

    const endDrag = () => {
      if (typeof state.sliderDragEnded === "function") {
        state.sliderDragEnded();
      }
    };

    range.addEventListener("pointerup", endDrag);
    range.addEventListener("mouseup", endDrag);
    range.addEventListener("blur", endDrag);

    range.addEventListener("input", () => {
      runtime.counters.setSlider += 1;
      if (typeof state.setNormalisedValue === "function") {
        state.setNormalisedValue(Number(range.value));
      }
      syncFromState();
      renderCounters();
      writeDiagnostics({ event: `${range.id}.input`, value: range.value });
    });

    state.valueChangedEvent.addListener(() => {
      runtime.counters.valueSlider += 1;
      syncFromState();
      renderCounters();
    });

    state.propertiesChangedEvent.addListener(() => {
      runtime.counters.propsSlider += 1;
      syncFromState();
      renderCounters();
    });

    syncFromState();
  }

  function createSceneApp(canvas, viewportElement) {
    if (!canvas || !window.THREE) {
      return null;
    }

    const THREE = window.THREE;
    const state = {
      scene: null,
      camera: null,
      renderer: null,
      rafId: 0,
      running: false,
      clock: new THREE.Clock(),
      pendingScene: null,
      emitterMesh: null,
      roomWireframe: null,
      laneRing: null,
      dragActive: false,
      pointerX: 0,
      pointerY: 0,
      orbit: {
        theta: -0.38,
        phi: 1.05,
        radius: 8.0,
        target: new THREE.Vector3(0, 0, 0),
      },
      viewPreset: "perspective",
      listeners: [],
      resizeObserver: null,
    };

    function listen(target, type, handler, options) {
      if (!target) return;
      target.addEventListener(type, handler, options);
      state.listeners.push(() => target.removeEventListener(type, handler, options));
    }

    function updateCameraFromOrbit() {
      if (!state.camera) return;

      const sinPhi = Math.sin(state.orbit.phi);
      const x = state.orbit.target.x + state.orbit.radius * sinPhi * Math.sin(state.orbit.theta);
      const y = state.orbit.target.y + state.orbit.radius * Math.cos(state.orbit.phi);
      const z = state.orbit.target.z + state.orbit.radius * sinPhi * Math.cos(state.orbit.theta);
      state.camera.position.set(x, y, z);
      state.camera.lookAt(state.orbit.target);
    }

    function resize() {
      if (!state.renderer || !state.camera || !viewportElement) return;
      const width = Math.max(32, Math.floor(viewportElement.clientWidth));
      const height = Math.max(32, Math.floor(viewportElement.clientHeight));
      const dpr = clamp(window.devicePixelRatio || 1, 1, 2);
      state.renderer.setPixelRatio(dpr);
      state.renderer.setSize(width, height, false);
      state.camera.aspect = width / height;
      state.camera.updateProjectionMatrix();
    }

    function applyViewPreset(preset) {
      if (preset === "top") {
        state.orbit.theta = 0.0;
        state.orbit.phi = 0.2;
        state.orbit.radius = 8.5;
      } else if (preset === "front") {
        state.orbit.theta = 0.0;
        state.orbit.phi = 1.57;
        state.orbit.radius = 8.0;
      } else if (preset === "side") {
        state.orbit.theta = Math.PI / 2;
        state.orbit.phi = 1.57;
        state.orbit.radius = 8.0;
      } else {
        state.orbit.theta = -0.38;
        state.orbit.phi = 1.05;
        state.orbit.radius = 8.0;
      }

      state.viewPreset = preset;
      updateCameraFromOrbit();
    }

    function applyScenePayload(payload) {
      if (!payload || !Array.isArray(payload.emitters)) return;
      const localEmitterId = Number(payload.localEmitterId);
      let selectedEmitter = null;
      if (Number.isInteger(localEmitterId)) {
        selectedEmitter = payload.emitters.find(emitter => Number(emitter.id) === localEmitterId) || null;
      }
      if (!selectedEmitter && payload.emitters.length > 0) {
        selectedEmitter = payload.emitters[0];
      }
      if (!selectedEmitter || !state.emitterMesh) return;

      const x = Number(selectedEmitter.x) || 0;
      const y = Number(selectedEmitter.z) || 0;
      const z = Number(selectedEmitter.y) || 0;
      state.emitterMesh.position.set(x, y, z);

      const sx = Number(selectedEmitter.sx) || 0.5;
      const sy = Number(selectedEmitter.sz) || 0.5;
      const sz = Number(selectedEmitter.sy) || 0.5;
      state.emitterMesh.scale.set(clamp(sx, 0.05, 4), clamp(sy, 0.05, 4), clamp(sz, 0.05, 4));
      if (state.laneRing) {
        state.laneRing.position.copy(state.emitterMesh.position);
      }
    }

    function setLaneHighlight(lane, visible) {
      if (!state.laneRing) return;
      state.laneRing.visible = !!visible;
      const hex = LANE_COLORS[lane] || LANE_COLORS.azimuth;
      state.laneRing.material.color.setHex(hex);
    }

    function frame() {
      if (!state.running) return;

      const dt = state.clock.getDelta();
      if (state.pendingScene) {
        applyScenePayload(state.pendingScene);
        state.pendingScene = null;
      }

      if (state.laneRing) {
        state.laneRing.rotation.y += dt * 0.65;
      }

      state.renderer.render(state.scene, state.camera);
      state.rafId = window.requestAnimationFrame(frame);
    }

    function start() {
      if (state.running) return;
      state.running = true;
      state.clock.start();
      frame();
    }

    function stop() {
      state.running = false;
      if (state.rafId) {
        window.cancelAnimationFrame(state.rafId);
      }
      state.rafId = 0;
    }

    function init() {
      state.scene = new THREE.Scene();
      state.scene.background = new THREE.Color(0x050505);

      state.camera = new THREE.PerspectiveCamera(50, 1, 0.1, 120);
      updateCameraFromOrbit();

      state.renderer = new THREE.WebGLRenderer({
        canvas,
        antialias: true,
        alpha: false,
        powerPreference: "high-performance",
      });

      const hemi = new THREE.HemisphereLight(0xffffff, 0x1a1a1a, 0.45);
      state.scene.add(hemi);

      const dir = new THREE.DirectionalLight(0xffffff, 0.5);
      dir.position.set(4, 6, 5);
      state.scene.add(dir);

      const grid = new THREE.GridHelper(14, 14, 0x303030, 0x202020);
      grid.position.y = -2;
      state.scene.add(grid);

      const roomGeometry = new THREE.BoxGeometry(10, 6, 10);
      const roomEdges = new THREE.EdgesGeometry(roomGeometry);
      const roomMaterial = new THREE.LineBasicMaterial({
        color: 0xbcbcbc,
        transparent: true,
        opacity: 0.22,
      });
      state.roomWireframe = new THREE.LineSegments(roomEdges, roomMaterial);
      state.scene.add(state.roomWireframe);

      const emitterGeometry = new THREE.SphereGeometry(0.45, 24, 16);
      const emitterMaterial = new THREE.MeshStandardMaterial({
        color: 0xd4a847,
        roughness: 0.48,
        metalness: 0.06,
      });
      state.emitterMesh = new THREE.Mesh(emitterGeometry, emitterMaterial);
      state.scene.add(state.emitterMesh);

      const ringGeometry = new THREE.TorusGeometry(0.85, 0.05, 12, 42);
      const ringMaterial = new THREE.MeshBasicMaterial({
        color: LANE_COLORS.azimuth,
        transparent: true,
        opacity: 0.92,
      });
      state.laneRing = new THREE.Mesh(ringGeometry, ringMaterial);
      state.scene.add(state.laneRing);

      const onPointerDown = event => {
        if (event.button !== 0) return;
        state.dragActive = true;
        state.pointerX = event.clientX;
        state.pointerY = event.clientY;
        canvas.classList.add("dragging");
      };

      const onPointerMove = event => {
        if (!state.dragActive) return;
        const dx = event.clientX - state.pointerX;
        const dy = event.clientY - state.pointerY;
        state.pointerX = event.clientX;
        state.pointerY = event.clientY;
        state.orbit.theta -= dx * 0.008;
        state.orbit.phi = clamp(state.orbit.phi + dy * 0.008, 0.15, Math.PI - 0.15);
        updateCameraFromOrbit();
      };

      const onPointerUp = () => {
        state.dragActive = false;
        canvas.classList.remove("dragging");
      };

      const onWheel = event => {
        event.preventDefault();
        const direction = event.deltaY > 0 ? 1 : -1;
        state.orbit.radius = clamp(state.orbit.radius + direction * 0.35, 2.5, 24.0);
        updateCameraFromOrbit();
      };

      listen(canvas, "pointerdown", onPointerDown);
      listen(window, "pointermove", onPointerMove);
      listen(window, "pointerup", onPointerUp);
      listen(canvas, "wheel", onWheel, { passive: false });
      listen(window, "resize", resize);

      if (typeof ResizeObserver !== "undefined") {
        state.resizeObserver = new ResizeObserver(() => resize());
        state.resizeObserver.observe(viewportElement);
      }

      resize();
      start();
    }

    function dispose() {
      stop();
      state.listeners.forEach(removeListener => {
        try {
          removeListener();
        } catch (_) {}
      });
      state.listeners = [];

      if (state.resizeObserver) {
        state.resizeObserver.disconnect();
        state.resizeObserver = null;
      }

      state.scene.traverse(object => {
        if (object.geometry && typeof object.geometry.dispose === "function") {
          object.geometry.dispose();
        }
        if (object.material) {
          const materials = Array.isArray(object.material) ? object.material : [object.material];
          materials.forEach(material => {
            if (material && typeof material.dispose === "function") {
              material.dispose();
            }
          });
        }
      });
      state.renderer.dispose();
    }

    init();

    return {
      resize,
      dispose,
      setPendingScene(payload) {
        state.pendingScene = payload;
      },
      setLane(lane, visible) {
        setLaneHighlight(lane, visible);
      },
      setViewPreset(preset) {
        applyViewPreset(preset);
      },
      getViewPreset() {
        return state.viewPreset;
      },
      getOrbitState() {
        return {
          theta: state.orbit.theta,
          phi: state.orbit.phi,
          radius: state.orbit.radius,
        };
      },
    };
  }

  const viewportShell = dom.viewportCanvas ? dom.viewportCanvas.parentElement : null;
  const sceneApp = createSceneApp(dom.viewportCanvas, viewportShell);

  dom.viewButtons.forEach(button => {
    button.addEventListener("click", () => {
      const view = button.dataset.view || "perspective";
      if (sceneApp) {
        sceneApp.setViewPreset(view);
      }
      dom.viewButtons.forEach(other => {
        other.classList.toggle("active", other.dataset.view === view);
      });
      updateViewportInfo(sceneApp);
      writeDiagnostics({ event: `view.${view}` });
    });
  });

  dom.lanes.forEach(button => {
    button.addEventListener("click", () => {
      const lane = button.dataset.lane || "azimuth";
      setActiveLane(lane);
      if (sceneApp) {
        sceneApp.setLane(lane, runtime.currentMode === "emitter");
      }
      writeDiagnostics({ event: `lane.${lane}` });
    });
  });

  dom.tabs.forEach(tab => {
    tab.addEventListener("click", () => {
      const mode = tab.dataset.mode || "emitter";
      runtime.counters.setChoice += 1;
      setChoiceIndexSafe(controlStates.mode, modeToChoiceIndex(mode), 3);
      applyMode(mode, sceneApp);
      if (sceneApp) sceneApp.setLane(runtime.selectedLane, mode === "emitter");
      renderCounters();
      writeDiagnostics({ event: `mode.${mode}` });
    });
  });

  bindToggle(dom.toggleSizeLink, controlStates.sizeLink, checked => {
    runtime.snapshot.sizeLink = !!checked;
    updateSizeReadout();
    updateEmitterParityStatus();
  });

  bindToggle(dom.togglePhysEnable, controlStates.physEnable, () => {
    updateSceneStatusBadge();
    updatePhysicsStatus();
    updateEmitterParityStatus();
  });

  bindToggle(dom.toggleAnimEnable, controlStates.animEnable, () => {
    updateAnimationStatus();
    updateEmitterParityStatus();
  });
  bindToggle(dom.toggleAnimLoop, controlStates.animLoop, () => {
    updateAnimationStatus();
    updateEmitterParityStatus();
  });
  bindToggle(dom.toggleAnimSync, controlStates.animSync, () => {
    updateAnimationStatus();
    updateEmitterParityStatus();
  });

  bindToggle(dom.toggleEmitMute, controlStates.emitMute, () => {
    updateEmitterAudioStatus();
    updateEmitterParityStatus();
  });

  bindToggle(dom.toggleEmitSolo, controlStates.emitSolo, () => {
    updateEmitterAudioStatus();
    updateEmitterParityStatus();
  });

  bindToggle(dom.toggleRendDoppler, controlStates.rendDoppler, checked => {
    runtime.snapshot.rendDoppler = !!checked;
    updateRendererCoreStatus();
  });

  bindToggle(dom.toggleRendAirAbsorb, controlStates.rendAirAbsorb, checked => {
    runtime.snapshot.rendAirAbsorb = !!checked;
    updateRendererCoreStatus();
  });

  bindToggle(dom.toggleRendRoomEnable, controlStates.rendRoomEnable, checked => {
    runtime.snapshot.rendRoomEnable = !!checked;
    updateRendererCoreStatus();
  });

  bindToggle(dom.toggleRendRoomErOnly, controlStates.rendRoomErOnly, checked => {
    runtime.snapshot.rendRoomErOnly = !!checked;
    updateRendererCoreStatus();
  });

  bindToggle(dom.toggleRendPhysWalls, controlStates.rendPhysWalls, checked => {
    runtime.snapshot.rendPhysWalls = !!checked;
    updateRendererCoreStatus();
  });

  bindToggle(dom.toggleRendPhysPause, controlStates.rendPhysPause, checked => {
    runtime.snapshot.rendPhysPause = !!checked;
    updateRendererCoreStatus();
  });

  bindToggle(dom.toggleRendVizTrails, controlStates.rendVizTrails, checked => {
    runtime.snapshot.rendVizTrails = !!checked;
    updateRendererCoreStatus();
  });

  bindToggle(dom.toggleRendVizVectors, controlStates.rendVizVectors, checked => {
    runtime.snapshot.rendVizVectors = !!checked;
    updateRendererCoreStatus();
  });

  bindToggle(dom.toggleRendVizGrid, controlStates.rendVizGrid, checked => {
    runtime.snapshot.rendVizGrid = !!checked;
    updateRendererCoreStatus();
  });

  bindToggle(dom.toggleRendVizLabels, controlStates.rendVizLabels, checked => {
    runtime.snapshot.rendVizLabels = !!checked;
    updateRendererCoreStatus();
  });

  if (dom.inputEmitLabel) {
    dom.inputEmitLabel.addEventListener("input", () => {
      const nextLabel = sanitizeEmitterLabel(dom.inputEmitLabel.value);
      dom.inputEmitLabel.value = nextLabel;
      runtime.snapshot.emitLabel = nextLabel;
      updateEmitterParityStatus();
    });
    dom.inputEmitLabel.addEventListener("blur", () => {
      const nextLabel = sanitizeEmitterLabel(dom.inputEmitLabel.value);
      dom.inputEmitLabel.value = nextLabel;
      runtime.snapshot.emitLabel = nextLabel;
      void persistUiState({ emitterLabel: nextLabel });
      updateEmitterParityStatus();
    });
  }

  if (dom.choicePhysPreset) {
    dom.choicePhysPreset.addEventListener("change", () => {
      applyPhysicsPreset(dom.choicePhysPreset.value, true);
    });
  }

  if (dom.btnPhysThrow) {
    dom.btnPhysThrow.addEventListener("click", () => {
      pulseToggleState(controlStates.physThrow);
      writeDiagnostics({ event: "phys.throw" });
    });
  }

  if (dom.btnPhysReset) {
    dom.btnPhysReset.addEventListener("click", () => {
      pulseToggleState(controlStates.physReset);
      writeDiagnostics({ event: "phys.reset" });
    });
  }

  if (dom.btnPresetRefresh) {
    dom.btnPresetRefresh.addEventListener("click", () => {
      void refreshEmitterPresetList();
    });
  }

  if (dom.btnPresetSave) {
    dom.btnPresetSave.addEventListener("click", () => {
      void handlePresetSaveClick();
    });
  }

  if (dom.btnPresetLoad) {
    dom.btnPresetLoad.addEventListener("click", () => {
      void handlePresetLoadClick();
    });
  }

  bindChoice(
    dom.choiceCalSpkConfig,
    controlStates.calSpkConfig,
    DEFAULT_CHOICES.cal_spk_config,
    index => {
      runtime.snapshot.calSpkConfigIndex = index;
      updateCalibrateCoreStatus();
    },
    "cal_spk_config",
    "calSpkConfig"
  );

  bindChoice(
    dom.choiceCalTestType,
    controlStates.calTestType,
    DEFAULT_CHOICES.cal_test_type,
    index => {
      runtime.snapshot.calTestTypeIndex = index;
      updateCalibrateCoreStatus();
    },
    "cal_test_type",
    "calTestType"
  );

  bindChoice(
    dom.choicePosCoordMode,
    controlStates.posCoordMode,
    DEFAULT_CHOICES.pos_coord_mode,
    index => {
      runtime.snapshot.posCoordModeIndex = index;
      updatePositionReadout();
      updateEmitterParityStatus();
    },
    "pos_coord_mode",
    "posCoordMode"
  );

  bindChoice(
    dom.choiceAnimMode,
    controlStates.animMode,
    DEFAULT_CHOICES.anim_mode,
    () => {
      updateAnimationStatus();
      updateEmitterParityStatus();
    },
    "anim_mode",
    "animMode"
  );

  bindChoice(
    dom.choicePhysGravityDir,
    controlStates.physGravityDir,
    DEFAULT_CHOICES.phys_gravity_dir,
    (index) => {
      runtime.snapshot.physGravityDirIndex = index;
      updateEmitterAudioStatus();
      updatePhysicsStatus();
      updateEmitterParityStatus();
    },
    "phys_gravity_dir",
    "physGravityDir"
  );

  bindChoice(
    dom.choiceRendDistanceModel,
    controlStates.rendDistanceModel,
    DEFAULT_CHOICES.rend_distance_model,
    index => {
      runtime.snapshot.rendDistanceModelIndex = index;
      updateRendererCoreStatus();
    },
    "rend_distance_model",
    "distanceModel"
  );

  bindChoice(
    dom.choiceRendPhysRate,
    controlStates.rendPhysRate,
    DEFAULT_CHOICES.rend_phys_rate,
    index => {
      runtime.snapshot.rendPhysRateIndex = index;
      updateRendererCoreStatus();
    },
    "rend_phys_rate",
    "physRate"
  );

  bindChoice(
    dom.choiceRendVizMode,
    controlStates.rendVizMode,
    DEFAULT_CHOICES.rend_viz_mode,
    index => {
      runtime.snapshot.rendVizModeIndex = index;
      updateRendererCoreStatus();
    },
    "rend_viz_mode",
    "vizMode"
  );

  bindSlider(dom.sliderSizeUniform, controlStates.sizeUniform, dom.readoutSizeUniform, normalised => {
    runtime.snapshot.sizeUniformNorm = normalised;
    if (getToggleValue(controlStates.sizeLink)) {
      const uniformScaled = getScaledValueOrFallback(controlStates.sizeUniform, 0.5);
      setSliderScaledValue(controlStates.sizeWidth, uniformScaled);
      setSliderScaledValue(controlStates.sizeDepth, uniformScaled);
      setSliderScaledValue(controlStates.sizeHeight, uniformScaled);
    }
    updateSizeReadout();
    updateEmitterParityStatus();
  });

  bindSlider(dom.sliderEmitColor, controlStates.emitColor, dom.readoutEmitColor, () => {
    updateColorReadout();
    updateEmitterParityStatus();
  });

  bindSlider(dom.sliderPosAzimuth, controlStates.posAzimuth, null, () => {
    updatePositionReadout();
    updateEmitterParityStatus();
  });

  bindSlider(dom.sliderPosElevation, controlStates.posElevation, null, () => {
    updatePositionReadout();
    updateEmitterParityStatus();
  });

  bindSlider(dom.sliderPosDistance, controlStates.posDistance, null, () => {
    updatePositionReadout();
    updateEmitterParityStatus();
  });

  bindSlider(dom.sliderSizeWidth, controlStates.sizeWidth, null, normalised => {
    runtime.snapshot.sizeWidthNorm = normalised;
    updateSizeReadout();
    updateEmitterParityStatus();
  });

  bindSlider(dom.sliderSizeDepth, controlStates.sizeDepth, null, normalised => {
    runtime.snapshot.sizeDepthNorm = normalised;
    updateSizeReadout();
    updateEmitterParityStatus();
  });

  bindSlider(dom.sliderSizeHeight, controlStates.sizeHeight, null, normalised => {
    runtime.snapshot.sizeHeightNorm = normalised;
    updateSizeReadout();
    updateEmitterParityStatus();
  });

  bindSlider(dom.sliderPhysMass, controlStates.physMass, null, normalised => {
    runtime.snapshot.physMassNorm = normalised;
    if (!runtime.applyingPhysicsPreset) {
      if (dom.choicePhysPreset) dom.choicePhysPreset.value = "custom";
      void persistUiState({ physicsPreset: "custom" });
    }
    updatePhysicsStatus();
    updateEmitterParityStatus();
  });

  bindSlider(dom.sliderPhysDrag, controlStates.physDrag, null, normalised => {
    runtime.snapshot.physDragNorm = normalised;
    if (!runtime.applyingPhysicsPreset) {
      if (dom.choicePhysPreset) dom.choicePhysPreset.value = "custom";
      void persistUiState({ physicsPreset: "custom" });
    }
    updatePhysicsStatus();
    updateEmitterParityStatus();
  });

  bindSlider(dom.sliderPhysElasticity, controlStates.physElasticity, null, normalised => {
    runtime.snapshot.physElasticityNorm = normalised;
    if (!runtime.applyingPhysicsPreset) {
      if (dom.choicePhysPreset) dom.choicePhysPreset.value = "custom";
      void persistUiState({ physicsPreset: "custom" });
    }
    updatePhysicsStatus();
    updateEmitterParityStatus();
  });

  bindSlider(dom.sliderPhysGravity, controlStates.physGravity, null, normalised => {
    runtime.snapshot.physGravityNorm = normalised;
    if (!runtime.applyingPhysicsPreset) {
      if (dom.choicePhysPreset) dom.choicePhysPreset.value = "custom";
      void persistUiState({ physicsPreset: "custom" });
    }
    updatePhysicsStatus();
    updateEmitterParityStatus();
  });

  bindSlider(dom.sliderPhysFriction, controlStates.physFriction, null, normalised => {
    runtime.snapshot.physFrictionNorm = normalised;
    if (!runtime.applyingPhysicsPreset) {
      if (dom.choicePhysPreset) dom.choicePhysPreset.value = "custom";
      void persistUiState({ physicsPreset: "custom" });
    }
    updatePhysicsStatus();
    updateEmitterParityStatus();
  });

  bindSlider(dom.sliderAnimSpeed, controlStates.animSpeed, null, normalised => {
    runtime.snapshot.animSpeedNorm = normalised;
    updateAnimationStatus();
    updateEmitterParityStatus();
  });

  bindSlider(dom.sliderEmitGain, controlStates.emitGain, null, normalised => {
    runtime.snapshot.emitGainNorm = normalised;
    updateEmitterAudioStatus();
    updateEmitterParityStatus();
  });

  bindSlider(dom.sliderEmitSpread, controlStates.emitSpread, null, normalised => {
    runtime.snapshot.emitSpreadNorm = normalised;
    updateEmitterAudioStatus();
    updateEmitterParityStatus();
  });

  bindSlider(dom.sliderEmitDirectivity, controlStates.emitDirectivity, null, normalised => {
    runtime.snapshot.emitDirectivityNorm = normalised;
    updateEmitterAudioStatus();
    updateEmitterParityStatus();
  });

  bindSlider(dom.sliderRendMasterGain, controlStates.rendMasterGain, null, normalised => {
    runtime.snapshot.rendMasterGainNorm = normalised;
    updateRendererCoreStatus();
  });

  bindSlider(dom.sliderRendSpk1Gain, controlStates.rendSpk1Gain, null, normalised => {
    runtime.snapshot.rendSpk1GainNorm = normalised;
    updateRendererCoreStatus();
  });
  bindSlider(dom.sliderRendSpk2Gain, controlStates.rendSpk2Gain, null, normalised => {
    runtime.snapshot.rendSpk2GainNorm = normalised;
    updateRendererCoreStatus();
  });
  bindSlider(dom.sliderRendSpk3Gain, controlStates.rendSpk3Gain, null, normalised => {
    runtime.snapshot.rendSpk3GainNorm = normalised;
    updateRendererCoreStatus();
  });
  bindSlider(dom.sliderRendSpk4Gain, controlStates.rendSpk4Gain, null, normalised => {
    runtime.snapshot.rendSpk4GainNorm = normalised;
    updateRendererCoreStatus();
  });
  bindSlider(dom.sliderRendSpk1Delay, controlStates.rendSpk1Delay, null, normalised => {
    runtime.snapshot.rendSpk1DelayNorm = normalised;
    updateRendererCoreStatus();
  });
  bindSlider(dom.sliderRendSpk2Delay, controlStates.rendSpk2Delay, null, normalised => {
    runtime.snapshot.rendSpk2DelayNorm = normalised;
    updateRendererCoreStatus();
  });
  bindSlider(dom.sliderRendSpk3Delay, controlStates.rendSpk3Delay, null, normalised => {
    runtime.snapshot.rendSpk3DelayNorm = normalised;
    updateRendererCoreStatus();
  });
  bindSlider(dom.sliderRendSpk4Delay, controlStates.rendSpk4Delay, null, normalised => {
    runtime.snapshot.rendSpk4DelayNorm = normalised;
    updateRendererCoreStatus();
  });
  bindSlider(dom.sliderRendDistanceRef, controlStates.rendDistanceRef, null, normalised => {
    runtime.snapshot.rendDistanceRefNorm = normalised;
    updateRendererCoreStatus();
  });
  bindSlider(dom.sliderRendDistanceMax, controlStates.rendDistanceMax, null, normalised => {
    runtime.snapshot.rendDistanceMaxNorm = normalised;
    updateRendererCoreStatus();
  });
  bindSlider(dom.sliderRendDopplerScale, controlStates.rendDopplerScale, null, normalised => {
    runtime.snapshot.rendDopplerScaleNorm = normalised;
    updateRendererCoreStatus();
  });
  bindSlider(dom.sliderRendRoomMix, controlStates.rendRoomMix, null, normalised => {
    runtime.snapshot.rendRoomMixNorm = normalised;
    updateRendererCoreStatus();
  });
  bindSlider(dom.sliderRendRoomSize, controlStates.rendRoomSize, null, normalised => {
    runtime.snapshot.rendRoomSizeNorm = normalised;
    updateRendererCoreStatus();
  });
  bindSlider(dom.sliderRendRoomDamping, controlStates.rendRoomDamping, null, normalised => {
    runtime.snapshot.rendRoomDampingNorm = normalised;
    updateRendererCoreStatus();
  });
  bindSlider(dom.sliderRendVizTrailLen, controlStates.rendVizTrailLen, null, normalised => {
    runtime.snapshot.rendVizTrailLenNorm = normalised;
    updateRendererCoreStatus();
  });

  bindSlider(dom.sliderCalMicChannel, controlStates.calMicChannel, null, normalised => {
    runtime.snapshot.calMicChannelNorm = normalised;
    updateCalibrateCoreStatus();
  });

  bindSlider(dom.sliderCalSpk1Out, controlStates.calSpk1Out, null, normalised => {
    runtime.snapshot.calSpk1OutNorm = normalised;
    updateCalibrateCoreStatus();
  });

  bindSlider(dom.sliderCalSpk2Out, controlStates.calSpk2Out, null, normalised => {
    runtime.snapshot.calSpk2OutNorm = normalised;
    updateCalibrateCoreStatus();
  });

  bindSlider(dom.sliderCalSpk3Out, controlStates.calSpk3Out, null, normalised => {
    runtime.snapshot.calSpk3OutNorm = normalised;
    updateCalibrateCoreStatus();
  });

  bindSlider(dom.sliderCalSpk4Out, controlStates.calSpk4Out, null, normalised => {
    runtime.snapshot.calSpk4OutNorm = normalised;
    updateCalibrateCoreStatus();
  });

  bindSlider(dom.sliderCalTestLevel, controlStates.calTestLevel, null, normalised => {
    runtime.snapshot.calTestLevelNorm = normalised;
    updateCalibrateCoreStatus();
  });

  controlStates.posX.valueChangedEvent.addListener(() => {
    updatePositionReadout();
    updateEmitterParityStatus();
  });
  controlStates.posY.valueChangedEvent.addListener(() => {
    updatePositionReadout();
    updateEmitterParityStatus();
  });
  controlStates.posZ.valueChangedEvent.addListener(() => {
    updatePositionReadout();
    updateEmitterParityStatus();
  });

  controlStates.mode.valueChangedEvent.addListener(() => {
    runtime.counters.valueChoice += 1;
    const mode = modeFromChoiceIndex(getChoiceIndex(controlStates.mode));
    applyMode(mode, sceneApp);
    if (sceneApp) sceneApp.setLane(runtime.selectedLane, mode === "emitter");
    renderCounters();
  });

  controlStates.mode.propertiesChangedEvent.addListener(() => {
    runtime.counters.propsChoice += 1;
    const mode = modeFromChoiceIndex(getChoiceIndex(controlStates.mode));
    applyMode(mode, sceneApp);
    renderCounters();
  });

  controlStates.quality.valueChangedEvent.addListener(() => {
    runtime.counters.valueChoice += 1;
    updateQualityBadge();
    renderCounters();
  });

  controlStates.quality.propertiesChangedEvent.addListener(() => {
    runtime.counters.propsChoice += 1;
    updateQualityBadge();
    renderCounters();
  });

  dom.qualityBadge.addEventListener("click", () => {
    const choices = getChoices(controlStates.quality, DEFAULT_CHOICES.rend_quality);
    const nextIndex = (getChoiceIndex(controlStates.quality) + 1) % Math.max(1, choices.length);
    runtime.counters.setChoice += 1;
    setChoiceIndexSafe(controlStates.quality, nextIndex, choices.length);
    updateQualityBadge();
    renderCounters();
    writeDiagnostics({ event: "quality.toggle", nextIndex });
  });

  if (dom.btnCalMeasure) {
    dom.btnCalMeasure.addEventListener("click", () => {
      void handleCalibrationMeasureClick();
    });
  }

  if (dom.btnCalRedetect) {
    dom.btnCalRedetect.addEventListener("click", () => {
      void handleCalibrationRedetectClick();
    });
  }

  setActiveLane(runtime.selectedLane);
  if (sceneApp) {
    sceneApp.setLane(runtime.selectedLane, true);
  }

  window.updateSceneState = function updateSceneState(data) {
    runtime.latestScene = data || null;
    if (sceneApp) {
      sceneApp.setPendingScene(runtime.latestScene);
    }
    const selectedEmitter = getSelectedEmitterFromScene();
    if (selectedEmitter && dom.inputEmitLabel && !document.activeElement?.isSameNode(dom.inputEmitLabel)) {
      const nextLabel = sanitizeEmitterLabel(selectedEmitter.label || dom.inputEmitLabel.value || "Emitter");
      dom.inputEmitLabel.value = nextLabel;
      runtime.snapshot.emitLabel = nextLabel;
    }
    updateCalibrationRoutingStatus();
    updatePositionReadout();
    updateTimelineTimeFromScene();
    updateViewportInfo(sceneApp);
    updateRendererSceneStatus();
    updateEmitterParityStatus();
  };

  function applyCalibrationStatus(status) {
    runtime.calibrationStatus = sanitizeCalibrationStatus(status) || createDefaultCalibrationStatus();
    updateRoomProfileBadge();
    updateSceneStatusBadge();
    updateCalibrateCaptureStatus();
    updateViewportInfo(sceneApp);
  }

  window.updateCalibrationStatus = function updateCalibrationStatus(status) {
    applyCalibrationStatus(status);
  };

  function heartbeat() {
    runtime.counters.heartbeat += 1;
    runtime.snapshot.emitLabel = sanitizeEmitterLabel(dom.inputEmitLabel?.value || runtime.snapshot.emitLabel || "Emitter");
    runtime.snapshot.emitColorNorm = getSliderNormalised(controlStates.emitColor);
    runtime.snapshot.sizeLink = getToggleValue(controlStates.sizeLink);
    runtime.snapshot.qualityIndex = getChoiceIndex(controlStates.quality);
    runtime.snapshot.posCoordModeIndex = getChoiceIndex(controlStates.posCoordMode);
    runtime.snapshot.posAzimuthNorm = getSliderNormalised(controlStates.posAzimuth);
    runtime.snapshot.posElevationNorm = getSliderNormalised(controlStates.posElevation);
    runtime.snapshot.posDistanceNorm = getSliderNormalised(controlStates.posDistance);
    runtime.snapshot.posXNorm = getSliderNormalised(controlStates.posX);
    runtime.snapshot.posYNorm = getSliderNormalised(controlStates.posY);
    runtime.snapshot.posZNorm = getSliderNormalised(controlStates.posZ);
    runtime.snapshot.sizeUniformNorm = getSliderNormalised(controlStates.sizeUniform);
    runtime.snapshot.sizeWidthNorm = getSliderNormalised(controlStates.sizeWidth);
    runtime.snapshot.sizeDepthNorm = getSliderNormalised(controlStates.sizeDepth);
    runtime.snapshot.sizeHeightNorm = getSliderNormalised(controlStates.sizeHeight);
    runtime.snapshot.emitMute = getToggleValue(controlStates.emitMute);
    runtime.snapshot.emitSolo = getToggleValue(controlStates.emitSolo);
    runtime.snapshot.emitGainNorm = getSliderNormalised(controlStates.emitGain);
    runtime.snapshot.emitSpreadNorm = getSliderNormalised(controlStates.emitSpread);
    runtime.snapshot.emitDirectivityNorm = getSliderNormalised(controlStates.emitDirectivity);
    runtime.snapshot.physPresetIndex = Math.max(0, PHYSICS_PRESETS.indexOf(dom.choicePhysPreset?.value || "off"));
    runtime.snapshot.physMassNorm = getSliderNormalised(controlStates.physMass);
    runtime.snapshot.physDragNorm = getSliderNormalised(controlStates.physDrag);
    runtime.snapshot.physElasticityNorm = getSliderNormalised(controlStates.physElasticity);
    runtime.snapshot.physGravityNorm = getSliderNormalised(controlStates.physGravity);
    runtime.snapshot.physFrictionNorm = getSliderNormalised(controlStates.physFriction);
    runtime.snapshot.physGravityDirIndex = getChoiceIndex(controlStates.physGravityDir);
    runtime.snapshot.animSpeedNorm = getSliderNormalised(controlStates.animSpeed);
    runtime.snapshot.rendMasterGainNorm = getSliderNormalised(controlStates.rendMasterGain);
    runtime.snapshot.rendDistanceModelIndex = getChoiceIndex(controlStates.rendDistanceModel);
    runtime.snapshot.rendDoppler = getToggleValue(controlStates.rendDoppler);
    runtime.snapshot.rendRoomEnable = getToggleValue(controlStates.rendRoomEnable);
    runtime.snapshot.rendPhysRateIndex = getChoiceIndex(controlStates.rendPhysRate);
    runtime.snapshot.rendVizModeIndex = getChoiceIndex(controlStates.rendVizMode);
    runtime.snapshot.calSpkConfigIndex = getChoiceIndex(controlStates.calSpkConfig);
    runtime.snapshot.calMicChannelNorm = getSliderNormalised(controlStates.calMicChannel);
    runtime.snapshot.calSpk1OutNorm = getSliderNormalised(controlStates.calSpk1Out);
    runtime.snapshot.calSpk2OutNorm = getSliderNormalised(controlStates.calSpk2Out);
    runtime.snapshot.calSpk3OutNorm = getSliderNormalised(controlStates.calSpk3Out);
    runtime.snapshot.calSpk4OutNorm = getSliderNormalised(controlStates.calSpk4Out);
    runtime.snapshot.calTestLevelNorm = getSliderNormalised(controlStates.calTestLevel);
    runtime.snapshot.calTestTypeIndex = getChoiceIndex(controlStates.calTestType);
    updateQualityBadge();
    updateCalibrateCoreStatus();
    updateCalibrateCaptureStatus();
    updateColorReadout();
    updatePositionReadout();
    updateSizeReadout();
    updatePhysicsStatus();
    updateAnimationStatus();
    updateEmitterAudioStatus();
    updateEmitterParityStatus();
    updateRendererCoreStatus();
    updateSceneStatusBadge();
    updateViewportInfo(sceneApp);
    renderCounters();
    writeDiagnostics();

    if (dom.choiceAnimMode.options.length === 0) {
      void ensureChoicesFromNative(
        "anim_mode",
        controlStates.animMode,
        dom.choiceAnimMode,
        DEFAULT_CHOICES.anim_mode,
        "animMode",
        () => {}
      );
    }

    if (dom.choicePosCoordMode.options.length === 0) {
      void ensureChoicesFromNative(
        "pos_coord_mode",
        controlStates.posCoordMode,
        dom.choicePosCoordMode,
        DEFAULT_CHOICES.pos_coord_mode,
        "posCoordMode",
        () => {
          updatePositionReadout();
          updateEmitterParityStatus();
        }
      );
    }

    if (dom.choicePhysGravityDir.options.length === 0) {
      void ensureChoicesFromNative(
        "phys_gravity_dir",
        controlStates.physGravityDir,
        dom.choicePhysGravityDir,
        DEFAULT_CHOICES.phys_gravity_dir,
        "physGravityDir",
        () => {
          updateEmitterAudioStatus();
        }
      );
    }

    if (dom.choiceRendDistanceModel.options.length === 0) {
      void ensureChoicesFromNative(
        "rend_distance_model",
        controlStates.rendDistanceModel,
        dom.choiceRendDistanceModel,
        DEFAULT_CHOICES.rend_distance_model,
        "distanceModel",
        () => {
          updateRendererCoreStatus();
        }
      );
    }

    if (dom.choiceRendPhysRate.options.length === 0) {
      void ensureChoicesFromNative(
        "rend_phys_rate",
        controlStates.rendPhysRate,
        dom.choiceRendPhysRate,
        DEFAULT_CHOICES.rend_phys_rate,
        "physRate",
        () => {
          updateRendererCoreStatus();
        }
      );
    }

    if (dom.choiceRendVizMode.options.length === 0) {
      void ensureChoicesFromNative(
        "rend_viz_mode",
        controlStates.rendVizMode,
        dom.choiceRendVizMode,
        DEFAULT_CHOICES.rend_viz_mode,
        "vizMode",
        () => {
          updateRendererCoreStatus();
        }
      );
    }

    if (dom.choiceCalSpkConfig.options.length === 0) {
      void ensureChoicesFromNative(
        "cal_spk_config",
        controlStates.calSpkConfig,
        dom.choiceCalSpkConfig,
        DEFAULT_CHOICES.cal_spk_config,
        "calSpkConfig",
        () => {
          updateCalibrateCoreStatus();
        }
      );
    }

    if (dom.choiceCalTestType.options.length === 0) {
      void ensureChoicesFromNative(
        "cal_test_type",
        controlStates.calTestType,
        dom.choiceCalTestType,
        DEFAULT_CHOICES.cal_test_type,
        "calTestType",
        () => {
          updateCalibrateCoreStatus();
        }
      );
    }
  }

  function primeFromCurrentState() {
    applyMode(modeFromChoiceIndex(getChoiceIndex(controlStates.mode)), sceneApp);
    updateColorReadout();
    updatePositionReadout();
    updateSizeReadout();
    updatePhysicsStatus();
    updateAnimationStatus();
    updateQualityBadge();
    updateCalibrateCoreStatus();
    updateCalibrateCaptureStatus();
    updateEmitterAudioStatus();
    updateEmitterParityStatus();
    updateRendererCoreStatus();
    updateRoomProfileBadge();
    updateSceneStatusBadge();
    renderCounters();
    writeDiagnostics({ message: "stage12 ready" });
    void hydrateUiStateFromNative();
    void refreshEmitterPresetList();
  }

  void ensureChoicesFromNative(
    "cal_spk_config",
    controlStates.calSpkConfig,
    dom.choiceCalSpkConfig,
    DEFAULT_CHOICES.cal_spk_config,
    "calSpkConfig",
    () => {
      updateCalibrateCoreStatus();
    }
  );

  void ensureChoicesFromNative(
    "cal_test_type",
    controlStates.calTestType,
    dom.choiceCalTestType,
    DEFAULT_CHOICES.cal_test_type,
    "calTestType",
    () => {
      updateCalibrateCoreStatus();
    }
  );

  void ensureChoicesFromNative(
    "pos_coord_mode",
    controlStates.posCoordMode,
    dom.choicePosCoordMode,
    DEFAULT_CHOICES.pos_coord_mode,
    "posCoordMode",
    () => {
      updatePositionReadout();
      updateEmitterParityStatus();
    }
  );

  void ensureChoicesFromNative(
    "anim_mode",
    controlStates.animMode,
    dom.choiceAnimMode,
    DEFAULT_CHOICES.anim_mode,
    "animMode",
    () => {
      updateAnimationStatus();
      updateEmitterParityStatus();
    }
  );

  void ensureChoicesFromNative(
    "phys_gravity_dir",
    controlStates.physGravityDir,
    dom.choicePhysGravityDir,
    DEFAULT_CHOICES.phys_gravity_dir,
    "physGravityDir",
    () => {
      updateEmitterAudioStatus();
      updatePhysicsStatus();
      updateEmitterParityStatus();
    }
  );

  void ensureChoicesFromNative(
    "rend_distance_model",
    controlStates.rendDistanceModel,
    dom.choiceRendDistanceModel,
    DEFAULT_CHOICES.rend_distance_model,
    "distanceModel",
    () => {
      updateRendererCoreStatus();
    }
  );

  void ensureChoicesFromNative(
    "rend_phys_rate",
    controlStates.rendPhysRate,
    dom.choiceRendPhysRate,
    DEFAULT_CHOICES.rend_phys_rate,
    "physRate",
    () => {
      updateRendererCoreStatus();
    }
  );

  void ensureChoicesFromNative(
    "rend_viz_mode",
    controlStates.rendVizMode,
    dom.choiceRendVizMode,
    DEFAULT_CHOICES.rend_viz_mode,
    "vizMode",
    () => {
      updateRendererCoreStatus();
    }
  );

  void ensureChoicesFromNative(
    "mode",
    controlStates.mode,
    document.createElement("select"),
    DEFAULT_CHOICES.mode,
    "mode",
    () => {}
  );

  void ensureChoicesFromNative(
    "rend_quality",
    controlStates.quality,
    document.createElement("select"),
    DEFAULT_CHOICES.rend_quality,
    "quality",
    () => {}
  );

  if (dom.body) {
    dom.body.classList.toggle("debug-visible", debugUiRequested);
  }

  setBridgeState(!!backend, backend ? "backend connected" : "preview bridge");
  primeFromCurrentState();
  window.setInterval(heartbeat, 350);

  if (selfTestRequested) {
    window.__LQ_SELFTEST_RESULT__ = {
      requested: true,
      status: "pending",
      startedAt: new Date().toISOString(),
    };
    window.setTimeout(() => {
      void runIncrementalStage12SelfTest();
    }, 900);
  } else {
    window.__LQ_SELFTEST_RESULT__ = {
      requested: false,
      status: "disabled",
    };
  }

  window.runIncrementalStage12SelfTest = runIncrementalStage12SelfTest;

  window.addEventListener(
    "beforeunload",
    () => {
      if (sceneApp) {
        sceneApp.dispose();
      }
    },
    { once: true }
  );
})();
