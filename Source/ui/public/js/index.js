function createFallbackListenerList() {
    const listeners = new Map();
    let nextId = 0;
    return {
        addListener(fn) {
            const id = nextId++;
            listeners.set(id, fn);
            return id;
        },
        removeListener(id) {
            listeners.delete(id);
        },
        callListeners(...args) {
            for (const fn of listeners.values()) {
                try {
                    fn(...args);
                } catch (error) {
                    console.error("LocusQ fallback listener failed:", error);
                }
            }
        },
    };
}

function createFallbackJuceBridge() {
    const backend = window.__JUCE__ && window.__JUCE__.backend ? window.__JUCE__.backend : null;

    const sliderStates = new Map();
    const toggleStates = new Map();
    const comboStates = new Map();

    const emit = (identifier, payload) => {
        if (!backend || typeof backend.emitEvent !== "function") return;
        backend.emitEvent(identifier, payload);
    };

    const attachBackendListener = (identifier, onEvent) => {
        if (!backend || typeof backend.addEventListener !== "function") return;
        backend.addEventListener(identifier, event => onEvent(event || {}));
        emit(identifier, { eventType: "requestInitialUpdate" });
    };

    const toScaled = (normalised, start, end, skew) => {
        const clamped = Math.max(0, Math.min(1, Number(normalised) || 0));
        const safeSkew = Number.isFinite(skew) && skew > 0 ? skew : 1;
        const safeStart = Number(start) || 0;
        const safeEnd = Number(end) || 1;
        return Math.pow(clamped, 1 / safeSkew) * (safeEnd - safeStart) + safeStart;
    };

    const toNormalised = (scaled, start, end, skew) => {
        const safeStart = Number(start) || 0;
        const safeEnd = Number(end) || 1;
        const safeSkew = Number.isFinite(skew) && skew > 0 ? skew : 1;
        const denom = safeEnd - safeStart;
        if (Math.abs(denom) < 1.0e-9) return 0;
        const linear = Math.max(0, Math.min(1, (Number(scaled) - safeStart) / denom));
        return Math.pow(linear, safeSkew);
    };

    const createSliderState = name => {
        const state = {
            name,
            identifier: `__juce__slider${name}`,
            scaledValue: 0,
            properties: {
                start: 0,
                end: 1,
                skew: 1,
                interval: 0,
                name: "",
                label: "",
                numSteps: 0,
                parameterIndex: -1,
            },
            valueChangedEvent: createFallbackListenerList(),
            propertiesChangedEvent: createFallbackListenerList(),
            getScaledValue() {
                return Number(this.scaledValue) || 0;
            },
            getNormalisedValue() {
                return toNormalised(this.scaledValue, this.properties.start, this.properties.end, this.properties.skew);
            },
            setNormalisedValue(newValue) {
                this.scaledValue = toScaled(
                    newValue,
                    this.properties.start,
                    this.properties.end,
                    this.properties.skew
                );
                emit(this.identifier, {
                    eventType: "valueChanged",
                    value: this.scaledValue,
                });
                this.valueChangedEvent.callListeners();
            },
            sliderDragStarted() {},
            sliderDragEnded() {},
        };

        attachBackendListener(state.identifier, event => {
            if (event.eventType === "valueChanged") {
                state.scaledValue = Number(event.value) || 0;
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
    };

    const createToggleState = name => {
        const state = {
            name,
            identifier: `__juce__toggle${name}`,
            value: false,
            properties: { name: "", parameterIndex: -1 },
            valueChangedEvent: createFallbackListenerList(),
            propertiesChangedEvent: createFallbackListenerList(),
            getValue() {
                return !!this.value;
            },
            setValue(nextValue) {
                this.value = !!nextValue;
                emit(this.identifier, {
                    eventType: "valueChanged",
                    value: this.value,
                });
                this.valueChangedEvent.callListeners();
            },
        };

        attachBackendListener(state.identifier, event => {
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
    };

    const createComboState = name => {
        const state = {
            name,
            identifier: `__juce__comboBox${name}`,
            value: 0,
            properties: { choices: [] },
            valueChangedEvent: createFallbackListenerList(),
            propertiesChangedEvent: createFallbackListenerList(),
            getChoiceIndex() {
                const choices = Array.isArray(this.properties.choices) ? this.properties.choices.length : 0;
                if (choices <= 1) return Math.max(0, Math.round(this.value || 0));
                const normalised = Math.max(0, Math.min(1, Number(this.value) || 0));
                return Math.max(0, Math.min(choices - 1, Math.round(normalised * (choices - 1))));
            },
            setChoiceIndex(index) {
                const choices = Array.isArray(this.properties.choices) ? this.properties.choices.length : 0;
                const clamped = Math.max(0, Math.round(Number(index) || 0));
                if (choices <= 1) {
                    this.value = clamped;
                } else {
                    this.value = Math.max(0, Math.min(1, clamped / (choices - 1)));
                }
                emit(this.identifier, {
                    eventType: "valueChanged",
                    value: this.value,
                });
                this.valueChangedEvent.callListeners();
            },
            getChosenItemIndex() {
                return this.getChoiceIndex() + 1;
            },
            setChosenItemIndex(oneBasedIndex) {
                this.setChoiceIndex(Math.max(0, Math.round(Number(oneBasedIndex) || 1) - 1));
            },
        };

        attachBackendListener(state.identifier, event => {
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
    };

    return {
        getNativeFunction(name) {
            return async function fallbackNativeFunction() {
                throw new Error(`Native function bridge unavailable: ${name}`);
            };
        },
        getSliderState(name) {
            if (!sliderStates.has(name)) sliderStates.set(name, createSliderState(name));
            return sliderStates.get(name);
        },
        getToggleState(name) {
            if (!toggleStates.has(name)) toggleStates.set(name, createToggleState(name));
            return toggleStates.get(name);
        },
        getComboBoxState(name) {
            if (!comboStates.has(name)) comboStates.set(name, createComboState(name));
            return comboStates.get(name);
        },
        getBackendResourceAddress(path) {
            return path;
        },
        ControlParameterIndexUpdater: class {},
    };
}

const hasNativeJuceBridge = typeof window.Juce !== "undefined";
const Juce = window.Juce || createFallbackJuceBridge();
if (!Juce) {
    throw new Error("LocusQ: JUCE bridge API unavailable (window.Juce missing)");
}
if (!window.Juce) {
    window.Juce = Juce;
    console.warn("LocusQ: using fallback JUCE bridge wrapper");
}

// ===========================================================================
// LocusQ WebView – JUCE Parameter Integration & Three.js Viewport
// ===========================================================================

// ===== PARAMETER STATES =====
const sliderStates = {
    pos_azimuth:    Juce.getSliderState("pos_azimuth"),
    pos_elevation:  Juce.getSliderState("pos_elevation"),
    pos_distance:   Juce.getSliderState("pos_distance"),
    pos_x:          Juce.getSliderState("pos_x"),
    pos_y:          Juce.getSliderState("pos_y"),
    pos_z:          Juce.getSliderState("pos_z"),
    size_uniform:   Juce.getSliderState("size_uniform"),
    emit_gain:      Juce.getSliderState("emit_gain"),
    emit_spread:    Juce.getSliderState("emit_spread"),
    emit_directivity: Juce.getSliderState("emit_directivity"),
    emit_color:     Juce.getSliderState("emit_color"),
    cal_mic_channel: Juce.getSliderState("cal_mic_channel"),
    cal_spk1_out:   Juce.getSliderState("cal_spk1_out"),
    cal_spk2_out:   Juce.getSliderState("cal_spk2_out"),
    cal_spk3_out:   Juce.getSliderState("cal_spk3_out"),
    cal_spk4_out:   Juce.getSliderState("cal_spk4_out"),
    cal_test_level: Juce.getSliderState("cal_test_level"),
    phys_mass:      Juce.getSliderState("phys_mass"),
    phys_drag:      Juce.getSliderState("phys_drag"),
    phys_elasticity: Juce.getSliderState("phys_elasticity"),
    phys_gravity:   Juce.getSliderState("phys_gravity"),
    phys_friction:  Juce.getSliderState("phys_friction"),
    anim_speed:     Juce.getSliderState("anim_speed"),
    rend_master_gain: Juce.getSliderState("rend_master_gain"),
};

const toggleStates = {
    bypass:      Juce.getToggleState("bypass"),
    size_link:   Juce.getToggleState("size_link"),
    emit_mute:   Juce.getToggleState("emit_mute"),
    emit_solo:   Juce.getToggleState("emit_solo"),
    phys_enable: Juce.getToggleState("phys_enable"),
    phys_throw:  Juce.getToggleState("phys_throw"),
    phys_reset:  Juce.getToggleState("phys_reset"),
    anim_enable: Juce.getToggleState("anim_enable"),
    anim_loop:   Juce.getToggleState("anim_loop"),
    anim_sync:   Juce.getToggleState("anim_sync"),
    rend_doppler: Juce.getToggleState("rend_doppler"),
    rend_air_absorb: Juce.getToggleState("rend_air_absorb"),
    rend_room_enable: Juce.getToggleState("rend_room_enable"),
    rend_room_er_only: Juce.getToggleState("rend_room_er_only"),
    rend_phys_walls: Juce.getToggleState("rend_phys_walls"),
    rend_phys_pause: Juce.getToggleState("rend_phys_pause"),
};

const comboStates = {
    cal_spk_config: Juce.getComboBoxState("cal_spk_config"),
    cal_test_type: Juce.getComboBoxState("cal_test_type"),
    mode:         Juce.getComboBoxState("mode"),
    pos_coord_mode: Juce.getComboBoxState("pos_coord_mode"),
    phys_gravity_dir: Juce.getComboBoxState("phys_gravity_dir"),
    anim_mode:    Juce.getComboBoxState("anim_mode"),
    rend_quality: Juce.getComboBoxState("rend_quality"),
    rend_distance_model: Juce.getComboBoxState("rend_distance_model"),
    rend_phys_rate: Juce.getComboBoxState("rend_phys_rate"),
    rend_viz_mode: Juce.getComboBoxState("rend_viz_mode"),
};

const nativeFunctions = {
    startCalibration: Juce.getNativeFunction("locusqStartCalibration"),
    abortCalibration: Juce.getNativeFunction("locusqAbortCalibration"),
    getKeyframeTimeline: Juce.getNativeFunction("locusqGetKeyframeTimeline"),
    setKeyframeTimeline: Juce.getNativeFunction("locusqSetKeyframeTimeline"),
    setTimelineTime: Juce.getNativeFunction("locusqSetTimelineTime"),
    listEmitterPresets: Juce.getNativeFunction("locusqListEmitterPresets"),
    saveEmitterPreset: Juce.getNativeFunction("locusqSaveEmitterPreset"),
    loadEmitterPreset: Juce.getNativeFunction("locusqLoadEmitterPreset"),
    getUiState: Juce.getNativeFunction("locusqGetUiState"),
    setUiState: Juce.getNativeFunction("locusqSetUiState"),
};

const NATIVE_CALL_TIMEOUT_MS = 3000;
const BASIC_CONTROL_VALUE_CHANGED_EVENT = "valueChanged";

function withNativeTimeout(promise, label) {
    return new Promise((resolve, reject) => {
        let settled = false;
        const timer = window.setTimeout(() => {
            if (settled) return;
            settled = true;
            reject(new Error(`${label} timed out after ${NATIVE_CALL_TIMEOUT_MS}ms`));
        }, NATIVE_CALL_TIMEOUT_MS);

        Promise.resolve(promise).then(
            value => {
                if (settled) return;
                settled = true;
                window.clearTimeout(timer);
                resolve(value);
            },
            error => {
                if (settled) return;
                settled = true;
                window.clearTimeout(timer);
                reject(error);
            }
        );
    });
}

async function callNative(label, fn, ...args) {
    if (typeof fn !== "function") {
        throw new Error(`${label} unavailable`);
    }
    return withNativeTimeout(fn(...args), label);
}

function notifyStateValueChanged(state) {
    if (hasNativeJuceBridge) return;
    if (!state || !state.valueChangedEvent) return;
    if (typeof state.valueChangedEvent.callListeners !== "function") return;
    try {
        state.valueChangedEvent.callListeners();
    } catch (error) {
        console.warn("LocusQ: failed to notify local valueChanged listener", error);
    }
}

function getToggleValue(state) {
    if (!state) return false;
    if (typeof state.getValue === "function") return !!state.getValue();
    return !!state.value;
}

function setToggleValue(state, nextValue) {
    if (!state) return;
    const value = !!nextValue;
    if (typeof state.setValue === "function") {
        state.setValue(value);
        return;
    }
    state.value = value;
    notifyStateValueChanged(state);
}

function getChoiceIndex(state) {
    if (!state) return 0;
    if (typeof state.getChoiceIndex === "function") return state.getChoiceIndex();
    if (typeof state.getChosenItemIndex === "function") return Math.max(0, state.getChosenItemIndex() - 1);
    return 0;
}

function emitChoiceWithFallback(state, index, assumedChoiceCount) {
    if (!state || !window.__JUCE__ || !window.__JUCE__.backend) return;
    if (!state.identifier || typeof window.__JUCE__.backend.emitEvent !== "function") return;

    const count = Number.isFinite(assumedChoiceCount) && assumedChoiceCount > 1
        ? assumedChoiceCount
        : Math.max(index + 1, 2);
    const normalised = Math.max(0, Math.min(1, index / Math.max(1, count - 1)));
    state.value = normalised;
    window.__JUCE__.backend.emitEvent(state.identifier, {
        eventType: BASIC_CONTROL_VALUE_CHANGED_EVENT,
        value: normalised,
    });
}

function setChoiceIndex(state, index, fallbackChoiceCount = 0) {
    if (!state) return;
    if (typeof state.setChoiceIndex === "function") {
        const choicesCount = Array.isArray(state.properties?.choices) ? state.properties.choices.length : 0;
        if (choicesCount <= 1 && fallbackChoiceCount > 1) {
            // Early clicks can happen before combo properties arrive from JUCE.
            emitChoiceWithFallback(state, index, fallbackChoiceCount);
        } else {
            state.setChoiceIndex(index);
        }
        return;
    }
    if (typeof state.setChosenItemIndex === "function") {
        state.setChosenItemIndex(index + 1);
        return;
    }
    notifyStateValueChanged(state);
}

function setToggleClass(id, isOn) {
    const el = document.getElementById(id);
    if (el) el.classList.toggle("on", !!isOn);
}

function toggleStateAndClass(toggleId, state) {
    const nextValue = !getToggleValue(state);
    setToggleValue(state, nextValue);
    if (toggleId) setToggleClass(toggleId, nextValue);
    return nextValue;
}

function bindControlActivate(element, handler) {
    if (!element || typeof handler !== "function") return;

    const trigger = event => {
        if (event && typeof event.preventDefault === "function") {
            event.preventDefault();
        }
        handler(event);
    };

    element.addEventListener("click", trigger, { passive: false });
    element.addEventListener("keydown", event => {
        if (event.key === "Enter" || event.key === " ") {
            trigger(event);
        }
    });
}

function setAnimationControlsEnabled(enabled) {
    const controls = document.getElementById("anim-controls");
    const source = document.getElementById("anim-source");
    if (controls) controls.style.opacity = enabled ? "1.0" : "0.4";
    if (source) source.disabled = !enabled;
}

function syncAnimationUI() {
    const enabled = !!toggleStates.anim_enable.getValue();
    setToggleClass("toggle-anim", enabled);
    setAnimationControlsEnabled(enabled);

    const source = document.getElementById("anim-source");
    if (source) source.selectedIndex = Math.max(0, Math.min(1, getChoiceIndex(comboStates.anim_mode)));

    const loopEnabled = !!toggleStates.anim_loop.getValue();
    setToggleClass("toggle-anim-loop", loopEnabled);
    setToggleClass("toggle-timeline-loop", loopEnabled);

    const syncEnabled = !!toggleStates.anim_sync.getValue();
    setToggleClass("toggle-timeline-sync", syncEnabled);

    updateValueDisplay("val-anim-speed", sliderStates.anim_speed.getScaledValue().toFixed(1), "x");
    timelineState.looping = loopEnabled;
    timelineState.playbackRate = sliderStates.anim_speed.getScaledValue();
}

function clamp(value, min, max) {
    return Math.min(max, Math.max(min, value));
}

function setSliderScaledValue(sliderState, scaledValue) {
    if (!sliderState || !sliderState.properties) return;

    const start = Number(sliderState.properties.start ?? 0.0);
    const end = Number(sliderState.properties.end ?? 1.0);
    const skew = Number(sliderState.properties.skew ?? 1.0);
    const denominator = end - start;
    if (!Number.isFinite(denominator) || Math.abs(denominator) < 1.0e-8) return;

    const clampedScaled = clamp(Number(scaledValue), Math.min(start, end), Math.max(start, end));
    const linearNorm = clamp((clampedScaled - start) / denominator, 0.0, 1.0);
    const safeSkew = Number.isFinite(skew) && skew > 0.0 ? skew : 1.0;
    sliderState.setNormalisedValue(Math.pow(linearNorm, safeSkew));
    notifyStateValueChanged(sliderState);
}

function sanitizeEmitterLabel(label) {
    const text = String(label ?? "").trim();
    if (!text) return "Emitter";
    return text.slice(0, 31);
}

function normalizePhysicsPresetName(name) {
    const value = String(name ?? "").trim().toLowerCase();
    if (value === "off" || value === "bounce" || value === "float" || value === "orbit") {
        return value;
    }
    return "custom";
}

let uiState = {
    emitterLabel: "Emitter",
    physicsPreset: "off",
};

let uiStateCommitTimer = null;
let isApplyingPhysicsPreset = false;

async function commitUiStateToNative() {
    try {
        await callNative("locusqSetUiState", nativeFunctions.setUiState, {
            emitterLabel: sanitizeEmitterLabel(uiState.emitterLabel),
            physicsPreset: normalizePhysicsPresetName(uiState.physicsPreset),
        });
    } catch (error) {
        console.warn("Failed to commit UI state:", error);
    }
}

function scheduleUiStateCommit(immediate = false) {
    if (uiStateCommitTimer !== null) {
        window.clearTimeout(uiStateCommitTimer);
        uiStateCommitTimer = null;
    }

    uiStateCommitTimer = window.setTimeout(() => {
        uiStateCommitTimer = null;
        commitUiStateToNative();
    }, immediate ? 0 : 80);
}

async function loadUiStateFromNative() {
    try {
        const payload = await callNative("locusqGetUiState", nativeFunctions.getUiState);
        if (!payload || typeof payload !== "object") return;
        uiState.emitterLabel = sanitizeEmitterLabel(payload.emitterLabel ?? uiState.emitterLabel);
        uiState.physicsPreset = normalizePhysicsPresetName(payload.physicsPreset ?? uiState.physicsPreset);
    } catch (error) {
        console.warn("Failed to load UI state:", error);
    }
}

function bindSelectToComboState(selectId, comboState) {
    const select = document.getElementById(selectId);
    if (!select || !comboState) return;

    select.addEventListener("change", () => {
        setChoiceIndex(comboState, select.selectedIndex);
    });

    comboState.valueChangedEvent.addListener(() => {
        const idx = getChoiceIndex(comboState);
        select.selectedIndex = clamp(idx, 0, Math.max(0, select.options.length - 1));
    });
}

function bindSelectToIntSliderState(selectId, sliderState, minValue = 1) {
    const select = document.getElementById(selectId);
    if (!select || !sliderState) return;

    select.addEventListener("change", () => {
        const nextValue = Number(select.selectedIndex) + Number(minValue);
        setSliderScaledValue(sliderState, nextValue);
    });

    sliderState.valueChangedEvent.addListener(() => {
        const scaled = Number(sliderState.getScaledValue());
        const rounded = Math.round(scaled);
        const index = clamp(rounded - Number(minValue), 0, Math.max(0, select.options.length - 1));
        select.selectedIndex = index;
    });
}

function bindValueStepper(displayId, sliderState, options = {}) {
    const display = document.getElementById(displayId);
    if (!display || !sliderState) return;

    const step = Number(options.step ?? 0.1);
    const min = Number.isFinite(options.min) ? Number(options.min) : Number(sliderState.properties?.start ?? -1.0e9);
    const max = Number.isFinite(options.max) ? Number(options.max) : Number(sliderState.properties?.end ?? 1.0e9);
    const roundDigits = Number.isFinite(options.roundDigits) ? Number(options.roundDigits) : 3;

    const applyDelta = (delta) => {
        const current = Number(sliderState.getScaledValue());
        const next = clamp(current + delta, Math.min(min, max), Math.max(min, max));
        const factor = Math.pow(10, Math.max(0, roundDigits));
        const rounded = Math.round(next * factor) / factor;
        sliderState.sliderDragStarted();
        setSliderScaledValue(sliderState, rounded);
        sliderState.sliderDragEnded();
    };

    display.style.cursor = "ns-resize";
    display.title = `Click: +${step} · Right-click: -${step} · Wheel adjust`;
    display.addEventListener("click", () => applyDelta(step));
    display.addEventListener("contextmenu", event => {
        event.preventDefault();
        applyDelta(-step);
    });
    display.addEventListener("wheel", event => {
        event.preventDefault();
        applyDelta(event.deltaY < 0 ? step : -step);
    }, { passive: false });
}

function pulseToggleParameter(toggleState) {
    if (!toggleState) return;
    setToggleValue(toggleState, true);
    window.setTimeout(() => setToggleValue(toggleState, false), 40);
}

function applyUiStateToControls() {
    const emitLabel = document.getElementById("emit-label");
    if (emitLabel && document.activeElement !== emitLabel) {
        emitLabel.value = sanitizeEmitterLabel(uiState.emitterLabel);
    }

    const physicsPreset = document.getElementById("physics-preset");
    if (physicsPreset) {
        const normalized = normalizePhysicsPresetName(uiState.physicsPreset);
        const target = normalized === "custom" ? "custom" : normalized;
        const matching = Array.from(physicsPreset.options).findIndex(option => option.value === target);
        if (matching >= 0) physicsPreset.selectedIndex = matching;
    }
}

function createKeyframe(timeSeconds, value, curve = "easeInOut") {
    return {
        uid: `kf_${nextKeyframeUid++}`,
        timeSeconds,
        value,
        curve,
    };
}

function getTrackForLane(lane) {
    const parameterId = laneTrackMap[lane];
    return timelineState.tracks[parameterId] || [];
}

function setTrackForLane(lane, track) {
    const parameterId = laneTrackMap[lane];
    timelineState.tracks[parameterId] = [...track].sort((a, b) => a.timeSeconds - b.timeSeconds);
}

function findKeyframeByUid(lane, uid) {
    const track = getTrackForLane(lane);
    return track.find(kf => kf.uid === uid) || null;
}

function normaliseTimelineFromNative(payload) {
    const state = {
        durationSeconds: clamp(Number(payload?.durationSeconds ?? 8.0), 0.25, 120.0),
        looping: payload?.looping !== undefined ? !!payload.looping : true,
        playbackRate: clamp(Number(payload?.playbackRate ?? 1.0), 0.1, 10.0),
        currentTimeSeconds: Math.max(0.0, Number(payload?.currentTimeSeconds ?? 0.0)),
        tracks: {},
    };

    if (Array.isArray(payload?.tracks)) {
        payload.tracks.forEach(track => {
            const parameterId = String(track?.parameterId || "").trim();
            if (!parameterId) return;

            const keyframes = [];
            if (Array.isArray(track?.keyframes)) {
                track.keyframes.forEach(kf => {
                    const timeSeconds = clamp(Number(kf?.timeSeconds ?? 0.0), 0.0, state.durationSeconds);
                    const value = Number(kf?.value ?? 0.0);
                    const curve = curveOrder.includes(kf?.curve) ? kf.curve : "linear";
                    keyframes.push(createKeyframe(timeSeconds, value, curve));
                });
            }

            if (keyframes.length > 0) {
                state.tracks[parameterId] = keyframes.sort((a, b) => a.timeSeconds - b.timeSeconds);
            }
        });
    }

    const laneDefaults = {
        azimuth: sliderStates.pos_azimuth.getScaledValue(),
        elevation: sliderStates.pos_elevation.getScaledValue(),
        distance: sliderStates.pos_distance.getScaledValue(),
        size: 0.5,
    };

    Object.keys(laneTrackMap).forEach(lane => {
        const parameterId = laneTrackMap[lane];
        if (!Array.isArray(state.tracks[parameterId]) || state.tracks[parameterId].length === 0) {
            const defaultValue = laneDefaults[lane] ?? 0.0;
            state.tracks[parameterId] = [
                createKeyframe(0.0, defaultValue, "easeInOut"),
                createKeyframe(state.durationSeconds, defaultValue, "easeInOut"),
            ];
        }
    });

    return state;
}

function serialiseTimelineForNative() {
    const tracks = Object.entries(timelineState.tracks).map(([parameterId, keyframes]) => ({
        parameterId,
        keyframes: keyframes.map(kf => ({
            timeSeconds: clamp(Number(kf.timeSeconds), 0.0, timelineState.durationSeconds),
            value: Number(kf.value),
            curve: curveOrder.includes(kf.curve) ? kf.curve : "linear",
        })),
    }));

    return {
        durationSeconds: timelineState.durationSeconds,
        looping: !!timelineState.looping,
        playbackRate: clamp(Number(timelineState.playbackRate || 1.0), 0.1, 10.0),
        currentTimeSeconds: clamp(Number(timelineState.currentTimeSeconds || 0.0), 0.0, timelineState.durationSeconds),
        tracks,
    };
}

function timelinePointFromPointer(event, lane, laneTrackElement) {
    const rect = laneTrackElement.getBoundingClientRect();
    const duration = Math.max(0.001, timelineState.durationSeconds);
    const range = laneRanges[lane] || { min: 0.0, max: 1.0 };
    const xNorm = clamp((event.clientX - rect.left) / Math.max(1.0, rect.width), 0.0, 1.0);
    const yNorm = clamp((event.clientY - rect.top) / Math.max(1.0, rect.height), 0.0, 1.0);

    const timeSeconds = xNorm * duration;
    const value = range.max - yNorm * (range.max - range.min);
    return { timeSeconds, value };
}

function updateLaneCurveBadge(lane) {
    const badge = document.getElementById(`lane-curve-${lane}`);
    if (!badge) return;

    const track = getTrackForLane(lane);
    if (track.length === 0) {
        badge.textContent = "-";
        return;
    }

    const time = clamp(Number(timelineState.currentTimeSeconds || 0.0), 0.0, timelineState.durationSeconds);
    let selectedCurve = track[0].curve || "linear";
    for (let i = 0; i < track.length - 1; i++) {
        const left = track[i];
        const right = track[i + 1];
        if (time >= left.timeSeconds && time <= right.timeSeconds) {
            selectedCurve = left.curve || "linear";
            break;
        }
    }

    badge.textContent = curveShortLabels[selectedCurve] || selectedCurve;
}

function updateTimelinePlayheads() {
    const duration = Math.max(0.001, timelineState.durationSeconds);
    const normalized = clamp((timelineState.currentTimeSeconds || 0.0) / duration, 0.0, 1.0);
    document.querySelectorAll(".timeline-playhead").forEach(playhead => {
        playhead.style.left = `${normalized * 100}%`;
    });
}

function renderTimelineLanes() {
    const duration = Math.max(0.001, timelineState.durationSeconds);

    document.querySelectorAll(".timeline-lane").forEach(laneElement => {
        const lane = laneElement.dataset.lane;
        const laneTrack = laneElement.querySelector(".lane-track");
        if (!laneTrack || !lane) return;

        const range = laneRanges[lane] || { min: 0.0, max: 1.0 };
        laneTrack.innerHTML = "";

        const playhead = document.createElement("div");
        playhead.className = "timeline-playhead";
        laneTrack.appendChild(playhead);

        const track = getTrackForLane(lane);
        track.forEach(keyframe => {
            const dot = document.createElement("div");
            dot.className = "keyframe-dot";
            dot.dataset.uid = keyframe.uid;
            dot.title = `t=${keyframe.timeSeconds.toFixed(3)}s · v=${keyframe.value.toFixed(2)} · ${keyframe.curve}`;

            const xNorm = clamp(keyframe.timeSeconds / duration, 0.0, 1.0);
            const yNorm = 1.0 - clamp((keyframe.value - range.min) / Math.max(0.0001, range.max - range.min), 0.0, 1.0);
            dot.style.left = `${xNorm * 100}%`;
            dot.style.top = `${yNorm * 100}%`;

            dot.addEventListener("pointerdown", event => {
                event.stopPropagation();
                dot.setPointerCapture(event.pointerId);
                dot.classList.add("dragging");
                draggingKeyframe = { lane, uid: keyframe.uid, pointerId: event.pointerId };
            });

            dot.addEventListener("pointermove", event => {
                if (!draggingKeyframe) return;
                if (draggingKeyframe.uid !== keyframe.uid || draggingKeyframe.pointerId !== event.pointerId) return;
                const moved = findKeyframeByUid(lane, keyframe.uid);
                if (!moved) return;
                const point = timelinePointFromPointer(event, lane, laneTrack);
                moved.timeSeconds = clamp(point.timeSeconds, 0.0, duration);
                moved.value = clamp(point.value, range.min, range.max);

                const movedX = clamp(moved.timeSeconds / duration, 0.0, 1.0);
                const movedY = 1.0 - clamp((moved.value - range.min) / Math.max(0.0001, range.max - range.min), 0.0, 1.0);
                dot.style.left = `${movedX * 100}%`;
                dot.style.top = `${movedY * 100}%`;
                dot.title = `t=${moved.timeSeconds.toFixed(3)}s · v=${moved.value.toFixed(2)} · ${moved.curve}`;
            });

            const commitDrag = (event) => {
                if (!draggingKeyframe) return;
                if (draggingKeyframe.uid !== keyframe.uid || draggingKeyframe.pointerId !== event.pointerId) return;
                draggingKeyframe = null;
                dot.classList.remove("dragging");
                setTrackForLane(lane, getTrackForLane(lane));
                renderTimelineLanes();
                scheduleTimelineCommit(true);
            };
            dot.addEventListener("pointerup", commitDrag);
            dot.addEventListener("pointercancel", commitDrag);

            dot.addEventListener("dblclick", event => {
                event.stopPropagation();
                const edited = findKeyframeByUid(lane, keyframe.uid);
                if (!edited) return;
                const idx = curveOrder.indexOf(edited.curve);
                edited.curve = curveOrder[(idx + 1) % curveOrder.length];
                renderTimelineLanes();
                scheduleTimelineCommit(true);
            });

            dot.addEventListener("contextmenu", event => {
                event.preventDefault();
                event.stopPropagation();
                const reduced = getTrackForLane(lane).filter(kf => kf.uid !== keyframe.uid);
                if (reduced.length < 2) return;
                setTrackForLane(lane, reduced);
                renderTimelineLanes();
                scheduleTimelineCommit(true);
            });

            laneTrack.appendChild(dot);
        });

        laneTrack.ondblclick = event => {
            if (event.target.closest(".keyframe-dot")) return;
            const point = timelinePointFromPointer(event, lane, laneTrack);
            const newKeyframe = createKeyframe(
                clamp(point.timeSeconds, 0.0, duration),
                clamp(point.value, range.min, range.max),
                "easeInOut"
            );
            const updatedTrack = [...getTrackForLane(lane), newKeyframe];
            setTrackForLane(lane, updatedTrack);
            renderTimelineLanes();
            scheduleTimelineCommit(true);
        };

        laneTrack.onclick = event => {
            if (event.target.closest(".keyframe-dot")) return;
            const point = timelinePointFromPointer(event, lane, laneTrack);
            timelineState.currentTimeSeconds = clamp(point.timeSeconds, 0.0, duration);
            updateTimelinePlayheads();
            callNative("locusqSetTimelineTime", nativeFunctions.setTimelineTime, timelineState.currentTimeSeconds).catch(() => {});
        };

        updateLaneCurveBadge(lane);
    });

    updateTimelinePlayheads();
}

function scheduleTimelineCommit(immediate = false) {
    if (!timelineLoaded) return;
    if (timelineCommitTimer !== null) {
        window.clearTimeout(timelineCommitTimer);
        timelineCommitTimer = null;
    }

    timelineCommitTimer = window.setTimeout(() => {
        timelineCommitTimer = null;
        commitTimelineToNative();
    }, immediate ? 0 : 80);
}

async function commitTimelineToNative() {
    try {
        await callNative("locusqSetKeyframeTimeline", nativeFunctions.setKeyframeTimeline, serialiseTimelineForNative());
    } catch (error) {
        console.warn("Failed to commit keyframe timeline:", error);
    }
}

async function loadTimelineFromNative() {
    try {
        const payload = await callNative("locusqGetKeyframeTimeline", nativeFunctions.getKeyframeTimeline);
        timelineState = normaliseTimelineFromNative(payload || {});
        timelineLoaded = true;
        renderTimelineLanes();
    } catch (error) {
        console.warn("Failed to load keyframe timeline from native API:", error);
        timelineState = normaliseTimelineFromNative({});
        timelineLoaded = true;
        renderTimelineLanes();
    }
}

function setPresetStatus(message, isError = false) {
    const status = document.getElementById("preset-status");
    if (!status) return;
    status.textContent = message;
    status.style.color = isError ? "var(--text-error, #D4736F)" : "var(--text-secondary)";
}

async function refreshPresetList() {
    const select = document.getElementById("preset-select");
    if (!select) return;

    select.innerHTML = "";

    try {
        const items = await callNative("locusqListEmitterPresets", nativeFunctions.listEmitterPresets);
        presetEntries = Array.isArray(items) ? items : [];
    } catch (error) {
        presetEntries = [];
        setPresetStatus("Preset listing failed", true);
        return;
    }

    if (presetEntries.length === 0) {
        const emptyOption = document.createElement("option");
        emptyOption.textContent = "No presets";
        emptyOption.value = "";
        select.appendChild(emptyOption);
        setPresetStatus("No presets saved yet");
        return;
    }

    presetEntries.forEach((entry, index) => {
        const option = document.createElement("option");
        option.value = entry.path || entry.file || "";
        option.textContent = entry.name || entry.file || `Preset ${index + 1}`;
        select.appendChild(option);
    });

    setPresetStatus(`${presetEntries.length} preset(s) available`);
}

// ===== DESATURATED EMITTER PALETTE (v2) =====
const emitterPalette = [
    0xD4736F, 0x5BBAB3, 0x5AADC0, 0x8DBEA7, 0xD8CFA0, 0xBF9ABD, 0x8CC5B7, 0xCCBA6E,
    0xA487B5, 0x7AAFC9, 0xC9A07A, 0x7DC49A, 0xC98A84, 0x96BAD0, 0xB3A0BF, 0x8EC8BD
];
const roomBounds = { halfWidth: 2.7, halfDepth: 1.7 };

// ===== APP STATE =====
let currentMode = "emitter";
let selectedLane = "azimuth";
let sceneData = {
    emitters: [],
    emitterCount: 0,
    rendererActive: false,
    outputChannels: 2,
    outputLayout: "stereo",
    rendererOutputChannels: ["L", "R"],
    rendererInternalSpeakers: ["FL", "FR", "RR", "RL"],
    rendererQuadMap: [0, 1, 3, 2],
};
let selectedEmitterId = -1;
let localEmitterId = -1;
const railScrollByMode = { calibrate: 0, emitter: 0, renderer: 0 };
let calibrationState = {
    state: "idle",
    running: false,
    complete: false,
    currentSpeaker: 1,
    completedSpeakers: 0,
    playPercent: 0,
    recordPercent: 0,
    overallPercent: 0,
    message: "Idle - press Start to begin calibration",
};

const laneTrackMap = {
    azimuth: "pos_azimuth",
    elevation: "pos_elevation",
    distance: "pos_distance",
    size: "size_uniform",
};

const laneRanges = {
    azimuth: { min: -180.0, max: 180.0 },
    elevation: { min: -90.0, max: 90.0 },
    distance: { min: 0.0, max: 50.0 },
    size: { min: 0.01, max: 20.0 },
};

const curveOrder = ["linear", "easeIn", "easeOut", "easeInOut", "step"];
const curveShortLabels = {
    linear: "lin",
    easeIn: "in",
    easeOut: "out",
    easeInOut: "ease",
    step: "step",
};

let nextKeyframeUid = 1;
let timelineState = {
    durationSeconds: 8.0,
    looping: true,
    playbackRate: 1.0,
    currentTimeSeconds: 0.0,
    tracks: {},
};
let timelineCommitTimer = null;
let timelineLoaded = false;
let draggingKeyframe = null;
let presetEntries = [];
const runtimeState = {
    viewportReady: false,
    viewportDegraded: false,
};

// ===== THREE.JS SETUP =====
let threeScene, camera, rendererGL, canvas;
let roomLines, gridHelper, speakers = [], speakerMeters = [];
let emitterMeshes = new Map();
let selectionRing, trail;
let azArc, elArc, distRing;
let spherical = { theta: Math.PI / 4, phi: Math.PI / 4, radius: 8 };
let orbitTarget;
let isDragging = false, isRight = false, prevMouse = { x: 0, y: 0 };
let animTime = 0;
let selectionRingBaseY = 0.0;
let viewportRaycaster;
let viewportPointer;
let dragPlane;
let dragPoint;
let dragOffset;
let dragTarget;
let emitterDragState = null;

// Lane highlight state
let highlightTargets = { azimuth: 0.25, elevation: 0, distance: 0, size: 0 };
let highlightCurrent = { azimuth: 0, elevation: 0, distance: 0, size: 0 };

function getPointerNdc(event) {
    if (!canvas || !viewportPointer) return null;
    const rect = canvas.getBoundingClientRect();
    if (rect.width <= 0 || rect.height <= 0) return null;

    viewportPointer.x = ((event.clientX - rect.left) / rect.width) * 2.0 - 1.0;
    viewportPointer.y = -((event.clientY - rect.top) / rect.height) * 2.0 + 1.0;
    return viewportPointer;
}

function getEmitterById(emitterId) {
    if (!Number.isInteger(emitterId)) return null;
    return sceneData.emitters?.find(em => em.id === emitterId) || null;
}

function getSelectedEmitter() {
    return getEmitterById(selectedEmitterId);
}

function updateSelectionRingFromState() {
    if (!selectionRing) return;

    const selectedMesh = emitterMeshes.get(selectedEmitterId);
    if (!selectedMesh || currentMode !== "emitter") {
        selectionRing.visible = false;
        return;
    }

    selectionRing.position.copy(selectedMesh.position);
    selectionRingBaseY = selectedMesh.position.y;
    selectionRing.visible = true;
}

function setSelectedEmitter(emitterId) {
    if (!Number.isInteger(emitterId)) return;
    selectedEmitterId = emitterId;
    updateSelectionRingFromState();
}

function pickEmitterIntersection(event) {
    if (!camera || !viewportRaycaster || emitterMeshes.size === 0) return null;
    const pointer = getPointerNdc(event);
    if (!pointer) return null;

    viewportRaycaster.setFromCamera(pointer, camera);
    const meshes = Array.from(emitterMeshes.values()).filter(mesh => mesh.visible);
    if (meshes.length === 0) return null;

    const intersections = viewportRaycaster.intersectObjects(meshes, false);
    return intersections.length > 0 ? intersections[0] : null;
}

function beginEmitterDrag(event, intersection) {
    if (!intersection || !dragPlane || !dragOffset || !dragTarget || !viewportRaycaster) return false;

    const emitterId = Number(intersection.object?.userData?.emitterId);
    if (!Number.isInteger(emitterId) || emitterId !== localEmitterId) return false;

    dragPlane.set(new THREE.Vector3(0, 1, 0), -intersection.object.position.y);
    dragOffset.copy(intersection.point).sub(intersection.object.position);

    sliderStates.pos_azimuth.sliderDragStarted();
    sliderStates.pos_elevation.sliderDragStarted();
    sliderStates.pos_distance.sliderDragStarted();
    sliderStates.pos_x.sliderDragStarted();
    sliderStates.pos_y.sliderDragStarted();
    sliderStates.pos_z.sliderDragStarted();

    emitterDragState = {
        emitterId,
        pointerId: event.pointerId ?? 0,
    };
    canvas.style.cursor = "grabbing";
    isDragging = false;
    return true;
}

function endEmitterDrag() {
    if (!emitterDragState) return;

    emitterDragState = null;
    canvas.style.cursor = "";
    sliderStates.pos_azimuth.sliderDragEnded();
    sliderStates.pos_elevation.sliderDragEnded();
    sliderStates.pos_distance.sliderDragEnded();
    sliderStates.pos_x.sliderDragEnded();
    sliderStates.pos_y.sliderDragEnded();
    sliderStates.pos_z.sliderDragEnded();
}

function applyEmitterWorldPositionToParameters(worldPosition) {
    const x = Number(worldPosition.x) || 0.0;
    const y = Number(worldPosition.y) || 0.0;
    const z = Number(worldPosition.z) || 0.0;

    const distance = clamp(Math.sqrt(x * x + y * y + z * z), laneRanges.distance.min, laneRanges.distance.max);
    const azimuth = clamp((Math.atan2(x, z) * 180.0) / Math.PI, laneRanges.azimuth.min, laneRanges.azimuth.max);
    const elevation = distance > 1.0e-5
        ? clamp((Math.asin(clamp(y / distance, -1.0, 1.0)) * 180.0) / Math.PI, laneRanges.elevation.min, laneRanges.elevation.max)
        : 0.0;

    setSliderScaledValue(sliderStates.pos_azimuth, azimuth);
    setSliderScaledValue(sliderStates.pos_elevation, elevation);
    setSliderScaledValue(sliderStates.pos_distance, distance);

    // Keep Cartesian APVTS coordinates in sync with viewport drag as well.
    // Processor mapping is: pos_x->x, pos_y->z, pos_z->y.
    setSliderScaledValue(sliderStates.pos_x, x);
    setSliderScaledValue(sliderStates.pos_y, z);
    setSliderScaledValue(sliderStates.pos_z, y);
}

function updateEmitterDrag(event) {
    if (!emitterDragState || !viewportRaycaster || !dragPlane || !dragPoint || !dragOffset || !dragTarget) return false;
    const pointer = getPointerNdc(event);
    if (!pointer || !camera) return false;

    viewportRaycaster.setFromCamera(pointer, camera);
    if (!viewportRaycaster.ray.intersectPlane(dragPlane, dragPoint)) return false;

    dragTarget.copy(dragPoint).sub(dragOffset);
    dragTarget.x = clamp(dragTarget.x, -roomBounds.halfWidth, roomBounds.halfWidth);
    dragTarget.z = clamp(dragTarget.z, -roomBounds.halfDepth, roomBounds.halfDepth);

    applyEmitterWorldPositionToParameters(dragTarget);

    const selectedMesh = emitterMeshes.get(emitterDragState.emitterId);
    if (selectedMesh) {
        selectedMesh.position.copy(dragTarget);
    }
    if (selectionRing && currentMode === "emitter") {
        selectionRing.position.copy(dragTarget);
        selectionRingBaseY = dragTarget.y;
        selectionRing.visible = true;
    }

    return true;
}

function getCalibrationSpeakerLevel(index) {
    const levels = calibrationState?.speakerLevels;
    if (Array.isArray(levels) && Number.isFinite(levels[index])) {
        return clamp(Number(levels[index]), 0.0, 1.0);
    }

    const complete = !!calibrationState.complete;
    const running = !!calibrationState.running;
    const completed = Math.max(0, Math.min(4, calibrationState.completedSpeakers || 0));
    const currentSpeaker = Math.max(1, Math.min(4, calibrationState.currentSpeaker || 1));
    const state = String(calibrationState.state || "idle");

    if (complete || index < completed) return 1.0;

    if (running && index === (currentSpeaker - 1)) {
        if (state === "playing") return clamp(Number(calibrationState.playPercent || 0.0), 0.0, 1.0);
        if (state === "recording") return clamp(Number(calibrationState.recordPercent || 0.0), 0.0, 1.0);
        if (state === "analyzing") return 1.0;
        return 0.65;
    }

    return 0.0;
}

function getCalibrationSpeakerColor(index) {
    const complete = !!calibrationState.complete;
    const running = !!calibrationState.running;
    const completed = Math.max(0, Math.min(4, calibrationState.completedSpeakers || 0));
    const currentSpeaker = Math.max(1, Math.min(4, calibrationState.currentSpeaker || 1));

    if (complete || index < completed) return 0x44AA66;
    if (running && index === (currentSpeaker - 1)) return 0xD4A847;
    if (currentMode === "calibrate") return 0x7A7A7A;
    return 0xE0E0E0;
}

function markViewportDegraded(error) {
    runtimeState.viewportReady = false;
    runtimeState.viewportDegraded = true;

    console.error("LocusQ viewport degraded mode:", error);

    const viewportInfo = document.getElementById("viewport-info");
    if (viewportInfo) {
        viewportInfo.textContent = "Viewport unavailable · controls remain active";
    }

    const viewportLock = document.getElementById("viewport-lock");
    if (viewportLock) {
        viewportLock.textContent = "VIEWPORT SAFE";
    }
}

async function initialiseUIRuntime() {
    try {
        initUIBindings();
    } catch (error) {
        console.error("LocusQ: initUIBindings failed:", error);
    }

    try {
        initParameterListeners();
    } catch (error) {
        console.error("LocusQ: initParameterListeners failed:", error);
    }

    applyUiStateToControls();
    applyCalibrationStatus();

    try {
        initThreeJS();
        runtimeState.viewportReady = true;
        runtimeState.viewportDegraded = false;
        animate();
    } catch (error) {
        markViewportDegraded(error);
    }

    const startupHydrationTasks = [
        (async () => {
            try {
                await loadUiStateFromNative();
                applyUiStateToControls();
                if (uiState.physicsPreset !== "off") {
                    applyPhysicsPreset(uiState.physicsPreset, false);
                }
            } catch (error) {
                console.error("LocusQ: loadUiStateFromNative failed:", error);
            }
        })(),
        (async () => {
            try {
                await loadTimelineFromNative();
            } catch (error) {
                console.error("LocusQ: loadTimelineFromNative failed:", error);
            }
        })(),
        (async () => {
            try {
                await refreshPresetList();
            } catch (error) {
                console.error("LocusQ: refreshPresetList failed:", error);
            }
        })(),
    ];

    await Promise.allSettled(startupHydrationTasks);

    console.log("LocusQ WebView initialized");
}

document.addEventListener("DOMContentLoaded", () => {
    initialiseUIRuntime().catch(error => {
        console.error("LocusQ: UI runtime initialisation failed:", error);
        markViewportDegraded(error);
    });
});

// ===== THREE.JS INITIALIZATION =====
function initThreeJS() {
    canvas = document.getElementById("viewport-canvas");
    if (!canvas) {
        throw new Error("Missing #viewport-canvas element");
    }
    if (typeof THREE === "undefined") {
        throw new Error("THREE runtime is unavailable");
    }

    threeScene = new THREE.Scene();
    threeScene.background = new THREE.Color(0x0A0A0A);
    orbitTarget = new THREE.Vector3(0, 1, 0);
    viewportRaycaster = new THREE.Raycaster();
    viewportPointer = new THREE.Vector2();
    dragPlane = new THREE.Plane(new THREE.Vector3(0, 1, 0), 0);
    dragPoint = new THREE.Vector3();
    dragOffset = new THREE.Vector3();
    dragTarget = new THREE.Vector3();

    camera = new THREE.PerspectiveCamera(60, 1, 0.1, 1000);
    rendererGL = new THREE.WebGLRenderer({ canvas, antialias: true });
    rendererGL.setPixelRatio(window.devicePixelRatio);

    // Room wireframe
    const roomW = 6, roomD = 4, roomH = 3;
    const roomGeo = new THREE.BoxGeometry(roomW, roomH, roomD);
    const roomEdges = new THREE.EdgesGeometry(roomGeo);
    const roomMat = new THREE.LineBasicMaterial({ color: 0xE0E0E0, transparent: true, opacity: 0.3 });
    roomLines = new THREE.LineSegments(roomEdges, roomMat);
    roomLines.position.y = roomH / 2;
    threeScene.add(roomLines);

    gridHelper = new THREE.GridHelper(roomW, roomW * 2, 0x2A2A2A, 0x1A1A1A);
    threeScene.add(gridHelper);

    // Speakers (quad corners)
    const spkPos = [
        new THREE.Vector3(-roomW/2+0.3, 1.2, -roomD/2+0.3),
        new THREE.Vector3( roomW/2-0.3, 1.2, -roomD/2+0.3),
        new THREE.Vector3( roomW/2-0.3, 1.2,  roomD/2-0.3),
        new THREE.Vector3(-roomW/2+0.3, 1.2,  roomD/2-0.3),
    ];

    spkPos.forEach((pos) => {
        const geo = new THREE.OctahedronGeometry(0.15);
        const mat = new THREE.MeshBasicMaterial({ color: 0xE0E0E0, wireframe: true });
        const mesh = new THREE.Mesh(geo, mat);
        mesh.position.copy(pos);
        threeScene.add(mesh);
        speakers.push(mesh);

        // Energy meter bar
        const mGeo = new THREE.BoxGeometry(0.04, 0.01, 0.04);
        const mMat = new THREE.MeshBasicMaterial({ color: 0xE0E0E0, transparent: true, opacity: 0.4 });
        const mMesh = new THREE.Mesh(mGeo, mMat);
        mMesh.position.set(pos.x + 0.25, pos.y, pos.z);
        threeScene.add(mMesh);
        speakerMeters.push({ mesh: mMesh, level: 0, target: 0, basePos: pos });
    });

    // Listener cross
    const lSize = 0.15;
    const lGeo = new THREE.BufferGeometry();
    lGeo.setAttribute("position", new THREE.BufferAttribute(new Float32Array([
        -lSize,0,0, lSize,0,0, 0,0,-lSize, 0,0,lSize, 0,-lSize,0, 0,lSize,0
    ]), 3));
    threeScene.add(new THREE.LineSegments(lGeo, new THREE.LineBasicMaterial({ color: 0x666666 })));

    // Selection ring (will follow selected emitter)
    const ringGeo = new THREE.RingGeometry(0.28, 0.3, 32);
    const ringMat = new THREE.MeshBasicMaterial({ color: 0xD4A847, side: THREE.DoubleSide, transparent: true, opacity: 0.8 });
    selectionRing = new THREE.Mesh(ringGeo, ringMat);
    selectionRing.rotation.x = -Math.PI / 2;
    selectionRing.visible = false;
    threeScene.add(selectionRing);

    // Lane highlight geometry
    const azArcPts = [];
    for (let i = 0; i <= 64; i++) {
        const a = (i / 64) * Math.PI * 2;
        azArcPts.push(new THREE.Vector3(Math.cos(a) * 2.5, 0, Math.sin(a) * 2.5));
    }
    azArc = new THREE.Line(
        new THREE.BufferGeometry().setFromPoints(azArcPts),
        new THREE.LineBasicMaterial({ color: 0xD4A847, transparent: true, opacity: 0.0 })
    );
    azArc.position.y = 1.2;
    threeScene.add(azArc);

    const distRingGeo = new THREE.RingGeometry(2.48, 2.52, 64);
    distRing = new THREE.Mesh(distRingGeo, new THREE.MeshBasicMaterial({
        color: 0xD4A847, side: THREE.DoubleSide, transparent: true, opacity: 0.0
    }));
    distRing.rotation.x = -Math.PI / 2;
    distRing.position.y = 0.01;
    threeScene.add(distRing);

    const elArcPts = [];
    for (let i = 0; i <= 32; i++) {
        const a = -Math.PI / 2 + (i / 32) * Math.PI;
        elArcPts.push(new THREE.Vector3(Math.cos(a) * 2.5, Math.sin(a) * 2.5 + 1.2, 0));
    }
    elArc = new THREE.Line(
        new THREE.BufferGeometry().setFromPoints(elArcPts),
        new THREE.LineBasicMaterial({ color: 0xD4A847, transparent: true, opacity: 0.0 })
    );
    threeScene.add(elArc);

    // Orbit + emitter drag controls
    canvas.addEventListener("mousedown", e => {
        if (e.button === 0 && currentMode === "emitter") {
            const intersection = pickEmitterIntersection(e);
            if (intersection) {
                const emitterId = Number(intersection.object?.userData?.emitterId);
                if (Number.isInteger(emitterId)) {
                    setSelectedEmitter(emitterId);
                }

                if (beginEmitterDrag(e, intersection)) {
                    return;
                }

                // Left-click on an emitter should select without entering orbit drag.
                isDragging = false;
                return;
            }
        }

        isDragging = true;
        isRight = e.button === 2;
        prevMouse = { x: e.clientX, y: e.clientY };
    });
    canvas.addEventListener("mousemove", e => {
        if (updateEmitterDrag(e)) {
            return;
        }

        if (!isDragging) return;
        const dx = e.clientX - prevMouse.x, dy = e.clientY - prevMouse.y;
        if (isRight) {
            const ps = 0.005 * spherical.radius;
            const r = new THREE.Vector3().crossVectors(camera.up, new THREE.Vector3().subVectors(orbitTarget, camera.position)).normalize();
            orbitTarget.add(r.multiplyScalar(dx * ps));
            orbitTarget.add(camera.up.clone().multiplyScalar(-dy * ps));
        } else {
            spherical.theta -= dx * 0.005;
            spherical.phi = Math.max(0.1, Math.min(Math.PI - 0.1, spherical.phi + dy * 0.005));
        }
        prevMouse = { x: e.clientX, y: e.clientY };
        updateCamera();
    });
    canvas.addEventListener("mouseup", () => {
        isDragging = false;
        endEmitterDrag();
    });
    canvas.addEventListener("mouseleave", () => {
        isDragging = false;
        endEmitterDrag();
    });
    canvas.addEventListener("contextmenu", e => e.preventDefault());
    canvas.addEventListener("wheel", e => {
        if (emitterDragState) {
            e.preventDefault();
            return;
        }
        spherical.radius *= 1 + e.deltaY * 0.001;
        spherical.radius = Math.max(2, Math.min(20, spherical.radius));
        updateCamera();
    });

    updateCamera();
    resize();
    window.addEventListener("resize", resize);
}

function updateCamera() {
    if (!camera || !orbitTarget) return;

    camera.position.x = orbitTarget.x + spherical.radius * Math.sin(spherical.phi) * Math.cos(spherical.theta);
    camera.position.y = orbitTarget.y + spherical.radius * Math.cos(spherical.phi);
    camera.position.z = orbitTarget.z + spherical.radius * Math.sin(spherical.phi) * Math.sin(spherical.theta);
    camera.lookAt(orbitTarget);
}

function resize() {
    if (!canvas || !canvas.parentElement || !rendererGL || !camera) return;
    const rect = canvas.parentElement.getBoundingClientRect();
    const tlEl = document.getElementById("timeline");
    const tlH = tlEl && tlEl.classList.contains("visible") ? 120 : 0;
    const w = rect.width, h = rect.height - tlH;
    if (w <= 0 || h <= 0) return;
    canvas.width = w * devicePixelRatio;
    canvas.height = h * devicePixelRatio;
    canvas.style.width = w + "px";
    canvas.style.height = h + "px";
    rendererGL.setSize(w, h);
    camera.aspect = w / h;
    camera.updateProjectionMatrix();
}

// ===== UI BINDINGS =====
const viewPresetByName = {
    perspective: { theta: Math.PI / 4, phi: Math.PI / 4, radius: 8 },
    top: { theta: 0, phi: 0.1, radius: 8 },
    front: { theta: 0, phi: Math.PI / 2, radius: 8 },
    side: { theta: Math.PI / 2, phi: Math.PI / 2, radius: 8 },
};

const viewOrder = ["perspective", "top", "front", "side"];

function setActiveView(viewName) {
    const resolved = viewPresetByName[viewName] ? viewName : "perspective";
    document.querySelectorAll(".view-btn").forEach(button => {
        button.classList.toggle("active", button.dataset.view === resolved);
    });

    Object.assign(spherical, viewPresetByName[resolved]);
    updateCamera();
}

function applyPhysicsPreset(presetName, persistUiState = true) {
    const normalized = normalizePhysicsPresetName(presetName);
    isApplyingPhysicsPreset = true;
    uiState.physicsPreset = normalized;

    const active = document.getElementById("physics-active");
    if (active) active.style.display = normalized === "off" ? "none" : "block";

    if (normalized === "off") {
        setToggleValue(toggleStates.phys_enable, false);
    } else {
        setToggleValue(toggleStates.phys_enable, true);
    }

    if (normalized === "bounce") {
        setSliderScaledValue(sliderStates.phys_mass, 1.0);
        setSliderScaledValue(sliderStates.phys_drag, 0.2);
        setSliderScaledValue(sliderStates.phys_elasticity, 0.82);
        setSliderScaledValue(sliderStates.phys_gravity, -9.8);
        setSliderScaledValue(sliderStates.phys_friction, 0.2);
        setChoiceIndex(comboStates.phys_gravity_dir, 0);
    } else if (normalized === "float") {
        setSliderScaledValue(sliderStates.phys_mass, 0.4);
        setSliderScaledValue(sliderStates.phys_drag, 0.65);
        setSliderScaledValue(sliderStates.phys_elasticity, 0.3);
        setSliderScaledValue(sliderStates.phys_gravity, 0.0);
        setSliderScaledValue(sliderStates.phys_friction, 0.45);
        setChoiceIndex(comboStates.phys_gravity_dir, 0);
    } else if (normalized === "orbit") {
        setSliderScaledValue(sliderStates.phys_mass, 0.8);
        setSliderScaledValue(sliderStates.phys_drag, 0.35);
        setSliderScaledValue(sliderStates.phys_elasticity, 0.55);
        setSliderScaledValue(sliderStates.phys_gravity, 6.0);
        setSliderScaledValue(sliderStates.phys_friction, 0.15);
        setChoiceIndex(comboStates.phys_gravity_dir, 2);
    }

    isApplyingPhysicsPreset = false;

    if (persistUiState) {
        scheduleUiStateCommit();
    }
}

function initUIBindings() {
    // Mode tabs
    document.querySelectorAll(".mode-tab").forEach(tab => {
        tab.addEventListener("click", () => {
            const mode = tab.dataset.mode;
            const modeMap = { calibrate: 0, emitter: 1, renderer: 2 };
            switchMode(modeMap[mode] !== undefined ? mode : currentMode);
            setChoiceIndex(comboStates.mode, modeMap[mode] ?? 1, 3);
        });
    });

    // Quality badge
    const qualityBadge = document.getElementById("quality-badge");
    if (qualityBadge) {
        qualityBadge.addEventListener("click", function() {
            const isCurrentlyDraft = this.classList.contains("draft");
            this.className = `quality-badge ${isCurrentlyDraft ? "final" : "draft"}`;
            this.textContent = isCurrentlyDraft ? "FINAL" : "DRAFT";
            setChoiceIndex(comboStates.rend_quality, isCurrentlyDraft ? 1 : 0, 2);
        });
    }

    bindSelectToComboState("cal-config", comboStates.cal_spk_config);
    bindSelectToIntSliderState("cal-mic", sliderStates.cal_mic_channel, 1);
    bindSelectToIntSliderState("cal-spk1", sliderStates.cal_spk1_out, 1);
    bindSelectToIntSliderState("cal-spk2", sliderStates.cal_spk2_out, 1);
    bindSelectToIntSliderState("cal-spk3", sliderStates.cal_spk3_out, 1);
    bindSelectToIntSliderState("cal-spk4", sliderStates.cal_spk4_out, 1);
    bindSelectToComboState("cal-type", comboStates.cal_test_type);
    bindValueStepper("cal-level", sliderStates.cal_test_level, { step: 1.0, min: -60.0, max: 0.0, roundDigits: 1 });

    const emitLabelInput = document.getElementById("emit-label");
    if (emitLabelInput) {
        emitLabelInput.addEventListener("input", () => {
            uiState.emitterLabel = sanitizeEmitterLabel(emitLabelInput.value);
            emitLabelInput.value = uiState.emitterLabel;
            scheduleUiStateCommit();
        });
        emitLabelInput.addEventListener("blur", () => {
            uiState.emitterLabel = sanitizeEmitterLabel(emitLabelInput.value);
            emitLabelInput.value = uiState.emitterLabel;
            scheduleUiStateCommit(true);
        });
    }

    const colorSwatch = document.getElementById("emit-color-swatch");
    if (colorSwatch) {
        colorSwatch.addEventListener("click", () => {
            const current = Math.round(Number(sliderStates.emit_color.getScaledValue()) || 0);
            const next = (current + 1) % 16;
            setSliderScaledValue(sliderStates.emit_color, next);
        });
    }

    bindSelectToComboState("pos-mode", comboStates.pos_coord_mode);

    const sizeLinkToggle = document.getElementById("toggle-size-link");
    if (sizeLinkToggle) {
        bindControlActivate(sizeLinkToggle, () => {
            toggleStateAndClass("toggle-size-link", toggleStates.size_link);
        });
    }

    bindValueStepper("val-azimuth", sliderStates.pos_azimuth, { step: 1.0, min: -180.0, max: 180.0, roundDigits: 1 });
    bindValueStepper("val-elevation", sliderStates.pos_elevation, { step: 1.0, min: -90.0, max: 90.0, roundDigits: 1 });
    bindValueStepper("val-distance", sliderStates.pos_distance, { step: 0.1, min: 0.0, max: 50.0, roundDigits: 2 });
    bindValueStepper("val-size", sliderStates.size_uniform, { step: 0.05, min: 0.01, max: 20.0, roundDigits: 2 });
    bindValueStepper("val-gain", sliderStates.emit_gain, { step: 0.5, min: -60.0, max: 12.0, roundDigits: 1 });
    bindValueStepper("val-spread", sliderStates.emit_spread, { step: 0.05, min: 0.0, max: 1.0, roundDigits: 2 });
    bindValueStepper("val-directivity", sliderStates.emit_directivity, { step: 0.05, min: 0.0, max: 1.0, roundDigits: 2 });
    bindValueStepper("val-master-gain", sliderStates.rend_master_gain, { step: 0.5, min: -60.0, max: 12.0, roundDigits: 1 });
    bindValueStepper("val-mass", sliderStates.phys_mass, { step: 0.1, min: 0.01, max: 100.0, roundDigits: 2 });
    bindValueStepper("val-drag", sliderStates.phys_drag, { step: 0.05, min: 0.0, max: 10.0, roundDigits: 2 });
    bindValueStepper("val-elasticity", sliderStates.phys_elasticity, { step: 0.05, min: 0.0, max: 1.0, roundDigits: 2 });
    bindValueStepper("val-gravity", sliderStates.phys_gravity, { step: 0.5, min: -20.0, max: 20.0, roundDigits: 1 });
    bindValueStepper("val-friction", sliderStates.phys_friction, { step: 0.05, min: 0.0, max: 1.0, roundDigits: 2 });

    // Emitter mute/solo
    const muteToggle = document.getElementById("toggle-mute");
    if (muteToggle) {
        bindControlActivate(muteToggle, () => {
            toggleStateAndClass("toggle-mute", toggleStates.emit_mute);
        });
    }
    const soloToggle = document.getElementById("toggle-solo");
    if (soloToggle) {
        bindControlActivate(soloToggle, () => {
            toggleStateAndClass("toggle-solo", toggleStates.emit_solo);
        });
    }

    // View buttons
    document.querySelectorAll(".view-btn").forEach(btn => {
        btn.addEventListener("click", () => {
            const view = btn.dataset.view;
            const viewIndex = Math.max(0, viewOrder.indexOf(view));
            setChoiceIndex(comboStates.rend_viz_mode, viewIndex);
            setActiveView(viewOrder[viewIndex]);
        });
    });

    bindSelectToComboState("phys-grav-dir", comboStates.phys_gravity_dir);

    // Physics preset
    const physicsPreset = document.getElementById("physics-preset");
    if (physicsPreset) {
        physicsPreset.addEventListener("change", function() {
            applyPhysicsPreset(this.value, true);
        });
    }

    const throwButton = document.getElementById("btn-throw");
    if (throwButton) {
        throwButton.addEventListener("click", () => pulseToggleParameter(toggleStates.phys_throw));
    }

    const resetButton = document.getElementById("btn-reset");
    if (resetButton) {
        resetButton.addEventListener("click", () => pulseToggleParameter(toggleStates.phys_reset));
    }

    // Physics advanced disclosure
    const physicsDisclosure = document.getElementById("physics-disclosure");
    if (physicsDisclosure) {
        physicsDisclosure.addEventListener("click", () => {
            const advanced = document.getElementById("physics-advanced");
            const arrow = document.getElementById("physics-arrow");
            if (advanced) advanced.classList.toggle("open");
            if (arrow) arrow.classList.toggle("open");
        });
    }

    // Timeline lane selection
    document.querySelectorAll(".timeline-lane").forEach(lane => {
        lane.addEventListener("click", () => {
            document.querySelectorAll(".timeline-lane").forEach(l => l.classList.remove("selected"));
            lane.classList.add("selected");
            selectedLane = lane.dataset.lane;
            setLaneHighlight(selectedLane);
        });
    });

    bindSelectToComboState("rend-dist-model", comboStates.rend_distance_model);
    bindSelectToComboState("rend-phys-rate", comboStates.rend_phys_rate);

    const dopplerToggle = document.getElementById("toggle-doppler");
    if (dopplerToggle) {
        bindControlActivate(dopplerToggle, () => {
            toggleStateAndClass("toggle-doppler", toggleStates.rend_doppler);
        });
    }

    const airAbsorbToggle = document.getElementById("toggle-air-absorb");
    if (airAbsorbToggle) {
        bindControlActivate(airAbsorbToggle, () => {
            toggleStateAndClass("toggle-air-absorb", toggleStates.rend_air_absorb);
        });
    }

    const roomEnableToggle = document.getElementById("toggle-room");
    if (roomEnableToggle) {
        bindControlActivate(roomEnableToggle, () => {
            toggleStateAndClass("toggle-room", toggleStates.rend_room_enable);
        });
    }

    const roomErOnlyToggle = document.getElementById("toggle-er-only");
    if (roomErOnlyToggle) {
        bindControlActivate(roomErOnlyToggle, () => {
            toggleStateAndClass("toggle-er-only", toggleStates.rend_room_er_only);
        });
    }

    const wallsToggle = document.getElementById("toggle-walls");
    if (wallsToggle) {
        bindControlActivate(wallsToggle, () => {
            toggleStateAndClass("toggle-walls", toggleStates.rend_phys_walls);
        });
    }

    const pausePhysicsButton = document.getElementById("btn-pause-physics");
    if (pausePhysicsButton) {
        pausePhysicsButton.addEventListener("click", () => {
            const paused = !getToggleValue(toggleStates.rend_phys_pause);
            setToggleValue(toggleStates.rend_phys_pause, paused);
            pausePhysicsButton.textContent = paused ? "RESUME ALL" : "PAUSE ALL";
        });
    }

    const rewindButton = document.getElementById("timeline-rewind-btn");
    if (rewindButton) {
        rewindButton.addEventListener("click", () => {
            const duration = Math.max(0.001, timelineState.durationSeconds);
            const nextTime = clamp((timelineState.currentTimeSeconds || 0.0) - 0.25, 0.0, duration);
            timelineState.currentTimeSeconds = nextTime;
            updateTimelinePlayheads();
            callNative("locusqSetTimelineTime", nativeFunctions.setTimelineTime, nextTime).catch(() => {});
        });
    }

    const stopButton = document.getElementById("timeline-stop-btn");
    if (stopButton) {
        stopButton.addEventListener("click", () => {
            timelineState.currentTimeSeconds = 0.0;
            updateTimelinePlayheads();
            setToggleValue(toggleStates.anim_enable, false);
            setToggleClass("toggle-anim", false);
            setAnimationControlsEnabled(false);
            callNative("locusqSetTimelineTime", nativeFunctions.setTimelineTime, 0.0).catch(() => {});
        });
    }

    const playButton = document.getElementById("timeline-play-btn");
    if (playButton) {
        playButton.addEventListener("click", () => {
            if (getChoiceIndex(comboStates.anim_mode) !== 1) {
                setChoiceIndex(comboStates.anim_mode, 1);
            }
            setToggleValue(toggleStates.anim_enable, true);
            setToggleClass("toggle-anim", true);
            setAnimationControlsEnabled(true);
        });
    }

    // Animation controls
    const animToggle = document.getElementById("toggle-anim");
    if (animToggle) {
        bindControlActivate(animToggle, () => {
            const enabled = toggleStateAndClass("toggle-anim", toggleStates.anim_enable);
            setAnimationControlsEnabled(enabled);
        });
    }

    const animSource = document.getElementById("anim-source");
    if (animSource) {
        animSource.addEventListener("change", () => {
            setChoiceIndex(comboStates.anim_mode, animSource.selectedIndex);
        });
    }

    ["toggle-anim-loop", "toggle-timeline-loop"].forEach(id => {
        const loopToggle = document.getElementById(id);
        if (loopToggle) {
            bindControlActivate(loopToggle, () => {
                const nextLoop = !getToggleValue(toggleStates.anim_loop);
                setToggleValue(toggleStates.anim_loop, nextLoop);
                setToggleClass("toggle-anim-loop", nextLoop);
                setToggleClass("toggle-timeline-loop", nextLoop);
                timelineState.looping = nextLoop;
                scheduleTimelineCommit();
            });
        }
    });

    const syncToggle = document.getElementById("toggle-timeline-sync");
    if (syncToggle) {
        bindControlActivate(syncToggle, () => {
            toggleStateAndClass("toggle-timeline-sync", toggleStates.anim_sync);
        });
    }

    bindValueStepper("val-anim-speed", sliderStates.anim_speed, { step: 0.1, min: 0.1, max: 10.0, roundDigits: 1 });

    const presetSaveButton = document.getElementById("preset-save-btn");
    if (presetSaveButton) {
        presetSaveButton.addEventListener("click", async () => {
            const suggestedName = `Preset_${new Date().toISOString().replace(/[-:]/g, "").slice(0, 15)}`;
            const inputName = window.prompt("Preset name", suggestedName);
            if (inputName === null) return;

            const trimmed = inputName.trim();
            if (!trimmed) {
                setPresetStatus("Preset name is required", true);
                return;
            }

            try {
                const result = await callNative("locusqSaveEmitterPreset", nativeFunctions.saveEmitterPreset, { name: trimmed });
                if (result?.ok) {
                    setPresetStatus(`Saved: ${result.name || trimmed}`);
                    await refreshPresetList();
                } else {
                    setPresetStatus(result?.message || "Preset save failed", true);
                }
            } catch (error) {
                setPresetStatus("Preset save failed", true);
                console.error("Failed to save preset:", error);
            }
        });
    }

    const presetLoadButton = document.getElementById("preset-load-btn");
    if (presetLoadButton) {
        presetLoadButton.addEventListener("click", async () => {
            const select = document.getElementById("preset-select");
            if (!select || !select.value) {
                setPresetStatus("Select a preset first", true);
                return;
            }

            try {
                const result = await callNative("locusqLoadEmitterPreset", nativeFunctions.loadEmitterPreset, { path: select.value });
                if (result?.ok) {
                    setPresetStatus(`Loaded: ${select.options[select.selectedIndex]?.textContent || "preset"}`);
                    await loadTimelineFromNative();
                    syncAnimationUI();
                } else {
                    setPresetStatus(result?.message || "Preset load failed", true);
                }
            } catch (error) {
                setPresetStatus("Preset load failed", true);
                console.error("Failed to load preset:", error);
            }
        });
    }

    const calStartButton = document.getElementById("cal-start-btn");
    if (calStartButton) {
        calStartButton.addEventListener("click", async () => {
            if (calibrationState.running) {
                await abortCalibration();
                return;
            }

            const options = collectCalibrationOptions();

            try {
                const started = await callNative("locusqStartCalibration", nativeFunctions.startCalibration, options);
                if (!started) {
                    console.warn("Calibration did not start. Ensure mode is Calibrate and engine is idle.");
                }
            } catch (error) {
                console.error("Failed to start calibration:", error);
            }
        });
    }

    applyUiStateToControls();
    if (uiState.physicsPreset !== "off") {
        applyPhysicsPreset(uiState.physicsPreset, false);
    }
}

// ===== PARAMETER LISTENERS =====
function initParameterListeners() {
    const updateQualityBadge = () => {
        const badge = document.getElementById("quality-badge");
        if (!badge) return;
        const isFinal = getChoiceIndex(comboStates.rend_quality) === 1;
        badge.className = "quality-badge " + (isFinal ? "final" : "draft");
        badge.textContent = isFinal ? "FINAL" : "DRAFT";
    };

    const markPhysicsPresetCustom = () => {
        if (isApplyingPhysicsPreset) return;
        const physicsPreset = document.getElementById("physics-preset");
        if (physicsPreset && physicsPreset.value !== "custom" && physicsPreset.value !== "off") {
            physicsPreset.value = "custom";
        }
        if (uiState.physicsPreset !== "custom") {
            uiState.physicsPreset = "custom";
            scheduleUiStateCommit();
        }
    };

    const updateViewMode = () => {
        const idx = clamp(getChoiceIndex(comboStates.rend_viz_mode), 0, viewOrder.length - 1);
        setActiveView(viewOrder[idx] || "perspective");
    };

    // Mode changes from DAW
    comboStates.mode.valueChangedEvent.addListener(() => {
        const idx = getChoiceIndex(comboStates.mode);
        const modes = ["calibrate", "emitter", "renderer"];
        const mode = modes[idx] || "emitter";
        switchMode(mode);
    });

    // Slider display updates
    sliderStates.pos_azimuth.valueChangedEvent.addListener(() => {
        updateValueDisplay("val-azimuth", sliderStates.pos_azimuth.getScaledValue().toFixed(1), "\u00B0");
    });
    sliderStates.pos_elevation.valueChangedEvent.addListener(() => {
        updateValueDisplay("val-elevation", sliderStates.pos_elevation.getScaledValue().toFixed(1), "\u00B0");
    });
    sliderStates.pos_distance.valueChangedEvent.addListener(() => {
        updateValueDisplay("val-distance", sliderStates.pos_distance.getScaledValue().toFixed(2), "m");
    });
    sliderStates.size_uniform.valueChangedEvent.addListener(() => {
        updateValueDisplay("val-size", sliderStates.size_uniform.getScaledValue().toFixed(2), "m");
    });
    sliderStates.emit_gain.valueChangedEvent.addListener(() => {
        updateValueDisplay("val-gain", sliderStates.emit_gain.getScaledValue().toFixed(1), "dB");
    });
    sliderStates.emit_spread.valueChangedEvent.addListener(() => {
        updateValueDisplay("val-spread", sliderStates.emit_spread.getScaledValue().toFixed(2), "");
    });
    sliderStates.emit_directivity.valueChangedEvent.addListener(() => {
        updateValueDisplay("val-directivity", sliderStates.emit_directivity.getScaledValue().toFixed(2), "");
    });
    sliderStates.emit_color.valueChangedEvent.addListener(() => {
        const swatch = document.getElementById("emit-color-swatch");
        if (!swatch) return;
        const idx = clamp(Math.round(sliderStates.emit_color.getScaledValue()), 0, emitterPalette.length - 1);
        const hex = "#" + emitterPalette[idx].toString(16).padStart(6, "0");
        swatch.style.background = hex;
    });
    sliderStates.cal_test_level.valueChangedEvent.addListener(() => {
        updateValueDisplay("cal-level", sliderStates.cal_test_level.getScaledValue().toFixed(1), "dBFS");
    });
    sliderStates.phys_mass.valueChangedEvent.addListener(() => {
        updateValueDisplay("val-mass", sliderStates.phys_mass.getScaledValue().toFixed(2), "kg");
        markPhysicsPresetCustom();
    });
    sliderStates.phys_drag.valueChangedEvent.addListener(() => {
        updateValueDisplay("val-drag", sliderStates.phys_drag.getScaledValue().toFixed(2), "");
        markPhysicsPresetCustom();
    });
    sliderStates.phys_elasticity.valueChangedEvent.addListener(() => {
        updateValueDisplay("val-elasticity", sliderStates.phys_elasticity.getScaledValue().toFixed(2), "");
        markPhysicsPresetCustom();
    });
    sliderStates.phys_gravity.valueChangedEvent.addListener(() => {
        updateValueDisplay("val-gravity", sliderStates.phys_gravity.getScaledValue().toFixed(1), "m/s&sup2;");
        markPhysicsPresetCustom();
    });
    sliderStates.phys_friction.valueChangedEvent.addListener(() => {
        updateValueDisplay("val-friction", sliderStates.phys_friction.getScaledValue().toFixed(2), "");
        markPhysicsPresetCustom();
    });
    sliderStates.rend_master_gain.valueChangedEvent.addListener(() => {
        updateValueDisplay("val-master-gain", sliderStates.rend_master_gain.getScaledValue().toFixed(1), "dB");
    });
    sliderStates.anim_speed.valueChangedEvent.addListener(() => {
        updateValueDisplay("val-anim-speed", sliderStates.anim_speed.getScaledValue().toFixed(1), "x");
        timelineState.playbackRate = sliderStates.anim_speed.getScaledValue();
        scheduleTimelineCommit();
    });

    toggleStates.emit_mute.valueChangedEvent.addListener(() => {
        setToggleClass("toggle-mute", !!toggleStates.emit_mute.getValue());
    });
    toggleStates.emit_solo.valueChangedEvent.addListener(() => {
        setToggleClass("toggle-solo", !!toggleStates.emit_solo.getValue());
    });
    toggleStates.size_link.valueChangedEvent.addListener(() => {
        setToggleClass("toggle-size-link", !!toggleStates.size_link.getValue());
    });
    toggleStates.phys_enable.valueChangedEvent.addListener(() => {
        const active = document.getElementById("physics-active");
        if (active) active.style.display = toggleStates.phys_enable.getValue() ? "block" : "none";
        if (!isApplyingPhysicsPreset && !toggleStates.phys_enable.getValue()) {
            uiState.physicsPreset = "off";
            const physicsPreset = document.getElementById("physics-preset");
            if (physicsPreset) physicsPreset.value = "off";
            scheduleUiStateCommit();
        }
    });

    toggleStates.anim_enable.valueChangedEvent.addListener(syncAnimationUI);
    comboStates.anim_mode.valueChangedEvent.addListener(() => {
        syncAnimationUI();
        scheduleTimelineCommit();
    });
    toggleStates.anim_loop.valueChangedEvent.addListener(() => {
        syncAnimationUI();
        scheduleTimelineCommit();
    });
    toggleStates.anim_sync.valueChangedEvent.addListener(syncAnimationUI);

    toggleStates.rend_doppler.valueChangedEvent.addListener(() => {
        setToggleClass("toggle-doppler", !!toggleStates.rend_doppler.getValue());
    });
    toggleStates.rend_air_absorb.valueChangedEvent.addListener(() => {
        setToggleClass("toggle-air-absorb", !!toggleStates.rend_air_absorb.getValue());
    });
    toggleStates.rend_room_enable.valueChangedEvent.addListener(() => {
        setToggleClass("toggle-room", !!toggleStates.rend_room_enable.getValue());
    });
    toggleStates.rend_room_er_only.valueChangedEvent.addListener(() => {
        setToggleClass("toggle-er-only", !!toggleStates.rend_room_er_only.getValue());
    });
    toggleStates.rend_phys_walls.valueChangedEvent.addListener(() => {
        setToggleClass("toggle-walls", !!toggleStates.rend_phys_walls.getValue());
    });
    toggleStates.rend_phys_pause.valueChangedEvent.addListener(() => {
        const button = document.getElementById("btn-pause-physics");
        if (button) {
            button.textContent = toggleStates.rend_phys_pause.getValue() ? "RESUME ALL" : "PAUSE ALL";
        }
    });

    comboStates.rend_viz_mode.valueChangedEvent.addListener(updateViewMode);
    comboStates.phys_gravity_dir.valueChangedEvent.addListener(markPhysicsPresetCustom);

    // Quality badge from DAW
    comboStates.rend_quality.valueChangedEvent.addListener(updateQualityBadge);

    // Initial sync
    const initialModeChoices = Array.isArray(comboStates.mode?.properties?.choices)
        ? comboStates.mode.properties.choices.length
        : 0;
    const initialMode = initialModeChoices >= 3
        ? (["calibrate", "emitter", "renderer"][getChoiceIndex(comboStates.mode)] || currentMode)
        : currentMode;
    switchMode(initialMode);
    updateQualityBadge();
    setToggleClass("toggle-mute", !!toggleStates.emit_mute.getValue());
    setToggleClass("toggle-solo", !!toggleStates.emit_solo.getValue());
    const active = document.getElementById("physics-active");
    if (active) active.style.display = toggleStates.phys_enable.getValue() ? "block" : "none";
    updateValueDisplay("val-azimuth", sliderStates.pos_azimuth.getScaledValue().toFixed(1), "\u00B0");
    updateValueDisplay("val-elevation", sliderStates.pos_elevation.getScaledValue().toFixed(1), "\u00B0");
    updateValueDisplay("val-distance", sliderStates.pos_distance.getScaledValue().toFixed(2), "m");
    updateValueDisplay("val-size", sliderStates.size_uniform.getScaledValue().toFixed(2), "m");
    updateValueDisplay("val-gain", sliderStates.emit_gain.getScaledValue().toFixed(1), "dB");
    updateValueDisplay("val-spread", sliderStates.emit_spread.getScaledValue().toFixed(2), "");
    updateValueDisplay("val-directivity", sliderStates.emit_directivity.getScaledValue().toFixed(2), "");
    updateValueDisplay("cal-level", sliderStates.cal_test_level.getScaledValue().toFixed(1), "dBFS");
    updateValueDisplay("val-mass", sliderStates.phys_mass.getScaledValue().toFixed(2), "kg");
    updateValueDisplay("val-drag", sliderStates.phys_drag.getScaledValue().toFixed(2), "");
    updateValueDisplay("val-elasticity", sliderStates.phys_elasticity.getScaledValue().toFixed(2), "");
    updateValueDisplay("val-gravity", sliderStates.phys_gravity.getScaledValue().toFixed(1), "m/s&sup2;");
    updateValueDisplay("val-friction", sliderStates.phys_friction.getScaledValue().toFixed(2), "");
    updateValueDisplay("val-master-gain", sliderStates.rend_master_gain.getScaledValue().toFixed(1), "dB");
    setToggleClass("toggle-size-link", !!toggleStates.size_link.getValue());
    setToggleClass("toggle-doppler", !!toggleStates.rend_doppler.getValue());
    setToggleClass("toggle-air-absorb", !!toggleStates.rend_air_absorb.getValue());
    setToggleClass("toggle-room", !!toggleStates.rend_room_enable.getValue());
    setToggleClass("toggle-er-only", !!toggleStates.rend_room_er_only.getValue());
    setToggleClass("toggle-walls", !!toggleStates.rend_phys_walls.getValue());
    const pauseButton = document.getElementById("btn-pause-physics");
    if (pauseButton) pauseButton.textContent = toggleStates.rend_phys_pause.getValue() ? "RESUME ALL" : "PAUSE ALL";
    const colorSwatch = document.getElementById("emit-color-swatch");
    if (colorSwatch) {
        const idx = clamp(Math.round(sliderStates.emit_color.getScaledValue()), 0, emitterPalette.length - 1);
        colorSwatch.style.background = "#" + emitterPalette[idx].toString(16).padStart(6, "0");
    }
    updateViewMode();
    syncAnimationUI();
}

function updateValueDisplay(id, value, unit) {
    const el = document.getElementById(id);
    if (el) {
        el.innerHTML = value + (unit ? '<span class="control-unit">' + unit + '</span>' : '');
    }
}

// ===== MODE SWITCHING =====
function applyModeShell(mode) {
    const body = document.body;
    if (body) {
        body.classList.remove("mode-calibrate", "mode-emitter", "mode-renderer");
        body.classList.add(`mode-${mode}`);
    }

    const rail = document.getElementById("rail");
    if (rail) {
        rail.scrollTop = railScrollByMode[mode] || 0;
    }
}

function switchMode(mode) {
    if (currentMode === mode) {
        applyModeShell(mode);
        return;
    }

    const rail = document.getElementById("rail");
    if (rail) {
        railScrollByMode[currentMode] = rail.scrollTop;
    }

    currentMode = mode;

    document.querySelectorAll(".mode-tab").forEach(t => t.classList.remove("active"));
    const activeTab = document.querySelector(`[data-mode="${mode}"]`);
    if (activeTab) activeTab.classList.add("active");

    document.querySelectorAll(".rail-panel").forEach(p => p.classList.remove("active"));
    const activePanel = document.querySelector(`[data-panel="${mode}"]`);
    if (activePanel) activePanel.classList.add("active");

    const tl = document.getElementById("timeline");
    if (mode === "emitter") {
        if (tl) tl.classList.add("visible");
        renderTimelineLanes();
    } else {
        if (tl) tl.classList.remove("visible");
    }

    // Scene status
    const ss = document.getElementById("scene-status");
    if (ss) {
        ss.className = "scene-status";
        if (mode === "calibrate") { ss.textContent = "NO PROFILE"; ss.classList.add("noprofile"); }
        else if (mode === "renderer") { ss.textContent = "READY"; ss.classList.add("ready"); }
        else { ss.textContent = "STABLE"; }
    }

    // 3D viewport adjustments
    if (mode !== "emitter") setLaneHighlight(null);
    else setLaneHighlight(selectedLane);
    updateSelectionRingFromState();
    if (roomLines?.material) {
        roomLines.material.opacity = mode === "calibrate" ? 0.15 : 0.3;
    }
    applyCalibrationStatus();
    applyModeShell(mode);

    if (runtimeState.viewportReady) {
        setTimeout(resize, 10);
    }
}

function setLaneHighlight(lane) {
    highlightTargets = { azimuth: 0, elevation: 0, distance: 0, size: 0 };
    if (lane) highlightTargets[lane] = lane === "distance" ? 0.15 : 0.25;
}

// ===== SCENE STATE FROM C++ =====
// Called by C++ via evaluateJavascript
window.updateSceneState = function(data) {
    const emitters = Array.isArray(data?.emitters) ? data.emitters : [];

    sceneData = {
        ...(data || {}),
        emitters,
    };

    if (Number.isInteger(data?.localEmitterId)) {
        localEmitterId = data.localEmitterId;
    }

    if (selectedEmitterId < 0 && Number.isInteger(localEmitterId) && localEmitterId >= 0) {
        selectedEmitterId = localEmitterId;
    }

    updateEmitterMeshes(emitters);
    updateSceneList(emitters);

    if (typeof data.animDuration === "number" && Number.isFinite(data.animDuration)) {
        timelineState.durationSeconds = clamp(data.animDuration, 0.25, 120.0);
    }
    if (typeof data.animLooping === "boolean") {
        timelineState.looping = data.animLooping;
    }

    const timeEl = document.getElementById("timeline-time");
    if (timeEl && typeof data.animTime === "number") {
        timelineState.currentTimeSeconds = Math.max(0.0, data.animTime);
        const totalMs = Math.max(0, Math.round(data.animTime * 1000));
        const minutes = Math.floor(totalMs / 60000);
        const seconds = Math.floor((totalMs % 60000) / 1000);
        const millis = totalMs % 1000;
        timeEl.textContent =
            `${String(minutes).padStart(2, "0")}:${String(seconds).padStart(2, "0")}.${String(millis).padStart(3, "0")}`;
        updateTimelinePlayheads();
        ["azimuth", "elevation", "distance", "size"].forEach(updateLaneCurveBadge);
    }

    const info = document.getElementById("viewport-info");
    const perfBlock = Number.isFinite(data.perfBlockMs) ? Number(data.perfBlockMs) : null;
    const perfEmitter = Number.isFinite(data.perfEmitterMs) ? Number(data.perfEmitterMs) : null;
    const perfRenderer = Number.isFinite(data.perfRendererMs) ? Number(data.perfRendererMs) : null;
    const selectedEmitter = getSelectedEmitter();
    const emitLabelInput = document.getElementById("emit-label");

    if (emitLabelInput && selectedEmitter && selectedEmitter.id === localEmitterId) {
        const sceneLabel = sanitizeEmitterLabel(selectedEmitter.label || uiState.emitterLabel);
        if (sceneLabel && document.activeElement !== emitLabelInput) {
            emitLabelInput.value = sceneLabel;
        }
        uiState.emitterLabel = sceneLabel;
    }

    if (info) {
        if (currentMode === "emitter") {
            const selectedText = selectedEmitter
                ? ` \u00B7 Sel ${selectedEmitter.label || `Emitter ${selectedEmitter.id + 1}`}`
                : "";
            const emitterPerfText = perfEmitter !== null ? ` \u00B7 Emit ${perfEmitter.toFixed(2)}ms` : "";
            info.textContent = "Emitter Mode \u00B7 " + data.emitterCount + " objects" + selectedText + emitterPerfText;
        } else if (currentMode === "renderer") {
            const rendererPerfText = perfRenderer !== null ? ` \u00B7 Render ${perfRenderer.toFixed(2)}ms` : "";
            const blockPerfText = perfBlock !== null ? ` \u00B7 Block ${perfBlock.toFixed(2)}ms` : "";
            const qualityBadge = document.getElementById("quality-badge");
            const qualityText = qualityBadge?.classList.contains("final") ? "Final" : "Draft";
            const outputChannels = Number.isFinite(data.outputChannels) ? Number(data.outputChannels) : 0;
            const layoutText = typeof data.outputLayout === "string" ? data.outputLayout.toUpperCase() : "";
            const outputRoute = Array.isArray(data.rendererOutputChannels)
                ? data.rendererOutputChannels.join("/")
                : "";
            const outputText = outputChannels > 0
                ? ` \u00B7 Out ${outputChannels}ch${layoutText ? ` ${layoutText}` : ""}${outputRoute ? ` (${outputRoute})` : ""}`
                : "";
            info.textContent = "Renderer Mode \u00B7 " + data.emitterCount + " emitters \u00B7 " +
                qualityText + outputText + rendererPerfText + blockPerfText;
        } else {
            info.textContent = "Calibrate Mode \u00B7 Room Profile Setup";
        }
    }
};

window.updateCalibrationStatus = function(status) {
    if (!status) return;

    calibrationState = {
        ...calibrationState,
        ...status,
    };

    applyCalibrationStatus();
};

function collectCalibrationOptions() {
    const getSelectedIndex = (id, fallback = 0) => {
        const el = document.getElementById(id);
        return el ? Math.max(0, el.selectedIndex) : fallback;
    };

    const levelText = document.getElementById("cal-level")?.textContent || "-20.0";
    const parsedLevel = parseFloat(levelText);

    return {
        testType: getSelectedIndex("cal-type", 0),
        testLevelDb: Number.isFinite(parsedLevel) ? parsedLevel : -20.0,
        sweepSeconds: 3.0,
        tailSeconds: 1.5,
        micChannel: getSelectedIndex("cal-mic", 0),
        speakerChannels: [
            getSelectedIndex("cal-spk1", 0),
            getSelectedIndex("cal-spk2", 1),
            getSelectedIndex("cal-spk3", 2),
            getSelectedIndex("cal-spk4", 3),
        ],
    };
}

async function abortCalibration() {
    try {
        await callNative("locusqAbortCalibration", nativeFunctions.abortCalibration);
    } catch (error) {
        console.error("Failed to abort calibration:", error);
    }
}

function applyCalibrationStatus() {
    const status = calibrationState || {};
    const state = status.state || "idle";
    const running = !!status.running;
    const complete = !!status.complete;
    const completed = Math.max(0, Math.min(4, status.completedSpeakers || 0));
    const currentSpeaker = Math.max(1, Math.min(4, status.currentSpeaker || 1));

    const startButton = document.getElementById("cal-start-btn");
    if (startButton) {
        if (running) startButton.textContent = "ABORT";
        else if (complete) startButton.textContent = "MEASURE AGAIN";
        else startButton.textContent = "START MEASURE";
    }

    const meter = document.getElementById("capture-meter");
    const bar = document.getElementById("capture-bar");
    const captureLabel = meter ? meter.querySelector(".capture-label") : null;

    if (meter && bar) {
        const showMeter = running || complete;
        meter.style.display = showMeter ? "flex" : "none";

        let phasePercent = status.overallPercent || 0;
        if (state === "playing") phasePercent = status.playPercent || 0;
        else if (state === "recording") phasePercent = status.recordPercent || 0;

        const percent = Math.max(0, Math.min(100, Math.round(phasePercent * 100)));
        bar.style.height = `${percent}%`;
    }

    if (captureLabel) {
        captureLabel.textContent = status.message || "Idle - press Start to begin calibration";
    }

    const statusRows = document.querySelectorAll("#cal-status .status-row");

    for (let i = 0; i < 4; i++) {
        const dot = document.getElementById(`spk${i + 1}-dot`);
        const textEl = statusRows[i]?.querySelector(".status-text");
        if (!dot || !textEl) continue;

        dot.classList.remove("active", "complete");

        if (complete || i < completed) {
            dot.classList.add("complete");
            textEl.textContent = `SPK${i + 1}: Measured`;
            continue;
        }

        if (running && i === (currentSpeaker - 1)) {
            dot.classList.add("active");

            let phase = "Measuring";
            let phasePercent = 0;
            if (state === "playing") {
                phase = "Playing test signal";
                phasePercent = status.playPercent || 0;
            } else if (state === "recording") {
                phase = "Recording response";
                phasePercent = status.recordPercent || 0;
            } else if (state === "analyzing") {
                phase = "Analyzing IR";
                phasePercent = 1;
            }

            const percentText = state === "analyzing"
                ? ""
                : ` (${Math.max(0, Math.min(100, Math.round(phasePercent * 100)))}%)`;

            textEl.textContent = `SPK${i + 1}: ${phase}${percentText}`;
            continue;
        }

        textEl.textContent = `SPK${i + 1}: Not measured`;
    }

    const roomDot = document.getElementById("room-dot");
    const roomLabel = document.getElementById("room-label");
    const profileReady = !!status.profileValid || complete;
    if (roomDot) {
        roomDot.classList.toggle("loaded", profileReady);
        roomDot.classList.toggle("none", !profileReady);
    }
    if (roomLabel) {
        roomLabel.textContent = profileReady ? "Profile Loaded" : "No Profile";
    }

    const sceneStatus = document.getElementById("scene-status");
    if (sceneStatus && currentMode === "calibrate") {
        sceneStatus.className = "scene-status";
        if (complete) {
            sceneStatus.textContent = "PROFILE READY";
            sceneStatus.classList.add("ready");
        } else if (running) {
            sceneStatus.textContent = "MEASURING";
        } else {
            sceneStatus.textContent = "NO PROFILE";
            sceneStatus.classList.add("noprofile");
        }
    }

    const viewportInfo = document.getElementById("viewport-info");
    if (viewportInfo && currentMode === "calibrate") {
        viewportInfo.textContent = "Calibrate Mode \u00B7 " + (status.message || "Room Profile Setup");
    }
}

function updateEmitterMeshes(emitters) {
    if (!threeScene) return;

    // Track which IDs are still active
    const activeIds = new Set();

    emitters.forEach(em => {
        activeIds.add(em.id);
        let mesh = emitterMeshes.get(em.id);

        if (!mesh) {
            // Create new emitter mesh
            const geo = new THREE.SphereGeometry(0.25, 12, 8);
            const color = emitterPalette[em.color % emitterPalette.length];
            const mat = new THREE.MeshBasicMaterial({
                color: color, wireframe: true,
                transparent: true, opacity: 0.7
            });
            mesh = new THREE.Mesh(geo, mat);
            mesh.userData.emitterId = em.id;
            threeScene.add(mesh);
            emitterMeshes.set(em.id, mesh);
        }

        // Update position and properties
        mesh.userData.emitterId = em.id;
        mesh.position.set(em.x, em.y, em.z);
        mesh.scale.set(em.sx * 2, em.sy * 2, em.sz * 2);
        const isSelected = em.id === selectedEmitterId;
        mesh.material.color.setHex(emitterPalette[em.color % emitterPalette.length]);
        mesh.material.opacity = em.muted ? 0.15 : (isSelected ? 0.92 : 0.7);
        mesh.visible = currentMode !== "calibrate";
    });

    // Remove meshes for emitters no longer active
    for (const [id, mesh] of emitterMeshes) {
        if (!activeIds.has(id)) {
            threeScene.remove(mesh);
            emitterMeshes.delete(id);
        }
    }

    if (!activeIds.has(selectedEmitterId)) {
        if (localEmitterId >= 0 && activeIds.has(localEmitterId)) {
            selectedEmitterId = localEmitterId;
        } else if (emitters.length > 0) {
            selectedEmitterId = emitters[0].id;
        } else {
            selectedEmitterId = -1;
        }
    }

    updateSelectionRingFromState();
}

function updateSceneList(emitters) {
    if (currentMode !== "renderer") return;
    const list = document.getElementById("scene-list");
    if (!list) return;

    list.innerHTML = "";
    emitters.forEach(em => {
        const color = "#" + (emitterPalette[em.color % emitterPalette.length]).toString(16).padStart(6, "0");
        const item = document.createElement("div");
        item.className = "scene-item";
        if (em.id === selectedEmitterId) {
            item.classList.add("selected");
        }
        item.innerHTML = `<span class="scene-dot" style="background:${color};"></span>` +
            `<span class="scene-name">${em.label}</span>` +
            `<button class="scene-action-btn">S</button>` +
            `<button class="scene-action-btn">M</button>`;
        item.addEventListener("click", () => {
            setSelectedEmitter(em.id);
            updateSceneList(sceneData.emitters || []);
        });

        const actionButtons = item.querySelectorAll(".scene-action-btn");
        const soloButton = actionButtons[0];
        const muteButton = actionButtons[1];
        if (soloButton) {
            soloButton.addEventListener("click", event => {
                event.stopPropagation();
                if (em.id !== localEmitterId) return;
                setToggleValue(toggleStates.emit_solo, !getToggleValue(toggleStates.emit_solo));
            });
        }
        if (muteButton) {
            muteButton.addEventListener("click", event => {
                event.stopPropagation();
                if (em.id !== localEmitterId) return;
                setToggleValue(toggleStates.emit_mute, !getToggleValue(toggleStates.emit_mute));
            });
        }

        list.appendChild(item);
    });
}

// ===== ANIMATION LOOP =====
function animate() {
    requestAnimationFrame(animate);
    animTime += 0.016;

    // Selection ring float
    if (selectionRing && selectionRing.visible) {
        selectionRing.position.y = selectionRingBaseY + Math.sin(animTime * 2.0) * 0.015;
    }

    const calibrationCurrentSpeaker = Math.max(0, Math.min(3, (calibrationState.currentSpeaker || 1) - 1));

    // Speaker energy meters
    speakerMeters.forEach((m, i) => {
        if (currentMode === "calibrate") {
            const level = getCalibrationSpeakerLevel(i);
            const pulse = (calibrationState.running && i === calibrationCurrentSpeaker)
                ? 0.8 + 0.2 * Math.sin(animTime * 10.0)
                : 1.0;
            m.target = level * 0.45 * pulse;
        } else if (currentMode === "renderer" || currentMode === "emitter") {
            m.target = 0.15 + Math.sin(animTime * 3 + i * 1.7) * 0.12 + Math.random() * 0.03;
        } else {
            m.target = 0;
        }
        m.level += (m.target - m.level) * 0.15;
        m.mesh.scale.y = Math.max(0.5, m.level * 40);
        m.mesh.position.y = m.basePos.y + m.level * 0.2;

        let meterColor = 0xE0E0E0;
        if (currentMode === "calibrate") {
            meterColor = getCalibrationSpeakerColor(i);
        } else {
            const t = Math.min(1, Math.max(0, (m.level - 0.2) / 0.3));
            meterColor = t > 0.5 ? 0xD4A847 : 0xE0E0E0;
        }

        speakers[i]?.material?.color?.setHex(currentMode === "calibrate" ? getCalibrationSpeakerColor(i) : 0xE0E0E0);
        m.mesh.material.color.setHex(meterColor);
        m.mesh.material.opacity = 0.4 + m.level * 0.6;
    });

    // Lane highlight transitions
    for (const key in highlightCurrent) {
        highlightCurrent[key] += (highlightTargets[key] - highlightCurrent[key]) * 0.12;
    }
    if (azArc) azArc.material.opacity = highlightCurrent.azimuth;
    if (distRing) distRing.material.opacity = highlightCurrent.distance;
    if (elArc) elArc.material.opacity = highlightCurrent.elevation;

    if (rendererGL && threeScene && camera) {
        rendererGL.render(threeScene, camera);
    }
}
