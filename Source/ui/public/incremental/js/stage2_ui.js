(() => {
  "use strict";

  const DEFAULT_CHOICES = {
    mode: ["Calibrate", "Emitter", "Renderer"],
    rend_quality: ["Draft", "Final"],
    anim_mode: ["DAW", "Internal"],
  };

  const MODE_ORDER = ["calibrate", "emitter", "renderer"];
  const LANE_COLORS = {
    azimuth: 0xd4a847,
    elevation: 0x44aa66,
    distance: 0xaa4444,
    size: 0x4e86d8,
  };

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
    choiceAnimMode: document.getElementById("choice-anim-mode"),
    sliderSizeUniform: document.getElementById("slider-size-uniform"),
    readoutSizeUniform: document.getElementById("readout-size-uniform"),
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
      sizeLink: false,
      qualityIndex: 0,
      sizeUniformNorm: 0,
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
      animMode: false,
    },
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

    const emit = (identifier, payload) => {
      backend.emitEvent(identifier, payload);
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
        if (name === "locusqGetChoiceItems") {
          return async function noopChoiceItems() {
            return [];
          };
        }

        return async function fallbackNative() {
          throw new Error(`Native function unavailable: ${name}`);
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
        size_uniform: { scaled: 0.5, start: 0.01, end: 20, skew: 0.4 },
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
        anim_enable: false,
        anim_loop: false,
        anim_sync: true,
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
        rend_quality: 0,
        anim_mode: 0,
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

  const controlStates = {
    mode: bridge.getComboBoxState("mode"),
    quality: bridge.getComboBoxState("rend_quality"),
    sizeLink: bridge.getToggleState("size_link"),
    sizeUniform: bridge.getSliderState("size_uniform"),
    physEnable: bridge.getToggleState("phys_enable"),
    animEnable: bridge.getToggleState("anim_enable"),
    animMode: bridge.getComboBoxState("anim_mode"),
    animLoop: bridge.getToggleState("anim_loop"),
    animSync: bridge.getToggleState("anim_sync"),
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
    const status = runtime.calibrationStatus || {};
    const profileReady = !!status.profileValid;
    dom.roomDot.classList.toggle("loaded", profileReady);
    dom.roomLabel.textContent = profileReady ? "Profile Ready" : "No Profile";
  }

  function updateSceneStatusBadge() {
    const sceneStatus = dom.sceneStatus;
    if (!sceneStatus) return;

    sceneStatus.className = "scene-status";

    if (runtime.currentMode === "calibrate") {
      const status = runtime.calibrationStatus || {};
      if (status.running) {
        sceneStatus.textContent = "MEASURING";
        sceneStatus.classList.add("measuring");
      } else if (status.complete || status.profileValid) {
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

  function updateViewportInfo(sceneApp) {
    const scene = runtime.latestScene || {};
    const emitterCount = Number.isFinite(scene.emitterCount) ? Number(scene.emitterCount) : 0;
    const orbit = sceneApp ? sceneApp.getOrbitState() : null;
    const orbitText = orbit
      ? ` - cam t:${orbit.theta.toFixed(2)} p:${orbit.phi.toFixed(2)} r:${orbit.radius.toFixed(2)}`
      : "";

    if (runtime.currentMode === "calibrate") {
      dom.viewportInfo.textContent = `Calibrate Mode - profile capture shell${orbitText}`;
      return;
    }

    if (runtime.currentMode === "renderer") {
      const channels = Number.isFinite(scene.outputChannels) ? Number(scene.outputChannels) : 0;
      dom.viewportInfo.textContent = `Renderer Mode - ${emitterCount} emitters - out ${channels}ch${orbitText}`;
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
      if (typeof onSynced === "function") onSynced();
      renderCounters();
      writeDiagnostics({ event: `${input.id}.change` });
    });

    state.valueChangedEvent.addListener(() => {
      runtime.counters.valueToggle += 1;
      input.checked = getToggleValue(state);
      if (typeof onSynced === "function") onSynced();
      renderCounters();
    });

    state.propertiesChangedEvent.addListener(() => {
      runtime.counters.propsToggle += 1;
      renderCounters();
    });

    input.checked = getToggleValue(state);
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
  });

  bindToggle(dom.togglePhysEnable, controlStates.physEnable, () => {
    updateSceneStatusBadge();
  });

  bindToggle(dom.toggleAnimEnable, controlStates.animEnable);
  bindToggle(dom.toggleAnimLoop, controlStates.animLoop);
  bindToggle(dom.toggleAnimSync, controlStates.animSync);

  bindChoice(
    dom.choiceAnimMode,
    controlStates.animMode,
    DEFAULT_CHOICES.anim_mode,
    () => {},
    "anim_mode",
    "animMode"
  );

  bindSlider(dom.sliderSizeUniform, controlStates.sizeUniform, dom.readoutSizeUniform, normalised => {
    runtime.snapshot.sizeUniformNorm = normalised;
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

  setActiveLane(runtime.selectedLane);
  if (sceneApp) {
    sceneApp.setLane(runtime.selectedLane, true);
  }

  window.updateSceneState = function updateSceneState(data) {
    runtime.latestScene = data || null;
    if (sceneApp) {
      sceneApp.setPendingScene(runtime.latestScene);
    }
    updateTimelineTimeFromScene();
    updateViewportInfo(sceneApp);
  };

  window.updateCalibrationStatus = function updateCalibrationStatus(status) {
    runtime.calibrationStatus = status || null;
    updateRoomProfileBadge();
    updateSceneStatusBadge();
  };

  function heartbeat() {
    runtime.counters.heartbeat += 1;
    runtime.snapshot.sizeLink = getToggleValue(controlStates.sizeLink);
    runtime.snapshot.qualityIndex = getChoiceIndex(controlStates.quality);
    runtime.snapshot.sizeUniformNorm = getSliderNormalised(controlStates.sizeUniform);
    updateQualityBadge();
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
  }

  function primeFromCurrentState() {
    applyMode(modeFromChoiceIndex(getChoiceIndex(controlStates.mode)), sceneApp);
    updateQualityBadge();
    updateRoomProfileBadge();
    updateSceneStatusBadge();
    renderCounters();
    writeDiagnostics({ message: "stage2 ready" });
  }

  void ensureChoicesFromNative(
    "anim_mode",
    controlStates.animMode,
    dom.choiceAnimMode,
    DEFAULT_CHOICES.anim_mode,
    "animMode",
    () => {}
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

  setBridgeState(!!backend, backend ? "backend connected" : "preview bridge");
  primeFromCurrentState();
  window.setInterval(heartbeat, 350);

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
