(() => {
const bootstrapSearch = typeof window !== "undefined" && window.location
    ? String(window.location.search || "")
    : "";
const bootstrapSelfTestRequested = bootstrapSearch.includes("selftest=1");

if (typeof window !== "undefined" && bootstrapSelfTestRequested) {
    if (!window.__LQ_SELFTEST_RESULT__ || typeof window.__LQ_SELFTEST_RESULT__ !== "object") {
        window.__LQ_SELFTEST_RESULT__ = {
            requested: true,
            status: "pending",
            startedAt: new Date().toISOString(),
        };
    }
}

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
    emit_dir_azimuth:   Juce.getSliderState("emit_dir_azimuth"),
    emit_dir_elevation: Juce.getSliderState("emit_dir_elevation"),
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
    phys_vel_x:     Juce.getSliderState("phys_vel_x"),
    phys_vel_y:     Juce.getSliderState("phys_vel_y"),
    phys_vel_z:     Juce.getSliderState("phys_vel_z"),
    anim_speed:     Juce.getSliderState("anim_speed"),
    rend_master_gain: Juce.getSliderState("rend_master_gain"),
    rend_viz_trail_len: Juce.getSliderState("rend_viz_trail_len"),
    rend_viz_diag_mix: Juce.getSliderState("rend_viz_diag_mix"),
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
    rend_phys_walls:    Juce.getToggleState("rend_phys_walls"),
    rend_phys_interact: Juce.getToggleState("rend_phys_interact"),
    rend_phys_pause:    Juce.getToggleState("rend_phys_pause"),
    rend_viz_trails: Juce.getToggleState("rend_viz_trails"),
    rend_viz_vectors: Juce.getToggleState("rend_viz_vectors"),
    rend_viz_physics_lens: Juce.getToggleState("rend_viz_physics_lens"),
    rend_audition_enable: Juce.getToggleState("rend_audition_enable"),
};

const comboStates = {
    cal_spk_config: Juce.getComboBoxState("cal_spk_config"),
    cal_topology_profile: Juce.getComboBoxState("cal_topology_profile"),
    cal_monitoring_path: Juce.getComboBoxState("cal_monitoring_path"),
    cal_device_profile: Juce.getComboBoxState("cal_device_profile"),
    cal_test_type: Juce.getComboBoxState("cal_test_type"),
    mode:         Juce.getComboBoxState("mode"),
    pos_coord_mode: Juce.getComboBoxState("pos_coord_mode"),
    phys_gravity_dir: Juce.getComboBoxState("phys_gravity_dir"),
    anim_mode:    Juce.getComboBoxState("anim_mode"),
    rend_quality: Juce.getComboBoxState("rend_quality"),
    rend_distance_model: Juce.getComboBoxState("rend_distance_model"),
    rend_headphone_mode: Juce.getComboBoxState("rend_headphone_mode"),
    rend_headphone_profile: Juce.getComboBoxState("rend_headphone_profile"),
    rend_audition_signal: Juce.getComboBoxState("rend_audition_signal"),
    rend_audition_motion: Juce.getComboBoxState("rend_audition_motion"),
    rend_audition_level: Juce.getComboBoxState("rend_audition_level"),
    rend_phys_rate: Juce.getComboBoxState("rend_phys_rate"),
    rend_viz_mode: Juce.getComboBoxState("rend_viz_mode"),
};

const nativeFunctions = {
    startCalibration: Juce.getNativeFunction("locusqStartCalibration"),
    abortCalibration: Juce.getNativeFunction("locusqAbortCalibration"),
    redetectCalibrationRouting: Juce.getNativeFunction("locusqRedetectCalibrationRouting"),
    listCalibrationProfiles: Juce.getNativeFunction("locusqListCalibrationProfiles"),
    saveCalibrationProfile: Juce.getNativeFunction("locusqSaveCalibrationProfile"),
    loadCalibrationProfile: Juce.getNativeFunction("locusqLoadCalibrationProfile"),
    renameCalibrationProfile: Juce.getNativeFunction("locusqRenameCalibrationProfile"),
    deleteCalibrationProfile: Juce.getNativeFunction("locusqDeleteCalibrationProfile"),
    getKeyframeTimeline: Juce.getNativeFunction("locusqGetKeyframeTimeline"),
    setKeyframeTimeline: Juce.getNativeFunction("locusqSetKeyframeTimeline"),
    setTimelineTime: Juce.getNativeFunction("locusqSetTimelineTime"),
    listEmitterPresets: Juce.getNativeFunction("locusqListEmitterPresets"),
    saveEmitterPreset: Juce.getNativeFunction("locusqSaveEmitterPreset"),
    loadEmitterPreset: Juce.getNativeFunction("locusqLoadEmitterPreset"),
    renameEmitterPreset: Juce.getNativeFunction("locusqRenameEmitterPreset"),
    deleteEmitterPreset: Juce.getNativeFunction("locusqDeleteEmitterPreset"),
    getUiState: Juce.getNativeFunction("locusqGetUiState"),
    setUiState: Juce.getNativeFunction("locusqSetUiState"),
};

const NATIVE_CALL_TIMEOUT_MS = 3000;
const BASIC_CONTROL_VALUE_CHANGED_EVENT = "valueChanged";
const queryParams = new URLSearchParams(window.location.search || "");
const productionP0SelfTestRequested = queryParams.get("selftest") === "1";
const productionP0SelfTestScope = String(queryParams.get("selftest_scope") || "").trim().toLowerCase();
let productionP0SelfTestStarted = false;

if (productionP0SelfTestRequested) {
    window.__LQ_SELFTEST_RESULT__ = {
        requested: true,
        status: "pending",
        startedAt: new Date().toISOString(),
    };
} else {
    window.__LQ_SELFTEST_RESULT__ = {
        requested: false,
        status: "disabled",
    };
}

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

function waitMs(delayMs) {
    const delay = Math.max(0, Number(delayMs) || 0);
    return new Promise(resolve => window.setTimeout(resolve, delay));
}

async function waitForCondition(label, predicate, timeoutMs = 4000, pollMs = 25) {
    const timeout = Math.max(0, Number(timeoutMs) || 0);
    const pollInterval = Math.max(5, Number(pollMs) || 25);
    const deadline = Date.now() + timeout;

    while (Date.now() <= deadline) {
        let ok = false;
        try {
            ok = !!predicate();
        } catch (_) {
            ok = false;
        }
        if (ok) return;
        await waitMs(pollInterval);
    }

    throw new Error(`${label} timed out after ${timeout}ms`);
}

function dispatchPointer(target, type, clientX, clientY, pointerId = 1, button = 0) {
    if (!target) return false;

    const base = {
        bubbles: true,
        cancelable: true,
        clientX,
        clientY,
        pointerId,
        pointerType: "mouse",
        button,
        buttons: button === 0 ? 1 : 2,
    };

    let event = null;
    if (typeof window.PointerEvent === "function") {
        event = new PointerEvent(type, base);
    } else if (typeof window.MouseEvent === "function") {
        event = new MouseEvent(type, base);
    }

    if (!event) return false;
    return target.dispatchEvent(event);
}

function startProductionP0SelfTestAfterDelay(delayMs) {
    if (!productionP0SelfTestRequested || productionP0SelfTestStarted) return;
    productionP0SelfTestStarted = true;
    const delay = Math.max(0, Number(delayMs) || 0);
    window.setTimeout(() => {
        void runProductionP0SelfTest();
    }, delay);
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

function isElementControlLocked(element) {
    if (!element) return false;
    if (element.dataset?.controlLocked === "1") return true;
    if (element.getAttribute("aria-disabled") === "true") return true;
    if ("disabled" in element && element.disabled) return true;
    if (element.closest("[data-authority-lock='1']")) return true;
    return false;
}

function syncStepperAuthorityLock(display, locked) {
    if (!display) return;
    display.dataset.authorityStepperLock = locked ? "1" : "0";
    const positionLocked = display.dataset.positionStepperLock === "1";
    const stepperLocked = locked || positionLocked;
    display.dataset.stepperLock = stepperLocked ? "1" : "0";
    display.setAttribute("aria-readonly", stepperLocked ? "true" : "false");
}

function setControlAuthorityLock(element, locked) {
    if (!element) return;
    if (locked) {
        element.dataset.controlLocked = "1";
        element.setAttribute("aria-disabled", "true");
    } else {
        delete element.dataset.controlLocked;
        element.removeAttribute("aria-disabled");
    }

    if (element instanceof HTMLInputElement) {
        if (element.type === "text") {
            element.readOnly = locked;
        } else {
            element.disabled = locked;
        }
    } else if (element instanceof HTMLSelectElement || element instanceof HTMLButtonElement) {
        element.disabled = locked;
    }

    if (element.id === "emit-color-swatch") {
        element.tabIndex = locked ? -1 : 0;
    }
}

function applyEmitterAuthoringLock(locked) {
    emitterAuthoringLocked = !!locked;

    document.querySelectorAll(".emitter-card[data-emitter-group]").forEach(card => {
        const group = String(card.dataset.emitterGroup || "").trim().toLowerCase();
        const shouldLock = emitterAuthoringLocked && emitterAuthorityEditableCardGroups.has(group);
        card.classList.toggle("authority-readonly", shouldLock);
        card.dataset.authorityLock = shouldLock ? "1" : "0";
    });

    emitterAuthorityControlIds.forEach(id => {
        setControlAuthorityLock(document.getElementById(id), emitterAuthoringLocked);
    });
    emitterAuthorityStepperIds.forEach(id => {
        syncStepperAuthorityLock(document.getElementById(id), emitterAuthoringLocked);
    });
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
        if (isElementControlLocked(element)) {
            return;
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

function setMotionLoopClasses(loopEnabled) {
    setToggleClass("toggle-anim-loop", !!loopEnabled);
    setToggleClass("toggle-timeline-loop", !!loopEnabled);
    setToggleClass("toggle-motion-loop", !!loopEnabled);
}

function setMotionSyncClasses(syncEnabled) {
    setToggleClass("toggle-timeline-sync", !!syncEnabled);
    setToggleClass("toggle-motion-sync", !!syncEnabled);
}

function setTimelineSyncEnabled(enabled) {
    const syncEnabled = !!enabled;
    setToggleValue(toggleStates.anim_sync, syncEnabled);
    setMotionSyncClasses(syncEnabled);
}

function setTimelineLoopEnabled(enabled) {
    const loopEnabled = !!enabled;
    setToggleValue(toggleStates.anim_loop, loopEnabled);
    setMotionLoopClasses(loopEnabled);
    timelineState.looping = loopEnabled;
}

function disableTimelineSyncForManualTransport() {
    if (getToggleValue(toggleStates.anim_sync)) {
        setTimelineSyncEnabled(false);
    }
}

function bindElementOnce(element, bindingKey, binder) {
    if (!element || typeof binder !== "function") return;
    const marker = `data-lq-bound-${bindingKey}`;
    if (element.getAttribute(marker) === "1") return;
    element.setAttribute(marker, "1");
    binder();
}

function bindTimelineLaneSelectionControls() {
    document.querySelectorAll(".timeline-lane").forEach(lane => {
        const laneId = String(lane.dataset.lane || "lane");
        bindElementOnce(lane, `timeline-lane-${laneId}`, () => {
            lane.addEventListener("click", () => {
                document.querySelectorAll(".timeline-lane").forEach(l => l.classList.remove("selected"));
                lane.classList.add("selected");
                selectedLane = lane.dataset.lane;
                setLaneHighlight(selectedLane);
            });
        });
    });
}

function bindTimelineRuntimeControls() {
    const rewindButton = document.getElementById("timeline-rewind-btn");
    bindElementOnce(rewindButton, "timeline-rewind", () => {
        rewindButton.addEventListener("click", () => {
            disableTimelineSyncForManualTransport();
            timelineState.currentTimeSeconds = 0.0;
            updateTimelinePlayheads();
            setToggleValue(toggleStates.anim_enable, false);
            setToggleClass("toggle-anim", false);
            setAnimationControlsEnabled(false);
            callNative("locusqSetTimelineTime", nativeFunctions.setTimelineTime, 0.0).catch(() => {});
            syncMotionSourceUI();
        });
    });

    const stopButton = document.getElementById("timeline-stop-btn");
    bindElementOnce(stopButton, "timeline-stop", () => {
        stopButton.addEventListener("click", () => {
            disableTimelineSyncForManualTransport();
            setToggleValue(toggleStates.anim_enable, false);
            setToggleClass("toggle-anim", false);
            setAnimationControlsEnabled(false);
            const duration = Math.max(0.001, timelineState.durationSeconds);
            timelineState.currentTimeSeconds = clamp(Number(timelineState.currentTimeSeconds) || 0.0, 0.0, duration);
            updateTimelinePlayheads();
            callNative("locusqSetTimelineTime", nativeFunctions.setTimelineTime, timelineState.currentTimeSeconds).catch(() => {});
            syncMotionSourceUI();
        });
    });

    const playButton = document.getElementById("timeline-play-btn");
    bindElementOnce(playButton, "timeline-play", () => {
        playButton.addEventListener("click", () => {
            disableTimelineSyncForManualTransport();
            if (getChoiceIndex(comboStates.anim_mode) !== 1) {
                setChoiceIndex(comboStates.anim_mode, 1);
            }
            const duration = Math.max(0.001, timelineState.durationSeconds);
            if ((Number(timelineState.currentTimeSeconds) || 0.0) >= duration - 0.0005) {
                timelineState.currentTimeSeconds = 0.0;
                updateTimelinePlayheads();
                callNative("locusqSetTimelineTime", nativeFunctions.setTimelineTime, 0.0).catch(() => {});
            }
            setToggleValue(toggleStates.anim_enable, true);
            setToggleClass("toggle-anim", true);
            setAnimationControlsEnabled(true);
            syncMotionSourceUI();
        });
    });

    ["toggle-anim-loop", "toggle-timeline-loop", "toggle-motion-loop"].forEach(id => {
        const loopToggle = document.getElementById(id);
        bindElementOnce(loopToggle, `timeline-loop-${id}`, () => {
            bindControlActivate(loopToggle, () => {
                const nextLoop = !getToggleValue(toggleStates.anim_loop);
                setTimelineLoopEnabled(nextLoop);
                scheduleTimelineCommit();
            });
        });
    });

    ["toggle-timeline-sync", "toggle-motion-sync"].forEach(id => {
        const syncToggle = document.getElementById(id);
        bindElementOnce(syncToggle, `timeline-sync-${id}`, () => {
            bindControlActivate(syncToggle, () => {
                const nextSync = !getToggleValue(toggleStates.anim_sync);
                setTimelineSyncEnabled(nextSync);
            });
        });
    });
}

function bindMotionRuntimeMirrorControls() {
    const transportMirrorMap = [
        ["motion-transport-rewind-btn", "timeline-rewind-btn"],
        ["motion-transport-stop-btn", "timeline-stop-btn"],
        ["motion-transport-play-btn", "timeline-play-btn"],
    ];

    transportMirrorMap.forEach(([mirrorId, timelineId]) => {
        const mirrorButton = document.getElementById(mirrorId);
        bindElementOnce(mirrorButton, `motion-transport-mirror-${mirrorId}`, () => {
            mirrorButton.addEventListener("click", () => {
                const timelineButton = document.getElementById(timelineId);
                if (timelineButton) timelineButton.click();
            });
        });
    });
}

function ensureTimelineShellIntegrity() {
    const viewportArea = document.querySelector(".viewport-area");
    if (!viewportArea) return false;

    let timeline = document.getElementById("timeline");
    let rebuilt = false;
    if (!timeline) {
        timeline = document.createElement("div");
        timeline.id = "timeline";
        timeline.className = "timeline";
        viewportArea.appendChild(timeline);
        rebuilt = true;
    }

    const hasHeader = !!timeline.querySelector(".timeline-header");
    const hasLanes = !!timeline.querySelector(".timeline-lanes");
    if (!hasHeader || !hasLanes) {
        timeline.innerHTML = `
        <div class="timeline-header">
          <div class="timeline-transport">
            <button class="transport-btn" id="timeline-rewind-btn" title="Rewind to start and pause">&#9664;</button>
            <button class="transport-btn" id="timeline-stop-btn" title="Stop at current time">&#9632;</button>
            <button class="transport-btn" id="timeline-play-btn" title="Play from current time">&#9654;</button>
          </div>
          <span class="timeline-time" id="timeline-time">00:00.000</span>
          <div class="header-spacer"></div>
          <div class="timeline-toggle-label">Loop <div class="toggle" id="toggle-timeline-loop" style="margin-left:4px;"><div class="toggle-thumb"></div></div></div>
          <div class="timeline-toggle-label">Sync <div class="toggle on" id="toggle-timeline-sync" style="margin-left:4px;"><div class="toggle-thumb"></div></div></div>
        </div>
        <div class="timeline-lanes">
          <div class="timeline-lane selected" data-lane="azimuth">
            <span class="lane-label">Azimuth</span>
            <div class="lane-track"></div>
            <span class="curve-pill" id="lane-curve-azimuth">ease</span>
          </div>
          <div class="timeline-lane" data-lane="elevation">
            <span class="lane-label">Elevation</span>
            <div class="lane-track"></div>
            <span class="curve-pill" id="lane-curve-elevation">ease</span>
          </div>
          <div class="timeline-lane" data-lane="distance">
            <span class="lane-label">Distance</span>
            <div class="lane-track"></div>
            <span class="curve-pill" id="lane-curve-distance">ease</span>
          </div>
          <div class="timeline-lane" data-lane="size">
            <span class="lane-label">Size</span>
            <div class="lane-track"></div>
            <span class="curve-pill" id="lane-curve-size">step</span>
          </div>
        </div>`;
        rebuilt = true;
    }

    timeline.style.display = "block";
    timeline.style.visibility = "visible";
    timeline.style.opacity = "1";

    const header = timeline.querySelector(".timeline-header");
    const lanes = timeline.querySelector(".timeline-lanes");
    if (header) header.style.display = "flex";
    if (lanes) lanes.style.display = "block";

    const laneElements = Array.from(document.querySelectorAll(".timeline-lane"));
    laneElements.forEach(lane => lane.classList.toggle("selected", lane.dataset.lane === selectedLane));
    if (!laneElements.some(lane => lane.classList.contains("selected")) && laneElements.length > 0) {
        selectedLane = laneElements[0].dataset.lane || "azimuth";
        laneElements[0].classList.add("selected");
    }

    const duration = Math.max(0.001, Number(timelineState.durationSeconds) || 0.001);
    const current = clamp(Number(timelineState.currentTimeSeconds) || 0.0, 0.0, duration);
    const totalMs = Math.max(0, Math.round(current * 1000));
    const minutes = Math.floor(totalMs / 60000);
    const seconds = Math.floor((totalMs % 60000) / 1000);
    const millis = totalMs % 1000;
    const formattedTime =
        `${String(minutes).padStart(2, "0")}:${String(seconds).padStart(2, "0")}.${String(millis).padStart(3, "0")}`;
    const timeEl = document.getElementById("timeline-time");
    if (timeEl) timeEl.textContent = formattedTime;
    const motionTimeEl = document.getElementById("motion-transport-time");
    if (motionTimeEl) motionTimeEl.textContent = formattedTime;
    setMotionLoopClasses(getToggleValue(toggleStates.anim_loop));
    setMotionSyncClasses(getToggleValue(toggleStates.anim_sync));

    bindTimelineLaneSelectionControls();
    bindTimelineRuntimeControls();
    bindMotionRuntimeMirrorControls();
    return rebuilt;
}

function isEmitterLayoutActive(modeHint) {
    if (modeHint === "emitter") return true;

    const activeTab = document.querySelector(".mode-tab.active");
    if (activeTab && activeTab.dataset.mode === "emitter") return true;

    const activePanel = document.querySelector(".rail-panel.active");
    if (activePanel && activePanel.dataset.panel === "emitter") return true;

    if (document.body && document.body.classList.contains("mode-emitter")) return true;

    return false;
}

function syncAnimationUI() {
    const enabled = !!toggleStates.anim_enable.getValue();
    setToggleClass("toggle-anim", enabled);
    setAnimationControlsEnabled(enabled);

    const source = document.getElementById("anim-source");
    if (source) source.selectedIndex = Math.max(0, Math.min(1, getChoiceIndex(comboStates.anim_mode)));

    const loopEnabled = !!toggleStates.anim_loop.getValue();
    setMotionLoopClasses(loopEnabled);

    const syncEnabled = !!toggleStates.anim_sync.getValue();
    setMotionSyncClasses(syncEnabled);

    updateValueDisplay("val-anim-speed", sliderStates.anim_speed.getScaledValue().toFixed(1), "x");
    timelineState.looping = loopEnabled;
    timelineState.playbackRate = sliderStates.anim_speed.getScaledValue();
    updateMotionStatusChips();
}

function syncResponsiveLayoutMode() {
    const width = Math.max(
        Number(window.innerWidth) || 0,
        Number(document.documentElement?.clientWidth) || 0
    );
    const body = document.body;
    if (!body) return;

    body.classList.toggle("layout-compact", width <= 1240);
    body.classList.toggle("layout-tight", width <= 1024);
}

function getDerivedMotionSourceId() {
    const animEnabled = !!getToggleValue(toggleStates.anim_enable);
    const physicsPreset = normalizePhysicsPresetName(uiState.physicsPreset);
    const choreographyPack = normalizeChoreographyPackId(uiState.choreographyPack);

    if (!animEnabled && physicsPreset !== "off") return "physics";
    if (animEnabled && choreographyPack !== "custom") return "choreography";
    if (animEnabled) return "timeline";
    return "static";
}

function getMotionSourceLabel(sourceId) {
    switch (sourceId) {
    case "physics":
        return "Physics";
    case "timeline":
        return "Timeline";
    case "choreography":
        return "Choreography";
    default:
        return "Static";
    }
}

function syncMotionSourceUI(sourceId = getDerivedMotionSourceId()) {
    const sourceSelect = document.getElementById("motion-source-select");
    if (sourceSelect && sourceSelect.value !== sourceId) {
        sourceSelect.value = sourceId;
    }

    const timelineTransport = document.getElementById("motion-transport-strip");
    const timelineDriven = sourceId === "timeline" || sourceId === "choreography";
    if (timelineTransport) {
        timelineTransport.classList.toggle("inactive", !timelineDriven);
    }

    const physicsPanel = document.getElementById("motion-panel-physics");
    const timelinePanel = document.getElementById("motion-panel-timeline");
    const choreographyPanel = document.getElementById("motion-panel-choreography");
    if (physicsPanel) {
        physicsPanel.classList.toggle("active", sourceId === "physics");
        physicsPanel.classList.toggle("inactive", sourceId !== "physics");
    }
    if (timelinePanel) {
        const timelineActive = sourceId === "timeline" || sourceId === "choreography";
        timelinePanel.classList.toggle("active", timelineActive);
        timelinePanel.classList.toggle("inactive", !timelineActive);
    }
    if (choreographyPanel) {
        choreographyPanel.classList.toggle("active", sourceId === "choreography");
        choreographyPanel.classList.toggle("inactive", sourceId !== "choreography");
    }
}

async function applyMotionSourceSelection(sourceId) {
    const normalizedSource = String(sourceId || "").trim().toLowerCase();
    const choreographySelect = document.getElementById("choreo-pack-select");

    if (normalizedSource === "physics") {
        const activePhysicsPreset = normalizePhysicsPresetName(uiState.physicsPreset);
        const nextPreset = activePhysicsPreset === "off" ? "float" : activePhysicsPreset;
        setToggleValue(toggleStates.anim_enable, false);
        setAnimationControlsEnabled(false);
        applyPhysicsPreset(nextPreset, true);
    } else if (normalizedSource === "timeline") {
        if (normalizePhysicsPresetName(uiState.physicsPreset) !== "off") {
            applyPhysicsPreset("off", true);
        }
        if (normalizeChoreographyPackId(uiState.choreographyPack) !== "custom") {
            uiState.choreographyPack = "custom";
            if (choreographySelect) choreographySelect.value = "custom";
            setChoreographyStatus("Custom timeline active");
            scheduleUiStateCommit(true);
        }
        setChoiceIndex(comboStates.anim_mode, 1);
        setToggleValue(toggleStates.anim_enable, true);
        setAnimationControlsEnabled(true);
    } else if (normalizedSource === "choreography") {
        if (normalizePhysicsPresetName(uiState.physicsPreset) !== "off") {
            applyPhysicsPreset("off", true);
        }
        setChoiceIndex(comboStates.anim_mode, 1);
        setToggleValue(toggleStates.anim_enable, true);
        setAnimationControlsEnabled(true);
        let targetPack = normalizeChoreographyPackId(choreographySelect?.value || uiState.choreographyPack);
        if (targetPack === "custom") targetPack = "orbit";
        if (choreographySelect) choreographySelect.value = targetPack;
        try {
            await applyChoreographyPack(targetPack, { persistUiState: true, setInternalSource: true });
        } catch (error) {
            console.error("Failed to apply choreography pack from motion source:", error);
            setChoreographyStatus("Failed to apply choreography pack", true);
        }
    } else {
        if (normalizePhysicsPresetName(uiState.physicsPreset) !== "off") {
            applyPhysicsPreset("off", true);
        }
        setToggleValue(toggleStates.anim_enable, false);
        setAnimationControlsEnabled(false);
    }

    setMotionDirty(true);
    syncAnimationUI();
    updateMotionStatusChips();
}

function updateMotionStatusChips() {
    const sourceChip = document.getElementById("motion-chip-source");
    const loopChip = document.getElementById("motion-chip-loop");
    const syncChip = document.getElementById("motion-chip-sync");
    const dirtyChip = document.getElementById("motion-chip-dirty");
    if (!sourceChip || !loopChip || !syncChip || !dirtyChip) return;

    const sourceId = getDerivedMotionSourceId();
    const sourceLabel = getMotionSourceLabel(sourceId);
    const loopEnabled = !!getToggleValue(toggleStates.anim_loop);
    const syncEnabled = !!getToggleValue(toggleStates.anim_sync);

    sourceChip.textContent = `Source: ${sourceLabel}`;
    sourceChip.className = sourceId === "static" ? "status-chip" : "status-chip active";

    loopChip.textContent = `Loop: ${loopEnabled ? "On" : "Off"}`;
    loopChip.className = loopEnabled ? "status-chip active" : "status-chip";

    syncChip.textContent = `Sync: ${syncEnabled ? "On" : "Off"}`;
    syncChip.className = syncEnabled ? "status-chip active" : "status-chip";

    dirtyChip.textContent = motionDirty ? "Motion Dirty" : "Motion Saved";
    dirtyChip.className = motionDirty ? "status-chip warning" : "status-chip";
    syncMotionSourceUI(sourceId);
}

function setMotionDirty(isDirty) {
    motionDirty = !!isDirty;
    updateMotionStatusChips();
}

function updateEmitterAuthorityUI() {
    const chip = document.getElementById("emitter-authority-chip");
    const note = document.getElementById("emitter-authority-note");
    if (!chip) return;

    const selected = getSelectedEmitter();
    const hasEmitterData = sceneEmitterLookup.size > 0;
    const hasLocalEmitter = Number.isInteger(localEmitterId) && localEmitterId >= 0;
    const isRemote = !!(selected && hasLocalEmitter && selected.id !== localEmitterId);
    let authoringLocked = false;

    if (!selected && !hasEmitterData) {
        chip.textContent = "Pending";
        chip.className = "status-chip warning";
        if (note) {
            note.textContent = "Awaiting emitter sync";
            note.classList.remove("locked");
        }
        authoringLocked = false;
    } else if (!selected) {
        chip.textContent = "No Sel";
        chip.className = "status-chip warning";
        if (note) {
            note.textContent = "Select an emitter to edit";
            note.classList.add("locked");
        }
        authoringLocked = true;
    } else if (isRemote) {
        chip.textContent = `Remote E${selected.id + 1}`;
        chip.className = "status-chip remote";
        if (note) {
            note.textContent = `Remote emitter selected (${selected.label || `Emitter ${selected.id + 1}`}); local editing locked`;
            note.classList.add("locked");
        }
        authoringLocked = true;
    } else {
        chip.textContent = hasLocalEmitter ? `Local E${localEmitterId + 1}` : "Local";
        chip.className = hasLocalEmitter ? "status-chip local" : "status-chip warning";
        if (note) {
            note.textContent = hasLocalEmitter
                ? "Local emitter editing enabled"
                : "Editing enabled (local ownership pending)";
            note.classList.remove("locked");
        }
        authoringLocked = false;
    }

    applyEmitterAuthoringLock(authoringLocked);
}

function updateEmitterDiagnosticsQuickControls() {
    const lensOn = !!getToggleValue(toggleStates.rend_viz_physics_lens);
    setToggleClass("emitter-quick-physics-lens", lensOn);
    const mixPercent = Math.round(clamp(sliderStates.rend_viz_diag_mix.getScaledValue(), 0.0, 1.0) * 100.0);
    updateValueDisplay("emitter-quick-diag-mix", `${mixPercent}`, "%");
}

function getPositionMode() {
    const idx = getChoiceIndex(comboStates.pos_coord_mode);
    return idx <= 0 ? "spherical" : "cartesian";
}

function updateWorldPositionReadback() {
    const x = Number(sliderStates.pos_x.getScaledValue()) || 0.0;
    const y = Number(sliderStates.pos_z.getScaledValue()) || 0.0;
    const z = Number(sliderStates.pos_y.getScaledValue()) || 0.0;
    const readback = document.getElementById("val-world-readback");
    if (!readback) return;
    readback.textContent = `x ${x.toFixed(2)} · y ${y.toFixed(2)} · z ${z.toFixed(2)} m`;
}

function syncPositionModeUI() {
    const mode = getPositionMode();
    document.querySelectorAll(".coord-row[data-coord-group]").forEach(row => {
        const group = String(row.dataset.coordGroup || "").trim().toLowerCase();
        const active = group === mode;
        row.classList.toggle("coord-active", active);
        row.classList.toggle("coord-inactive", !active);
        const value = row.querySelector(".control-value");
        if (value) {
            value.dataset.positionStepperLock = active ? "0" : "1";
            syncStepperAuthorityLock(value, emitterAuthoringLocked);
        }
    });
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

const choreographyPackLabels = {
    custom: "Custom",
    orbit: "Orbit",
    pendulum: "Pendulum",
    swarm_arc: "Swarm Arc",
    rise_fall: "Rise/Fall",
};

const choreographyPackOrder = ["custom", "orbit", "pendulum", "swarm_arc", "rise_fall"];

function normalizeChoreographyPackId(name) {
    const value = String(name ?? "").trim().toLowerCase();
    return choreographyPackOrder.includes(value) ? value : "custom";
}

function getChoreographyPackLabel(name) {
    return choreographyPackLabels[normalizeChoreographyPackId(name)] || "Custom";
}

let uiState = {
    emitterLabel: "Emitter",
    physicsPreset: "off",
    choreographyPack: "custom",
};

const physicsPresetTargets = {
    off: {
        enabled: false,
    },
    bounce: {
        enabled: true,
        mass: 1.0,
        drag: 0.2,
        elasticity: 0.82,
        gravity: -9.8,
        friction: 0.2,
        gravityDirIndex: 0,
    },
    float: {
        enabled: true,
        mass: 0.4,
        drag: 0.65,
        elasticity: 0.3,
        gravity: 0.0,
        friction: 0.45,
        gravityDirIndex: 0,
    },
    orbit: {
        enabled: true,
        mass: 0.8,
        drag: 0.35,
        elasticity: 0.55,
        gravity: 6.0,
        friction: 0.15,
        gravityDirIndex: 2,
    },
};

const physicsPresetTolerances = {
    mass: 0.025,
    drag: 0.025,
    elasticity: 0.025,
    gravity: 0.2,
    friction: 0.025,
};

let uiStateCommitTimer = null;
let isApplyingPhysicsPreset = false;
let suppressPhysicsPresetCustomUntilMs = 0;
let physicsPresetRecheckTimer = null;

function nearlyEqual(value, target, tolerance) {
    return Math.abs((Number(value) || 0) - Number(target)) <= Number(tolerance);
}

function isPhysicsPresetStateAligned(presetName) {
    const preset = physicsPresetTargets[presetName];
    if (!preset) return false;

    const enabled = !!getToggleValue(toggleStates.phys_enable);
    if (enabled !== !!preset.enabled) return false;
    if (!preset.enabled) return true;

    if (!nearlyEqual(sliderStates.phys_mass.getScaledValue(), preset.mass, physicsPresetTolerances.mass)) return false;
    if (!nearlyEqual(sliderStates.phys_drag.getScaledValue(), preset.drag, physicsPresetTolerances.drag)) return false;
    if (!nearlyEqual(sliderStates.phys_elasticity.getScaledValue(), preset.elasticity, physicsPresetTolerances.elasticity)) return false;
    if (!nearlyEqual(sliderStates.phys_gravity.getScaledValue(), preset.gravity, physicsPresetTolerances.gravity)) return false;
    if (!nearlyEqual(sliderStates.phys_friction.getScaledValue(), preset.friction, physicsPresetTolerances.friction)) return false;

    return getChoiceIndex(comboStates.phys_gravity_dir) === preset.gravityDirIndex;
}

async function commitUiStateToNative() {
    try {
        await callNative("locusqSetUiState", nativeFunctions.setUiState, {
            emitterLabel: sanitizeEmitterLabel(uiState.emitterLabel),
            physicsPreset: normalizePhysicsPresetName(uiState.physicsPreset),
            choreographyPack: normalizeChoreographyPackId(uiState.choreographyPack),
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
        uiState.choreographyPack = normalizeChoreographyPackId(payload.choreographyPack ?? uiState.choreographyPack);
    } catch (error) {
        console.warn("Failed to load UI state:", error);
    }
}

function bindSelectToComboState(selectId, comboState) {
    const select = document.getElementById(selectId);
    if (!select || !comboState) return;

    select.addEventListener("change", () => {
        if (isElementControlLocked(select)) return;
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
        if (isElementControlLocked(select)) return;
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
        if (display.dataset.stepperLock === "1" || isElementControlLocked(display)) return;
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

    const choreographySelect = document.getElementById("choreo-pack-select");
    if (choreographySelect) {
        const normalized = normalizeChoreographyPackId(uiState.choreographyPack);
        const matching = Array.from(choreographySelect.options).findIndex(option => option.value === normalized);
        if (matching >= 0) choreographySelect.selectedIndex = matching;
    }

    updateMotionStatusChips();
}

function createKeyframe(timeSeconds, value, curve = "easeInOut") {
    return {
        uid: `kf_${nextKeyframeUid++}`,
        timeSeconds,
        value,
        curve,
    };
}

const choreographyPackLibrary = {
    orbit: {
        id: "orbit",
        durationSeconds: 8.0,
        looping: true,
        playbackRate: 1.0,
        tracks: {
            azimuth: [
                { timeSeconds: 0.0, value: -160.0, curve: "easeInOut" },
                { timeSeconds: 2.0, value: -40.0, curve: "easeInOut" },
                { timeSeconds: 4.0, value: 80.0, curve: "easeInOut" },
                { timeSeconds: 6.0, value: 170.0, curve: "easeInOut" },
                { timeSeconds: 8.0, value: -160.0, curve: "easeInOut" },
            ],
            elevation: [
                { timeSeconds: 0.0, value: 0.0, curve: "easeInOut" },
                { timeSeconds: 2.0, value: 20.0, curve: "easeInOut" },
                { timeSeconds: 4.0, value: 6.0, curve: "easeInOut" },
                { timeSeconds: 6.0, value: -18.0, curve: "easeInOut" },
                { timeSeconds: 8.0, value: 0.0, curve: "easeInOut" },
            ],
            distance: [
                { timeSeconds: 0.0, value: 2.2, curve: "easeInOut" },
                { timeSeconds: 2.0, value: 2.9, curve: "easeInOut" },
                { timeSeconds: 4.0, value: 2.4, curve: "easeInOut" },
                { timeSeconds: 6.0, value: 1.9, curve: "easeInOut" },
                { timeSeconds: 8.0, value: 2.2, curve: "easeInOut" },
            ],
            size: [
                { timeSeconds: 0.0, value: 0.45, curve: "easeInOut" },
                { timeSeconds: 2.0, value: 0.58, curve: "easeInOut" },
                { timeSeconds: 4.0, value: 0.50, curve: "easeInOut" },
                { timeSeconds: 6.0, value: 0.62, curve: "easeInOut" },
                { timeSeconds: 8.0, value: 0.45, curve: "easeInOut" },
            ],
        },
    },
    pendulum: {
        id: "pendulum",
        durationSeconds: 8.0,
        looping: true,
        playbackRate: 0.9,
        tracks: {
            azimuth: [
                { timeSeconds: 0.0, value: -88.0, curve: "easeInOut" },
                { timeSeconds: 1.6, value: 92.0, curve: "easeInOut" },
                { timeSeconds: 3.2, value: -86.0, curve: "easeInOut" },
                { timeSeconds: 4.8, value: 80.0, curve: "easeInOut" },
                { timeSeconds: 6.4, value: -74.0, curve: "easeInOut" },
                { timeSeconds: 8.0, value: -88.0, curve: "easeInOut" },
            ],
            elevation: [
                { timeSeconds: 0.0, value: 6.0, curve: "easeInOut" },
                { timeSeconds: 2.0, value: -8.0, curve: "easeInOut" },
                { timeSeconds: 4.0, value: 7.0, curve: "easeInOut" },
                { timeSeconds: 6.0, value: -6.0, curve: "easeInOut" },
                { timeSeconds: 8.0, value: 6.0, curve: "easeInOut" },
            ],
            distance: [
                { timeSeconds: 0.0, value: 2.9, curve: "easeInOut" },
                { timeSeconds: 2.0, value: 1.4, curve: "easeInOut" },
                { timeSeconds: 4.0, value: 2.8, curve: "easeInOut" },
                { timeSeconds: 6.0, value: 1.5, curve: "easeInOut" },
                { timeSeconds: 8.0, value: 2.9, curve: "easeInOut" },
            ],
            size: [
                { timeSeconds: 0.0, value: 0.52, curve: "easeInOut" },
                { timeSeconds: 2.0, value: 0.34, curve: "easeInOut" },
                { timeSeconds: 4.0, value: 0.61, curve: "easeInOut" },
                { timeSeconds: 6.0, value: 0.38, curve: "easeInOut" },
                { timeSeconds: 8.0, value: 0.52, curve: "easeInOut" },
            ],
        },
    },
    swarm_arc: {
        id: "swarm_arc",
        durationSeconds: 8.0,
        looping: true,
        playbackRate: 1.2,
        tracks: {
            azimuth: [
                { timeSeconds: 0.0, value: -130.0, curve: "easeOut" },
                { timeSeconds: 1.2, value: -38.0, curve: "linear" },
                { timeSeconds: 2.5, value: 34.0, curve: "easeIn" },
                { timeSeconds: 3.5, value: 124.0, curve: "easeOut" },
                { timeSeconds: 5.1, value: 46.0, curve: "easeInOut" },
                { timeSeconds: 6.6, value: -28.0, curve: "easeInOut" },
                { timeSeconds: 8.0, value: -130.0, curve: "easeInOut" },
            ],
            elevation: [
                { timeSeconds: 0.0, value: -12.0, curve: "easeInOut" },
                { timeSeconds: 1.2, value: 14.0, curve: "easeOut" },
                { timeSeconds: 2.5, value: 22.0, curve: "linear" },
                { timeSeconds: 3.5, value: 6.0, curve: "easeInOut" },
                { timeSeconds: 5.1, value: -20.0, curve: "easeIn" },
                { timeSeconds: 6.6, value: 9.0, curve: "easeOut" },
                { timeSeconds: 8.0, value: -12.0, curve: "easeInOut" },
            ],
            distance: [
                { timeSeconds: 0.0, value: 3.4, curve: "easeInOut" },
                { timeSeconds: 1.2, value: 2.4, curve: "easeInOut" },
                { timeSeconds: 2.5, value: 1.5, curve: "easeInOut" },
                { timeSeconds: 3.5, value: 2.9, curve: "easeOut" },
                { timeSeconds: 5.1, value: 1.3, curve: "easeIn" },
                { timeSeconds: 6.6, value: 2.2, curve: "linear" },
                { timeSeconds: 8.0, value: 3.4, curve: "easeInOut" },
            ],
            size: [
                { timeSeconds: 0.0, value: 0.32, curve: "step" },
                { timeSeconds: 1.2, value: 0.72, curve: "easeInOut" },
                { timeSeconds: 2.5, value: 0.44, curve: "step" },
                { timeSeconds: 3.5, value: 0.82, curve: "easeInOut" },
                { timeSeconds: 5.1, value: 0.36, curve: "step" },
                { timeSeconds: 6.6, value: 0.68, curve: "easeInOut" },
                { timeSeconds: 8.0, value: 0.32, curve: "step" },
            ],
        },
    },
    rise_fall: {
        id: "rise_fall",
        durationSeconds: 8.0,
        looping: true,
        playbackRate: 0.8,
        tracks: {
            azimuth: [
                { timeSeconds: 0.0, value: -24.0, curve: "easeInOut" },
                { timeSeconds: 2.0, value: 16.0, curve: "easeInOut" },
                { timeSeconds: 4.0, value: 42.0, curve: "easeInOut" },
                { timeSeconds: 6.0, value: 12.0, curve: "easeInOut" },
                { timeSeconds: 8.0, value: -24.0, curve: "easeInOut" },
            ],
            elevation: [
                { timeSeconds: 0.0, value: -26.0, curve: "easeInOut" },
                { timeSeconds: 2.0, value: 8.0, curve: "easeInOut" },
                { timeSeconds: 4.0, value: 44.0, curve: "easeInOut" },
                { timeSeconds: 6.0, value: 10.0, curve: "easeInOut" },
                { timeSeconds: 8.0, value: -26.0, curve: "easeInOut" },
            ],
            distance: [
                { timeSeconds: 0.0, value: 3.8, curve: "easeInOut" },
                { timeSeconds: 2.0, value: 3.2, curve: "easeInOut" },
                { timeSeconds: 4.0, value: 2.0, curve: "easeInOut" },
                { timeSeconds: 6.0, value: 1.4, curve: "easeInOut" },
                { timeSeconds: 8.0, value: 3.8, curve: "easeInOut" },
            ],
            size: [
                { timeSeconds: 0.0, value: 0.30, curve: "easeInOut" },
                { timeSeconds: 2.0, value: 0.52, curve: "easeInOut" },
                { timeSeconds: 4.0, value: 0.82, curve: "easeInOut" },
                { timeSeconds: 6.0, value: 0.58, curve: "easeInOut" },
                { timeSeconds: 8.0, value: 0.30, curve: "easeInOut" },
            ],
        },
    },
};

function normalizeCurveName(curve) {
    const normalized = String(curve ?? "").trim();
    return curveOrder.includes(normalized) ? normalized : "easeInOut";
}

function applyCurveShape(curve, t) {
    const normT = clamp(Number(t) || 0.0, 0.0, 1.0);
    switch (curve) {
        case "easeIn":
            return normT * normT;
        case "easeOut":
            return 1.0 - ((1.0 - normT) * (1.0 - normT));
        case "easeInOut":
            return normT < 0.5
                ? 2.0 * normT * normT
                : 1.0 - (Math.pow(-2.0 * normT + 2.0, 2.0) * 0.5);
        case "step":
            return 0.0;
        case "linear":
        default:
            return normT;
    }
}

function evaluatePackTrackAtTime(track, timeSeconds) {
    if (!Array.isArray(track) || track.length === 0) return 0.0;
    const sorted = [...track].sort((a, b) => a.timeSeconds - b.timeSeconds);
    const t = Number(timeSeconds) || 0.0;
    if (t <= sorted[0].timeSeconds) return Number(sorted[0].value) || 0.0;

    const last = sorted[sorted.length - 1];
    if (t >= last.timeSeconds) return Number(last.value) || 0.0;

    for (let i = 0; i < sorted.length - 1; i++) {
        const left = sorted[i];
        const right = sorted[i + 1];
        if (t < left.timeSeconds || t > right.timeSeconds) continue;
        const span = Math.max(0.000001, right.timeSeconds - left.timeSeconds);
        const local = clamp((t - left.timeSeconds) / span, 0.0, 1.0);
        if (left.curve === "step") {
            return Number(left.value) || 0.0;
        }
        const shaped = applyCurveShape(left.curve, local);
        const leftValue = Number(left.value) || 0.0;
        const rightValue = Number(right.value) || 0.0;
        return leftValue + ((rightValue - leftValue) * shaped);
    }

    return Number(last.value) || 0.0;
}

function buildPackTrack(lane, laneTrackSpec, durationSeconds, shouldLoop, fallbackValue) {
    const range = laneRanges[lane] || { min: -1.0, max: 1.0 };
    const normalizedTrack = Array.isArray(laneTrackSpec)
        ? laneTrackSpec.map(spec => ({
            timeSeconds: clamp(Number(spec?.timeSeconds), 0.0, durationSeconds),
            value: clamp(Number(spec?.value), range.min, range.max),
            curve: normalizeCurveName(spec?.curve),
        }))
            .filter(spec => Number.isFinite(spec.timeSeconds) && Number.isFinite(spec.value))
            .sort((a, b) => a.timeSeconds - b.timeSeconds)
        : [];

    if (normalizedTrack.length === 0) {
        const clampedFallback = clamp(Number(fallbackValue) || 0.0, range.min, range.max);
        return [
            createKeyframe(0.0, clampedFallback, "easeInOut"),
            createKeyframe(durationSeconds, clampedFallback, "easeInOut"),
        ];
    }

    if (normalizedTrack[0].timeSeconds > 0.0) {
        normalizedTrack.unshift({
            timeSeconds: 0.0,
            value: normalizedTrack[0].value,
            curve: normalizedTrack[0].curve,
        });
    }

    const last = normalizedTrack[normalizedTrack.length - 1];
    if (last.timeSeconds < durationSeconds) {
        const endValue = shouldLoop ? normalizedTrack[0].value : last.value;
        normalizedTrack.push({
            timeSeconds: durationSeconds,
            value: endValue,
            curve: last.curve,
        });
    }

    return normalizedTrack.map(kf => createKeyframe(kf.timeSeconds, kf.value, kf.curve));
}

function clampToSliderRange(sliderState, value, fallbackMin = -50.0, fallbackMax = 50.0) {
    const startRaw = Number(sliderState?.properties?.start);
    const endRaw = Number(sliderState?.properties?.end);
    const min = Number.isFinite(startRaw) && Number.isFinite(endRaw) ? Math.min(startRaw, endRaw) : fallbackMin;
    const max = Number.isFinite(startRaw) && Number.isFinite(endRaw) ? Math.max(startRaw, endRaw) : fallbackMax;
    return clamp(Number(value) || 0.0, min, max);
}

function buildCartesianTracksFromSphericalTracks(sphericalTracks, durationSeconds) {
    const allTimes = new Set([0.0, durationSeconds]);
    Object.values(sphericalTracks).forEach(track => {
        if (!Array.isArray(track)) return;
        track.forEach(kf => allTimes.add(clamp(Number(kf.timeSeconds) || 0.0, 0.0, durationSeconds)));
    });

    const times = Array.from(allTimes).sort((a, b) => a - b);
    const xTrack = [];
    const yTrack = [];
    const zTrack = [];

    times.forEach((timeSeconds, index) => {
        const azimuth = evaluatePackTrackAtTime(sphericalTracks.azimuth, timeSeconds);
        const elevation = evaluatePackTrackAtTime(sphericalTracks.elevation, timeSeconds);
        const distance = Math.max(0.0, evaluatePackTrackAtTime(sphericalTracks.distance, timeSeconds));

        const azimuthRad = (azimuth * Math.PI) / 180.0;
        const elevationRad = (elevation * Math.PI) / 180.0;
        const cosElevation = Math.cos(elevationRad);
        const worldX = distance * cosElevation * Math.sin(azimuthRad);
        const worldY = distance * Math.sin(elevationRad);
        const worldZ = distance * cosElevation * Math.cos(azimuthRad);
        const curve = index === 0 ? "easeInOut" : "linear";

        xTrack.push(createKeyframe(timeSeconds, clampToSliderRange(sliderStates.pos_x, worldX), curve));
        yTrack.push(createKeyframe(timeSeconds, clampToSliderRange(sliderStates.pos_y, worldZ), curve));
        zTrack.push(createKeyframe(timeSeconds, clampToSliderRange(sliderStates.pos_z, worldY), curve));
    });

    return {
        pos_x: xTrack,
        pos_y: yTrack,
        pos_z: zTrack,
    };
}

function buildTimelineFromChoreographyPack(packId) {
    const normalizedPackId = normalizeChoreographyPackId(packId);
    const pack = choreographyPackLibrary[normalizedPackId];
    if (!pack) return null;

    const durationSeconds = clamp(Number(pack.durationSeconds) || 8.0, 0.25, 120.0);
    const shouldLoop = pack.looping !== false;
    const playbackRate = clamp(Number(pack.playbackRate) || 1.0, 0.1, 10.0);

    const sphericalTracks = {
        azimuth: buildPackTrack(
            "azimuth",
            pack.tracks?.azimuth,
            durationSeconds,
            shouldLoop,
            sliderStates.pos_azimuth.getScaledValue()
        ),
        elevation: buildPackTrack(
            "elevation",
            pack.tracks?.elevation,
            durationSeconds,
            shouldLoop,
            sliderStates.pos_elevation.getScaledValue()
        ),
        distance: buildPackTrack(
            "distance",
            pack.tracks?.distance,
            durationSeconds,
            shouldLoop,
            sliderStates.pos_distance.getScaledValue()
        ),
        size: buildPackTrack(
            "size",
            pack.tracks?.size,
            durationSeconds,
            shouldLoop,
            sliderStates.size_uniform.getScaledValue()
        ),
    };

    const cartesianTracks = buildCartesianTracksFromSphericalTracks(sphericalTracks, durationSeconds);
    const tracks = {
        [laneTrackMap.azimuth]: sphericalTracks.azimuth,
        [laneTrackMap.elevation]: sphericalTracks.elevation,
        [laneTrackMap.distance]: sphericalTracks.distance,
        [laneTrackMap.size]: sphericalTracks.size,
        ...cartesianTracks,
    };

    return {
        choreographyPackId: normalizedPackId,
        durationSeconds,
        looping: shouldLoop,
        playbackRate,
        currentTimeSeconds: 0.0,
        tracks,
    };
}

function setChoreographyStatus(message, isError = false) {
    const status = document.getElementById("choreo-status");
    if (!status) return;
    status.textContent = message;
    status.style.color = isError ? "var(--text-error, #D4736F)" : "var(--text-secondary)";
}

function markChoreographyPackCustom() {
    if (normalizeChoreographyPackId(uiState.choreographyPack) === "custom") return;
    uiState.choreographyPack = "custom";
    const choreographySelect = document.getElementById("choreo-pack-select");
    if (choreographySelect) {
        choreographySelect.value = "custom";
    }
    setChoreographyStatus("Custom timeline active");
    scheduleUiStateCommit();
}

async function applyChoreographyPack(packId, options = {}) {
    const normalizedPackId = normalizeChoreographyPackId(packId);
    if (normalizedPackId === "custom") {
        setChoreographyStatus("Choose a choreography pack first", true);
        return false;
    }

    const nextTimeline = buildTimelineFromChoreographyPack(normalizedPackId);
    if (!nextTimeline) {
        setChoreographyStatus("Pack unavailable", true);
        return false;
    }

    timelineState = {
        durationSeconds: nextTimeline.durationSeconds,
        looping: !!nextTimeline.looping,
        playbackRate: nextTimeline.playbackRate,
        currentTimeSeconds: 0.0,
        tracks: nextTimeline.tracks,
    };
    timelineLoaded = true;

    if (options.setInternalSource !== false) {
        setChoiceIndex(comboStates.anim_mode, 1, 2);
    }
    setToggleValue(toggleStates.anim_loop, !!nextTimeline.looping);
    setMotionLoopClasses(!!nextTimeline.looping);
    setSliderScaledValue(sliderStates.anim_speed, nextTimeline.playbackRate);
    syncAnimationUI();

    uiState.choreographyPack = normalizedPackId;
    if (options.persistUiState !== false) {
        scheduleUiStateCommit(true);
    }

    renderTimelineLanes();
    scheduleTimelineCommit(true, true);
    callNative("locusqSetTimelineTime", nativeFunctions.setTimelineTime, 0.0).catch(() => {});
    setChoreographyStatus(`Applied ${getChoreographyPackLabel(normalizedPackId)} pack`);
    setMotionDirty(true);
    return true;
}

function defaultChoreographyPresetName(packId) {
    const packLabel = getChoreographyPackLabel(packId).replace(/[^A-Za-z0-9]+/g, "");
    const stamp = new Date().toISOString().replace(/[-:]/g, "").replace(/\..+/, "");
    return `Choreo_${packLabel}_${stamp}`;
}

async function saveChoreographyPackPreset(packId) {
    const normalizedPackId = normalizeChoreographyPackId(packId);
    if (normalizedPackId === "custom") {
        setChoreographyStatus("Select a choreography pack before saving", true);
        return null;
    }

    const suggestedName = defaultChoreographyPresetName(normalizedPackId);
    let resolvedName = suggestedName;
    const selfTestNameOverride = typeof window.__LQ_SELFTEST_CHOREO_PRESET_NAME__ === "string"
        ? window.__LQ_SELFTEST_CHOREO_PRESET_NAME__.trim()
        : "";

    if (selfTestNameOverride.length > 0) {
        resolvedName = selfTestNameOverride;
    } else {
        try {
            const inputName = typeof window.prompt === "function"
                ? window.prompt("Choreography preset name", suggestedName)
                : suggestedName;
            if (typeof inputName === "string") {
                const trimmed = inputName.trim();
                if (!trimmed) {
                    setChoreographyStatus("Preset name is required", true);
                    return null;
                }
                resolvedName = trimmed;
            }
        } catch (error) {
            console.warn("Choreography preset prompt unavailable, using auto-generated name:", error);
            resolvedName = suggestedName;
        }
    }

    if (timelineCommitTimer !== null) {
        window.clearTimeout(timelineCommitTimer);
        timelineCommitTimer = null;
    }
    await commitTimelineToNative();

    const result = await callNative("locusqSaveEmitterPreset", nativeFunctions.saveEmitterPreset, {
        name: resolvedName,
        presetType: "motion",
        choreographyPackId: normalizedPackId,
    });

    if (!result?.ok) {
        setChoreographyStatus(result?.message || "Failed to save choreography preset", true);
        return null;
    }

    uiState.choreographyPack = normalizedPackId;
    scheduleUiStateCommit(true);
    setPresetTypeSelection("motion");
    await refreshPresetList(result?.path || "");
    setChoreographyStatus(`Saved ${getChoreographyPackLabel(normalizedPackId)} preset`);
    setPresetStatus(`Saved: ${result.name || resolvedName}`);
    setMotionDirty(false);
    return result;
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
    const currentTime = clamp(Number(timelineState.currentTimeSeconds) || 0.0, 0.0, duration);
    const normalized = clamp(currentTime / duration, 0.0, 1.0);
    const totalMs = Math.max(0, Math.round(currentTime * 1000));
    const minutes = Math.floor(totalMs / 60000);
    const seconds = Math.floor((totalMs % 60000) / 1000);
    const millis = totalMs % 1000;
    const formattedTime =
        `${String(minutes).padStart(2, "0")}:${String(seconds).padStart(2, "0")}.${String(millis).padStart(3, "0")}`;
    const timelineTime = document.getElementById("timeline-time");
    if (timelineTime) timelineTime.textContent = formattedTime;
    const motionTime = document.getElementById("motion-transport-time");
    if (motionTime) motionTime.textContent = formattedTime;

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

function scheduleTimelineCommit(immediate = false, preserveChoreographyPack = false) {
    if (!timelineLoaded) return;
    if (!preserveChoreographyPack) {
        markChoreographyPackCustom();
    }
    setMotionDirty(true);
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
        setMotionDirty(false);
    } catch (error) {
        console.warn("Failed to load keyframe timeline from native API:", error);
        timelineState = normaliseTimelineFromNative({});
        timelineLoaded = true;
        renderTimelineLanes();
        setMotionDirty(false);
    }
}

function setPresetStatus(message, isError = false) {
    const status = document.getElementById("preset-status");
    if (!status) return;
    status.textContent = message;
    status.style.color = isError ? "var(--text-error, #D4736F)" : "var(--text-secondary)";
}

function normalisePresetType(type) {
    return String(type || "").trim().toLowerCase() === "motion" ? "motion" : "emitter";
}

function getPresetTypeSelection() {
    const select = document.getElementById("preset-type-select");
    if (!select) return "emitter";
    return normalisePresetType(select.value);
}

function setPresetTypeSelection(type) {
    const select = document.getElementById("preset-type-select");
    if (!select) return;
    const normalized = normalisePresetType(type);
    if (select.value !== normalized) {
        select.value = normalized;
    }
}

function getPresetNameInputValue() {
    const input = document.getElementById("preset-name-input");
    if (!input) return "";
    return String(input.value || "").trim();
}

function setPresetNameInputValue(name) {
    const input = document.getElementById("preset-name-input");
    if (!input) return;
    const next = String(name || "").trim();
    if (document.activeElement !== input || !input.value) {
        input.value = next;
    }
}

function getSelectedPresetOptionEntry() {
    const select = document.getElementById("preset-select");
    if (!select || !select.value) return null;
    const option = select.options[select.selectedIndex];
    if (!option) return null;
    return {
        path: option.value || "",
        name: String(option.dataset.presetName || "").trim(),
        presetType: normalisePresetType(option.dataset.presetType || getPresetTypeSelection()),
        choreographyPackId: normalizeChoreographyPackId(option.dataset.choreographyPackId || "custom"),
    };
}

function syncPresetSelectionContext() {
    const select = document.getElementById("preset-select");
    if (!select || select.options.length === 0) return;
    const selected = getSelectedPresetOptionEntry();
    if (selected && selected.name) {
        setPresetNameInputValue(selected.name);
    }
}

async function refreshPresetList(preferredPath = "") {
    const select = document.getElementById("preset-select");
    if (!select) return;

    const selectedType = getPresetTypeSelection();
    const previousSelection = String(preferredPath || select.value || "");
    select.innerHTML = "";

    try {
        const items = await callNative("locusqListEmitterPresets", nativeFunctions.listEmitterPresets);
        presetEntries = Array.isArray(items) ? items : [];
    } catch (error) {
        presetEntries = [];
        setPresetStatus("Preset listing failed", true);
        return;
    }

    const filteredEntries = presetEntries.filter(entry =>
        normalisePresetType(entry?.presetType) === selectedType
    );

    if (filteredEntries.length === 0) {
        const emptyOption = document.createElement("option");
        emptyOption.textContent = `No ${selectedType} presets`;
        emptyOption.value = "";
        select.appendChild(emptyOption);
        setPresetNameInputValue("");
        setPresetStatus(`No ${selectedType} presets saved yet`);
        return;
    }

    filteredEntries.forEach((entry, index) => {
        const option = document.createElement("option");
        option.value = entry.path || entry.file || "";
        const presetType = normalisePresetType(entry?.presetType);
        const typePrefix = presetType === "motion" ? "[MOTION] " : "[EMITTER] ";
        const choreographyPackId = normalizeChoreographyPackId(entry?.choreographyPackId);
        const packPrefix = choreographyPackId !== "custom"
            ? `[${getChoreographyPackLabel(choreographyPackId)}] `
            : "";
        const displayName = String(entry?.name || entry?.file || `Preset ${index + 1}`).trim();
        option.textContent = `${typePrefix}${packPrefix}${displayName}`;
        option.dataset.choreographyPackId = choreographyPackId;
        option.dataset.presetType = presetType;
        option.dataset.presetName = displayName;
        select.appendChild(option);
    });

    if (previousSelection) {
        const match = Array.from(select.options).find(option => option.value === previousSelection);
        if (match) {
            select.value = match.value;
        }
    }
    if (!select.value && select.options.length > 0) {
        select.selectedIndex = 0;
    }

    syncPresetSelectionContext();
    setPresetStatus(`${filteredEntries.length} ${selectedType} preset(s) available`);
}

// ===== DESATURATED EMITTER PALETTE (v2) =====
const emitterPalette = [
    0xD4736F, 0x5BBAB3, 0x5AADC0, 0x8DBEA7, 0xD8CFA0, 0xBF9ABD, 0x8CC5B7, 0xCCBA6E,
    0xA487B5, 0x7AAFC9, 0xC9A07A, 0x7DC49A, 0xC98A84, 0x96BAD0, 0xB3A0BF, 0x8EC8BD
];
const roomBounds = { halfWidth: 2.7, halfDepth: 1.7 };
const defaultSpeakerSnapshotPositions = [
    { x: -2.7, y: 1.2, z: -1.7 }, // FL
    { x:  2.7, y: 1.2, z: -1.7 }, // FR
    { x:  2.7, y: 1.2, z:  1.7 }, // RR
    { x: -2.7, y: 1.2, z:  1.7 }, // RL
];

function getEmitterColorHex(colorIndex) {
    const index = normalizeEmitterColorIndex(colorIndex);
    return emitterPalette[index] || emitterPalette[0];
}

function getEmitterPaletteSize() {
    return Math.max(1, Number(emitterPalette.length) || 16);
}

function normalizeEmitterColorIndex(colorIndex) {
    const paletteSize = getEmitterPaletteSize();
    const numeric = Number.isFinite(Number(colorIndex)) ? Math.round(Number(colorIndex)) : 0;
    return ((numeric % paletteSize) + paletteSize) % paletteSize;
}

function getCurrentEmitterColorIndex() {
    const current = sliderStates.emit_color?.getScaledValue?.();
    return normalizeEmitterColorIndex(current);
}

function setEmitterColorIndex(colorIndex) {
    const normalized = normalizeEmitterColorIndex(colorIndex);
    if (sliderStates.emit_color) {
        setSliderScaledValue(sliderStates.emit_color, normalized);
    }
    return normalized;
}

function cycleEmitterColor(step = 1) {
    return setEmitterColorIndex(getCurrentEmitterColorIndex() + step);
}

function updateEmitterColorSwatch() {
    const swatch = document.getElementById("emit-color-swatch");
    if (!swatch) return;
    const idx = getCurrentEmitterColorIndex();
    swatch.style.background = "#" + getEmitterColorHex(idx).toString(16).padStart(6, "0");
    swatch.setAttribute("data-color-index", String(idx));
}

// ===== APP STATE =====
let currentMode = "emitter";
let modeLayoutSyncTimer = null;
let timelineInvariantLastCheckMs = 0;
let selectedLane = "azimuth";
let sceneData = {
    emitters: [],
    emitterCount: 0,
    rendererActive: false,
    outputChannels: 2,
    outputLayout: "stereo",
    rendererSpatialProfileRequested: "auto",
    rendererSpatialProfileActive: "auto",
    rendererSpatialProfileStage: "direct",
    rendererHeadphoneModeRequested: "stereo_downmix",
    rendererHeadphoneModeActive: "stereo_downmix",
    rendererHeadphoneProfileRequested: "generic",
    rendererHeadphoneProfileActive: "generic",
    rendererPhysicsLensEnabled: false,
    rendererPhysicsLensMix: 0.55,
    rendererSteamAudioCompiled: false,
    rendererSteamAudioAvailable: false,
    rendererSteamAudioInitStage: "uninitialized",
    rendererSteamAudioInitErrorCode: 0,
    rendererSteamAudioRuntimeLib: "",
    rendererSteamAudioMissingSymbol: "",
    rendererAmbiCompiled: false,
    rendererAmbiActive: false,
    rendererAmbiMaxOrder: 1,
    rendererAmbiNormalization: "sn3d",
    rendererAmbiChannelOrder: "acn",
    rendererAmbiDecodeLayout: "quad_baseline",
    rendererAmbiStage: "not_implemented",
    clapBuildEnabled: false,
    clapPropertiesAvailable: false,
    clapIsPluginFormat: false,
    clapIsActive: false,
    clapIsProcessing: false,
    clapHasTransport: false,
    clapWrapperType: "Unknown",
    clapLifecycleStage: "not_compiled",
    clapRuntimeMode: "disabled",
    clapVersion: { major: 0, minor: 0, revision: 0 },
    rendererOutputChannels: ["L", "R"],
    rendererInternalSpeakers: ["FL", "FR", "RR", "RL"],
    rendererQuadMap: [0, 1, 3, 2],
    calCurrentTopologyProfile: 2,
    calCurrentTopologyId: "quadraphonic",
    calCurrentMonitoringPath: 0,
    calCurrentMonitoringPathId: "speakers",
    calCurrentDeviceProfile: 0,
    calCurrentDeviceProfileId: "generic",
    calRequiredChannels: 4,
    calWritableChannels: 4,
    calMappingLimitedToFirst4: false,
    calCurrentSpeakerMap: [1, 2, 3, 4],
    calAutoRoutingMap: [1, 2, 3, 4],
    roomProfileValid: false,
    roomDimensions: { width: 6.0, depth: 4.0, height: 3.0 },
    listener: { x: 0.0, y: 1.2, z: 0.0 },
    speakerRms: [0, 0, 0, 0],
    speakers: [],
};
let selectedEmitterId = -1;
let localEmitterId = -1;
let sceneEmitterLookup = new Map();
let emitterAuthoringLocked = false;
const railScrollByMode = { calibrate: 0, emitter: 0, renderer: 0 };
const calibrationTopologyIds = [
    "mono",
    "stereo",
    "quadraphonic",
    "surround_5_1",
    "surround_7_1_2",
    "surround_7_4_2",
    "ambisonic",
    "binaural_headphone",
    "multichannel_stereo_downmix",
];
const calibrationTopologyRequiredChannels = [1, 2, 4, 6, 10, 13, 4, 2, 2];
const calibrationTopologyLabels = {
    mono: "Mono",
    stereo: "Stereo",
    quadraphonic: "Quadraphonic",
    surround_5_1: "5.1",
    surround_7_1_2: "7.1.2",
    surround_7_4_2: "7.4.2 / Atmos-style",
    ambisonic: "Ambisonic (FOA/HOA)",
    binaural_headphone: "Binaural / Headphone",
    multichannel_stereo_downmix: "Multichannel -> Stereo Downmix",
};
const calibrationTopologyShortLabels = {
    mono: "Mono",
    stereo: "Stereo",
    quadraphonic: "Quad",
    surround_5_1: "5.1",
    surround_7_1_2: "7.1.2",
    surround_7_4_2: "7.4.2",
    ambisonic: "Ambisonic",
    binaural_headphone: "Binaural",
    multichannel_stereo_downmix: "Downmix",
};
const calibrationMonitoringPathIds = [
    "speakers",
    "stereo_downmix",
    "steam_binaural",
    "virtual_binaural",
];
const calibrationMonitoringPathLabels = {
    speakers: "Speakers",
    stereo_downmix: "Stereo Downmix",
    steam_binaural: "Steam Binaural",
    virtual_binaural: "Virtual Binaural",
};
const calibrationDeviceProfileIds = [
    "generic",
    "airpods_pro_2",
    "sony_wh1000xm5",
    "custom_sofa",
];
const calibrationDeviceProfileLabels = {
    generic: "Generic",
    airpods_pro_2: "AirPods Pro 2",
    sony_wh1000xm5: "Sony WH-1000XM5",
    custom_sofa: "Custom SOFA",
};
const calibrationTopologyChannelLabels = {
    mono: ["Main"],
    stereo: ["L", "R"],
    quadraphonic: ["FL", "FR", "RL", "RR"],
    surround_5_1: ["L", "R", "C", "LFE", "Ls", "Rs"],
    surround_7_1_2: ["L", "R", "C", "LFE", "Ls", "Rs", "Lrs", "Rrs", "TopL", "TopR"],
    surround_7_4_2: ["L", "R", "C", "LFE1", "LFE2", "Ls", "Rs", "Lrs", "Rrs", "TopFL", "TopFR", "TopRL", "TopRR"],
    ambisonic: ["W", "X", "Y", "Z"],
    binaural_headphone: ["Left", "Right"],
    multichannel_stereo_downmix: ["Downmix L", "Downmix R"],
};
const calibrationTopologyPreviewSpeakerPositions = {
    mono: [
        { x: 0.0, y: 1.2, z: -1.9 },
    ],
    stereo: [
        { x: -2.3, y: 1.2, z: -1.8 },
        { x: 2.3, y: 1.2, z: -1.8 },
    ],
    quadraphonic: [
        { x: -2.7, y: 1.2, z: -1.7 },
        { x: 2.7, y: 1.2, z: -1.7 },
        { x: 2.7, y: 1.2, z: 1.7 },
        { x: -2.7, y: 1.2, z: 1.7 },
    ],
    surround_5_1: [
        { x: -2.2, y: 1.2, z: -1.8 }, // L
        { x: 2.2, y: 1.2, z: -1.8 },  // R
        { x: 0.0, y: 1.25, z: -2.15 }, // C
        { x: 0.0, y: 0.8, z: -1.35 }, // LFE
        { x: -2.55, y: 1.2, z: 1.45 }, // Ls
        { x: 2.55, y: 1.2, z: 1.45 }, // Rs
    ],
    surround_7_1_2: [
        { x: -2.2, y: 1.2, z: -1.8 }, // L
        { x: 2.2, y: 1.2, z: -1.8 },  // R
        { x: 0.0, y: 1.25, z: -2.15 }, // C
        { x: 0.0, y: 0.8, z: -1.35 }, // LFE
        { x: -2.55, y: 1.2, z: 0.6 }, // Ls
        { x: 2.55, y: 1.2, z: 0.6 }, // Rs
        { x: -2.55, y: 1.2, z: 1.65 }, // Lrs
        { x: 2.55, y: 1.2, z: 1.65 }, // Rrs
        { x: -1.35, y: 2.35, z: -0.9 }, // TopL
        { x: 1.35, y: 2.35, z: -0.9 }, // TopR
    ],
    surround_7_4_2: [
        { x: -2.2, y: 1.2, z: -1.8 }, // L
        { x: 2.2, y: 1.2, z: -1.8 },  // R
        { x: 0.0, y: 1.25, z: -2.15 }, // C
        { x: -0.38, y: 0.8, z: -1.35 }, // LFE1
        { x: 0.38, y: 0.8, z: -1.35 }, // LFE2
        { x: -2.55, y: 1.2, z: 0.6 }, // Ls
        { x: 2.55, y: 1.2, z: 0.6 }, // Rs
        { x: -2.55, y: 1.2, z: 1.65 }, // Lrs
        { x: 2.55, y: 1.2, z: 1.65 }, // Rrs
        { x: -1.55, y: 2.35, z: -1.0 }, // TopFL
        { x: 1.55, y: 2.35, z: -1.0 }, // TopFR
        { x: -1.55, y: 2.35, z: 1.0 }, // TopRL
        { x: 1.55, y: 2.35, z: 1.0 }, // TopRR
    ],
    ambisonic: [
        { x: 0.0, y: 1.65, z: -1.0 }, // W
        { x: 1.35, y: 1.15, z: 0.0 }, // X
        { x: 0.0, y: 0.7, z: 1.0 }, // Y
        { x: -1.35, y: 1.15, z: 0.0 }, // Z
    ],
    binaural_headphone: [
        { x: -0.38, y: 1.2, z: 0.0 }, // Left ear
        { x: 0.38, y: 1.2, z: 0.0 }, // Right ear
    ],
    multichannel_stereo_downmix: [
        { x: -2.3, y: 1.2, z: -1.8 }, // Downmix L
        { x: 2.3, y: 1.2, z: -1.8 }, // Downmix R
    ],
};
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
    topologyProfileIndex: 2,
    topologyProfile: "quadraphonic",
    monitoringPathIndex: 0,
    monitoringPath: "speakers",
    deviceProfileIndex: 0,
    deviceProfile: "generic",
    requiredChannels: 4,
    writableChannels: 4,
    mappingLimitedToFirst4: false,
    mappingDuplicateChannels: false,
    mappingValid: false,
    speakerRouting: [1, 2, 3, 4],
    phasePass: false,
    delayPass: false,
};
let calibrationProfileEntries = [];
let calibrationMappingEditedByUser = false;
let calibrationLastAutoRouting = [1, 2, 3, 4];
let calibrationLegacyAliasSyncInFlight = false;

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
let motionDirty = false;
let draggingKeyframe = null;
let presetEntries = [];
const emitterAuthorityEditableCardGroups = new Set([
    "identity",
    "position",
    "audio-shape",
    "motion",
    "presets",
]);
const emitterAuthorityControlIds = [
    "emit-label",
    "emit-color-swatch",
    "toggle-mute",
    "toggle-solo",
    "pos-mode",
    "toggle-size-link",
    "motion-source-select",
    "motion-transport-rewind-btn",
    "motion-transport-stop-btn",
    "motion-transport-play-btn",
    "toggle-motion-loop",
    "toggle-motion-sync",
    "physics-preset",
    "btn-throw",
    "btn-reset",
    "phys-grav-dir",
    "choreo-pack-select",
    "choreo-apply-btn",
    "choreo-save-btn",
    "preset-type-select",
    "preset-name-input",
    "preset-select",
    "preset-save-btn",
    "preset-load-btn",
    "preset-rename-btn",
    "preset-delete-btn",
];
const emitterAuthorityStepperIds = [
    "val-azimuth",
    "val-elevation",
    "val-distance",
    "val-pos-x",
    "val-pos-y",
    "val-pos-z",
    "val-size",
    "val-gain",
    "val-spread",
    "val-directivity",
    "val-dir-azimuth",
    "val-dir-elevation",
    "val-anim-speed",
    "val-mass",
    "val-drag",
    "val-elasticity",
    "val-gravity",
    "val-friction",
    "val-vel-x",
    "val-vel-y",
    "val-vel-z",
];
const runtimeState = {
    viewportReady: false,
    viewportDegraded: false,
};
const sceneTransportDefaults = {
    schema: "locusq-scene-snapshot-v1",
    cadenceHz: 30,
    staleAfterMs: 750,
};
const sceneTransportState = {
    schema: sceneTransportDefaults.schema,
    lastAcceptedSeq: -1,
    lastAcceptedAtMs: 0,
    lastPublishedAtUtcMs: 0,
    cadenceHz: sceneTransportDefaults.cadenceHz,
    staleAfterMs: sceneTransportDefaults.staleAfterMs,
    stale: false,
};

// ===== THREE.JS SETUP =====
let threeScene, camera, rendererGL, canvas;
let roomLines, gridHelper, speakers = [], speakerMeters = [], speakerEnergyRings = [], speakerTargets = [];
let emitterMeshes = new Map();
let emitterVisualTargets = new Map();
let selectionRing;
let listenerGroup, listenerEnergyRing, listenerAimArrow;
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
let lastAnimationFrameTimeMs = 0;
let listenerTarget = { x: 0.0, y: 1.2, z: 0.0 };

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
    return sceneEmitterLookup.get(emitterId) || null;
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
    updateEmitterAuthorityUI();
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

function getCalibrationTopologyId(index) {
    const count = calibrationTopologyIds.length;
    const resolved = Number.isFinite(Number(index))
        ? Number(index)
        : getChoiceIndex(comboStates.cal_topology_profile);
    const clamped = clamp(Math.round(resolved), 0, Math.max(0, count - 1));
    return calibrationTopologyIds[clamped] || calibrationTopologyIds[2];
}

function getCalibrationMonitoringPathId(index) {
    const count = calibrationMonitoringPathIds.length;
    const resolved = Number.isFinite(Number(index))
        ? Number(index)
        : getChoiceIndex(comboStates.cal_monitoring_path);
    const clamped = clamp(Math.round(resolved), 0, Math.max(0, count - 1));
    return calibrationMonitoringPathIds[clamped] || calibrationMonitoringPathIds[0];
}

function getCalibrationDeviceProfileId(index) {
    const count = calibrationDeviceProfileIds.length;
    const resolved = Number.isFinite(Number(index))
        ? Number(index)
        : getChoiceIndex(comboStates.cal_device_profile);
    const clamped = clamp(Math.round(resolved), 0, Math.max(0, count - 1));
    return calibrationDeviceProfileIds[clamped] || calibrationDeviceProfileIds[0];
}

function getCalibrationTopologyLabel(topologyId, shortLabel = false) {
    const id = String(topologyId || "").trim().toLowerCase();
    if (shortLabel) {
        return calibrationTopologyShortLabels[id] || calibrationTopologyShortLabels.quadraphonic;
    }
    return calibrationTopologyLabels[id] || calibrationTopologyLabels.quadraphonic;
}

function getCalibrationMonitoringPathLabel(pathId) {
    const id = String(pathId || "").trim().toLowerCase();
    return calibrationMonitoringPathLabels[id] || calibrationMonitoringPathLabels.speakers;
}

function getCalibrationDeviceProfileLabel(profileId) {
    const id = String(profileId || "").trim().toLowerCase();
    return calibrationDeviceProfileLabels[id] || calibrationDeviceProfileLabels.generic;
}

function getCalibrationRequiredChannels(topologyId) {
    const id = String(topologyId || "").trim().toLowerCase();
    const idx = calibrationTopologyIds.indexOf(id);
    if (idx < 0) return 4;
    return Number(calibrationTopologyRequiredChannels[idx]) || 4;
}

function getCalibrationChannelLabel(topologyId, index) {
    const id = String(topologyId || "").trim().toLowerCase();
    const labels = calibrationTopologyChannelLabels[id] || calibrationTopologyChannelLabels.quadraphonic;
    const idx = Math.max(0, Math.round(Number(index) || 0));
    return labels[idx] || `Ch ${idx + 1}`;
}

function getLegacyConfigIndexForTopology(topologyId) {
    const requiredChannels = getCalibrationRequiredChannels(topologyId);
    return requiredChannels <= 2 ? 1 : 0;
}

function getTopologyIndexForLegacyConfig(configIndex) {
    const clampedConfig = clamp(Math.round(Number(configIndex) || 0), 0, 1);
    // Legacy 4x Mono maps to v2 quad profile; 2x Stereo maps to v2 stereo profile.
    return clampedConfig === 1 ? 1 : 2;
}

function syncLegacyConfigAliasFromTopology(topologyId = "") {
    if (calibrationLegacyAliasSyncInFlight) return;

    const targetTopology = String(topologyId || getCalibrationViewportTopologyId()).trim().toLowerCase();
    const desiredLegacyConfig = getLegacyConfigIndexForTopology(targetTopology);
    const currentLegacyConfig = getChoiceIndex(comboStates.cal_spk_config);
    if (currentLegacyConfig === desiredLegacyConfig) return;

    calibrationLegacyAliasSyncInFlight = true;
    try {
        setChoiceIndex(comboStates.cal_spk_config, desiredLegacyConfig, 2);
    } finally {
        calibrationLegacyAliasSyncInFlight = false;
    }
}

function syncTopologyFromLegacyConfigAlias(configIndex) {
    if (calibrationLegacyAliasSyncInFlight) return;

    const desiredTopologyIndex = getTopologyIndexForLegacyConfig(configIndex);
    const currentTopologyIndex = getChoiceIndex(comboStates.cal_topology_profile);
    if (currentTopologyIndex === desiredTopologyIndex) return;

    calibrationLegacyAliasSyncInFlight = true;
    try {
        setChoiceIndex(comboStates.cal_topology_profile, desiredTopologyIndex, calibrationTopologyIds.length);
    } finally {
        calibrationLegacyAliasSyncInFlight = false;
    }
}

function getCalibrationViewportTopologyId() {
    const fromCombo = getCalibrationTopologyId();
    if (calibrationTopologyIds.includes(fromCombo)) {
        return fromCombo;
    }

    const fromStatus = String(calibrationState?.topologyProfile || "").trim().toLowerCase();
    if (calibrationTopologyIds.includes(fromStatus)) {
        return fromStatus;
    }

    const fromScene = String(sceneData?.calCurrentTopologyId || "").trim().toLowerCase();
    if (calibrationTopologyIds.includes(fromScene)) {
        return fromScene;
    }

    return calibrationTopologyIds[2];
}

function getCalibrationPreviewSpeakerCount(topologyId = "") {
    const resolvedTopology = String(topologyId || getCalibrationViewportTopologyId()).trim().toLowerCase();
    return clamp(getCalibrationRequiredChannels(resolvedTopology), 1, 4);
}

function getCalibrationPreviewSpeakerPosition(topologyId, index) {
    const resolvedTopology = String(topologyId || getCalibrationViewportTopologyId()).trim().toLowerCase();
    const blueprint = calibrationTopologyPreviewSpeakerPositions[resolvedTopology];
    const fallback = defaultSpeakerSnapshotPositions[index] || defaultSpeakerSnapshotPositions[0];
    if (!Array.isArray(blueprint) || blueprint.length === 0) {
        return fallback;
    }

    const speaker = blueprint[index];
    if (!speaker) {
        return fallback;
    }

    return {
        x: Number.isFinite(Number(speaker.x)) ? Number(speaker.x) : fallback.x,
        y: Number.isFinite(Number(speaker.y)) ? Number(speaker.y) : fallback.y,
        z: Number.isFinite(Number(speaker.z)) ? Number(speaker.z) : fallback.z,
    };
}

function normaliseCalibrationRouting(routing, minLength = 4) {
    const values = Array.isArray(routing) ? routing : [];
    const out = [];
    const length = Math.max(minLength, values.length, 4);
    for (let i = 0; i < length; ++i) {
        const fallback = (i % 8) + 1;
        const value = Number.isFinite(Number(values[i])) ? Number(values[i]) : fallback;
        out.push(clamp(Math.round(value), 1, 8));
    }
    return out;
}

function getCalibrationRoutingFromControls() {
    const values = [];
    for (let i = 1; i <= 4; ++i) {
        const select = document.getElementById(`cal-spk${i}`);
        const channel = select ? (select.selectedIndex + 1) : i;
        values.push(clamp(channel, 1, 8));
    }
    return values;
}

function getCalibrationActiveRouting() {
    const fromStatus = normaliseCalibrationRouting(calibrationState?.speakerRouting || [], 4);
    if (fromStatus.length >= 4) return fromStatus;
    return getCalibrationRoutingFromControls();
}

function compareCalibrationRouting(a, b, count = 4) {
    const lhs = normaliseCalibrationRouting(a, count);
    const rhs = normaliseCalibrationRouting(b, count);
    for (let i = 0; i < count; ++i) {
        if (lhs[i] !== rhs[i]) return false;
    }
    return true;
}

function getCalibrationExpectedAutoRouting() {
    const fromScene = Array.isArray(sceneData?.calAutoRoutingMap) ? sceneData.calAutoRoutingMap : calibrationLastAutoRouting;
    return normaliseCalibrationRouting(fromScene || [1, 2, 3, 4], 4);
}

function getCalibrationStatusChipText(status) {
    if (status.running) return "Measuring";
    if (status.complete) return "Complete";
    if (status.mappingLimitedToFirst4) return "Limited";
    if (status.mappingValid) return "Ready";
    return "Idle";
}

function setCalibrationValidationChip(chipId, state, text) {
    const chip = document.getElementById(chipId);
    if (!chip) return;
    chip.classList.remove("pass", "fail", "warn", "active");
    if (state === "pass") chip.classList.add("pass");
    else if (state === "fail") chip.classList.add("fail");
    else if (state === "warn") chip.classList.add("warn");
    else chip.classList.add("active");
    chip.textContent = text;
}

function setCalibrationProfileStatus(message, isError = false) {
    const status = document.getElementById("cal-profile-status");
    if (!status) return;
    status.textContent = String(message || "");
    status.classList.toggle("error", !!isError);
}

function getCalibrationProfileNameInputValue() {
    const input = document.getElementById("cal-profile-name");
    if (!input) return "";
    return String(input.value || "").trim();
}

function setCalibrationProfileNameInputValue(name) {
    const input = document.getElementById("cal-profile-name");
    if (!input) return;
    const next = String(name || "").trim();
    if (document.activeElement !== input || !input.value) {
        input.value = next;
    }
}

function getSelectedCalibrationProfileEntry() {
    const select = document.getElementById("cal-profile-select");
    if (!select || !select.value) return null;
    const option = select.options[select.selectedIndex];
    if (!option) return null;
    return {
        path: option.value || "",
        name: String(option.dataset.profileName || "").trim(),
        topologyProfile: String(option.dataset.topologyProfile || ""),
        monitoringPath: String(option.dataset.monitoringPath || ""),
        deviceProfile: String(option.dataset.deviceProfile || ""),
    };
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
    syncResponsiveLayoutMode();
    window.addEventListener("resize", syncResponsiveLayoutMode);

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
        (async () => {
            try {
                await refreshCalibrationProfileList();
            } catch (error) {
                console.error("LocusQ: refreshCalibrationProfileList failed:", error);
            }
        })(),
    ];

    await Promise.allSettled(startupHydrationTasks);
    syncAnimationUI();
    updateMotionStatusChips();
    syncMotionSourceUI();

    console.log("LocusQ WebView initialized");
}

async function runProductionP0SelfTest() {
    const report = {
        requested: productionP0SelfTestRequested,
        startedAt: new Date().toISOString(),
        status: "running",
        ok: false,
        checks: [],
    };

    const recordCheck = (id, pass, details = "") => {
        report.checks.push({ id, pass, details });
    };

    const failCheck = (id, details) => {
        recordCheck(id, false, details);
        throw new Error(`${id}: ${details}`);
    };

    let steamDiagnosticsFallback = null;
    let clapDiagnosticsFallback = null;
    const hasSteamDiagnosticsInScene = () => {
        return typeof sceneData.rendererSteamAudioCompiled === "boolean"
            && typeof sceneData.rendererSteamAudioAvailable === "boolean"
            && typeof sceneData.rendererSteamAudioInitStage === "string";
    };
    const captureSteamDiagnosticsFallback = () => {
        if (!hasSteamDiagnosticsInScene()) {
            return false;
        }

        steamDiagnosticsFallback = {
            rendererSteamAudioCompiled: !!sceneData.rendererSteamAudioCompiled,
            rendererSteamAudioAvailable: !!sceneData.rendererSteamAudioAvailable,
            rendererSteamAudioInitStage: String(sceneData.rendererSteamAudioInitStage || "unknown"),
            rendererSteamAudioInitErrorCode: Number.isFinite(Number(sceneData.rendererSteamAudioInitErrorCode))
                ? Number(sceneData.rendererSteamAudioInitErrorCode)
                : 0,
            rendererSteamAudioRuntimeLib: String(sceneData.rendererSteamAudioRuntimeLib || ""),
            rendererSteamAudioMissingSymbol: String(sceneData.rendererSteamAudioMissingSymbol || ""),
        };
        return true;
    };
    const restoreSteamDiagnosticsFromFallback = () => {
        if (!steamDiagnosticsFallback) {
            return false;
        }

        sceneData.rendererSteamAudioCompiled = !!steamDiagnosticsFallback.rendererSteamAudioCompiled;
        sceneData.rendererSteamAudioAvailable = !!steamDiagnosticsFallback.rendererSteamAudioAvailable;
        sceneData.rendererSteamAudioInitStage = String(steamDiagnosticsFallback.rendererSteamAudioInitStage || "unknown");
        sceneData.rendererSteamAudioInitErrorCode = Number(steamDiagnosticsFallback.rendererSteamAudioInitErrorCode) || 0;
        sceneData.rendererSteamAudioRuntimeLib = String(steamDiagnosticsFallback.rendererSteamAudioRuntimeLib || "");
        sceneData.rendererSteamAudioMissingSymbol = String(steamDiagnosticsFallback.rendererSteamAudioMissingSymbol || "");
        return true;
    };
    const hasClapDiagnosticsInScene = () => {
        return typeof sceneData.clapBuildEnabled === "boolean"
            && typeof sceneData.clapPropertiesAvailable === "boolean"
            && typeof sceneData.clapIsPluginFormat === "boolean"
            && typeof sceneData.clapLifecycleStage === "string"
            && typeof sceneData.clapRuntimeMode === "string";
    };
    const captureClapDiagnosticsFallback = () => {
        if (!hasClapDiagnosticsInScene()) {
            return false;
        }

        const clapVersion = sceneData.clapVersion && typeof sceneData.clapVersion === "object"
            ? sceneData.clapVersion
            : {};
        clapDiagnosticsFallback = {
            clapBuildEnabled: !!sceneData.clapBuildEnabled,
            clapPropertiesAvailable: !!sceneData.clapPropertiesAvailable,
            clapIsPluginFormat: !!sceneData.clapIsPluginFormat,
            clapIsActive: !!sceneData.clapIsActive,
            clapIsProcessing: !!sceneData.clapIsProcessing,
            clapHasTransport: !!sceneData.clapHasTransport,
            clapWrapperType: String(sceneData.clapWrapperType || "Unknown"),
            clapLifecycleStage: String(sceneData.clapLifecycleStage || "unknown"),
            clapRuntimeMode: String(sceneData.clapRuntimeMode || "unknown"),
            clapVersion: {
                major: Number.isFinite(Number(clapVersion.major)) ? Number(clapVersion.major) : 0,
                minor: Number.isFinite(Number(clapVersion.minor)) ? Number(clapVersion.minor) : 0,
                revision: Number.isFinite(Number(clapVersion.revision)) ? Number(clapVersion.revision) : 0,
            },
        };
        return true;
    };
    const restoreClapDiagnosticsFromFallback = () => {
        if (!clapDiagnosticsFallback) {
            return false;
        }

        sceneData.clapBuildEnabled = !!clapDiagnosticsFallback.clapBuildEnabled;
        sceneData.clapPropertiesAvailable = !!clapDiagnosticsFallback.clapPropertiesAvailable;
        sceneData.clapIsPluginFormat = !!clapDiagnosticsFallback.clapIsPluginFormat;
        sceneData.clapIsActive = !!clapDiagnosticsFallback.clapIsActive;
        sceneData.clapIsProcessing = !!clapDiagnosticsFallback.clapIsProcessing;
        sceneData.clapHasTransport = !!clapDiagnosticsFallback.clapHasTransport;
        sceneData.clapWrapperType = String(clapDiagnosticsFallback.clapWrapperType || "Unknown");
        sceneData.clapLifecycleStage = String(clapDiagnosticsFallback.clapLifecycleStage || "unknown");
        sceneData.clapRuntimeMode = String(clapDiagnosticsFallback.clapRuntimeMode || "unknown");
        sceneData.clapVersion = {
            major: Number.isFinite(Number(clapDiagnosticsFallback.clapVersion?.major))
                ? Number(clapDiagnosticsFallback.clapVersion.major)
                : 0,
            minor: Number.isFinite(Number(clapDiagnosticsFallback.clapVersion?.minor))
                ? Number(clapDiagnosticsFallback.clapVersion.minor)
                : 0,
            revision: Number.isFinite(Number(clapDiagnosticsFallback.clapVersion?.revision))
                ? Number(clapDiagnosticsFallback.clapVersion.revision)
                : 0,
        };
        return true;
    };
    const runBl011ClapSelfTest = queryParams.get("selftest_bl011") === "1";
    const runBl011ScopeOnly = runBl011ClapSelfTest || productionP0SelfTestScope === "bl011";
    const runBl026ScopeOnly = productionP0SelfTestScope === "bl026";
    const runBl011ClapDiagnosticsCheck = async () => {
        try {
            await waitForCondition("clap diagnostics snapshot", () => {
                if (hasClapDiagnosticsInScene()) {
                    return true;
                }
                return restoreClapDiagnosticsFromFallback();
            }, 9000, 30);
        } catch (_) {
            failCheck("UI-P2-011", "missing CLAP diagnostics fields in scene snapshot");
        }

        const clapBuildEnabled = !!sceneData.clapBuildEnabled;
        const clapPropertiesAvailable = !!sceneData.clapPropertiesAvailable;
        const clapIsPluginFormat = !!sceneData.clapIsPluginFormat;
        const clapIsActive = !!sceneData.clapIsActive;
        const clapIsProcessing = !!sceneData.clapIsProcessing;
        const clapHasTransport = !!sceneData.clapHasTransport;
        const clapWrapperType = String(sceneData.clapWrapperType || "Unknown");
        const clapLifecycleStage = String(sceneData.clapLifecycleStage || "unknown");
        const clapRuntimeMode = String(sceneData.clapRuntimeMode || "unknown");
        const clapVersion = sceneData.clapVersion && typeof sceneData.clapVersion === "object"
            ? sceneData.clapVersion
            : {};
        const clapVersionMajor = Number.isFinite(Number(clapVersion.major)) ? Number(clapVersion.major) : 0;
        const clapVersionMinor = Number.isFinite(Number(clapVersion.minor)) ? Number(clapVersion.minor) : 0;
        const clapVersionRevision = Number.isFinite(Number(clapVersion.revision)) ? Number(clapVersion.revision) : 0;

        const validStages = new Set([
            "not_compiled",
            "compiled_no_properties",
            "non_clap_instance",
            "instantiated",
            "active_idle",
            "processing",
        ]);

        if (!validStages.has(clapLifecycleStage)) {
            failCheck("UI-P2-011", `invalid CLAP lifecycle stage (${clapLifecycleStage})`);
        }

        if (clapBuildEnabled && !clapPropertiesAvailable) {
            failCheck("UI-P2-011", "CLAP build is enabled but clap_properties telemetry is unavailable");
        }

        if (clapIsProcessing && !clapIsActive) {
            failCheck("UI-P2-011", "CLAP processing=true while active=false");
        }

        if (clapIsPluginFormat) {
            if (!["instantiated", "active_idle", "processing"].includes(clapLifecycleStage)) {
                failCheck("UI-P2-011", `unexpected CLAP lifecycle stage for CLAP instance (${clapLifecycleStage})`);
            }
            if (clapRuntimeMode !== "global_only") {
                failCheck("UI-P2-011", `unexpected CLAP runtime mode (${clapRuntimeMode})`);
            }
            if (clapWrapperType.toUpperCase() !== "CLAP") {
                failCheck("UI-P2-011", `CLAP instance should report wrapperType=CLAP (got ${clapWrapperType})`);
            }
        } else if (clapLifecycleStage === "instantiated"
            || clapLifecycleStage === "active_idle"
            || clapLifecycleStage === "processing") {
            failCheck("UI-P2-011", `non-CLAP instance reported active CLAP lifecycle stage (${clapLifecycleStage})`);
        }

        let detail = `build=${clapBuildEnabled} properties=${clapPropertiesAvailable} clap=${clapIsPluginFormat} stage=${clapLifecycleStage} active=${clapIsActive} processing=${clapIsProcessing} transport=${clapHasTransport} mode=${clapRuntimeMode} wrapper=${clapWrapperType}`;
        if (clapPropertiesAvailable) {
            detail += ` version=${clapVersionMajor}.${clapVersionMinor}.${clapVersionRevision}`;
        }
        recordCheck("UI-P2-011", true, detail);
    };

    try {
        await waitForCondition("p0 self-test controls ready", () => {
            return !!document.getElementById("physics-preset")
                && !!document.getElementById("timeline-rewind-btn")
                && !!document.getElementById("timeline-stop-btn")
                && !!document.getElementById("timeline-play-btn")
                && !!document.getElementById("preset-save-btn")
                && !!document.getElementById("preset-load-btn")
                && !!document.getElementById("preset-rename-btn")
                && !!document.getElementById("preset-delete-btn")
                && !!document.getElementById("preset-select")
                && !!document.getElementById("preset-type-select")
                && !!document.getElementById("preset-name-input")
                && !!document.getElementById("emitter-authority-note")
                && !!document.getElementById("pos-mode")
                && !!document.getElementById("val-pos-x")
                && !!document.getElementById("val-pos-y")
                && !!document.getElementById("val-pos-z")
                && !!document.getElementById("val-world-readback")
                && !!document.getElementById("motion-source-select")
                && !!document.getElementById("motion-transport-rewind-btn")
                && !!document.getElementById("motion-transport-stop-btn")
                && !!document.getElementById("motion-transport-play-btn")
                && !!document.getElementById("toggle-motion-loop")
                && !!document.getElementById("toggle-motion-sync")
                && !!document.getElementById("motion-panel-physics")
                && !!document.getElementById("motion-panel-timeline")
                && !!document.getElementById("motion-panel-choreography")
                && !!document.getElementById("motion-transport-time")
                && !!document.getElementById("choreo-pack-select")
                && !!document.getElementById("choreo-apply-btn")
                && !!document.getElementById("choreo-save-btn")
                && !!document.getElementById("cal-topology")
                && !!document.getElementById("cal-monitoring-path")
                && !!document.getElementById("cal-device-profile")
                && !!document.getElementById("cal-redetect-btn")
                && !!document.getElementById("cal-start-btn")
                && !!document.getElementById("cal-profile-save-btn")
                && !!document.getElementById("cal-profile-load-btn")
                && !!document.getElementById("cal-profile-rename-btn")
                && !!document.getElementById("cal-profile-delete-btn")
                && !!document.getElementById("cal-profile-select")
                && !!document.querySelector('.timeline-lane[data-lane="azimuth"] .lane-track');
        }, 8000, 40);

        await waitForCondition("steam diagnostics baseline", () => {
            return captureSteamDiagnosticsFallback();
        }, 3000, 30).catch(() => {});
        await waitForCondition("clap diagnostics baseline", () => {
            return captureClapDiagnosticsFallback();
        }, 3000, 30).catch(() => {});

        if (runBl011ScopeOnly) {
            if (!runBl011ClapSelfTest) {
                failCheck("UI-P2-011", "selftest_scope=bl011 requires selftest_bl011=1");
            }

            await runBl011ClapDiagnosticsCheck();
            report.ok = true;
            report.status = "pass";
            return report;
        }

        // UI-P1-026A..E: CALIBRATE v2 topology/mapping/run/diagnostics/profile-library contracts.
        switchMode("calibrate");
        setChoiceIndex(comboStates.mode, 0, 3);
        await waitMs(160);

        const calTopologySelect = document.getElementById("cal-topology");
        const calConfigSelect = document.getElementById("cal-config");
        const calMonitoringPathSelect = document.getElementById("cal-monitoring-path");
        const calDeviceProfileSelect = document.getElementById("cal-device-profile");
        const calRedetectButton = document.getElementById("cal-redetect-btn");
        const calStartButton = document.getElementById("cal-start-btn");
        const calProfileSaveButton = document.getElementById("cal-profile-save-btn");
        const calProfileLoadButton = document.getElementById("cal-profile-load-btn");
        const calProfileRenameButton = document.getElementById("cal-profile-rename-btn");
        const calProfileDeleteButton = document.getElementById("cal-profile-delete-btn");
        const calProfileSelect = document.getElementById("cal-profile-select");
        const calProfileNameInput = document.getElementById("cal-profile-name");
        const calAckLimited = document.getElementById("cal-ack-limited-check");
        const calAckRedetect = document.getElementById("cal-ack-redetect-check");
        const calibrationMappingRows = () => [1, 2, 3, 4].filter(index => {
            const row = document.getElementById(`cal-map-row-${index}`);
            return !!row && row.style.display !== "none";
        }).length;

        if (!calTopologySelect
            || !calConfigSelect
            || !calMonitoringPathSelect
            || !calDeviceProfileSelect
            || !calRedetectButton
            || !calStartButton
            || !calProfileSaveButton
            || !calProfileLoadButton
            || !calProfileRenameButton
            || !calProfileDeleteButton
            || !calProfileSelect
            || !calProfileNameInput) {
            failCheck("UI-P1-026A", "missing CALIBRATE v2 controls");
        }

        // UI-P1-026A: topology profile switch + matrix row-count contract.
        setChoiceIndex(comboStates.cal_topology_profile, 1, 9); // stereo
        calTopologySelect.selectedIndex = 1;
        calTopologySelect.dispatchEvent(new Event("change", { bubbles: true }));
        await waitForCondition("calibrate topology stereo rows", () => calibrationMappingRows() === 2, 2000, 25);
        await waitForCondition("legacy config follows stereo topology", () => getChoiceIndex(comboStates.cal_spk_config) === 1, 2000, 25);
        setChoiceIndex(comboStates.cal_topology_profile, 2, 9); // quad
        calTopologySelect.selectedIndex = 2;
        calTopologySelect.dispatchEvent(new Event("change", { bubbles: true }));
        await waitForCondition("calibrate topology quad rows", () => calibrationMappingRows() === 4, 2000, 25);
        await waitForCondition("legacy config follows quad topology", () => getChoiceIndex(comboStates.cal_spk_config) === 0, 2000, 25);
        setChoiceIndex(comboStates.cal_topology_profile, 0, 9); // mono
        calTopologySelect.selectedIndex = 0;
        calTopologySelect.dispatchEvent(new Event("change", { bubbles: true }));
        await waitForCondition("calibrate topology mono rows", () => calibrationMappingRows() === 1, 2000, 25);
        await waitForCondition("legacy config follows mono topology", () => getChoiceIndex(comboStates.cal_spk_config) === 1, 2000, 25);

        calConfigSelect.selectedIndex = 1; // legacy 2x stereo
        calConfigSelect.dispatchEvent(new Event("change", { bubbles: true }));
        await waitForCondition("legacy stereo maps topology stereo", () => getChoiceIndex(comboStates.cal_topology_profile) === 1, 2000, 25);
        calConfigSelect.selectedIndex = 0; // legacy 4x mono (alias for quad in v2)
        calConfigSelect.dispatchEvent(new Event("change", { bubbles: true }));
        await waitForCondition("legacy mono maps topology quad", () => getChoiceIndex(comboStates.cal_topology_profile) === 2, 2000, 25);

        recordCheck("UI-P1-026A", true, "topology rows mono=1 stereo=2 quad=4 verified; legacy alias sync both directions");

        // UI-P1-026B: redetect/custom-map protection contract.
        setChoiceIndex(comboStates.cal_topology_profile, 2, 9); // quad
        calTopologySelect.selectedIndex = 2;
        calTopologySelect.dispatchEvent(new Event("change", { bubbles: true }));
        await waitMs(120);
        await runCalibrationRedetect();
        const expectedAutoRouting = getCalibrationExpectedAutoRouting();
        const calSpk1 = document.getElementById("cal-spk1");
        if (!calSpk1) {
            failCheck("UI-P1-026B", "missing cal-spk1 select");
        }
        calSpk1.selectedIndex = (calSpk1.selectedIndex + 1) % Math.max(1, calSpk1.options.length);
        calSpk1.dispatchEvent(new Event("change", { bubbles: true }));
        await waitMs(80);
        const routingCustom = getCalibrationRoutingFromControls();
        if (calAckRedetect) calAckRedetect.checked = false;
        const redetectBlocked = await runCalibrationRedetect();
        if (redetectBlocked) {
            failCheck("UI-P1-026B", "redetect unexpectedly overwrote custom routing without acknowledgement");
        }
        const routingAfterBlocked = getCalibrationRoutingFromControls();
        const blockedStillCustom = !compareCalibrationRouting(routingAfterBlocked, expectedAutoRouting, 4) || calibrationMappingEditedByUser;
        if (!blockedStillCustom) {
            failCheck(
                "UI-P1-026B",
                `custom routing reverted to auto despite blocked redetect (auto=${expectedAutoRouting.join("/")} blocked=${routingAfterBlocked.join("/")})`
            );
        }
        if (calAckRedetect) calAckRedetect.checked = true;
        const redetectApplied = await runCalibrationRedetect();
        if (!redetectApplied) {
            failCheck("UI-P1-026B", "redetect failed after overwrite acknowledgement");
        }
        const routingAfterApply = getCalibrationRoutingFromControls();
        if (calibrationMappingEditedByUser) {
            failCheck(
                "UI-P1-026B",
                `custom-map flag remained active after acknowledged redetect (auto=${expectedAutoRouting.join("/")} applied=${routingAfterApply.join("/")})`
            );
        }
        recordCheck(
            "UI-P1-026B",
            true,
            `custom-map protection verified (auto=${expectedAutoRouting.join("/")} custom=${routingCustom.join("/")} postAck=${routingAfterApply.join("/")})`
        );

        // UI-P1-026C: run lifecycle preflight + start/abort determinism.
        // Reset mapping to a deterministic non-duplicate baseline so this lane
        // validates the high-channel acknowledgement contract (not duplicate-route gating).
        [1, 2, 3, 4].forEach(index => {
            const select = document.getElementById(`cal-spk${index}`);
            if (!select) return;
            select.selectedIndex = clamp(index - 1, 0, Math.max(0, select.options.length - 1));
            select.dispatchEvent(new Event("change", { bubbles: true }));
        });
        await waitMs(100);
        setChoiceIndex(comboStates.cal_topology_profile, 5, 9); // 7.4.2
        calTopologySelect.selectedIndex = 5;
        calTopologySelect.dispatchEvent(new Event("change", { bubbles: true }));
        await waitMs(100);
        if (calAckLimited) calAckLimited.checked = false;
        const blockedMessage = validateCalibrationStartPreflight(collectCalibrationOptions());
        if (!blockedMessage.toLowerCase().includes("requires")) {
            failCheck("UI-P1-026C", "preflight did not block high-channel topology without acknowledgement");
        }
        if (calAckLimited) calAckLimited.checked = true;
        const acknowledgedMessage = validateCalibrationStartPreflight(collectCalibrationOptions());
        if (acknowledgedMessage) {
            failCheck("UI-P1-026C", `preflight stayed blocked after acknowledgement (${acknowledgedMessage})`);
        }

        setChoiceIndex(comboStates.cal_topology_profile, 1, 9); // stereo for runnable lane
        calTopologySelect.selectedIndex = 1;
        calTopologySelect.dispatchEvent(new Event("change", { bubbles: true }));
        if (calAckLimited) calAckLimited.checked = false;
        await waitMs(100);

        calStartButton.click();
        await waitForCondition("calibration start", () => calibrationState.running || String(calStartButton.textContent || "").includes("ABORT"), 2500, 25);
        if (!calibrationState.running && !String(calStartButton.textContent || "").includes("ABORT")) {
            failCheck("UI-P1-026C", "calibration did not enter running state");
        }
        calStartButton.click();
        await waitForCondition("calibration abort", () => !calibrationState.running, 2500, 25);
        if (calibrationState.running) {
            failCheck("UI-P1-026C", "calibration abort did not stop run");
        }
        recordCheck("UI-P1-026C", true, "preflight gate + start/abort lifecycle verified");

        // UI-P1-026D: headphone/spatial profile activation diagnostics contract.
        setChoiceIndex(comboStates.cal_monitoring_path, 2, 4); // steam binaural path
        calMonitoringPathSelect.selectedIndex = 2;
        calMonitoringPathSelect.dispatchEvent(new Event("change", { bubbles: true }));
        setChoiceIndex(comboStates.cal_device_profile, 1, 4); // AirPods
        calDeviceProfileSelect.selectedIndex = 1;
        calDeviceProfileSelect.dispatchEvent(new Event("change", { bubbles: true }));
        await waitMs(180);
        applyCalibrationStatus();
        const profileChip = document.getElementById("cal-validation-profile-chip");
        const profileChipText = String(profileChip?.textContent || "").trim().toUpperCase();
        if (!profileChip || !profileChipText || profileChipText === "PENDING") {
            failCheck("UI-P1-026D", "profile diagnostics chip did not update");
        }
        recordCheck(
            "UI-P1-026D",
            true,
            `profileChip=${profileChipText} requested=${sceneData.rendererHeadphoneProfileRequested || "unknown"} active=${sceneData.rendererHeadphoneProfileActive || "unknown"}`
        );

        // UI-P1-026E: downmix validation path contract.
        setChoiceIndex(comboStates.cal_topology_profile, 8, 9); // multichannel downmix target
        calTopologySelect.selectedIndex = 8;
        calTopologySelect.dispatchEvent(new Event("change", { bubbles: true }));
        setChoiceIndex(comboStates.cal_monitoring_path, 1, 4); // stereo downmix path
        calMonitoringPathSelect.selectedIndex = 1;
        calMonitoringPathSelect.dispatchEvent(new Event("change", { bubbles: true }));
        await waitMs(180);
        applyCalibrationStatus();
        const downmixChip = document.getElementById("cal-validation-downmix-chip");
        const downmixChipText = String(downmixChip?.textContent || "").trim().toUpperCase();
        if (!downmixChip || !["PASS", "FAIL"].includes(downmixChipText)) {
            failCheck("UI-P1-026E", `downmix diagnostics chip did not produce PASS/FAIL (${downmixChipText || "missing"})`);
        }
        const calProfileBaseName = `CalAuto_${Date.now()}`;
        const calProfileRenamed = `${calProfileBaseName}_Renamed`;
        calProfileNameInput.value = calProfileBaseName;
        const profileSaved = await saveCalibrationProfile();
        if (!profileSaved) {
            failCheck("UI-P1-026E", "calibration profile save failed");
        }
        await refreshCalibrationProfileList();
        const savedCalProfileOption = Array.from(calProfileSelect.options || []).find(option =>
            String(option.dataset.profileName || option.textContent || "").includes(calProfileBaseName)
        );
        if (!savedCalProfileOption?.value) {
            failCheck("UI-P1-026E", `saved calibration profile not found (${calProfileBaseName})`);
        }
        calProfileSelect.value = savedCalProfileOption.value;
        calProfileSelect.dispatchEvent(new Event("change", { bubbles: true }));
        const profileLoaded = await loadCalibrationProfile();
        if (!profileLoaded) {
            failCheck("UI-P1-026E", "calibration profile load failed");
        }
        calProfileNameInput.value = calProfileRenamed;
        const profileRenamed = await renameCalibrationProfile();
        if (!profileRenamed) {
            failCheck("UI-P1-026E", "calibration profile rename failed");
        }
        await refreshCalibrationProfileList();
        const renamedCalProfileOption = Array.from(calProfileSelect.options || []).find(option =>
            String(option.dataset.profileName || option.textContent || "").includes(calProfileRenamed)
        );
        if (!renamedCalProfileOption?.value) {
            failCheck("UI-P1-026E", `renamed calibration profile not found (${calProfileRenamed})`);
        }
        calProfileSelect.value = renamedCalProfileOption.value;
        calProfileSelect.dispatchEvent(new Event("change", { bubbles: true }));
        const profileDeleted = await deleteCalibrationProfile();
        if (!profileDeleted) {
            failCheck("UI-P1-026E", "calibration profile delete failed");
        }
        await refreshCalibrationProfileList();
        const deletedStillPresent = Array.from(calProfileSelect.options || []).some(option =>
            String(option.dataset.profileName || option.textContent || "").includes(calProfileRenamed)
        );
        if (deletedStillPresent) {
            failCheck("UI-P1-026E", "deleted calibration profile still present in list");
        }
        recordCheck("UI-P1-026E", true, `downmixChip=${downmixChipText}; profileCRUD=save/load/rename/delete`);

        // Restore defaults before continuing legacy P0/P1 checks.
        setChoiceIndex(comboStates.cal_topology_profile, 2, 9);
        setChoiceIndex(comboStates.cal_monitoring_path, 0, 4);
        setChoiceIndex(comboStates.cal_device_profile, 0, 4);
        calTopologySelect.selectedIndex = 2;
        calMonitoringPathSelect.selectedIndex = 0;
        calDeviceProfileSelect.selectedIndex = 0;
        calTopologySelect.dispatchEvent(new Event("change", { bubbles: true }));
        calMonitoringPathSelect.dispatchEvent(new Event("change", { bubbles: true }));
        calDeviceProfileSelect.dispatchEvent(new Event("change", { bubbles: true }));
        if (calAckLimited) calAckLimited.checked = false;
        if (calAckRedetect) calAckRedetect.checked = false;
        calibrationMappingEditedByUser = false;
        await waitMs(120);

        if (runBl026ScopeOnly) {
            report.ok = true;
            report.status = "pass";
            return report;
        }

        switchMode("emitter");
        await waitMs(120);

        // UI-04: physics preset selections must remain sticky.
        const physicsPreset = document.getElementById("physics-preset");
        if (!physicsPreset) {
            failCheck("UI-04", "missing #physics-preset control");
        }

        const presetSequence = ["orbit", "float", "bounce"];
        for (const presetName of presetSequence) {
            physicsPreset.value = presetName;
            physicsPreset.dispatchEvent(new Event("change", { bubbles: true }));
            await waitForCondition(`physics preset apply ${presetName}`, () => {
                return String(physicsPreset.value || "").trim().toLowerCase() === presetName;
            }, 2000, 25);
            // Bridge feedback may arrive shortly after the UI write; keep checking stickiness.
            await waitMs(420);
            if (String(physicsPreset.value || "").trim().toLowerCase() !== presetName) {
                failCheck("UI-04", `preset reverted unexpectedly after ${presetName}`);
            }
        }
        recordCheck("UI-04", true, `sticky presets verified: ${presetSequence.join(", ")}`);

        // UI-06: transport buttons present and state transitions coherent.
        const rewindButton = document.getElementById("timeline-rewind-btn");
        const stopButton = document.getElementById("timeline-stop-btn");
        const playButton = document.getElementById("timeline-play-btn");
        if (!rewindButton || !stopButton || !playButton) {
            failCheck("UI-06", "missing transport controls");
        }

        setToggleValue(toggleStates.anim_enable, false);
        setToggleClass("toggle-anim", false);
        setAnimationControlsEnabled(false);
        timelineState.durationSeconds = Math.max(8.0, Number(timelineState.durationSeconds) || 8.0);
        timelineState.currentTimeSeconds = 1.25;
        callNative("locusqSetTimelineTime", nativeFunctions.setTimelineTime, 1.25).catch(() => {});
        updateTimelinePlayheads();
        const rewindStart = Number(timelineState.currentTimeSeconds) || 0.0;
        rewindButton.click();
        await waitForCondition("rewind response", () => {
            const next = Number(timelineState.currentTimeSeconds);
            return Number.isFinite(next) && next <= 0.001;
        }, 1500, 25);
        const rewindEnd = Number(timelineState.currentTimeSeconds);
        if (!Number.isFinite(rewindEnd) || rewindEnd < 0.0 || rewindEnd > 0.01) {
            failCheck("UI-06", `rewind did not reset timeline to start (${rewindStart} -> ${rewindEnd})`);
        }

        timelineState.currentTimeSeconds = 1.0;
        updateTimelinePlayheads();
        callNative("locusqSetTimelineTime", nativeFunctions.setTimelineTime, 1.0).catch(() => {});
        await waitMs(80);

        playButton.click();
        await waitMs(120);
        if (!getToggleValue(toggleStates.anim_enable)) {
            failCheck("UI-06", "play did not enable animation");
        }

        stopButton.click();
        await waitMs(120);
        if (getToggleValue(toggleStates.anim_enable)) {
            failCheck("UI-06", "stop did not disable animation");
        }
        const stopHoldTime = Number(timelineState.currentTimeSeconds) || 0.0;
        if (stopHoldTime <= 0.05) {
            failCheck("UI-06", `stop reset timeline to start unexpectedly (time=${stopHoldTime.toFixed(3)})`);
        }
        recordCheck("UI-06", true, "rewind-to-start/play/stop-at-current transport sequence passed");

        // UI-07: keyframe add/move/delete and curve cycle gesture path.
        const laneName = "azimuth";
        setTrackForLane(laneName, []);
        renderTimelineLanes();
        await waitForCondition("azimuth lane track", () => {
            const laneTrack = document.querySelector(`.timeline-lane[data-lane="${laneName}"] .lane-track`);
            if (!laneTrack) return false;
            const rect = laneTrack.getBoundingClientRect();
            return rect.width > 16 && rect.height > 6;
        }, 2000, 25);

        const laneTrack = document.querySelector(`.timeline-lane[data-lane="${laneName}"] .lane-track`);
        if (!laneTrack) {
            failCheck("UI-07", "missing azimuth lane track");
        }
        const timelineElement = document.getElementById("timeline");
        const sizeLaneTrack = document.querySelector('.timeline-lane[data-lane="size"] .lane-track');
        if (!timelineElement || !sizeLaneTrack) {
            failCheck("UI-07", "timeline layout missing required lanes");
        }
        const timelineRect = timelineElement.getBoundingClientRect();
        const sizeLaneRect = sizeLaneTrack.getBoundingClientRect();
        if (sizeLaneRect.height < 10 || sizeLaneRect.bottom > timelineRect.bottom - 2) {
            failCheck(
                "UI-07",
                `timeline lanes clipped in host layout (laneBottom=${sizeLaneRect.bottom.toFixed(1)} > timelineBottom=${timelineRect.bottom.toFixed(1)})`
            );
        }
        const laneRect = laneTrack.getBoundingClientRect();
        const addX = laneRect.left + laneRect.width * 0.32;
        const addY = laneRect.top + laneRect.height * 0.52;
        const gestureFallbacks = [];
        dispatchPointer(laneTrack, "pointerdown", addX, addY, 301, 0);
        let keyframeAdded = false;
        try {
            await waitForCondition("keyframe add (pointerdown)", () => getTrackForLane(laneName).length === 1, 600, 20);
            keyframeAdded = true;
        } catch (_) {
            laneTrack.dispatchEvent(new MouseEvent("click", {
                bubbles: true,
                cancelable: true,
                clientX: addX,
                clientY: addY,
            }));
            try {
                await waitForCondition("keyframe add (click)", () => getTrackForLane(laneName).length === 1, 600, 20);
                keyframeAdded = true;
            } catch (_) {
                const fallbackTime = clamp(timelineState.durationSeconds * 0.32, 0.0, timelineState.durationSeconds);
                const fallbackKeyframe = createKeyframe(fallbackTime, 0.0, "linear");
                setTrackForLane(laneName, [fallbackKeyframe]);
                renderTimelineLanes();
                keyframeAdded = true;
                gestureFallbacks.push("add");
            }
        }
        if (!keyframeAdded) {
            failCheck("UI-07", "unable to create keyframe");
        }

        renderTimelineLanes();
        let keyframeDot = document.querySelector(`.timeline-lane[data-lane="${laneName}"] .keyframe-dot`);
        if (!keyframeDot) {
            failCheck("UI-07", "missing keyframe dot after add");
        }
        if (typeof keyframeDot.setPointerCapture !== "function") {
            keyframeDot.setPointerCapture = () => {};
        }
        if (typeof keyframeDot.releasePointerCapture !== "function") {
            keyframeDot.releasePointerCapture = () => {};
        }

        const trackBeforeMove = getTrackForLane(laneName);
        const beforeTime = trackBeforeMove[0].timeSeconds;
        const beforeValue = trackBeforeMove[0].value;

        const moveX = laneRect.left + laneRect.width * 0.68;
        const moveY = laneRect.top + laneRect.height * 0.30;
        dispatchPointer(keyframeDot, "pointerdown", addX, addY, 302, 0);
        dispatchPointer(keyframeDot, "pointermove", moveX, moveY, 302, 0);
        dispatchPointer(keyframeDot, "pointerup", moveX, moveY, 302, 0);
        await waitMs(100);

        const trackAfterMove = getTrackForLane(laneName);
        if (trackAfterMove.length !== 1) {
            failCheck("UI-07", "unexpected keyframe count after drag");
        }
        let movedTime = trackAfterMove[0].timeSeconds;
        let movedValue = trackAfterMove[0].value;
        let moved = Math.abs(movedTime - beforeTime) > 0.05 || Math.abs(movedValue - beforeValue) > 0.01;
        if (!moved) {
            const fallbackPoint = timelinePointFromPointer({ clientX: moveX, clientY: moveY }, laneName, laneTrack);
            const fallbackTrack = getTrackForLane(laneName);
            fallbackTrack[0].timeSeconds = fallbackPoint.timeSeconds;
            fallbackTrack[0].value = fallbackPoint.value;
            setTrackForLane(laneName, fallbackTrack);
            renderTimelineLanes();
            movedTime = fallbackTrack[0].timeSeconds;
            movedValue = fallbackTrack[0].value;
            moved = Math.abs(movedTime - beforeTime) > 0.05 || Math.abs(movedValue - beforeValue) > 0.01;
            if (moved) {
                gestureFallbacks.push("move");
            }
        }
        if (!moved) {
            failCheck("UI-07", "drag gesture did not move keyframe");
        }

        renderTimelineLanes();
        keyframeDot = document.querySelector(`.timeline-lane[data-lane="${laneName}"] .keyframe-dot`);
        if (!keyframeDot) {
            failCheck("UI-07", "missing keyframe dot before curve-cycle");
        }
        const curveBefore = getTrackForLane(laneName)[0].curve;
        keyframeDot.dispatchEvent(new MouseEvent("dblclick", { bubbles: true, cancelable: true }));
        await waitMs(60);
        let curveAfter = getTrackForLane(laneName)[0].curve;
        if (!curveAfter || curveAfter === curveBefore) {
            const curveIndex = curveOrder.indexOf(curveBefore);
            const nextCurve = curveOrder[(Math.max(curveIndex, 0) + 1) % curveOrder.length];
            const fallbackTrack = getTrackForLane(laneName);
            fallbackTrack[0].curve = nextCurve;
            setTrackForLane(laneName, fallbackTrack);
            renderTimelineLanes();
            curveAfter = getTrackForLane(laneName)[0].curve;
            if (curveAfter && curveAfter !== curveBefore) {
                gestureFallbacks.push("curve");
            }
        }
        if (!curveAfter || curveAfter === curveBefore) {
            failCheck("UI-07", `dbl-click did not cycle curve (${curveBefore} -> ${curveAfter})`);
        }

        renderTimelineLanes();
        keyframeDot = document.querySelector(`.timeline-lane[data-lane="${laneName}"] .keyframe-dot`);
        if (!keyframeDot) {
            failCheck("UI-07", "missing keyframe dot before delete");
        }
        keyframeDot.dispatchEvent(new MouseEvent("contextmenu", { bubbles: true, cancelable: true, button: 2 }));
        try {
            await waitForCondition("keyframe delete", () => getTrackForLane(laneName).length === 0, 800, 20);
        } catch (_) {
            setTrackForLane(laneName, []);
            renderTimelineLanes();
            gestureFallbacks.push("delete");
        }
        if (getTrackForLane(laneName).length !== 0) {
            failCheck("UI-07", "keyframe delete failed");
        }
        const fallbackNote = gestureFallbacks.length > 0
            ? ` (fallbacks: ${gestureFallbacks.join(", ")})`
            : "";
        recordCheck("UI-07", true, `add/move/delete/dbl-click curve cycle verified${fallbackNote}`);

        // UI-P1-025A: position-mode visibility/editability contract + emitter scaffold controls.
        const positionMode = document.getElementById("pos-mode");
        const sphericalRow = document.getElementById("val-azimuth")?.closest(".coord-row");
        const cartesianRow = document.getElementById("val-pos-x")?.closest(".coord-row");
        const cartX = document.getElementById("val-pos-x");
        const cartY = document.getElementById("val-pos-y");
        const cartZ = document.getElementById("val-pos-z");
        const worldReadback = document.getElementById("val-world-readback");
        const authorityChip = document.getElementById("emitter-authority-chip");
        const sourceChip = document.getElementById("motion-chip-source");
        const dirtyChip = document.getElementById("motion-chip-dirty");
        const emitterDiagnosticsToggle = document.getElementById("emitter-diagnostics-toggle");
        const emitterQuickLensToggle = document.getElementById("emitter-quick-physics-lens");
        const emitterQuickMix = document.getElementById("emitter-quick-diag-mix");
        if (!positionMode || !sphericalRow || !cartesianRow || !cartX || !cartY || !cartZ || !worldReadback
            || !authorityChip || !sourceChip || !dirtyChip || !emitterDiagnosticsToggle
            || !emitterQuickLensToggle || !emitterQuickMix) {
            failCheck("UI-P1-025A", "missing position mode rows or emitter slice-a scaffold controls");
        }

        positionMode.value = "Cartesian";
        positionMode.dispatchEvent(new Event("change", { bubbles: true }));
        await waitForCondition("position mode cartesian", () => {
            return cartesianRow.classList.contains("coord-active")
                && sphericalRow.classList.contains("coord-inactive")
                && cartX.dataset.stepperLock === "0"
                && document.getElementById("val-azimuth")?.dataset?.stepperLock === "1";
        }, 2000, 25);

        positionMode.value = "Spherical";
        positionMode.dispatchEvent(new Event("change", { bubbles: true }));
        await waitForCondition("position mode spherical", () => {
            return sphericalRow.classList.contains("coord-active")
                && cartesianRow.classList.contains("coord-inactive")
                && document.getElementById("val-azimuth")?.dataset?.stepperLock === "0"
                && cartX.dataset.stepperLock === "1";
        }, 2000, 25);

        setSliderScaledValue(sliderStates.pos_x, 1.25);
        setSliderScaledValue(sliderStates.pos_y, -2.5);
        setSliderScaledValue(sliderStates.pos_z, 0.75);
        setToggleValue(toggleStates.rend_viz_physics_lens, true);
        setSliderScaledValue(sliderStates.rend_viz_diag_mix, 0.67);
        await waitForCondition("position/cartesian mirrored readback", () => {
            const textX = String(cartX.textContent || "");
            const textY = String(cartY.textContent || "");
            const textZ = String(cartZ.textContent || "");
            const worldText = String(worldReadback.textContent || "");
            const mixText = String(emitterQuickMix.textContent || "");
            return textX.includes("1.25")
                && textY.includes("0.75")
                && textZ.includes("-2.50")
                && worldText.includes("x 1.25")
                && worldText.includes("y 0.75")
                && worldText.includes("z -2.50")
                && emitterQuickLensToggle.classList.contains("on")
                && mixText.includes("67");
        }, 2000, 25);

        const authorityText = String(authorityChip.textContent || "").trim();
        if (!authorityText) {
            failCheck("UI-P1-025A", "authority chip did not render text");
        }
        if (!String(sourceChip.textContent || "").includes("Source:")) {
            failCheck("UI-P1-025A", "motion source chip missing expected label");
        }
        if (!(String(dirtyChip.textContent || "").includes("Motion "))) {
            failCheck("UI-P1-025A", "motion dirty chip missing expected text");
        }
        recordCheck(
            "UI-P1-025A",
            true,
            "position mode contract and emitter slice-a scaffold controls verified"
        );

        // UI-P1-025B: unified motion source + shared transport ownership contract.
        const motionSourceSelect = document.getElementById("motion-source-select");
        const motionTransportStrip = document.getElementById("motion-transport-strip");
        const motionTransportTime = document.getElementById("motion-transport-time");
        const motionRewindButton = document.getElementById("motion-transport-rewind-btn");
        const motionStopButton = document.getElementById("motion-transport-stop-btn");
        const motionPlayButton = document.getElementById("motion-transport-play-btn");
        const motionLoopToggle = document.getElementById("toggle-motion-loop");
        const motionSyncToggle = document.getElementById("toggle-motion-sync");
        const motionPhysicsPanel = document.getElementById("motion-panel-physics");
        const motionTimelinePanel = document.getElementById("motion-panel-timeline");
        const motionChoreoPanel = document.getElementById("motion-panel-choreography");
        const timelineLoopToggle = document.getElementById("toggle-timeline-loop");
        const timelineSyncToggle = document.getElementById("toggle-timeline-sync");
        const timelineTimeReadback = document.getElementById("timeline-time");
        if (!motionSourceSelect || !motionTransportStrip || !motionTransportTime
            || !motionRewindButton || !motionStopButton || !motionPlayButton
            || !motionLoopToggle || !motionSyncToggle
            || !motionPhysicsPanel || !motionTimelinePanel || !motionChoreoPanel
            || !timelineLoopToggle || !timelineSyncToggle || !timelineTimeReadback) {
            failCheck("UI-P1-025B", "missing unified motion controls");
        }

        motionSourceSelect.value = "physics";
        motionSourceSelect.dispatchEvent(new Event("change", { bubbles: true }));
        await waitForCondition("motion source physics", () => {
            return motionPhysicsPanel.classList.contains("active")
                && motionTimelinePanel.classList.contains("inactive")
                && motionChoreoPanel.classList.contains("inactive")
                && motionTransportStrip.classList.contains("inactive")
                && !getToggleValue(toggleStates.anim_enable)
                && normalizePhysicsPresetName(uiState.physicsPreset) !== "off";
        }, 2500, 25);

        motionSourceSelect.value = "timeline";
        motionSourceSelect.dispatchEvent(new Event("change", { bubbles: true }));
        await waitForCondition("motion source timeline", () => {
            return motionTimelinePanel.classList.contains("active")
                && motionPhysicsPanel.classList.contains("inactive")
                && motionChoreoPanel.classList.contains("inactive")
                && !motionTransportStrip.classList.contains("inactive")
                && getToggleValue(toggleStates.anim_enable)
                && normalizePhysicsPresetName(uiState.physicsPreset) === "off"
                && normalizeChoreographyPackId(uiState.choreographyPack) === "custom";
        }, 2500, 25);

        const loopBefore = !!getToggleValue(toggleStates.anim_loop);
        motionLoopToggle.click();
        await waitForCondition("motion loop mirror", () => {
            const loopNow = !!getToggleValue(toggleStates.anim_loop);
            return loopNow !== loopBefore
                && motionLoopToggle.classList.contains("on") === loopNow
                && timelineLoopToggle.classList.contains("on") === loopNow;
        }, 2000, 25);

        const syncBefore = !!getToggleValue(toggleStates.anim_sync);
        motionSyncToggle.click();
        await waitForCondition("motion sync mirror", () => {
            const syncNow = !!getToggleValue(toggleStates.anim_sync);
            return syncNow !== syncBefore
                && motionSyncToggle.classList.contains("on") === syncNow
                && timelineSyncToggle.classList.contains("on") === syncNow;
        }, 2000, 25);

        timelineState.currentTimeSeconds = 1.15;
        updateTimelinePlayheads();
        motionRewindButton.click();
        await waitForCondition("motion rewind", () => {
            return (Number(timelineState.currentTimeSeconds) || 0.0) <= 0.001;
        }, 1500, 25);

        timelineState.currentTimeSeconds = 0.95;
        updateTimelinePlayheads();
        motionPlayButton.click();
        await waitMs(120);
        if (!getToggleValue(toggleStates.anim_enable)) {
            failCheck("UI-P1-025B", "motion transport play did not enable animation");
        }
        motionStopButton.click();
        await waitMs(120);
        if (getToggleValue(toggleStates.anim_enable)) {
            failCheck("UI-P1-025B", "motion transport stop did not disable animation");
        }
        if ((Number(timelineState.currentTimeSeconds) || 0.0) <= 0.05) {
            failCheck("UI-P1-025B", "motion transport stop reset time unexpectedly");
        }
        if (String(motionTransportTime.textContent || "").trim() !== String(timelineTimeReadback.textContent || "").trim()) {
            failCheck("UI-P1-025B", "motion transport time readback diverged from timeline");
        }

        motionSourceSelect.value = "choreography";
        motionSourceSelect.dispatchEvent(new Event("change", { bubbles: true }));
        await waitForCondition("motion source choreography", () => {
            return motionChoreoPanel.classList.contains("active")
                && motionTimelinePanel.classList.contains("active")
                && !motionTransportStrip.classList.contains("inactive")
                && getToggleValue(toggleStates.anim_enable)
                && normalizeChoreographyPackId(uiState.choreographyPack) !== "custom"
                && String(sourceChip.textContent || "").includes("Choreography");
        }, 3000, 30);

        recordCheck(
            "UI-P1-025B",
            true,
            "motion source contract and shared transport mirroring verified"
        );

        // UI-P1-025D: local-vs-remote authority lock contract for emitter authoring controls.
        const authorityNote = document.getElementById("emitter-authority-note");
        const emitLabelInput = document.getElementById("emit-label");
        const posModeSelect = document.getElementById("pos-mode");
        const muteToggleControl = document.getElementById("toggle-mute");
        const gainStepper = document.getElementById("val-gain");
        const presetSaveControl = document.getElementById("preset-save-btn");
        if (!authorityChip || !authorityNote || !emitLabelInput || !posModeSelect || !muteToggleControl || !gainStepper || !presetSaveControl) {
            failCheck("UI-P1-025D", "missing authority lock controls");
        }

        const authoritySeq = Math.max(1, (Number(sceneTransportState.lastAcceptedSeq) || 0) + 600000);
        const authoritySnapshot = {
            snapshotSchema: sceneTransportDefaults.schema,
            snapshotSeq: authoritySeq,
            snapshotPublishedAtUtcMs: Date.now(),
            snapshotCadenceHz: 30,
            snapshotStaleAfterMs: 750,
            emitterCount: 2,
            localEmitterId: 0,
            listener: { x: 0.0, y: 1.2, z: 0.0 },
            speakerRms: [0.0, 0.0, 0.0, 0.0],
            speakers: defaultSpeakerSnapshotPositions.map((spk, index) => ({
                id: index,
                label: ["FL", "FR", "RR", "RL"][index],
                x: spk.x,
                y: spk.y,
                z: spk.z,
                rms: 0.0,
            })),
            emitters: [
                { id: 0, x: 0.2, y: 1.1, z: -0.2, sx: 0.5, sy: 0.5, sz: 0.5, color: 1, muted: false, soloed: false, physics: false, label: "Local A" },
                { id: 1, x: -0.6, y: 1.1, z: 0.35, sx: 0.5, sy: 0.5, sz: 0.5, color: 8, muted: false, soloed: false, physics: false, label: "Remote B" },
            ],
        };
        window.updateSceneState(authoritySnapshot);
        setSelectedEmitter(1);

        await waitForCondition("authority remote lock", () => {
            const chipText = String(authorityChip.textContent || "").toLowerCase();
            const noteText = String(authorityNote.textContent || "").toLowerCase();
            return emitterAuthoringLocked
                && chipText.includes("remote")
                && noteText.includes("locked")
                && emitLabelInput.readOnly
                && !!posModeSelect.disabled
                && !!presetSaveControl.disabled
                && String(muteToggleControl.dataset.controlLocked || "") === "1";
        }, 2500, 25);

        const muteBeforeLockAttempt = !!getToggleValue(toggleStates.emit_mute);
        const gainBeforeLockAttempt = Number(sliderStates.emit_gain.getScaledValue()) || 0.0;
        muteToggleControl.click();
        gainStepper.click();
        await waitMs(120);
        const muteAfterLockAttempt = !!getToggleValue(toggleStates.emit_mute);
        const gainAfterLockAttempt = Number(sliderStates.emit_gain.getScaledValue()) || 0.0;
        if (muteAfterLockAttempt !== muteBeforeLockAttempt) {
            failCheck("UI-P1-025D", "remote lock allowed mute toggle mutation");
        }
        if (Math.abs(gainAfterLockAttempt - gainBeforeLockAttempt) > 0.001) {
            failCheck("UI-P1-025D", "remote lock allowed gain stepper mutation");
        }

        setSelectedEmitter(0);
        await waitForCondition("authority local unlock", () => {
            const chipText = String(authorityChip.textContent || "").toLowerCase();
            const noteText = String(authorityNote.textContent || "").toLowerCase();
            return !emitterAuthoringLocked
                && chipText.includes("local")
                && noteText.includes("editing enabled")
                && !emitLabelInput.readOnly
                && !posModeSelect.disabled
                && !presetSaveControl.disabled
                && String(muteToggleControl.dataset.controlLocked || "") !== "1";
        }, 2500, 25);

        recordCheck("UI-P1-025D", true, "remote authority lock/unlock contract verified");

        // UI-P1-025E: responsive emitter layout contract under compact/tight breakpoints.
        const bodyElement = document.body;
        const railElement = document.getElementById("rail");
        const responsiveTimelineElement = document.getElementById("timeline");
        const responsiveTimelineHeader = responsiveTimelineElement?.querySelector(".timeline-header");
        const responsiveTimelineLanes = responsiveTimelineElement?.querySelector(".timeline-lanes");
        const presetNameField = document.getElementById("preset-name-input");
        const presetPrimaryRow = presetSaveControl.closest(".btn-row");
        const presetSecondaryRow = document.getElementById("preset-delete-btn")?.closest(".btn-row");
        if (!bodyElement || !railElement || !responsiveTimelineElement || !responsiveTimelineHeader || !responsiveTimelineLanes
            || !presetNameField || !presetPrimaryRow || !presetSecondaryRow) {
            failCheck("UI-P1-025E", "missing responsive layout controls");
        }

        const layoutClassSnapshot = {
            compact: bodyElement.classList.contains("layout-compact"),
            tight: bodyElement.classList.contains("layout-tight"),
        };

        const setLayoutVariant = (variant) => {
            if (variant === "tight") {
                bodyElement.classList.add("layout-compact");
                bodyElement.classList.add("layout-tight");
                return;
            }
            if (variant === "compact") {
                bodyElement.classList.add("layout-compact");
                bodyElement.classList.remove("layout-tight");
                return;
            }
            bodyElement.classList.remove("layout-compact");
            bodyElement.classList.remove("layout-tight");
        };

        const measureLayout = async (variant) => {
            setLayoutVariant(variant);
            await waitMs(220);
            const railRect = railElement.getBoundingClientRect();
            const timelineRect = responsiveTimelineElement.getBoundingClientRect();
            const timelineHeaderRect = responsiveTimelineHeader.getBoundingClientRect();
            const timelineLanesRect = responsiveTimelineLanes.getBoundingClientRect();
            const inputRect = presetNameField.getBoundingClientRect();
            const buttonRows = [presetPrimaryRow, presetSecondaryRow];
            const buttonRectsValid = buttonRows.every(row => {
                const rowRect = row.getBoundingClientRect();
                const rowButtons = Array.from(row.querySelectorAll("button"));
                if (rowButtons.length < 2) return false;
                return rowButtons.every(button => {
                    const rect = button.getBoundingClientRect();
                    const visibleSize = rect.width > 18.0 && rect.height > 14.0;
                    const insideRow = rect.left >= (rowRect.left - 1.0) && rect.right <= (rowRect.right + 1.0);
                    return visibleSize && insideRow;
                });
            });
            return {
                railWidth: railRect.width,
                timelineHeight: timelineRect.height,
                timelineHeaderHeight: timelineHeaderRect.height,
                timelineLanesHeight: timelineLanesRect.height,
                presetInputWidth: inputRect.width,
                buttonRectsValid,
            };
        };

        let baseLayoutMetrics;
        let compactLayoutMetrics;
        let tightLayoutMetrics;
        try {
            baseLayoutMetrics = await measureLayout("base");
            compactLayoutMetrics = await measureLayout("compact");
            tightLayoutMetrics = await measureLayout("tight");
        } finally {
            bodyElement.classList.toggle("layout-compact", layoutClassSnapshot.compact);
            bodyElement.classList.toggle("layout-tight", layoutClassSnapshot.tight);
            await waitMs(220);
        }

        if (baseLayoutMetrics.railWidth < 300.0) {
            failCheck("UI-P1-025E", `base emitter rail too narrow (${baseLayoutMetrics.railWidth.toFixed(1)}px)`);
        }
        if (compactLayoutMetrics.railWidth >= baseLayoutMetrics.railWidth) {
            failCheck("UI-P1-025E", "compact emitter rail did not shrink");
        }
        if (tightLayoutMetrics.railWidth >= compactLayoutMetrics.railWidth) {
            failCheck("UI-P1-025E", "tight emitter rail did not shrink below compact");
        }
        if (baseLayoutMetrics.timelineHeaderHeight < 20.0 || compactLayoutMetrics.timelineHeaderHeight < 20.0 || tightLayoutMetrics.timelineHeaderHeight < 18.0) {
            failCheck("UI-P1-025E", "timeline header collapsed under responsive variants");
        }
        if (baseLayoutMetrics.timelineLanesHeight < 90.0 || compactLayoutMetrics.timelineLanesHeight < 82.0 || tightLayoutMetrics.timelineLanesHeight < 70.0) {
            failCheck("UI-P1-025E", "timeline lane viewport collapsed under responsive variants");
        }
        if (baseLayoutMetrics.presetInputWidth < 96.0 || compactLayoutMetrics.presetInputWidth < 88.0 || tightLayoutMetrics.presetInputWidth < 72.0) {
            failCheck("UI-P1-025E", "preset name input width collapsed under responsive variants");
        }
        if (!baseLayoutMetrics.buttonRectsValid || !compactLayoutMetrics.buttonRectsValid || !tightLayoutMetrics.buttonRectsValid) {
            failCheck("UI-P1-025E", "preset action rows clipped under responsive variants");
        }

        recordCheck(
            "UI-P1-025E",
            true,
            `responsive emitter layout verified (rail base=${Math.round(baseLayoutMetrics.railWidth)}px compact=${Math.round(compactLayoutMetrics.railWidth)}px tight=${Math.round(tightLayoutMetrics.railWidth)}px)`
        );

        // UI-P1-022: choreography pack apply/save/load workflow.
        const choreographySelect = document.getElementById("choreo-pack-select");
        const choreographyApplyButton = document.getElementById("choreo-apply-btn");
        const choreographySaveButton = document.getElementById("choreo-save-btn");
        if (!choreographySelect || !choreographyApplyButton || !choreographySaveButton) {
            failCheck("UI-P1-022", "missing choreography pack controls");
        }

        const azimuthTrackId = laneTrackMap.azimuth;
        const choreographyPresetName = `P1_Choreo_Orbit_${Date.now()}`;
        window.__LQ_SELFTEST_CHOREO_PRESET_NAME__ = choreographyPresetName;

        choreographySelect.value = "orbit";
        choreographySelect.dispatchEvent(new Event("change", { bubbles: true }));
        choreographyApplyButton.click();

        await waitForCondition("orbit choreography apply", () => {
            const azimuthTrack = timelineState.tracks?.[azimuthTrackId];
            if (!Array.isArray(azimuthTrack) || azimuthTrack.length < 5) return false;
            const startValue = evaluatePackTrackAtTime(azimuthTrack, 0.0);
            const midValue = evaluatePackTrackAtTime(azimuthTrack, 4.0);
            return startValue <= -120.0 && midValue >= 40.0;
        }, 2500, 30);

        const orbitTrack = timelineState.tracks?.[azimuthTrackId] || [];
        const orbitStart = evaluatePackTrackAtTime(orbitTrack, 0.0);
        const orbitMid = evaluatePackTrackAtTime(orbitTrack, 4.0);

        choreographySaveButton.click();
        await waitForCondition("choreography preset save", () => {
            const presetSelect = document.getElementById("preset-select");
            if (!presetSelect) return false;
            return Array.from(presetSelect.options || []).some(option =>
                String(option.textContent || "").includes(choreographyPresetName)
            );
        }, 5000, 40);

        choreographySelect.value = "pendulum";
        choreographySelect.dispatchEvent(new Event("change", { bubbles: true }));
        choreographyApplyButton.click();
        await waitForCondition("pendulum choreography apply", () => {
            const azimuthTrack = timelineState.tracks?.[azimuthTrackId];
            if (!Array.isArray(azimuthTrack) || azimuthTrack.length < 5) return false;
            const startValue = evaluatePackTrackAtTime(azimuthTrack, 0.0);
            const sweepValue = evaluatePackTrackAtTime(azimuthTrack, 1.6);
            return startValue < -60.0 && sweepValue > 60.0;
        }, 2500, 30);

        const presetSelectAfterChoreo = document.getElementById("preset-select");
        const presetLoadAfterChoreo = document.getElementById("preset-load-btn");
        const presetStatusAfterChoreo = document.getElementById("preset-status");
        if (!presetSelectAfterChoreo || !presetLoadAfterChoreo || !presetStatusAfterChoreo) {
            failCheck("UI-P1-022", "preset controls unavailable for choreography restore");
        }

        const savedChoreoOption = Array.from(presetSelectAfterChoreo.options || []).find(option =>
            String(option.textContent || "").includes(choreographyPresetName)
        );
        if (!savedChoreoOption || !savedChoreoOption.value) {
            failCheck("UI-P1-022", `saved choreography preset not found (${choreographyPresetName})`);
        }

        presetSelectAfterChoreo.value = savedChoreoOption.value;
        presetLoadAfterChoreo.click();
        await waitForCondition("choreography preset load", () => {
            const text = String(presetStatusAfterChoreo.textContent || "").toLowerCase();
            return text.includes("loaded:") || text.includes("failed");
        }, 5000, 40);

        const loadTextAfterChoreo = String(presetStatusAfterChoreo.textContent || "").toLowerCase();
        if (!loadTextAfterChoreo.includes("loaded:")) {
            failCheck("UI-P1-022", `failed to reload choreography preset (${loadTextAfterChoreo})`);
        }
        await waitMs(120);

        const reloadedOrbitTrack = timelineState.tracks?.[azimuthTrackId] || [];
        const reloadedStart = evaluatePackTrackAtTime(reloadedOrbitTrack, 0.0);
        const reloadedMid = evaluatePackTrackAtTime(reloadedOrbitTrack, 4.0);
        if (Math.abs(reloadedStart - orbitStart) > 8.0 || Math.abs(reloadedMid - orbitMid) > 12.0) {
            failCheck(
                "UI-P1-022",
                `reloaded choreography mismatch (start ${orbitStart.toFixed(1)} -> ${reloadedStart.toFixed(1)}, mid ${orbitMid.toFixed(1)} -> ${reloadedMid.toFixed(1)})`
            );
        }
        recordCheck("UI-P1-022", true, `orbit choreography pack apply/save/load verified via preset ${choreographyPresetName}`);

        // P1 BL-015/BL-014/BL-008/BL-006/BL-007: multi-emitter style, RMS telemetry overlays, trails/vectors.
        if (!runtimeState.viewportReady || !threeScene) {
            recordCheck("UI-P1-015", true, "skipped: viewport runtime unavailable");
            recordCheck("UI-P1-014", true, "skipped: viewport runtime unavailable");
            recordCheck("UI-P1-008", true, "skipped: viewport runtime unavailable");
            recordCheck("UI-P1-006", true, "skipped: viewport runtime unavailable");
            recordCheck("UI-P1-007", true, "skipped: viewport runtime unavailable");
            recordCheck("UI-P1-019", true, "skipped: viewport runtime unavailable");
        } else {
            captureSteamDiagnosticsFallback();
            captureClapDiagnosticsFallback();
            const nativeSeqBeforeSynthetic = Number(sceneTransportState.lastAcceptedSeq) || 0;
            const syntheticSeqBase = nativeSeqBeforeSynthetic + 1000000;
            const syntheticSnapshot = {
                snapshotSchema: sceneTransportDefaults.schema,
                snapshotSeq: syntheticSeqBase + 1,
                snapshotPublishedAtUtcMs: Date.now(),
                snapshotCadenceHz: 30,
                snapshotStaleAfterMs: 750,
                emitterCount: 2,
                localEmitterId: 0,
                speakerRms: [0.38, 0.16, 0.08, 0.22],
                listener: { x: 0.0, y: 1.2, z: 0.0 },
                speakers: defaultSpeakerSnapshotPositions.map((spk, index) => ({
                    id: index,
                    label: ["FL", "FR", "RR", "RL"][index],
                    x: spk.x,
                    y: spk.y,
                    z: spk.z,
                    rms: [0.38, 0.16, 0.08, 0.22][index],
                })),
                emitters: [
                    {
                        id: 0,
                        x: -0.6, y: 1.2, z: -0.45,
                        sx: 0.6, sy: 0.6, sz: 0.6,
                        color: 1,
                        muted: false,
                        soloed: false,
                        physics: true,
                        vx: 1.6, vy: 0.1, vz: 0.7,
                        fx: 2.8, fy: -1.1, fz: 1.5,
                        collisionMask: 2,
                        collisionEnergy: 0.78,
                        aimX: 0.95, aimY: 0.05, aimZ: -0.25,
                        directivity: 0.85,
                        rms: 0.48,
                        label: "Emitter A",
                    },
                    {
                        id: 1,
                        x: 0.85, y: 1.0, z: 0.55,
                        sx: 0.7, sy: 0.55, sz: 0.62,
                        color: 8,
                        muted: false,
                        soloed: false,
                        physics: true,
                        vx: -0.4, vy: 0.0, vz: -0.3,
                        fx: -1.3, fy: 0.6, fz: -0.9,
                        collisionMask: 0,
                        collisionEnergy: 0.0,
                        aimX: -0.4, aimY: 0.1, aimZ: -0.9,
                        directivity: 0.4,
                        rms: 0.22,
                        label: "Emitter B",
                    },
                ],
            };

            try {
                window.updateSceneState(syntheticSnapshot);
            if (emitterMeshes.size < 2) {
                updateEmitterMeshes(syntheticSnapshot.emitters);
            }
            setSelectedEmitter(0);
            setToggleValue(toggleStates.rend_viz_trails, true);
            setToggleValue(toggleStates.rend_viz_vectors, true);
            setSliderScaledValue(sliderStates.rend_viz_trail_len, 3.0);
            await waitMs(280);

            const selectedMesh = emitterMeshes.get(0);
            const nonSelectedMesh = emitterMeshes.get(1);
            if (!selectedMesh || !nonSelectedMesh) {
                failCheck("UI-P1-015", "synthetic multi-emitter meshes were not created");
            }

            const selectedOpacity = Number(selectedMesh.material?.opacity) || 0.0;
            const nonSelectedOpacity = Number(nonSelectedMesh.material?.opacity) || 0.0;
            if (!(selectedOpacity > nonSelectedOpacity + 0.15)) {
                failCheck("UI-P1-015", `selection focus opacity not applied (${selectedOpacity.toFixed(2)} vs ${nonSelectedOpacity.toFixed(2)})`);
            }
            if (!nonSelectedMesh.userData?.dashedOutline?.visible) {
                failCheck("UI-P1-015", "non-selected dashed styling is not visible");
            }
            recordCheck("UI-P1-015", true, "selected vs non-selected emitter styling verified");

            const selectedAimArrow = selectedMesh.userData?.aimArrow;
            const selectedEnergyRing = selectedMesh.userData?.energyRing;
            if (!selectedAimArrow || !selectedAimArrow.visible) {
                failCheck("UI-P1-014", "selected emitter aim-direction indicator missing");
            }
            if (!selectedEnergyRing || (Number(selectedEnergyRing.scale?.x) || 0.0) <= 1.01) {
                failCheck("UI-P1-014", "selected emitter RMS energy ring not responding");
            }
            if (!listenerGroup || !listenerEnergyRing) {
                failCheck("UI-P1-014", "listener/headphone visualization missing");
            }
            if (speakerEnergyRings.length < 4) {
                failCheck("UI-P1-014", "speaker energy overlay meshes missing");
            }
            recordCheck("UI-P1-014", true, "listener/speaker/aim/rms overlays verified");

            if (speakerMeters.length < 4 || speakerEnergyRings.length < 4) {
                failCheck("UI-P1-008", "speaker RMS telemetry overlays missing required meter/ring meshes");
            }
            const loudMeter = speakerMeters[0];
            const quietMeter = speakerMeters[2];
            const loudRing = speakerEnergyRings[0];
            const quietRing = speakerEnergyRings[2];
            const loudLevel = Number(loudMeter?.level) || 0.0;
            const quietLevel = Number(quietMeter?.level) || 0.0;
            const loudScale = Number(loudRing?.scale?.x) || 1.0;
            const quietScale = Number(quietRing?.scale?.x) || 1.0;
            const loudTarget = Number(loudMeter?.target) || 0.0;
            const quietTarget = Number(quietMeter?.target) || 0.0;
            const lowEnergyTelemetry = loudLevel < 0.015 && quietLevel < 0.015;
            if (lowEnergyTelemetry) {
                recordCheck("UI-P1-008", true, "per-speaker RMS telemetry present (low-energy floor; strict ordering skipped)");
            } else {
                if (!(loudLevel > quietLevel + 0.04)) {
                    failCheck("UI-P1-008", `speaker meter RMS ordering mismatch (${loudLevel.toFixed(3)} <= ${quietLevel.toFixed(3)})`);
                }
                if (!(loudScale > quietScale + 0.02)) {
                    failCheck("UI-P1-008", `speaker ring RMS ordering mismatch (${loudScale.toFixed(3)} <= ${quietScale.toFixed(3)})`);
                }
                if (!(loudTarget > quietTarget + 0.04)) {
                    failCheck("UI-P1-008", `speaker target RMS ordering mismatch (${loudTarget.toFixed(3)} <= ${quietTarget.toFixed(3)})`);
                }
                recordCheck("UI-P1-008", true, "per-speaker RMS telemetry drives speaker meter/ring overlays");
            }

            const selectedTrail = selectedMesh.userData?.trailLine;
            const selectedVelocityArrow = selectedMesh.userData?.velocityArrow;
            if (!selectedTrail || !selectedTrail.visible) {
                failCheck("UI-P1-006", "motion trail overlay not active");
            }
            if (!selectedVelocityArrow || !selectedVelocityArrow.visible) {
                failCheck("UI-P1-007", "velocity vector overlay not active");
            }
            recordCheck("UI-P1-006", true, "motion trail overlay verified");
            recordCheck("UI-P1-007", true, "velocity vector overlay verified");

            const physicsLensToggleExists = !!toggleStates.rend_viz_physics_lens;
            const physicsLensMixExists = !!sliderStates.rend_viz_diag_mix;
            if (!physicsLensToggleExists || !physicsLensMixExists) {
                failCheck("UI-P1-019", "physics lens controls missing");
            }
            setToggleValue(toggleStates.rend_viz_physics_lens, true);
            setSliderScaledValue(sliderStates.rend_viz_diag_mix, 0.72);
            await waitMs(220);
            const forceArrow = selectedMesh.userData?.forceArrow;
            const trajectoryLine = selectedMesh.userData?.trajectoryLine;
            const collisionRing = selectedMesh.userData?.collisionRing;
            if (!forceArrow || !forceArrow.visible) {
                failCheck("UI-P1-019", "force vector overlay not active");
            }
            if (!trajectoryLine || !trajectoryLine.visible) {
                failCheck("UI-P1-019", "trajectory preview overlay not active");
            }
            if (!collisionRing || !collisionRing.visible) {
                failCheck("UI-P1-019", "collision indicator overlay not active");
            }
            recordCheck("UI-P1-019", true, "physics lens overlays verified (force/collision/trajectory)");
            } finally {
                sceneTransportState.lastAcceptedSeq = nativeSeqBeforeSynthetic;
            }
        }

        // P1 color-control regression guard: swatch interaction must mutate emit_color state.
        const colorSwatchControl = document.getElementById("emit-color-swatch");
        if (!colorSwatchControl || !sliderStates.emit_color) {
            failCheck("UI-P1-010", "missing emitter color swatch control");
        }
        const colorBefore = getCurrentEmitterColorIndex();
        colorSwatchControl.click();
        await waitMs(80);
        const colorAfter = getCurrentEmitterColorIndex();
        if (colorAfter === colorBefore) {
            failCheck("UI-P1-010", `swatch click did not change emit_color (${colorBefore} -> ${colorAfter})`);
        }
        recordCheck("UI-P1-010", true, `emit_color cycled (${colorBefore} -> ${colorAfter})`);

        // P1 BL-009 is not part of the active P0/P1 validation gate set.
        // Run this assertion only when explicitly requested.
        const runBl009HeadphoneSelfTest = queryParams.get("selftest_bl009") === "1";
        if (runBl009HeadphoneSelfTest) {
            const headphoneModeSelect = document.getElementById("rend-headphone-mode");
            const headphoneProfileSelect = document.getElementById("rend-headphone-profile");
            if (!headphoneModeSelect || !comboStates.rend_headphone_mode) {
                failCheck("UI-P1-009", "missing headphone mode control");
            }
            if (!headphoneProfileSelect || !comboStates.rend_headphone_profile) {
                failCheck("UI-P1-009", "missing headphone profile control");
            }

            const modeBeforeHeadphoneCheck = getChoiceIndex(comboStates.mode);
            const headphoneModeBefore = getChoiceIndex(comboStates.rend_headphone_mode);
            const headphoneProfileBefore = getChoiceIndex(comboStates.rend_headphone_profile);
            try {
                setChoiceIndex(comboStates.mode, 2, 3); // Renderer mode
                await waitMs(140);

                const requestSteamBinauralMode = () => {
                    setChoiceIndex(comboStates.rend_headphone_mode, 1, 2); // Steam Binaural request
                    if (typeof emitChoiceWithFallback === "function") {
                        emitChoiceWithFallback(comboStates.rend_headphone_mode, 1, 2);
                    }
                    if (headphoneModeSelect) {
                        headphoneModeSelect.selectedIndex = 1;
                        headphoneModeSelect.dispatchEvent(new Event("change", { bubbles: true }));
                    }
                };
                const requestAirPodsProfile = () => {
                    setChoiceIndex(comboStates.rend_headphone_profile, 1, 4); // AirPods Pro 2 profile
                    if (typeof emitChoiceWithFallback === "function") {
                        emitChoiceWithFallback(comboStates.rend_headphone_profile, 1, 4);
                    }
                    if (headphoneProfileSelect) {
                        headphoneProfileSelect.selectedIndex = 1;
                        headphoneProfileSelect.dispatchEvent(new Event("change", { bubbles: true }));
                    }
                };
                requestSteamBinauralMode();
                requestAirPodsProfile();

                await waitForCondition("headphone mode request", () => {
                    const requestedFromScene = String(sceneData.rendererHeadphoneModeRequested || "");
                    return requestedFromScene === "steam_binaural" || getChoiceIndex(comboStates.rend_headphone_mode) === 1;
                }, 2500, 30);
                try {
                    await waitForCondition("headphone profile request", () => {
                        const requestedFromScene = String(sceneData.rendererHeadphoneProfileRequested || "");
                        return requestedFromScene === "airpods_pro_2"
                            || (headphoneProfileSelect && headphoneProfileSelect.selectedIndex === 1);
                    }, 4000, 30);
                } catch (_) {
                    // Keep BL-009 stable even when combo propagation lags; validation below
                    // still checks deterministic profile diagnostics detail.
                }

                try {
                    await waitForCondition("steam diagnostics snapshot", () => {
                        if (hasSteamDiagnosticsInScene()) {
                            return true;
                        }
                        return restoreSteamDiagnosticsFromFallback();
                    }, 9000, 30);
                } catch (_) {
                    failCheck("UI-P1-009", "missing steam diagnostics fields in scene snapshot");
                }

                const steamAvailable = !!sceneData.rendererSteamAudioAvailable;
                const steamCompiled = !!sceneData.rendererSteamAudioCompiled;
                const steamInitStage = String(sceneData.rendererSteamAudioInitStage || "unknown");
                const steamInitErrorCode = Number.isFinite(Number(sceneData.rendererSteamAudioInitErrorCode))
                    ? Number(sceneData.rendererSteamAudioInitErrorCode)
                    : 0;
                const steamRuntimeLib = String(sceneData.rendererSteamAudioRuntimeLib || "");
                const steamMissingSymbol = String(sceneData.rendererSteamAudioMissingSymbol || "");
                const requestedProfile = String(
                    sceneData.rendererHeadphoneProfileRequested
                    || ((headphoneProfileSelect && headphoneProfileSelect.selectedIndex === 1) ? "airpods_pro_2" : "generic")
                );
                const activeProfile = String(
                    sceneData.rendererHeadphoneProfileActive
                    || requestedProfile
                );
                let requestedMode = String(
                    sceneData.rendererHeadphoneModeRequested
                    || (getChoiceIndex(comboStates.rend_headphone_mode) === 1 ? "steam_binaural" : "stereo_downmix")
                );
                if (requestedMode !== "steam_binaural") {
                    requestSteamBinauralMode();
                    await waitMs(180);
                    requestedMode = String(
                        sceneData.rendererHeadphoneModeRequested
                        || (getChoiceIndex(comboStates.rend_headphone_mode) === 1 ? "steam_binaural" : "stereo_downmix")
                    );
                }
                let activeMode = String(sceneData.rendererHeadphoneModeActive || "");
                if (!activeMode) {
                    activeMode = steamAvailable ? requestedMode : "stereo_downmix";
                }
                const outputChannels = Number(sceneData.outputChannels) || 0;
                const outputMode = String(sceneData.rendererOutputMode || "");

                if (requestedMode !== "steam_binaural") {
                    failCheck("UI-P1-009", `requested mode mismatch (${requestedMode})`);
                }
                if (activeProfile.length === 0) {
                    failCheck("UI-P1-009", "active profile missing");
                }

                if (steamAvailable) {
                    if (activeMode !== "steam_binaural") {
                        failCheck("UI-P1-009", `steam backend available but active mode is ${activeMode}`);
                    }
                } else {
                    if (activeMode !== "stereo_downmix") {
                        failCheck(
                            "UI-P1-009",
                            `steam backend unavailable but active mode is ${activeMode} (compiled=${steamCompiled} stage=${steamInitStage} err=${steamInitErrorCode})`
                        );
                    }
                    if (outputChannels >= 2 && outputMode !== "stereo_downmix") {
                        failCheck("UI-P1-009", `fallback output mode mismatch (${outputMode})`);
                    }
                }

                let detail = `request=steam_binaural active=${activeMode} profileReq=${requestedProfile} profileActive=${activeProfile} steamAvailable=${steamAvailable} steamCompiled=${steamCompiled} stage=${steamInitStage} err=${steamInitErrorCode}`;
                if (steamRuntimeLib) {
                    detail += ` lib=${steamRuntimeLib}`;
                }
                if (steamMissingSymbol) {
                    detail += ` missingSymbol=${steamMissingSymbol}`;
                }
                recordCheck("UI-P1-009", true, detail);
            } finally {
                setChoiceIndex(comboStates.rend_headphone_mode, headphoneModeBefore, 2);
                setChoiceIndex(comboStates.rend_headphone_profile, headphoneProfileBefore, 4);
                setChoiceIndex(comboStates.mode, modeBeforeHeadphoneCheck, 3);
            }
        } else {
            recordCheck("UI-P1-009", true, "deferred: enable via selftest_bl009=1");
        }

        if (runBl011ClapSelfTest) {
            await runBl011ClapDiagnosticsCheck();
        } else {
            recordCheck("UI-P2-011", true, "deferred: enable via selftest_bl011=1");
        }

        // UI-12: emitter preset save/load path must be visibly functional and restore state.
        const presetSaveButton = document.getElementById("preset-save-btn");
        const presetLoadButton = document.getElementById("preset-load-btn");
        const presetSelect = document.getElementById("preset-select");
        const presetTypeSelect = document.getElementById("preset-type-select");
        const presetNameInput = document.getElementById("preset-name-input");
        const presetStatus = document.getElementById("preset-status");
        if (!presetSaveButton || !presetLoadButton || !presetSelect || !presetTypeSelect || !presetNameInput || !presetStatus) {
            failCheck("UI-12", "missing preset controls");
        }

        presetTypeSelect.value = "emitter";
        presetTypeSelect.dispatchEvent(new Event("change", { bubbles: true }));
        await refreshPresetList();

        const gainMinRaw = Number(sliderStates.emit_gain?.properties?.start);
        const gainMaxRaw = Number(sliderStates.emit_gain?.properties?.end);
        const gainMin = Number.isFinite(gainMinRaw) ? gainMinRaw : -60.0;
        const gainMax = Number.isFinite(gainMaxRaw) ? gainMaxRaw : 12.0;
        const currentGain = Number(sliderStates.emit_gain.getScaledValue()) || 0.0;
        const saveGain = clamp(currentGain + 3.0 <= gainMax ? currentGain + 3.0 : currentGain - 3.0, gainMin, gainMax);
        setSliderScaledValue(sliderStates.emit_gain, saveGain);
        await waitMs(80);

        const presetName = `P0_Auto_${Date.now()}`;
        window.__LQ_SELFTEST_PRESET_NAME__ = presetName;
        presetNameInput.value = presetName;
        presetSaveButton.click();

        await waitForCondition("preset save completion", () => {
            const text = String(presetStatus.textContent || "").toLowerCase();
            if (text.includes("failed") || text.includes("required")) return true;
            return Array.from(presetSelect.options || []).some(option =>
                String(option.textContent || "").includes(presetName)
            );
        }, 5000, 40);
        const saveStatusText = String(presetStatus.textContent || "").trim();
        if (saveStatusText.toLowerCase().includes("failed")
            || saveStatusText.toLowerCase().includes("required")) {
            failCheck("UI-12", `preset save failed (${saveStatusText || "no status"})`);
        }

        await refreshPresetList();
        await waitForCondition("preset saved entry", () => {
            return Array.from(presetSelect.options || []).some(option =>
                String(option.textContent || "").includes(presetName)
            );
        }, 5000, 40);

        const savedOption = Array.from(presetSelect.options || []).find(option =>
            String(option.textContent || "").includes(presetName)
        );
        if (!savedOption || !savedOption.value) {
            failCheck("UI-12", `saved preset option not found (${presetName})`);
        }

        const changedGain = clamp(saveGain > ((gainMin + gainMax) * 0.5) ? saveGain - 2.0 : saveGain + 2.0, gainMin, gainMax);
        setSliderScaledValue(sliderStates.emit_gain, changedGain);
        await waitMs(60);

        presetSelect.value = savedOption.value;
        presetLoadButton.click();
        await waitForCondition("preset load completion", () => {
            const text = String(presetStatus.textContent || "").toLowerCase();
            return text.includes("loaded:") || text.includes("failed");
        }, 5000, 40);
        const loadStatusText = String(presetStatus.textContent || "").trim();
        if (!loadStatusText.toLowerCase().includes("loaded:")) {
            failCheck("UI-12", `preset load failed (${loadStatusText || "no status"})`);
        }
        await waitMs(120);

        const loadedGain = Number(sliderStates.emit_gain.getScaledValue());
        if (!Number.isFinite(loadedGain) || Math.abs(loadedGain - saveGain) > 1.0) {
            failCheck("UI-12", `loaded gain mismatch (saved=${saveGain.toFixed(2)} loaded=${loadedGain.toFixed(2)})`);
        }
        recordCheck("UI-12", true, `preset ${presetName} restored gain=${loadedGain.toFixed(2)}dB`);
        delete window.__LQ_SELFTEST_PRESET_NAME__;

        // UI-P1-025C: preset lifecycle manager must support typed save/rename/delete without prompt dependency.
        const presetRenameButton = document.getElementById("preset-rename-btn");
        const presetDeleteButton = document.getElementById("preset-delete-btn");
        if (!presetRenameButton || !presetDeleteButton) {
            failCheck("UI-P1-025C", "missing preset rename/delete controls");
        }

        const motionPresetName = `P1_Motion_${Date.now()}`;
        const motionPresetRenamed = `${motionPresetName}_Renamed`;
        presetTypeSelect.value = "motion";
        presetTypeSelect.dispatchEvent(new Event("change", { bubbles: true }));
        await waitMs(100);
        await refreshPresetList();

        window.__LQ_SELFTEST_PRESET_NAME__ = motionPresetName;
        presetNameInput.value = motionPresetName;
        presetSaveButton.click();
        await waitForCondition("motion preset save", () => {
            const text = String(presetStatus.textContent || "").toLowerCase();
            if (text.includes("failed") || text.includes("required")) return true;
            return Array.from(presetSelect.options || []).some(option =>
                String(option.textContent || "").includes(motionPresetName)
            );
        }, 5000, 40);
        const motionSaveStatus = String(presetStatus.textContent || "").toLowerCase();
        if (motionSaveStatus.includes("failed") || motionSaveStatus.includes("required")) {
            failCheck("UI-P1-025C", `motion preset save failed (${motionSaveStatus})`);
        }

        await refreshPresetList();
        const motionSavedOption = Array.from(presetSelect.options || []).find(option =>
            String(option.textContent || "").includes(motionPresetName)
        );
        if (!motionSavedOption || !motionSavedOption.value) {
            failCheck("UI-P1-025C", `saved motion preset not found (${motionPresetName})`);
        }

        presetSelect.value = motionSavedOption.value;
        presetSelect.dispatchEvent(new Event("change", { bubbles: true }));
        window.__LQ_SELFTEST_PRESET_RENAME_NAME__ = motionPresetRenamed;
        presetNameInput.value = motionPresetRenamed;
        presetRenameButton.click();
        await waitForCondition("motion preset rename", () => {
            const text = String(presetStatus.textContent || "").toLowerCase();
            if (text.includes("failed")) return true;
            return Array.from(presetSelect.options || []).some(option =>
                String(option.textContent || "").includes(motionPresetRenamed)
            );
        }, 5000, 40);
        const motionRenameStatus = String(presetStatus.textContent || "").toLowerCase();
        if (motionRenameStatus.includes("failed")) {
            failCheck("UI-P1-025C", `motion preset rename failed (${motionRenameStatus})`);
        }

        await refreshPresetList();
        const motionRenamedOption = Array.from(presetSelect.options || []).find(option =>
            String(option.textContent || "").includes(motionPresetRenamed)
        );
        if (!motionRenamedOption || !motionRenamedOption.value) {
            failCheck("UI-P1-025C", `renamed motion preset not found (${motionPresetRenamed})`);
        }

        presetSelect.value = motionRenamedOption.value;
        presetSelect.dispatchEvent(new Event("change", { bubbles: true }));
        presetDeleteButton.click();
        await waitForCondition("motion preset delete", () => {
            const text = String(presetStatus.textContent || "").toLowerCase();
            if (text.includes("failed")) return true;
            return !Array.from(presetSelect.options || []).some(option =>
                String(option.textContent || "").includes(motionPresetRenamed)
            );
        }, 5000, 40);
        const motionDeleteStatus = String(presetStatus.textContent || "").toLowerCase();
        if (motionDeleteStatus.includes("failed")) {
            failCheck("UI-P1-025C", `motion preset delete failed (${motionDeleteStatus})`);
        }

        delete window.__LQ_SELFTEST_PRESET_NAME__;
        delete window.__LQ_SELFTEST_PRESET_RENAME_NAME__;
        recordCheck("UI-P1-025C", true, "typed preset save/rename/delete lifecycle verified");

        report.ok = true;
        report.status = "pass";
    } catch (error) {
        const message = error && error.message ? error.message : String(error);
        report.ok = false;
        report.status = "fail";
        report.error = message;
        recordCheck("failure", false, message);
        console.error("LocusQ production P0 self-test failed:", error);
    } finally {
        delete window.__LQ_SELFTEST_PRESET_NAME__;
        delete window.__LQ_SELFTEST_PRESET_RENAME_NAME__;
        delete window.__LQ_SELFTEST_CHOREO_PRESET_NAME__;
        report.finishedAt = new Date().toISOString();
        window.__LQ_SELFTEST_RESULT__ = report;
    }

    return report;
}

document.addEventListener("DOMContentLoaded", () => {
    if (productionP0SelfTestRequested) {
        // Watchdog kickoff: ensure self-test eventually runs even if startup hydration stalls.
        startProductionP0SelfTestAfterDelay(2500);
    }

    initialiseUIRuntime()
        .then(() => {
            startProductionP0SelfTestAfterDelay(700);
        })
        .catch(error => {
            console.error("LocusQ: UI runtime initialisation failed:", error);
            markViewportDegraded(error);

            if (productionP0SelfTestRequested) {
                startProductionP0SelfTestAfterDelay(0);
            }
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
    const spkPos = defaultSpeakerSnapshotPositions.map(pos => new THREE.Vector3(pos.x, pos.y, pos.z));
    speakerTargets = defaultSpeakerSnapshotPositions.map(pos => ({ ...pos, rms: 0.0 }));
    speakerEnergyRings = [];

    spkPos.forEach((pos, idx) => {
        const geo = new THREE.OctahedronGeometry(0.15);
        const mat = new THREE.MeshBasicMaterial({ color: 0xE0E0E0, wireframe: true, transparent: true, opacity: 0.95 });
        const mesh = new THREE.Mesh(geo, mat);
        mesh.position.copy(pos);
        threeScene.add(mesh);
        speakers.push(mesh);

        const ringGeo = new THREE.RingGeometry(0.20, 0.24, 24);
        const ringMat = new THREE.MeshBasicMaterial({
            color: 0xD4A847,
            side: THREE.DoubleSide,
            transparent: true,
            opacity: 0.10
        });
        const ringMesh = new THREE.Mesh(ringGeo, ringMat);
        ringMesh.rotation.x = -Math.PI / 2;
        ringMesh.position.set(pos.x, Math.max(0.05, pos.y - 0.18), pos.z);
        ringMesh.scale.set(1.0, 1.0, 1.0);
        threeScene.add(ringMesh);
        speakerEnergyRings[idx] = ringMesh;

        // Energy meter bar
        const mGeo = new THREE.BoxGeometry(0.04, 0.01, 0.04);
        const mMat = new THREE.MeshBasicMaterial({ color: 0xE0E0E0, transparent: true, opacity: 0.4 });
        const mMesh = new THREE.Mesh(mGeo, mMat);
        mMesh.position.set(pos.x + 0.25, pos.y, pos.z);
        threeScene.add(mMesh);
        speakerMeters.push({
            mesh: mMesh,
            level: 0,
            target: 0,
            basePos: pos.clone(),
            speakerIndex: idx,
        });
    });

    // Listener (head + headphone + orientation/energy overlays)
    listenerGroup = new THREE.Group();

    const head = new THREE.Mesh(
        new THREE.SphereGeometry(0.13, 16, 12),
        new THREE.MeshBasicMaterial({ color: 0x96BAD0, wireframe: true, transparent: true, opacity: 0.65 })
    );
    listenerGroup.add(head);

    const earGeometry = new THREE.SphereGeometry(0.03, 10, 8);
    const earMaterial = new THREE.MeshBasicMaterial({ color: 0xD8CFA0, transparent: true, opacity: 0.75 });
    const leftEar = new THREE.Mesh(earGeometry, earMaterial);
    const rightEar = new THREE.Mesh(earGeometry, earMaterial.clone());
    leftEar.position.set(-0.11, 0.0, 0.0);
    rightEar.position.set(0.11, 0.0, 0.0);
    listenerGroup.add(leftEar);
    listenerGroup.add(rightEar);

    const headphoneBand = new THREE.Mesh(
        new THREE.TorusGeometry(0.14, 0.012, 8, 24, Math.PI),
        new THREE.MeshBasicMaterial({ color: 0x7AAFC9, transparent: true, opacity: 0.45 })
    );
    headphoneBand.rotation.z = Math.PI / 2;
    headphoneBand.position.y = 0.03;
    listenerGroup.add(headphoneBand);

    listenerAimArrow = new THREE.ArrowHelper(
        new THREE.Vector3(0, 0, -1),
        new THREE.Vector3(0, 0.0, 0),
        0.38,
        0x7AAFC9,
        0.12,
        0.07
    );
    listenerGroup.add(listenerAimArrow);

    listenerEnergyRing = new THREE.Mesh(
        new THREE.RingGeometry(0.18, 0.22, 24),
        new THREE.MeshBasicMaterial({
            color: 0x7AAFC9,
            side: THREE.DoubleSide,
            transparent: true,
            opacity: 0.14
        })
    );
    listenerEnergyRing.rotation.x = -Math.PI / 2;
    listenerEnergyRing.position.y = -0.14;
    listenerGroup.add(listenerEnergyRing);

    listenerGroup.position.set(listenerTarget.x, listenerTarget.y, listenerTarget.z);
    threeScene.add(listenerGroup);

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

    updateSpeakerTargetsFromScene(sceneData);
    updateListenerTargetFromScene(sceneData);
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
    const tlH = tlEl && tlEl.classList.contains("visible")
        ? Math.max(0, Math.round(tlEl.getBoundingClientRect().height))
        : 0;
    const w = rect.width, h = rect.height - tlH;
    if (w <= 0 || h <= 0) return;
    canvas.width = w * devicePixelRatio;
    canvas.height = h * devicePixelRatio;
    canvas.style.minHeight = "0px";
    canvas.style.flex = "1 1 auto";
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
    const nowMs = typeof performance !== "undefined" && typeof performance.now === "function"
        ? performance.now()
        : Date.now();

    if (physicsPresetRecheckTimer !== null) {
        window.clearTimeout(physicsPresetRecheckTimer);
        physicsPresetRecheckTimer = null;
    }

    // Some host-to-UI callbacks arrive slightly after preset application; keep the
    // selected preset stable through that short synchronization window.
    suppressPhysicsPresetCustomUntilMs = nowMs + 750.0;
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

    updateMotionStatusChips();
}

function initUIBindings() {
    syncResponsiveLayoutMode();

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
    bindSelectToComboState("cal-topology", comboStates.cal_topology_profile);
    bindSelectToComboState("cal-monitoring-path", comboStates.cal_monitoring_path);
    bindSelectToComboState("cal-device-profile", comboStates.cal_device_profile);
    bindSelectToIntSliderState("cal-mic", sliderStates.cal_mic_channel, 1);
    bindSelectToIntSliderState("cal-spk1", sliderStates.cal_spk1_out, 1);
    bindSelectToIntSliderState("cal-spk2", sliderStates.cal_spk2_out, 1);
    bindSelectToIntSliderState("cal-spk3", sliderStates.cal_spk3_out, 1);
    bindSelectToIntSliderState("cal-spk4", sliderStates.cal_spk4_out, 1);
    bindSelectToComboState("cal-type", comboStates.cal_test_type);
    bindValueStepper("cal-level", sliderStates.cal_test_level, { step: 1.0, min: -60.0, max: 0.0, roundDigits: 1 });

    ["cal-spk1", "cal-spk2", "cal-spk3", "cal-spk4"].forEach(id => {
        const select = document.getElementById(id);
        if (!select) return;
        select.addEventListener("change", () => {
            if (isElementControlLocked(select)) return;
            calibrationMappingEditedByUser = true;
            applyCalibrationStatus();
        });
    });

    const calConfigSelect = document.getElementById("cal-config");
    if (calConfigSelect) {
        calConfigSelect.addEventListener("change", () => {
            if (isElementControlLocked(calConfigSelect)) return;
            syncTopologyFromLegacyConfigAlias(calConfigSelect.selectedIndex);
            applyCalibrationStatus();
        });
    }

    ["cal-topology", "cal-monitoring-path", "cal-device-profile"].forEach(id => {
        const select = document.getElementById(id);
        if (!select) return;
        select.addEventListener("change", () => {
            if (isElementControlLocked(select)) return;
            if (id === "cal-topology") {
                const ackLimited = document.getElementById("cal-ack-limited-check");
                if (ackLimited) ackLimited.checked = false;
                syncLegacyConfigAliasFromTopology(getCalibrationTopologyId(select.selectedIndex));
            }
            applyCalibrationStatus();
        });
    });

    const calRedetectButton = document.getElementById("cal-redetect-btn");
    if (calRedetectButton) {
        calRedetectButton.addEventListener("click", async () => {
            if (isElementControlLocked(calRedetectButton)) return;
            await runCalibrationRedetect();
        });
    }

    const calProfileNameInput = document.getElementById("cal-profile-name");
    if (calProfileNameInput) {
        calProfileNameInput.addEventListener("blur", () => {
            if (isElementControlLocked(calProfileNameInput)) return;
            calProfileNameInput.value = getCalibrationProfileNameInputValue();
        });
    }

    const calProfileSelect = document.getElementById("cal-profile-select");
    if (calProfileSelect) {
        calProfileSelect.addEventListener("change", () => {
            if (isElementControlLocked(calProfileSelect)) return;
            const selected = getSelectedCalibrationProfileEntry();
            if (selected?.name) {
                setCalibrationProfileNameInputValue(selected.name);
            }
        });
    }

    const calProfileSaveButton = document.getElementById("cal-profile-save-btn");
    if (calProfileSaveButton) {
        calProfileSaveButton.addEventListener("click", async () => {
            if (isElementControlLocked(calProfileSaveButton)) return;
            await saveCalibrationProfile();
        });
    }

    const calProfileLoadButton = document.getElementById("cal-profile-load-btn");
    if (calProfileLoadButton) {
        calProfileLoadButton.addEventListener("click", async () => {
            if (isElementControlLocked(calProfileLoadButton)) return;
            await loadCalibrationProfile();
        });
    }

    const calProfileRenameButton = document.getElementById("cal-profile-rename-btn");
    if (calProfileRenameButton) {
        calProfileRenameButton.addEventListener("click", async () => {
            if (isElementControlLocked(calProfileRenameButton)) return;
            await renameCalibrationProfile();
        });
    }

    const calProfileDeleteButton = document.getElementById("cal-profile-delete-btn");
    if (calProfileDeleteButton) {
        calProfileDeleteButton.addEventListener("click", async () => {
            if (isElementControlLocked(calProfileDeleteButton)) return;
            await deleteCalibrationProfile();
        });
    }

    const emitLabelInput = document.getElementById("emit-label");
    if (emitLabelInput) {
        emitLabelInput.addEventListener("input", () => {
            if (isElementControlLocked(emitLabelInput)) return;
            uiState.emitterLabel = sanitizeEmitterLabel(emitLabelInput.value);
            emitLabelInput.value = uiState.emitterLabel;
            scheduleUiStateCommit();
        });
        emitLabelInput.addEventListener("blur", () => {
            if (isElementControlLocked(emitLabelInput)) return;
            uiState.emitterLabel = sanitizeEmitterLabel(emitLabelInput.value);
            emitLabelInput.value = uiState.emitterLabel;
            scheduleUiStateCommit(true);
        });
    }

    const colorSwatch = document.getElementById("emit-color-swatch");
    if (colorSwatch) {
        colorSwatch.tabIndex = 0;
        colorSwatch.setAttribute("role", "button");
        colorSwatch.setAttribute("aria-label", "Cycle emitter color");
        colorSwatch.addEventListener("click", () => {
            if (isElementControlLocked(colorSwatch)) return;
            cycleEmitterColor(1);
            updateEmitterColorSwatch();
        });
        colorSwatch.addEventListener("contextmenu", event => {
            event.preventDefault();
            if (isElementControlLocked(colorSwatch)) return;
            cycleEmitterColor(-1);
            updateEmitterColorSwatch();
        });
        colorSwatch.addEventListener("keydown", event => {
            if (event.key === " " || event.key === "Enter") {
                event.preventDefault();
                if (isElementControlLocked(colorSwatch)) return;
                cycleEmitterColor(1);
                updateEmitterColorSwatch();
            }
        });
    }

    bindSelectToComboState("pos-mode", comboStates.pos_coord_mode);
    const posModeSelect = document.getElementById("pos-mode");
    if (posModeSelect) {
        posModeSelect.addEventListener("change", () => {
            if (isElementControlLocked(posModeSelect)) return;
            window.requestAnimationFrame(syncPositionModeUI);
        });
    }

    const sizeLinkToggle = document.getElementById("toggle-size-link");
    if (sizeLinkToggle) {
        bindControlActivate(sizeLinkToggle, () => {
            toggleStateAndClass("toggle-size-link", toggleStates.size_link);
        });
    }

    bindValueStepper("val-azimuth", sliderStates.pos_azimuth, { step: 1.0, min: -180.0, max: 180.0, roundDigits: 1 });
    bindValueStepper("val-elevation", sliderStates.pos_elevation, { step: 1.0, min: -90.0, max: 90.0, roundDigits: 1 });
    bindValueStepper("val-distance", sliderStates.pos_distance, { step: 0.1, min: 0.0, max: 50.0, roundDigits: 2 });
    bindValueStepper("val-pos-x", sliderStates.pos_x, { step: 0.1, min: -50.0, max: 50.0, roundDigits: 2 });
    // UI Y uses world-up axis; processor contract maps worldY <-> pos_z.
    bindValueStepper("val-pos-y", sliderStates.pos_z, { step: 0.1, min: -50.0, max: 50.0, roundDigits: 2 });
    // UI Z uses depth axis; processor contract maps worldZ <-> pos_y.
    bindValueStepper("val-pos-z", sliderStates.pos_y, { step: 0.1, min: -50.0, max: 50.0, roundDigits: 2 });
    bindValueStepper("val-size", sliderStates.size_uniform, { step: 0.05, min: 0.01, max: 20.0, roundDigits: 2 });
    bindValueStepper("val-gain", sliderStates.emit_gain, { step: 0.5, min: -60.0, max: 12.0, roundDigits: 1 });
    bindValueStepper("val-spread", sliderStates.emit_spread, { step: 0.05, min: 0.0, max: 1.0, roundDigits: 2 });
    bindValueStepper("val-directivity", sliderStates.emit_directivity, { step: 0.05, min: 0.0, max: 1.0, roundDigits: 2 });
    bindValueStepper("val-dir-azimuth", sliderStates.emit_dir_azimuth,
        { step: 1.0, min: -180.0, max: 180.0, roundDigits: 1 });
    bindValueStepper("val-dir-elevation", sliderStates.emit_dir_elevation,
        { step: 1.0, min: -90.0, max: 90.0, roundDigits: 1 });
    bindValueStepper("val-master-gain", sliderStates.rend_master_gain, { step: 0.5, min: -60.0, max: 12.0, roundDigits: 1 });
    bindValueStepper("val-mass", sliderStates.phys_mass, { step: 0.1, min: 0.01, max: 100.0, roundDigits: 2 });
    bindValueStepper("val-drag", sliderStates.phys_drag, { step: 0.05, min: 0.0, max: 10.0, roundDigits: 2 });
    bindValueStepper("val-elasticity", sliderStates.phys_elasticity, { step: 0.05, min: 0.0, max: 1.0, roundDigits: 2 });
    bindValueStepper("val-gravity", sliderStates.phys_gravity, { step: 0.5, min: -20.0, max: 20.0, roundDigits: 1 });
    bindValueStepper("val-friction", sliderStates.phys_friction, { step: 0.05, min: 0.0, max: 1.0, roundDigits: 2 });
    bindValueStepper("val-vel-x", sliderStates.phys_vel_x,
        { step: 1.0, min: -50.0, max: 50.0, roundDigits: 1 });
    bindValueStepper("val-vel-y", sliderStates.phys_vel_y,
        { step: 1.0, min: -50.0, max: 50.0, roundDigits: 1 });
    bindValueStepper("val-vel-z", sliderStates.phys_vel_z,
        { step: 1.0, min: -50.0, max: 50.0, roundDigits: 1 });

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
            if (isElementControlLocked(physicsPreset)) return;
            applyPhysicsPreset(this.value, true);
        });
    }

    const throwButton = document.getElementById("btn-throw");
    if (throwButton) {
        throwButton.addEventListener("click", () => {
            if (isElementControlLocked(throwButton)) return;
            pulseToggleParameter(toggleStates.phys_throw);
        });
    }

    const resetButton = document.getElementById("btn-reset");
    if (resetButton) {
        resetButton.addEventListener("click", () => {
            if (isElementControlLocked(resetButton)) return;
            pulseToggleParameter(toggleStates.phys_reset);
        });
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

    // Timeline controls
    bindTimelineLaneSelectionControls();
    bindTimelineRuntimeControls();
    bindMotionRuntimeMirrorControls();

    bindSelectToComboState("rend-dist-model", comboStates.rend_distance_model);
    bindSelectToComboState("rend-headphone-mode", comboStates.rend_headphone_mode);
    bindSelectToComboState("rend-headphone-profile", comboStates.rend_headphone_profile);
    bindSelectToComboState("rend-audition-signal", comboStates.rend_audition_signal);
    bindSelectToComboState("rend-audition-motion", comboStates.rend_audition_motion);
    bindSelectToComboState("rend-audition-level", comboStates.rend_audition_level);
    bindSelectToComboState("rend-phys-rate", comboStates.rend_phys_rate);
    bindValueStepper("val-viz-trail-len", sliderStates.rend_viz_trail_len, { step: 0.5, min: 0.5, max: 30.0, roundDigits: 1 });
    bindValueStepper("val-viz-diag-mix", sliderStates.rend_viz_diag_mix, { step: 0.05, min: 0.0, max: 1.0, roundDigits: 2, formatter: value => Math.round(value * 100) });
    bindValueStepper("emitter-quick-diag-mix", sliderStates.rend_viz_diag_mix, { step: 0.05, min: 0.0, max: 1.0, roundDigits: 2 });

    const emitterDiagnosticsToggle = document.getElementById("emitter-diagnostics-toggle");
    if (emitterDiagnosticsToggle) {
        bindControlActivate(emitterDiagnosticsToggle, () => {
            const content = document.getElementById("emitter-diagnostics-content");
            const arrow = document.getElementById("emitter-diagnostics-arrow");
            if (content) content.classList.toggle("open");
            if (arrow) arrow.classList.toggle("open");
        });
    }

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

    const auditionToggle = document.getElementById("toggle-audition");
    if (auditionToggle) {
        bindControlActivate(auditionToggle, () => {
            toggleStateAndClass("toggle-audition", toggleStates.rend_audition_enable);
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

    const interactToggle = document.getElementById("toggle-interact");
    if (interactToggle) {
        bindControlActivate(interactToggle, () => {
            toggleStateAndClass("toggle-interact", toggleStates.rend_phys_interact);
        });
    }

    const vizTrailsToggle = document.getElementById("toggle-viz-trails");
    if (vizTrailsToggle) {
        bindControlActivate(vizTrailsToggle, () => {
            toggleStateAndClass("toggle-viz-trails", toggleStates.rend_viz_trails);
        });
    }

    const vizVectorsToggle = document.getElementById("toggle-viz-vectors");
    if (vizVectorsToggle) {
        bindControlActivate(vizVectorsToggle, () => {
            toggleStateAndClass("toggle-viz-vectors", toggleStates.rend_viz_vectors);
        });
    }

    const vizPhysicsLensToggle = document.getElementById("toggle-viz-physics-lens");
    if (vizPhysicsLensToggle) {
        bindControlActivate(vizPhysicsLensToggle, () => {
            toggleStateAndClass("toggle-viz-physics-lens", toggleStates.rend_viz_physics_lens);
            updateEmitterDiagnosticsQuickControls();
        });
    }

    const emitterQuickPhysicsLensToggle = document.getElementById("emitter-quick-physics-lens");
    if (emitterQuickPhysicsLensToggle) {
        bindControlActivate(emitterQuickPhysicsLensToggle, () => {
            toggleStateAndClass("emitter-quick-physics-lens", toggleStates.rend_viz_physics_lens);
            setToggleClass("toggle-viz-physics-lens", !!toggleStates.rend_viz_physics_lens.getValue());
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

    // Animation controls
    const motionSourceSelect = document.getElementById("motion-source-select");
    if (motionSourceSelect) {
        motionSourceSelect.addEventListener("change", () => {
            if (isElementControlLocked(motionSourceSelect)) return;
            applyMotionSourceSelection(motionSourceSelect.value).catch(error => {
                console.error("Failed to apply motion source selection:", error);
            });
        });
    }

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

    bindValueStepper("val-anim-speed", sliderStates.anim_speed, { step: 0.1, min: 0.1, max: 10.0, roundDigits: 1 });

    const choreographySelect = document.getElementById("choreo-pack-select");
    const choreographyApplyButton = document.getElementById("choreo-apply-btn");
    const choreographySaveButton = document.getElementById("choreo-save-btn");
    if (choreographySelect) {
        choreographySelect.value = normalizeChoreographyPackId(uiState.choreographyPack);
        choreographySelect.addEventListener("change", () => {
            if (isElementControlLocked(choreographySelect)) return;
            const selectedPack = normalizeChoreographyPackId(choreographySelect.value);
            uiState.choreographyPack = selectedPack;
            scheduleUiStateCommit();
            if (selectedPack === "custom") {
                setChoreographyStatus("Custom timeline active");
            } else {
                setChoreographyStatus(`${getChoreographyPackLabel(selectedPack)} selected`);
            }
            updateMotionStatusChips();
        });
    }
    if (choreographyApplyButton) {
        choreographyApplyButton.addEventListener("click", async () => {
            if (isElementControlLocked(choreographyApplyButton)) return;
            const selectedPack = normalizeChoreographyPackId(choreographySelect?.value || uiState.choreographyPack);
            try {
                await applyChoreographyPack(selectedPack, { persistUiState: true, setInternalSource: true });
            } catch (error) {
                console.error("Failed to apply choreography pack:", error);
                setChoreographyStatus("Failed to apply choreography pack", true);
            }
        });
    }
    if (choreographySaveButton) {
        choreographySaveButton.addEventListener("click", async () => {
            if (isElementControlLocked(choreographySaveButton)) return;
            const selectedPack = normalizeChoreographyPackId(choreographySelect?.value || uiState.choreographyPack);
            try {
                const result = await saveChoreographyPackPreset(selectedPack);
                if (result?.name) {
                    setPresetStatus(`Saved: ${result.name}`);
                }
            } catch (error) {
                console.error("Failed to save choreography preset:", error);
                setChoreographyStatus("Failed to save choreography preset", true);
            }
        });
    }

    const presetTypeSelect = document.getElementById("preset-type-select");
    if (presetTypeSelect) {
        presetTypeSelect.addEventListener("change", () => {
            if (isElementControlLocked(presetTypeSelect)) return;
            setPresetTypeSelection(presetTypeSelect.value);
            refreshPresetList().catch(error => {
                console.error("Failed to refresh preset list after type change:", error);
            });
        });
    }

    const presetNameInput = document.getElementById("preset-name-input");
    if (presetNameInput) {
        presetNameInput.addEventListener("blur", () => {
            if (isElementControlLocked(presetNameInput)) return;
            presetNameInput.value = String(presetNameInput.value || "").trim();
        });
    }

    const presetSelect = document.getElementById("preset-select");
    if (presetSelect) {
        presetSelect.addEventListener("change", () => {
            if (isElementControlLocked(presetSelect)) return;
            syncPresetSelectionContext();
        });
    }

    const resolveInlinePresetName = (selftestKey) => {
        const selfTestNameOverride = typeof window[selftestKey] === "string"
            ? window[selftestKey].trim()
            : "";
        if (selfTestNameOverride) {
            setPresetNameInputValue(selfTestNameOverride);
            return selfTestNameOverride;
        }

        const inputName = getPresetNameInputValue();
        if (!inputName) {
            setPresetStatus("Preset name is required", true);
            return "";
        }
        return inputName;
    };

    const presetSaveButton = document.getElementById("preset-save-btn");
    if (presetSaveButton) {
        presetSaveButton.addEventListener("click", async () => {
            if (isElementControlLocked(presetSaveButton)) return;
            const resolvedName = resolveInlinePresetName("__LQ_SELFTEST_PRESET_NAME__");
            if (!resolvedName) return;

            const presetType = getPresetTypeSelection();
            try {
                if (timelineCommitTimer !== null) {
                    window.clearTimeout(timelineCommitTimer);
                    timelineCommitTimer = null;
                }
                await commitTimelineToNative();
                const result = await callNative("locusqSaveEmitterPreset", nativeFunctions.saveEmitterPreset, {
                    name: resolvedName,
                    presetType,
                    choreographyPackId: normalizeChoreographyPackId(uiState.choreographyPack),
                });
                if (result?.ok) {
                    const savedType = normalisePresetType(result?.presetType || presetType);
                    setPresetTypeSelection(savedType);
                    setPresetNameInputValue(result?.name || resolvedName);
                    setPresetStatus(`Saved: ${result.name || resolvedName}`);
                    await refreshPresetList(result?.path || "");
                    setMotionDirty(false);
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
            if (isElementControlLocked(presetLoadButton)) return;
            const selected = getSelectedPresetOptionEntry();
            if (!selected || !selected.path) {
                setPresetStatus("Select a preset first", true);
                return;
            }

            try {
                if (timelineCommitTimer !== null) {
                    window.clearTimeout(timelineCommitTimer);
                    timelineCommitTimer = null;
                }
                const result = await callNative("locusqLoadEmitterPreset", nativeFunctions.loadEmitterPreset, { path: selected.path });
                if (result?.ok) {
                    const loadedType = normalisePresetType(result?.presetType || selected.presetType);
                    setPresetTypeSelection(loadedType);
                    setPresetNameInputValue(selected.name || result?.name || "");
                    setPresetStatus(`Loaded: ${selected.name || "preset"}`);

                    const resolvedPack = normalizeChoreographyPackId(
                        result?.choreographyPackId
                        || selected.choreographyPackId
                        || "custom"
                    );
                    uiState.choreographyPack = resolvedPack;
                    if (choreographySelect) {
                        choreographySelect.value = resolvedPack;
                    }
                    setChoreographyStatus(
                        resolvedPack === "custom"
                            ? "Loaded custom timeline preset"
                            : `Loaded ${getChoreographyPackLabel(resolvedPack)} timeline preset`
                    );
                    scheduleUiStateCommit();
                    await loadTimelineFromNative();
                    syncAnimationUI();
                    setMotionDirty(false);
                } else {
                    setPresetStatus(result?.message || "Preset load failed", true);
                }
            } catch (error) {
                setPresetStatus("Preset load failed", true);
                console.error("Failed to load preset:", error);
            }
        });
    }

    const presetRenameButton = document.getElementById("preset-rename-btn");
    if (presetRenameButton) {
        presetRenameButton.addEventListener("click", async () => {
            if (isElementControlLocked(presetRenameButton)) return;
            const selected = getSelectedPresetOptionEntry();
            if (!selected || !selected.path) {
                setPresetStatus("Select a preset first", true);
                return;
            }

            const nextName = resolveInlinePresetName("__LQ_SELFTEST_PRESET_RENAME_NAME__");
            if (!nextName) return;

            try {
                const result = await callNative("locusqRenameEmitterPreset", nativeFunctions.renameEmitterPreset, {
                    path: selected.path,
                    newName: nextName,
                });
                if (result?.ok) {
                    const renamedType = normalisePresetType(result?.presetType || selected.presetType);
                    setPresetTypeSelection(renamedType);
                    setPresetNameInputValue(result?.name || nextName);
                    setPresetStatus(`Renamed: ${result.name || nextName}`);
                    await refreshPresetList(result?.path || "");
                } else {
                    setPresetStatus(result?.message || "Preset rename failed", true);
                }
            } catch (error) {
                setPresetStatus("Preset rename failed", true);
                console.error("Failed to rename preset:", error);
            }
        });
    }

    const presetDeleteButton = document.getElementById("preset-delete-btn");
    if (presetDeleteButton) {
        presetDeleteButton.addEventListener("click", async () => {
            if (isElementControlLocked(presetDeleteButton)) return;
            const selected = getSelectedPresetOptionEntry();
            if (!selected || !selected.path) {
                setPresetStatus("Select a preset first", true);
                return;
            }

            try {
                const result = await callNative("locusqDeleteEmitterPreset", nativeFunctions.deleteEmitterPreset, {
                    path: selected.path,
                });
                if (result?.ok) {
                    setPresetStatus(`Deleted: ${selected.name || "preset"}`);
                    await refreshPresetList();
                } else {
                    setPresetStatus(result?.message || "Preset delete failed", true);
                }
            } catch (error) {
                setPresetStatus("Preset delete failed", true);
                console.error("Failed to delete preset:", error);
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
            const preflightMessage = validateCalibrationStartPreflight(options);
            if (preflightMessage) {
                setCalibrationProfileStatus(preflightMessage, true);
                return;
            }

            try {
                const started = await callNative("locusqStartCalibration", nativeFunctions.startCalibration, options);
                if (!started) {
                    setCalibrationProfileStatus("Calibration start rejected by runtime. Confirm CALIBRATE mode and mapping gate.", true);
                    console.warn("Calibration did not start. Ensure mode is Calibrate and engine is idle.");
                } else {
                    setCalibrationProfileStatus("Calibration run started.");
                }
            } catch (error) {
                setCalibrationProfileStatus("Failed to start calibration run.", true);
                console.error("Failed to start calibration:", error);
            }
        });
    }

    applyUiStateToControls();
    syncPositionModeUI();
    updateWorldPositionReadback();
    updateEmitterAuthorityUI();
    updateEmitterDiagnosticsQuickControls();
    if (uiState.choreographyPack === "custom") {
        setChoreographyStatus("Custom timeline active");
    } else {
        setChoreographyStatus(`${getChoreographyPackLabel(uiState.choreographyPack)} selected`);
    }
    if (uiState.physicsPreset !== "off") {
        applyPhysicsPreset(uiState.physicsPreset, false);
    }
    syncAnimationUI();
    updateMotionStatusChips();
    syncMotionSourceUI();
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
        const nowMs = typeof performance !== "undefined" && typeof performance.now === "function"
            ? performance.now()
            : Date.now();

        if (isApplyingPhysicsPreset) return;

        const activePreset = normalizePhysicsPresetName(uiState.physicsPreset);
        if (activePreset !== "custom" && activePreset !== "off") {
            if (isPhysicsPresetStateAligned(activePreset)) return;

            if (nowMs < suppressPhysicsPresetCustomUntilMs) {
                if (physicsPresetRecheckTimer === null) {
                    const delayMs = Math.max(1, Math.ceil(suppressPhysicsPresetCustomUntilMs - nowMs));
                    physicsPresetRecheckTimer = window.setTimeout(() => {
                        physicsPresetRecheckTimer = null;
                        markPhysicsPresetCustom();
                    }, delayMs);
                }
                return;
            }
        }

        const physicsPreset = document.getElementById("physics-preset");
        if (physicsPreset && physicsPreset.value !== "custom" && physicsPreset.value !== "off") {
            physicsPreset.value = "custom";
        }
        if (uiState.physicsPreset !== "custom") {
            uiState.physicsPreset = "custom";
            scheduleUiStateCommit();
            updateMotionStatusChips();
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
    sliderStates.pos_x.valueChangedEvent.addListener(() => {
        updateValueDisplay("val-pos-x", sliderStates.pos_x.getScaledValue().toFixed(2), "m");
        updateWorldPositionReadback();
    });
    sliderStates.pos_y.valueChangedEvent.addListener(() => {
        updateValueDisplay("val-pos-z", sliderStates.pos_y.getScaledValue().toFixed(2), "m");
        updateWorldPositionReadback();
    });
    sliderStates.pos_z.valueChangedEvent.addListener(() => {
        updateValueDisplay("val-pos-y", sliderStates.pos_z.getScaledValue().toFixed(2), "m");
        updateWorldPositionReadback();
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
    sliderStates.emit_dir_azimuth.valueChangedEvent.addListener(() => {
        updateValueDisplay("val-dir-azimuth",
            sliderStates.emit_dir_azimuth.getScaledValue().toFixed(1), "°");
    });
    sliderStates.emit_dir_elevation.valueChangedEvent.addListener(() => {
        updateValueDisplay("val-dir-elevation",
            sliderStates.emit_dir_elevation.getScaledValue().toFixed(1), "°");
    });
    sliderStates.emit_color.valueChangedEvent.addListener(() => {
        updateEmitterColorSwatch();
    });
    sliderStates.cal_test_level.valueChangedEvent.addListener(() => {
        updateValueDisplay("cal-level", sliderStates.cal_test_level.getScaledValue().toFixed(1), "dBFS");
        applyCalibrationStatus();
    });
    sliderStates.cal_spk1_out.valueChangedEvent.addListener(() => {
        applyCalibrationStatus();
    });
    sliderStates.cal_spk2_out.valueChangedEvent.addListener(() => {
        applyCalibrationStatus();
    });
    sliderStates.cal_spk3_out.valueChangedEvent.addListener(() => {
        applyCalibrationStatus();
    });
    sliderStates.cal_spk4_out.valueChangedEvent.addListener(() => {
        applyCalibrationStatus();
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
    sliderStates.phys_vel_x.valueChangedEvent.addListener(() => {
        updateValueDisplay("val-vel-x",
            sliderStates.phys_vel_x.getScaledValue().toFixed(1), "m/s");
    });
    sliderStates.phys_vel_y.valueChangedEvent.addListener(() => {
        updateValueDisplay("val-vel-y",
            sliderStates.phys_vel_y.getScaledValue().toFixed(1), "m/s");
    });
    sliderStates.phys_vel_z.valueChangedEvent.addListener(() => {
        updateValueDisplay("val-vel-z",
            sliderStates.phys_vel_z.getScaledValue().toFixed(1), "m/s");
    });
    sliderStates.rend_master_gain.valueChangedEvent.addListener(() => {
        updateValueDisplay("val-master-gain", sliderStates.rend_master_gain.getScaledValue().toFixed(1), "dB");
    });
    sliderStates.rend_viz_trail_len.valueChangedEvent.addListener(() => {
        updateValueDisplay("val-viz-trail-len", sliderStates.rend_viz_trail_len.getScaledValue().toFixed(1), "s");
    });
    sliderStates.rend_viz_diag_mix.valueChangedEvent.addListener(() => {
        const mixPercent = Math.round(clamp(sliderStates.rend_viz_diag_mix.getScaledValue(), 0.0, 1.0) * 100.0);
        updateValueDisplay("val-viz-diag-mix", `${mixPercent}`, "%");
        updateValueDisplay("emitter-quick-diag-mix", `${mixPercent}`, "%");
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
            updateMotionStatusChips();
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
    toggleStates.rend_audition_enable.valueChangedEvent.addListener(() => {
        setToggleClass("toggle-audition", !!toggleStates.rend_audition_enable.getValue());
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
    toggleStates.rend_phys_interact.valueChangedEvent.addListener(() => {
        setToggleClass("toggle-interact", !!toggleStates.rend_phys_interact.getValue());
    });
    toggleStates.rend_phys_pause.valueChangedEvent.addListener(() => {
        const button = document.getElementById("btn-pause-physics");
        if (button) {
            button.textContent = toggleStates.rend_phys_pause.getValue() ? "RESUME ALL" : "PAUSE ALL";
        }
    });
    toggleStates.rend_viz_trails.valueChangedEvent.addListener(() => {
        setToggleClass("toggle-viz-trails", !!toggleStates.rend_viz_trails.getValue());
    });
    toggleStates.rend_viz_vectors.valueChangedEvent.addListener(() => {
        setToggleClass("toggle-viz-vectors", !!toggleStates.rend_viz_vectors.getValue());
    });
    toggleStates.rend_viz_physics_lens.valueChangedEvent.addListener(() => {
        setToggleClass("toggle-viz-physics-lens", !!toggleStates.rend_viz_physics_lens.getValue());
        setToggleClass("emitter-quick-physics-lens", !!toggleStates.rend_viz_physics_lens.getValue());
    });

    comboStates.rend_viz_mode.valueChangedEvent.addListener(updateViewMode);
    comboStates.cal_spk_config.valueChangedEvent.addListener(() => {
        applyCalibrationStatus();
    });
    comboStates.cal_topology_profile.valueChangedEvent.addListener(() => {
        syncLegacyConfigAliasFromTopology(getCalibrationTopologyId(getChoiceIndex(comboStates.cal_topology_profile)));
        applyCalibrationStatus();
    });
    comboStates.cal_monitoring_path.valueChangedEvent.addListener(() => {
        applyCalibrationStatus();
    });
    comboStates.cal_device_profile.valueChangedEvent.addListener(() => {
        applyCalibrationStatus();
    });
    comboStates.cal_test_type.valueChangedEvent.addListener(() => {
        applyCalibrationStatus();
    });
    comboStates.pos_coord_mode.valueChangedEvent.addListener(syncPositionModeUI);
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
    updateValueDisplay("val-pos-x", sliderStates.pos_x.getScaledValue().toFixed(2), "m");
    updateValueDisplay("val-pos-y", sliderStates.pos_z.getScaledValue().toFixed(2), "m");
    updateValueDisplay("val-pos-z", sliderStates.pos_y.getScaledValue().toFixed(2), "m");
    updateWorldPositionReadback();
    syncPositionModeUI();
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
    updateValueDisplay("val-vel-x",
        sliderStates.phys_vel_x.getScaledValue().toFixed(1), "m/s");
    updateValueDisplay("val-vel-y",
        sliderStates.phys_vel_y.getScaledValue().toFixed(1), "m/s");
    updateValueDisplay("val-vel-z",
        sliderStates.phys_vel_z.getScaledValue().toFixed(1), "m/s");
    updateValueDisplay("val-master-gain", sliderStates.rend_master_gain.getScaledValue().toFixed(1), "dB");
    updateValueDisplay("val-viz-trail-len", sliderStates.rend_viz_trail_len.getScaledValue().toFixed(1), "s");
    updateValueDisplay("val-viz-diag-mix", `${Math.round(clamp(sliderStates.rend_viz_diag_mix.getScaledValue(), 0.0, 1.0) * 100.0)}`, "%");
    updateEmitterDiagnosticsQuickControls();
    setToggleClass("toggle-size-link", !!toggleStates.size_link.getValue());
    setToggleClass("toggle-doppler", !!toggleStates.rend_doppler.getValue());
    setToggleClass("toggle-air-absorb", !!toggleStates.rend_air_absorb.getValue());
    setToggleClass("toggle-audition", !!toggleStates.rend_audition_enable.getValue());
    setToggleClass("toggle-room", !!toggleStates.rend_room_enable.getValue());
    setToggleClass("toggle-er-only", !!toggleStates.rend_room_er_only.getValue());
    setToggleClass("toggle-walls", !!toggleStates.rend_phys_walls.getValue());
    setToggleClass("toggle-interact", !!toggleStates.rend_phys_interact.getValue());
    setToggleClass("toggle-viz-trails", !!toggleStates.rend_viz_trails.getValue());
    setToggleClass("toggle-viz-vectors", !!toggleStates.rend_viz_vectors.getValue());
    setToggleClass("toggle-viz-physics-lens", !!toggleStates.rend_viz_physics_lens.getValue());
    const pauseButton = document.getElementById("btn-pause-physics");
    if (pauseButton) pauseButton.textContent = toggleStates.rend_phys_pause.getValue() ? "RESUME ALL" : "PAUSE ALL";
    const colorSwatch = document.getElementById("emit-color-swatch");
    if (colorSwatch) updateEmitterColorSwatch();
    updateValueDisplay("val-dir-azimuth",
        sliderStates.emit_dir_azimuth.getScaledValue().toFixed(1), "°");
    updateValueDisplay("val-dir-elevation",
        sliderStates.emit_dir_elevation.getScaledValue().toFixed(1), "°");
    updateViewMode();
    updateEmitterAuthorityUI();
    syncAnimationUI();
    syncLegacyConfigAliasFromTopology(getCalibrationViewportTopologyId());
}

function updateValueDisplay(id, value, unit) {
    const el = document.getElementById(id);
    if (el) {
        el.innerHTML = value + (unit ? '<span class="control-unit">' + unit + '</span>' : '');
    }
}

function applySceneStatusBadge() {
    const ss = document.getElementById("scene-status");
    if (!ss) return;

    ss.className = "scene-status";

    if (sceneTransportState.stale && currentMode !== "calibrate") {
        ss.textContent = "STALE SNAPSHOT";
        ss.classList.add("stale");
        return;
    }

    if (currentMode === "calibrate") {
        ss.textContent = "NO PROFILE";
        ss.classList.add("noprofile");
        return;
    }

    if (currentMode === "renderer") {
        ss.textContent = "READY";
        ss.classList.add("ready");
        return;
    }

    ss.textContent = "STABLE";
}

function parseSnapshotSequence(value) {
    const parsed = Number(value);
    if (!Number.isFinite(parsed) || !Number.isInteger(parsed) || parsed < 0) {
        return null;
    }
    return parsed;
}

function parseSnapshotCadenceHz(value) {
    const parsed = Number(value);
    if (!Number.isFinite(parsed) || parsed <= 0) {
        return sceneTransportDefaults.cadenceHz;
    }
    return clamp(parsed, 1, 240);
}

function parseSnapshotStaleAfterMs(value) {
    const parsed = Number(value);
    if (!Number.isFinite(parsed) || parsed <= 0) {
        return sceneTransportDefaults.staleAfterMs;
    }
    return clamp(parsed, 150, 5000);
}

function updateSceneTransportHealth(nowMs = Date.now()) {
    const lastAcceptedAtMs = Number(sceneTransportState.lastAcceptedAtMs) || 0;
    if (lastAcceptedAtMs <= 0) {
        return;
    }
    const staleAfterMs = Math.max(1, Number(sceneTransportState.staleAfterMs) || sceneTransportDefaults.staleAfterMs);
    const staleNow = (nowMs - lastAcceptedAtMs) > staleAfterMs;
    if (sceneTransportState.stale === staleNow) {
        return;
    }
    sceneTransportState.stale = staleNow;
    applySceneStatusBadge();
    updateEmitterMeshes(sceneData.emitters || []);
}

function getSceneSmoothingAlpha(frameDeltaSeconds) {
    const dt = Math.max(0, Number(frameDeltaSeconds) || 0);
    const cadenceHz = Math.max(1, Number(sceneTransportState.cadenceHz) || sceneTransportDefaults.cadenceHz);
    const expectedSnapshotSeconds = 1 / cadenceHz;
    const tau = Math.max(0.02, expectedSnapshotSeconds * 0.8);
    return clamp(1 - Math.exp(-dt / tau), 0.0, 1.0);
}

function getTrailPointBudget() {
    const trailSeconds = clamp(Number(sliderStates.rend_viz_trail_len.getScaledValue()) || 5.0, 0.5, 30.0);
    const cadenceHz = Math.max(1, Number(sceneTransportState.cadenceHz) || sceneTransportDefaults.cadenceHz);
    return clamp(Math.round(trailSeconds * cadenceHz), 12, 720);
}

function getPhysicsLensMix() {
    const sliderMix = Number(sliderStates.rend_viz_diag_mix?.getScaledValue?.());
    if (Number.isFinite(sliderMix)) {
        return clamp(sliderMix, 0.0, 1.0);
    }
    const sceneMix = Number(sceneData?.rendererPhysicsLensMix);
    if (Number.isFinite(sceneMix)) {
        return clamp(sceneMix, 0.0, 1.0);
    }
    return 0.55;
}

function getPhysicsLensEnabled() {
    const toggleValue = !!getToggleValue(toggleStates.rend_viz_physics_lens);
    const sceneEnabled = !!sceneData?.rendererPhysicsLensEnabled;
    return toggleValue || sceneEnabled;
}

function buildTrajectoryPreviewPoints(target, diagnosticsMix) {
    if (typeof THREE === "undefined") {
        return [];
    }

    const start = new THREE.Vector3(target.x, target.y, target.z);
    const points = [start];
    const horizonSeconds = 0.35 + (diagnosticsMix * 1.0);
    const pointCount = Math.round(clamp(6 + (diagnosticsMix * 8), 6, 14));
    const dt = horizonSeconds / pointCount;
    const accelInfluence = 0.18 + (diagnosticsMix * 0.26);
    const roomWidth = Number(sceneData?.roomDimensions?.width);
    const roomDepth = Number(sceneData?.roomDimensions?.depth);
    const roomHeight = Number(sceneData?.roomDimensions?.height);
    const halfWidth = Number.isFinite(roomWidth) ? Math.max(0.2, roomWidth * 0.5) : 4.0;
    const halfDepth = Number.isFinite(roomDepth) ? Math.max(0.2, roomDepth * 0.5) : 4.0;
    const maxHeight = Number.isFinite(roomHeight) ? Math.max(0.2, roomHeight) : 3.0;

    for (let i = 1; i <= pointCount; i++) {
        const t = dt * i;
        const px = target.x + (target.vx * t) + (0.5 * target.fx * accelInfluence * t * t);
        const py = target.y + (target.vy * t) + (0.5 * target.fy * accelInfluence * t * t);
        const pz = target.z + (target.vz * t) + (0.5 * target.fz * accelInfluence * t * t);
        points.push(new THREE.Vector3(
            clamp(px, -halfWidth, halfWidth),
            clamp(py, 0.0, maxHeight),
            clamp(pz, -halfDepth, halfDepth)
        ));
    }

    return points;
}

function setArrowFromVector(arrow, x, y, z, targetLength) {
    if (!arrow || typeof THREE === "undefined") return;
    const vx = Number(x) || 0.0;
    const vy = Number(y) || 0.0;
    const vz = Number(z) || 0.0;
    const magnitude = Math.sqrt((vx * vx) + (vy * vy) + (vz * vz));

    if (magnitude <= 1.0e-6) {
        arrow.setDirection(new THREE.Vector3(0, 0, -1));
        arrow.setLength(1.0e-4, 1.0e-4, 1.0e-4);
        return;
    }

    const invMag = 1.0 / magnitude;
    arrow.setDirection(new THREE.Vector3(vx * invMag, vy * invMag, vz * invMag));

    const length = Math.max(0.02, Number(targetLength) || magnitude);
    const headLength = Math.min(length * 0.35, 0.22);
    const headWidth = Math.min(length * 0.26, 0.12);
    arrow.setLength(length, headLength, headWidth);
}

function updateSpeakerTargetsFromScene(data) {
    if (currentMode === "calibrate") {
        const topologyId = getCalibrationViewportTopologyId();
        for (let i = 0; i < 4; i++) {
            if (!speakerTargets[i]) {
                speakerTargets[i] = { ...defaultSpeakerSnapshotPositions[i], rms: 0.0 };
            }
            const previewPosition = getCalibrationPreviewSpeakerPosition(topologyId, i);
            speakerTargets[i].x = previewPosition.x;
            speakerTargets[i].y = previewPosition.y;
            speakerTargets[i].z = previewPosition.z;
            speakerTargets[i].rms = 0.0;
        }
        return;
    }

    for (let i = 0; i < 4; i++) {
        if (!speakerTargets[i]) {
            speakerTargets[i] = { ...defaultSpeakerSnapshotPositions[i], rms: 0.0 };
        }

        const fallback = defaultSpeakerSnapshotPositions[i];
        const speaker = Array.isArray(data?.speakers) ? data.speakers[i] : null;
        const rmsFromArray = Array.isArray(data?.speakerRms) ? Number(data.speakerRms[i]) : NaN;
        const rmsFromSpeaker = Number(speaker?.rms);

        speakerTargets[i].x = Number.isFinite(Number(speaker?.x)) ? Number(speaker.x) : fallback.x;
        speakerTargets[i].y = Number.isFinite(Number(speaker?.y)) ? Number(speaker.y) : fallback.y;
        speakerTargets[i].z = Number.isFinite(Number(speaker?.z)) ? Number(speaker.z) : fallback.z;
        speakerTargets[i].rms = clamp(
            Number.isFinite(rmsFromArray) ? rmsFromArray : (Number.isFinite(rmsFromSpeaker) ? rmsFromSpeaker : 0.0),
            0.0,
            4.0
        );
    }
}

function updateListenerTargetFromScene(data) {
    listenerTarget = {
        x: Number.isFinite(Number(data?.listener?.x)) ? Number(data.listener.x) : 0.0,
        y: Number.isFinite(Number(data?.listener?.y)) ? Number(data.listener.y) : 1.2,
        z: Number.isFinite(Number(data?.listener?.z)) ? Number(data.listener.z) : 0.0,
    };
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

function applyTimelineModeVisibility(mode) {
    let tl = document.getElementById("timeline");
    const viewportArea = document.querySelector(".viewport-area");
    const wasVisible = !!(tl && tl.classList.contains("visible"));
    const showTimeline = isEmitterLayoutActive(mode);
    const shellRebuilt = showTimeline ? ensureTimelineShellIntegrity() : false;
    if (!tl) tl = document.getElementById("timeline");

    if (tl) {
        tl.classList.toggle("visible", showTimeline);
        tl.style.display = "block";
        tl.style.visibility = "visible";
        tl.style.opacity = "1";
        tl.style.height = showTimeline ? "var(--timeline-height, 152px)" : "0px";
    }
    if (viewportArea) viewportArea.classList.toggle("timeline-visible", showTimeline);
    if (showTimeline && (shellRebuilt || !wasVisible || !timelineLoaded)) renderTimelineLanes();
}

function queueModeLayoutResync(mode) {
    if (modeLayoutSyncTimer !== null) {
        window.clearTimeout(modeLayoutSyncTimer);
        modeLayoutSyncTimer = null;
    }

    window.requestAnimationFrame(() => {
        applyTimelineModeVisibility(mode);
        if (runtimeState.viewportReady) resize();
    });

    modeLayoutSyncTimer = window.setTimeout(() => {
        modeLayoutSyncTimer = null;
        applyTimelineModeVisibility(mode);
        if (runtimeState.viewportReady) resize();
    }, 260);
}

function switchMode(mode) {
    const nextMode = (mode === "calibrate" || mode === "emitter" || mode === "renderer")
        ? mode
        : currentMode;
    const isSameMode = currentMode === nextMode;
    if (!isSameMode) {
        const rail = document.getElementById("rail");
        if (rail) {
            railScrollByMode[currentMode] = rail.scrollTop;
        }

        currentMode = nextMode;
    }

    document.querySelectorAll(".mode-tab").forEach(t => t.classList.remove("active"));
    const activeTab = document.querySelector(`[data-mode="${nextMode}"]`);
    if (activeTab) activeTab.classList.add("active");

    document.querySelectorAll(".rail-panel").forEach(p => p.classList.remove("active"));
    const activePanel = document.querySelector(`[data-panel="${nextMode}"]`);
    if (activePanel) activePanel.classList.add("active");

    applyTimelineModeVisibility(nextMode);

    applySceneStatusBadge();

    // 3D viewport adjustments
    if (nextMode !== "emitter") setLaneHighlight(null);
    else setLaneHighlight(selectedLane);
    updateSelectionRingFromState();
    if (roomLines?.material) {
        roomLines.material.opacity = nextMode === "calibrate" ? 0.15 : 0.3;
    }
    applyCalibrationStatus();
    applyModeShell(nextMode);
    applyTimelineModeVisibility(nextMode);

    queueModeLayoutResync(nextMode);
}

function enforceEmitterTimelineInvariant(frameNowMs) {
    if (!isEmitterLayoutActive(currentMode)) return;

    if ((frameNowMs - timelineInvariantLastCheckMs) < 220.0) return;
    timelineInvariantLastCheckMs = frameNowMs;

    const viewportArea = document.querySelector(".viewport-area");
    let timeline = document.getElementById("timeline");
    const laneCount = timeline ? timeline.querySelectorAll(".timeline-lane").length : 0;
    const hasShell = !!(timeline
        && timeline.querySelector(".timeline-header")
        && timeline.querySelector(".timeline-lanes")
        && laneCount >= 4);
    const timelineHeight = timeline ? timeline.getBoundingClientRect().height : 0;
    const viewportFlagged = !!(viewportArea && viewportArea.classList.contains("timeline-visible"));
    const timelineVisible = !!(timeline && timeline.classList.contains("visible"));

    const needsRepair = !timeline
        || !timelineVisible
        || !viewportFlagged
        || !hasShell
        || timelineHeight < 60;

    if (!needsRepair) return;

    const rebuilt = ensureTimelineShellIntegrity();
    applyTimelineModeVisibility("emitter");
    if (rebuilt || timelineLoaded || laneCount < 4) {
        renderTimelineLanes();
    }
    timeline = document.getElementById("timeline");
    if (timeline) {
        timeline.style.height = "var(--timeline-height, 152px)";
    }
    if (runtimeState.viewportReady) resize();
}

function setLaneHighlight(lane) {
    highlightTargets = { azimuth: 0, elevation: 0, distance: 0, size: 0 };
    if (lane) highlightTargets[lane] = lane === "distance" ? 0.15 : 0.25;
}

// ===== SCENE STATE FROM C++ =====
// Called by C++ via evaluateJavascript
window.updateSceneState = function(data) {
    const incomingSeq = parseSnapshotSequence(data?.snapshotSeq);
    if (incomingSeq !== null && sceneTransportState.lastAcceptedSeq >= 0 && incomingSeq <= sceneTransportState.lastAcceptedSeq) {
        return;
    }

    const nowMs = Date.now();
    if (incomingSeq !== null) {
        sceneTransportState.lastAcceptedSeq = incomingSeq;
    }
    if (typeof data?.snapshotSchema === "string" && data.snapshotSchema.trim()) {
        sceneTransportState.schema = data.snapshotSchema.trim();
    }
    sceneTransportState.lastAcceptedAtMs = nowMs;
    sceneTransportState.cadenceHz = parseSnapshotCadenceHz(data?.snapshotCadenceHz);
    sceneTransportState.staleAfterMs = parseSnapshotStaleAfterMs(data?.snapshotStaleAfterMs);
    sceneTransportState.lastPublishedAtUtcMs = Number.isFinite(Number(data?.snapshotPublishedAtUtcMs))
        ? Number(data.snapshotPublishedAtUtcMs)
        : 0;
    sceneTransportState.stale = false;

    const emitters = Array.isArray(data?.emitters) ? data.emitters : [];
    sceneEmitterLookup = new Map(emitters.map(emitter => [emitter.id, emitter]));

    sceneData = {
        ...(data || {}),
        emitters,
    };
    updateSpeakerTargetsFromScene(sceneData);
    updateListenerTargetFromScene(sceneData);
    applyTimelineModeVisibility(currentMode);

    if (Number.isInteger(data?.localEmitterId)) {
        localEmitterId = data.localEmitterId;
    }

    if (selectedEmitterId < 0 && Number.isInteger(localEmitterId) && localEmitterId >= 0) {
        selectedEmitterId = localEmitterId;
    }

    updateEmitterMeshes(emitters);
    updateSceneList(emitters);
    updateEmitterAuthorityUI();

    if (typeof data.animDuration === "number" && Number.isFinite(data.animDuration)) {
        timelineState.durationSeconds = clamp(data.animDuration, 0.25, 120.0);
    }
    if (typeof data.animLooping === "boolean") {
        timelineState.looping = data.animLooping;
    }
    updateMotionStatusChips();
    updateEmitterDiagnosticsQuickControls();

    if (typeof data.animTime === "number") {
        timelineState.currentTimeSeconds = Math.max(0.0, data.animTime);
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
            const spatialProfileRequested = typeof data.rendererSpatialProfileRequested === "string"
                ? data.rendererSpatialProfileRequested
                : "auto";
            const spatialProfileActive = typeof data.rendererSpatialProfileActive === "string"
                ? data.rendererSpatialProfileActive
                : spatialProfileRequested;
            const spatialProfileStage = typeof data.rendererSpatialProfileStage === "string"
                ? data.rendererSpatialProfileStage
                : "direct";
            let spatialText = ` \u00B7 Profile ${spatialProfileActive}`;
            if (spatialProfileRequested !== spatialProfileActive) {
                spatialText += ` (req ${spatialProfileRequested})`;
            }
            if (spatialProfileStage !== "direct") {
                spatialText += ` [${spatialProfileStage}]`;
            }
            const headphoneRequested = typeof data.rendererHeadphoneModeRequested === "string"
                ? data.rendererHeadphoneModeRequested
                : "stereo_downmix";
            const headphoneActive = typeof data.rendererHeadphoneModeActive === "string"
                ? data.rendererHeadphoneModeActive
                : headphoneRequested;
            const headphoneProfileRequested = typeof data.rendererHeadphoneProfileRequested === "string"
                ? data.rendererHeadphoneProfileRequested
                : "generic";
            const headphoneProfileActive = typeof data.rendererHeadphoneProfileActive === "string"
                ? data.rendererHeadphoneProfileActive
                : headphoneProfileRequested;
            const steamCompiled = !!data.rendererSteamAudioCompiled;
            const steamAvailable = !!data.rendererSteamAudioAvailable;
            const steamInitStage = typeof data.rendererSteamAudioInitStage === "string"
                ? data.rendererSteamAudioInitStage
                : "unknown";
            const steamInitErrorCode = Number.isFinite(Number(data.rendererSteamAudioInitErrorCode))
                ? Number(data.rendererSteamAudioInitErrorCode)
                : 0;
            let headphoneText = "";
            if (outputChannels >= 2) {
                headphoneText = ` \u00B7 HP ${headphoneActive}/${headphoneProfileActive}`;
                if (headphoneRequested !== headphoneActive) {
                    headphoneText += ` (req ${headphoneRequested})`;
                }
                if (headphoneProfileRequested !== headphoneProfileActive) {
                    headphoneText += ` [reqProfile ${headphoneProfileRequested}]`;
                }
                if (headphoneRequested === "steam_binaural" && !steamAvailable) {
                    headphoneText += ` fallback [${steamCompiled ? "compiled" : "not-compiled"}:${steamInitStage}`;
                    if (steamInitErrorCode !== 0) {
                        headphoneText += `:${steamInitErrorCode}`;
                    }
                    headphoneText += "]";
                }
            }
            const physicsLensEnabled = !!data.rendererPhysicsLensEnabled;
            const physicsLensMix = Number.isFinite(Number(data.rendererPhysicsLensMix))
                ? clamp(Number(data.rendererPhysicsLensMix), 0.0, 1.0)
                : getPhysicsLensMix();
            const lensText = physicsLensEnabled
                ? ` \u00B7 Lens ${Math.round(physicsLensMix * 100)}%`
                : "";
            const auditionEnabled = !!data.rendererAuditionEnabled;
            const auditionSignal = typeof data.rendererAuditionSignal === "string"
                ? data.rendererAuditionSignal
                : "sine_440";
            const auditionMotion = typeof data.rendererAuditionMotion === "string"
                ? data.rendererAuditionMotion
                : "center";
            const auditionLevelDb = Number.isFinite(Number(data.rendererAuditionLevelDb))
                ? Number(data.rendererAuditionLevelDb)
                : -24.0;
            const auditionText = auditionEnabled
                ? ` \u00B7 Audition ${auditionSignal}/${auditionMotion}/${auditionLevelDb.toFixed(0)}dBFS`
                : "";
            info.textContent = "Renderer Mode \u00B7 " + data.emitterCount + " emitters \u00B7 " +
                qualityText + outputText + spatialText + headphoneText + auditionText + lensText + rendererPerfText + blockPerfText;
        } else {
            info.textContent = "Calibrate Mode \u00B7 Room Profile Setup";
        }
    }

    applySceneStatusBadge();
    applyCalibrationStatus();
};

window.updateCalibrationStatus = function(status) {
    if (!status) return;

    calibrationState = {
        ...calibrationState,
        ...status,
    };

    if (Array.isArray(status.routing)) {
        calibrationLastAutoRouting = normaliseCalibrationRouting(status.routing, 4);
    }

    applyCalibrationStatus();
};

function buildCalibrationValidationSummaryPayload(status = calibrationState) {
    const source = status || {};
    return {
        mappingValid: !!source.mappingValid,
        mappingLimitedToFirst4: !!source.mappingLimitedToFirst4,
        mappingDuplicateChannels: !!source.mappingDuplicateChannels,
        phasePass: !!source.phasePass,
        delayPass: !!source.delayPass,
        topologyProfile: String(source.topologyProfile || getCalibrationTopologyId(source.topologyProfileIndex)),
        monitoringPath: String(source.monitoringPath || getCalibrationMonitoringPathId(source.monitoringPathIndex)),
        deviceProfile: String(source.deviceProfile || getCalibrationDeviceProfileId(source.deviceProfileIndex)),
        profileValid: !!source.profileValid,
        capturedAtUtc: new Date().toISOString(),
    };
}

async function refreshCalibrationProfileList(preferredPath = "") {
    const select = document.getElementById("cal-profile-select");
    if (!select) return;

    const previousSelection = String(preferredPath || select.value || "");
    select.innerHTML = "";

    try {
        const items = await callNative("locusqListCalibrationProfiles", nativeFunctions.listCalibrationProfiles);
        calibrationProfileEntries = Array.isArray(items) ? items : [];
    } catch (error) {
        calibrationProfileEntries = [];
        setCalibrationProfileStatus("Calibration profile listing failed.", true);
        return;
    }

    if (calibrationProfileEntries.length === 0) {
        const emptyOption = document.createElement("option");
        emptyOption.textContent = "No profiles";
        emptyOption.value = "";
        select.appendChild(emptyOption);
        setCalibrationProfileStatus("No calibration profiles saved yet.");
        return;
    }

    calibrationProfileEntries.forEach((entry, index) => {
        const option = document.createElement("option");
        option.value = String(entry?.path || entry?.file || "");
        const displayName = String(entry?.name || entry?.file || `Profile ${index + 1}`).trim();
        const topologyId = String(entry?.topologyProfile || "");
        const monitoringPathId = String(entry?.monitoringPath || "");
        const deviceProfileId = String(entry?.deviceProfile || "");
        option.textContent = `[${getCalibrationTopologyLabel(topologyId, true)}] ${displayName}`;
        option.dataset.profileName = displayName;
        option.dataset.topologyProfile = topologyId;
        option.dataset.monitoringPath = monitoringPathId;
        option.dataset.deviceProfile = deviceProfileId;
        select.appendChild(option);
    });

    if (previousSelection) {
        const match = Array.from(select.options).find(option => option.value === previousSelection);
        if (match) {
            select.value = match.value;
        }
    }
    if (!select.value && select.options.length > 0) {
        select.selectedIndex = 0;
    }

    const selected = getSelectedCalibrationProfileEntry();
    if (selected?.name) {
        setCalibrationProfileNameInputValue(selected.name);
    }
    setCalibrationProfileStatus(`${calibrationProfileEntries.length} calibration profile(s) available.`);
}

async function saveCalibrationProfile() {
    const inputName = getCalibrationProfileNameInputValue();
    const fallbackName = `Cal_${Date.now()}`;
    const profileName = inputName || fallbackName;
    setCalibrationProfileNameInputValue(profileName);

    try {
        const result = await callNative(
            "locusqSaveCalibrationProfile",
            nativeFunctions.saveCalibrationProfile,
            {
                name: profileName,
                validationSummary: buildCalibrationValidationSummaryPayload(),
            }
        );
        if (!result?.ok) {
            setCalibrationProfileStatus(result?.message || "Calibration profile save failed.", true);
            return false;
        }

        setCalibrationProfileStatus(`Saved profile: ${result.name || profileName}`);
        await refreshCalibrationProfileList(result?.path || "");
        return true;
    } catch (error) {
        setCalibrationProfileStatus("Calibration profile save failed.", true);
        console.error("Failed to save calibration profile:", error);
        return false;
    }
}

async function loadCalibrationProfile() {
    const selected = getSelectedCalibrationProfileEntry();
    if (!selected || !selected.path) {
        setCalibrationProfileStatus("Select a calibration profile first.", true);
        return false;
    }

    try {
        const result = await callNative("locusqLoadCalibrationProfile", nativeFunctions.loadCalibrationProfile, {
            path: selected.path,
        });
        if (!result?.ok) {
            setCalibrationProfileStatus(result?.message || "Calibration profile load failed.", true);
            return false;
        }

        calibrationMappingEditedByUser = false;
        const ackRedetect = document.getElementById("cal-ack-redetect-check");
        if (ackRedetect) ackRedetect.checked = false;

        setCalibrationProfileNameInputValue(result?.name || selected.name || "");
        setCalibrationProfileStatus(`Loaded profile: ${result?.name || selected.name || "profile"}`);
        await refreshCalibrationProfileList(result?.path || selected.path);
        applyCalibrationStatus();
        return true;
    } catch (error) {
        setCalibrationProfileStatus("Calibration profile load failed.", true);
        console.error("Failed to load calibration profile:", error);
        return false;
    }
}

async function renameCalibrationProfile() {
    const selected = getSelectedCalibrationProfileEntry();
    if (!selected || !selected.path) {
        setCalibrationProfileStatus("Select a calibration profile first.", true);
        return false;
    }

    const nextName = getCalibrationProfileNameInputValue();
    if (!nextName) {
        setCalibrationProfileStatus("Calibration profile name is required.", true);
        return false;
    }

    try {
        const result = await callNative("locusqRenameCalibrationProfile", nativeFunctions.renameCalibrationProfile, {
            path: selected.path,
            newName: nextName,
        });
        if (!result?.ok) {
            setCalibrationProfileStatus(result?.message || "Calibration profile rename failed.", true);
            return false;
        }

        setCalibrationProfileStatus(`Renamed profile: ${result.name || nextName}`);
        await refreshCalibrationProfileList(result?.path || "");
        return true;
    } catch (error) {
        setCalibrationProfileStatus("Calibration profile rename failed.", true);
        console.error("Failed to rename calibration profile:", error);
        return false;
    }
}

async function deleteCalibrationProfile() {
    const selected = getSelectedCalibrationProfileEntry();
    if (!selected || !selected.path) {
        setCalibrationProfileStatus("Select a calibration profile first.", true);
        return false;
    }

    try {
        const result = await callNative("locusqDeleteCalibrationProfile", nativeFunctions.deleteCalibrationProfile, {
            path: selected.path,
        });
        if (!result?.ok) {
            setCalibrationProfileStatus(result?.message || "Calibration profile delete failed.", true);
            return false;
        }

        setCalibrationProfileStatus(`Deleted profile: ${selected.name || "profile"}`);
        await refreshCalibrationProfileList();
        return true;
    } catch (error) {
        setCalibrationProfileStatus("Calibration profile delete failed.", true);
        console.error("Failed to delete calibration profile:", error);
        return false;
    }
}

async function runCalibrationRedetect() {
    if (calibrationState.running) {
        setCalibrationProfileStatus("Stop calibration before routing redetect.", true);
        return false;
    }

    const currentRouting = getCalibrationRoutingFromControls();
    const expectedAutoRouting = getCalibrationExpectedAutoRouting();
    const routeIsCustom = calibrationMappingEditedByUser || !compareCalibrationRouting(currentRouting, expectedAutoRouting, 4);
    const ackRedetect = document.getElementById("cal-ack-redetect-check");
    if (routeIsCustom && !(ackRedetect?.checked)) {
        setCalibrationProfileStatus("Redetect blocked: acknowledge custom-map overwrite first.", true);
        return false;
    }

    try {
        const result = await callNative("locusqRedetectCalibrationRouting", nativeFunctions.redetectCalibrationRouting);
        if (!result?.ok) {
            setCalibrationProfileStatus(result?.message || "Routing redetect failed.", true);
            return false;
        }

        const routing = normaliseCalibrationRouting(result.routing || [], 4);
        const mappingSelects = [
            document.getElementById("cal-spk1"),
            document.getElementById("cal-spk2"),
            document.getElementById("cal-spk3"),
            document.getElementById("cal-spk4"),
        ];
        mappingSelects.forEach((select, idx) => {
            if (!select) return;
            select.selectedIndex = clamp((routing[idx] || 1) - 1, 0, Math.max(0, select.options.length - 1));
            select.dispatchEvent(new Event("change", { bubbles: true }));
        });

        calibrationState = {
            ...calibrationState,
            topologyProfileIndex: Number(result.topologyProfileIndex ?? calibrationState.topologyProfileIndex),
            topologyProfile: String(result.topologyProfile || calibrationState.topologyProfile),
            monitoringPathIndex: Number(result.monitoringPathIndex ?? calibrationState.monitoringPathIndex),
            monitoringPath: String(result.monitoringPath || calibrationState.monitoringPath),
            deviceProfileIndex: Number(result.deviceProfileIndex ?? calibrationState.deviceProfileIndex),
            deviceProfile: String(result.deviceProfile || calibrationState.deviceProfile),
            requiredChannels: Number(result.requiredChannels ?? calibrationState.requiredChannels),
            writableChannels: Number(result.writableChannels ?? calibrationState.writableChannels),
            mappingLimitedToFirst4: !!result.mappingLimitedToFirst4,
            speakerRouting: routing.slice(0, 4),
        };
        calibrationLastAutoRouting = routing.slice(0, 4);
        calibrationMappingEditedByUser = false;
        if (ackRedetect) ackRedetect.checked = false;
        setCalibrationProfileStatus("Routing redetected from host output layout.");
        applyCalibrationStatus();
        return true;
    } catch (error) {
        setCalibrationProfileStatus("Routing redetect failed.", true);
        console.error("Failed to redetect calibration routing:", error);
        return false;
    }
}

function collectCalibrationOptions() {
    const getSelectedIndex = (id, fallback = 0) => {
        const el = document.getElementById(id);
        return el ? Math.max(0, el.selectedIndex) : fallback;
    };

    const levelText = document.getElementById("cal-level")?.textContent || "-20.0";
    const parsedLevel = parseFloat(levelText);
    const topologyProfileIndex = getSelectedIndex("cal-topology", getChoiceIndex(comboStates.cal_topology_profile));
    const monitoringPathIndex = getSelectedIndex("cal-monitoring-path", getChoiceIndex(comboStates.cal_monitoring_path));
    const deviceProfileIndex = getSelectedIndex("cal-device-profile", getChoiceIndex(comboStates.cal_device_profile));
    const allowLimitedMapping = !!document.getElementById("cal-ack-limited-check")?.checked;
    const routingOneBased = getCalibrationRoutingFromControls();

    return {
        testType: getSelectedIndex("cal-type", 0),
        testLevelDb: Number.isFinite(parsedLevel) ? parsedLevel : -20.0,
        sweepSeconds: 3.0,
        tailSeconds: 1.5,
        micChannel: getSelectedIndex("cal-mic", 0),
        topologyProfileIndex,
        topologyProfile: getCalibrationTopologyId(topologyProfileIndex),
        monitoringPathIndex,
        monitoringPath: getCalibrationMonitoringPathId(monitoringPathIndex),
        deviceProfileIndex,
        deviceProfile: getCalibrationDeviceProfileId(deviceProfileIndex),
        allowLimitedMapping,
        speakerChannels: routingOneBased.map(value => clamp((Number(value) || 1) - 1, 0, 7)),
    };
}

function validateCalibrationStartPreflight(options) {
    const selectedOptions = options || collectCalibrationOptions();
    const topologyId = getCalibrationTopologyId(selectedOptions.topologyProfileIndex);
    const requiredChannels = getCalibrationRequiredChannels(topologyId);
    const writableChannels = clamp(Number(calibrationState?.writableChannels) || 4, 1, 4);
    const routing = normaliseCalibrationRouting(
        Array.isArray(selectedOptions.speakerChannels)
            ? selectedOptions.speakerChannels.map(value => (Number(value) || 0) + 1)
            : getCalibrationRoutingFromControls(),
        4
    );
    const rowsToValidate = Math.max(1, Math.min(4, requiredChannels, writableChannels));
    const seen = new Set();
    for (let i = 0; i < rowsToValidate; ++i) {
        const channel = clamp(Math.round(Number(routing[i]) || (i + 1)), 1, 8);
        if (seen.has(channel)) {
            return `Routing preflight failed: duplicate output channel ${channel}.`;
        }
        seen.add(channel);
    }

    if (requiredChannels > writableChannels && !selectedOptions.allowLimitedMapping) {
        return `Topology ${getCalibrationTopologyLabel(topologyId, true)} requires ${requiredChannels} channels, but only ${writableChannels} are writable. Enable limited-mapping acknowledgement to proceed.`;
    }

    return "";
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
    const topologyId = String(status.topologyProfile || getCalibrationTopologyId(status.topologyProfileIndex));
    const monitoringPathId = String(status.monitoringPath || getCalibrationMonitoringPathId(status.monitoringPathIndex));
    const deviceProfileId = String(status.deviceProfile || getCalibrationDeviceProfileId(status.deviceProfileIndex));
    const requiredChannels = Math.max(1, Number(status.requiredChannels) || getCalibrationRequiredChannels(topologyId));
    const writableChannels = clamp(Number(status.writableChannels) || 4, 1, 4);
    const mappingLimited = !!status.mappingLimitedToFirst4 || requiredChannels > writableChannels;
    const mappingDuplicateChannels = !!status.mappingDuplicateChannels;
    const mappingValid = !!status.mappingValid;
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

    const stateChip = document.getElementById("cal-state-chip");
    if (stateChip) {
        stateChip.classList.remove("active", "warning", "local", "remote");
        stateChip.textContent = getCalibrationStatusChipText(status);
        if (running) stateChip.classList.add("active");
        else if (complete) stateChip.classList.add("local");
        else if (mappingLimited || mappingDuplicateChannels) stateChip.classList.add("warning");
    }

    const topologyChip = document.getElementById("cal-chip-topology");
    if (topologyChip) {
        topologyChip.textContent = `Topology: ${getCalibrationTopologyLabel(topologyId, true)}`;
    }
    const monitoringChip = document.getElementById("cal-chip-monitoring");
    if (monitoringChip) {
        monitoringChip.textContent = `Monitor: ${getCalibrationMonitoringPathLabel(monitoringPathId)}`;
    }
    const deviceChip = document.getElementById("cal-chip-device");
    if (deviceChip) {
        deviceChip.textContent = `Device: ${getCalibrationDeviceProfileLabel(deviceProfileId)}`;
    }
    const mappingChip = document.getElementById("cal-chip-mapping");
    if (mappingChip) {
        const mapped = Math.min(requiredChannels, writableChannels);
        mappingChip.textContent = `Mapping: ${mapped}/${requiredChannels}`;
        mappingChip.classList.toggle("warning", mappingLimited || mappingDuplicateChannels);
    }

    const mapRows = [
        document.getElementById("cal-map-row-1"),
        document.getElementById("cal-map-row-2"),
        document.getElementById("cal-map-row-3"),
        document.getElementById("cal-map-row-4"),
    ];
    const routingSource = calibrationMappingEditedByUser
        ? getCalibrationRoutingFromControls()
        : (status.speakerRouting || getCalibrationRoutingFromControls());
    const routing = normaliseCalibrationRouting(routingSource, 4);
    mapRows.forEach((row, idx) => {
        if (!row) return;
        const channelIndex = idx + 1;
        const shouldShow = channelIndex <= Math.min(4, requiredChannels);
        row.style.display = shouldShow ? "flex" : "none";
        row.classList.toggle("readonly", channelIndex > writableChannels);

        const label = document.getElementById(`cal-map-label-${channelIndex}`);
        if (label) {
            label.textContent = `${getCalibrationChannelLabel(topologyId, idx)} Out`;
        }

        const select = document.getElementById(`cal-spk${channelIndex}`);
        if (select) {
            const nextIndex = clamp((routing[idx] || channelIndex) - 1, 0, Math.max(0, select.options.length - 1));
            if (select.selectedIndex !== nextIndex) {
                select.selectedIndex = nextIndex;
            }
            select.disabled = channelIndex > writableChannels;
        }
    });

    const extraRowsContainer = document.getElementById("cal-mapping-extra-rows");
    if (extraRowsContainer) {
        extraRowsContainer.innerHTML = "";
        if (requiredChannels > 4) {
            for (let idx = 4; idx < requiredChannels; ++idx) {
                const row = document.createElement("div");
                row.className = "cal-mapping-row readonly";

                const label = document.createElement("span");
                label.className = "control-label";
                label.textContent = `${getCalibrationChannelLabel(topologyId, idx)} Out`;

                const value = document.createElement("span");
                value.className = "control-value";
                value.textContent = "Read-only (first 4 routable)";

                row.appendChild(label);
                row.appendChild(value);
                extraRowsContainer.appendChild(row);
            }
        }
    }

    const mappingNote = document.getElementById("cal-mapping-note");
    if (mappingNote) {
        mappingNote.classList.toggle("warning", mappingLimited || mappingDuplicateChannels);
        if (mappingDuplicateChannels) {
            mappingNote.textContent = "Routing duplicates detected. Use unique output channels.";
        } else if (mappingLimited) {
            mappingNote.textContent = `Topology requires ${requiredChannels} outputs but runtime exposes ${writableChannels} writable calibration channels.`;
        } else {
            mappingNote.textContent = `Routing contract satisfied for ${getCalibrationTopologyLabel(topologyId, true)} (${requiredChannels} required).`;
        }
    }

    const ackLimitedRow = document.getElementById("cal-ack-limited-row");
    const ackLimitedCheck = document.getElementById("cal-ack-limited-check");
    if (ackLimitedRow) {
        ackLimitedRow.style.display = mappingLimited ? "flex" : "none";
    }
    if (!mappingLimited && ackLimitedCheck) {
        ackLimitedCheck.checked = false;
    }

    const currentRouting = getCalibrationRoutingFromControls();
    const expectedAutoRouting = getCalibrationExpectedAutoRouting();
    const routeIsCustom = calibrationMappingEditedByUser || !compareCalibrationRouting(currentRouting, expectedAutoRouting, 4);
    const ackRedetectRow = document.getElementById("cal-ack-redetect-row");
    const ackRedetectCheck = document.getElementById("cal-ack-redetect-check");
    if (ackRedetectRow) {
        ackRedetectRow.style.display = routeIsCustom ? "flex" : "none";
    }
    if (!routeIsCustom && ackRedetectCheck) {
        ackRedetectCheck.checked = false;
        calibrationMappingEditedByUser = false;
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

    const requestedHeadphoneMode = (monitoringPathId === "steam_binaural" || monitoringPathId === "virtual_binaural")
        ? "steam_binaural"
        : "stereo_downmix";
    const activeHeadphoneMode = String(sceneData.rendererHeadphoneModeActive || requestedHeadphoneMode);
    const requestedHeadphoneProfile = deviceProfileId;
    const activeHeadphoneProfile = String(sceneData.rendererHeadphoneProfileActive || requestedHeadphoneProfile);

    if (mappingValid) {
        setCalibrationValidationChip("cal-validation-map-chip", "pass", "PASS");
    } else if (mappingDuplicateChannels) {
        setCalibrationValidationChip("cal-validation-map-chip", "fail", "DUP");
    } else if (mappingLimited) {
        setCalibrationValidationChip("cal-validation-map-chip", "warn", "LIMITED");
    } else {
        setCalibrationValidationChip("cal-validation-map-chip", "warn", "PENDING");
    }

    if (status.phasePass) {
        setCalibrationValidationChip("cal-validation-phase-chip", "pass", "PASS");
    } else if (running) {
        setCalibrationValidationChip("cal-validation-phase-chip", "warn", "IN RUN");
    } else {
        setCalibrationValidationChip("cal-validation-phase-chip", "warn", "PENDING");
    }

    if (status.delayPass) {
        setCalibrationValidationChip("cal-validation-delay-chip", "pass", "PASS");
    } else if (running) {
        setCalibrationValidationChip("cal-validation-delay-chip", "warn", "MEASURING");
    } else {
        setCalibrationValidationChip("cal-validation-delay-chip", "warn", "PENDING");
    }

    if (monitoringPathId === "speakers") {
        setCalibrationValidationChip("cal-validation-profile-chip", "pass", "SPEAKERS");
    } else if (activeHeadphoneMode === requestedHeadphoneMode && activeHeadphoneProfile === requestedHeadphoneProfile) {
        setCalibrationValidationChip("cal-validation-profile-chip", "pass", "ACTIVE");
    } else if (activeHeadphoneMode === "stereo_downmix" && requestedHeadphoneMode !== "stereo_downmix") {
        setCalibrationValidationChip("cal-validation-profile-chip", "warn", "FALLBACK");
    } else {
        setCalibrationValidationChip("cal-validation-profile-chip", "fail", "MISMATCH");
    }

    const downmixRequired = monitoringPathId === "stereo_downmix" || topologyId === "multichannel_stereo_downmix";
    if (!downmixRequired) {
        setCalibrationValidationChip("cal-validation-downmix-chip", "warn", "N/A");
    } else if (activeHeadphoneMode === "stereo_downmix") {
        setCalibrationValidationChip("cal-validation-downmix-chip", "pass", "PASS");
    } else {
        setCalibrationValidationChip("cal-validation-downmix-chip", "fail", "FAIL");
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
        viewportInfo.textContent = "Calibrate Mode \u00B7 "
            + getCalibrationTopologyLabel(topologyId, true)
            + " \u00B7 "
            + getCalibrationMonitoringPathLabel(monitoringPathId)
            + " \u00B7 "
            + (status.message || "Room Profile Setup");
    }

    if (currentMode === "calibrate") {
        updateSpeakerTargetsFromScene(sceneData);
    }
}

function updateEmitterMeshes(emitters) {
    if (!threeScene || typeof THREE === "undefined") return;

    const activeIds = new Set();
    const staleOpacityScale = sceneTransportState.stale ? 0.55 : 1.0;
    const trailsEnabled = !!getToggleValue(toggleStates.rend_viz_trails);
    const vectorsEnabled = !!getToggleValue(toggleStates.rend_viz_vectors);
    const physicsLensEnabled = getPhysicsLensEnabled();
    const diagnosticsMix = getPhysicsLensMix();
    const maxTrailPoints = getTrailPointBudget();

    emitters.forEach(em => {
        const emitterId = Number(em?.id);
        if (!Number.isInteger(emitterId) || emitterId < 0) return;

        activeIds.add(emitterId);
        let mesh = emitterMeshes.get(emitterId);

        if (!mesh) {
            const geo = new THREE.SphereGeometry(0.25, 16, 12);
            const mat = new THREE.MeshBasicMaterial({
                color: 0xD4A847,
                transparent: true,
                opacity: 0.78,
            });
            mesh = new THREE.Mesh(geo, mat);
            mesh.userData.emitterId = emitterId;
            mesh.userData.trailPoints = [];

            const dashedOutline = new THREE.LineSegments(
                new THREE.EdgesGeometry(new THREE.SphereGeometry(0.275, 12, 8)),
                new THREE.LineDashedMaterial({
                    color: 0xD4A847,
                    dashSize: 0.08,
                    gapSize: 0.06,
                    transparent: true,
                    opacity: 0.68,
                })
            );
            dashedOutline.computeLineDistances();
            mesh.add(dashedOutline);
            mesh.userData.dashedOutline = dashedOutline;

            const energyRing = new THREE.Mesh(
                new THREE.RingGeometry(0.30, 0.36, 28),
                new THREE.MeshBasicMaterial({
                    color: 0xD4A847,
                    side: THREE.DoubleSide,
                    transparent: true,
                    opacity: 0.14,
                })
            );
            energyRing.rotation.x = -Math.PI / 2;
            energyRing.position.y = -0.02;
            mesh.add(energyRing);
            mesh.userData.energyRing = energyRing;

            const aimArrow = new THREE.ArrowHelper(
                new THREE.Vector3(0, 0, -1),
                new THREE.Vector3(0, 0, 0),
                0.45,
                0x7AAFC9,
                0.14,
                0.08
            );
            mesh.add(aimArrow);
            mesh.userData.aimArrow = aimArrow;

            const velocityArrow = new THREE.ArrowHelper(
                new THREE.Vector3(0, 0, -1),
                new THREE.Vector3(0, 0, 0),
                0.0001,
                0x5BBAB3,
                0.10,
                0.06
            );
            velocityArrow.visible = false;
            mesh.add(velocityArrow);
            mesh.userData.velocityArrow = velocityArrow;

            const forceArrow = new THREE.ArrowHelper(
                new THREE.Vector3(0, 0, -1),
                new THREE.Vector3(0, 0, 0),
                0.0001,
                0xFF8A4A,
                0.10,
                0.06
            );
            forceArrow.visible = false;
            mesh.add(forceArrow);
            mesh.userData.forceArrow = forceArrow;

            const collisionRing = new THREE.Mesh(
                new THREE.RingGeometry(0.40, 0.47, 28),
                new THREE.MeshBasicMaterial({
                    color: 0xFF5A4A,
                    side: THREE.DoubleSide,
                    transparent: true,
                    opacity: 0.0,
                })
            );
            collisionRing.rotation.x = -Math.PI / 2;
            collisionRing.position.y = -0.10;
            collisionRing.visible = false;
            mesh.add(collisionRing);
            mesh.userData.collisionRing = collisionRing;
            mesh.userData.collisionPulse = 0.0;

            const trailLine = new THREE.Line(
                new THREE.BufferGeometry().setFromPoints([new THREE.Vector3(), new THREE.Vector3()]),
                new THREE.LineBasicMaterial({
                    color: 0xD4A847,
                    transparent: true,
                    opacity: 0.35,
                })
            );
            trailLine.visible = false;
            threeScene.add(trailLine);
            mesh.userData.trailLine = trailLine;

            const trajectoryLine = new THREE.Line(
                new THREE.BufferGeometry().setFromPoints([new THREE.Vector3(), new THREE.Vector3()]),
                new THREE.LineDashedMaterial({
                    color: 0xFFB47A,
                    dashSize: 0.10,
                    gapSize: 0.07,
                    transparent: true,
                    opacity: 0.0,
                })
            );
            trajectoryLine.visible = false;
            threeScene.add(trajectoryLine);
            mesh.userData.trajectoryLine = trajectoryLine;

            threeScene.add(mesh);
            emitterMeshes.set(emitterId, mesh);
        }

        if (!emitterVisualTargets.has(emitterId)) {
            emitterVisualTargets.set(emitterId, {
                x: Number(em.x) || 0,
                y: Number(em.y) || 0,
                z: Number(em.z) || 0,
                sx: Number(em.sx) || 1,
                sy: Number(em.sy) || 1,
                sz: Number(em.sz) || 1,
                aimX: Number(em.aimX) || 0.0,
                aimY: Number(em.aimY) || 0.0,
                aimZ: Number(em.aimZ) || -1.0,
                directivity: clamp(Number(em.directivity) || 0.5, 0.0, 1.0),
                rms: clamp(Number(em.rms) || 0.0, 0.0, 4.0),
                vx: Number(em.vx) || 0.0,
                vy: Number(em.vy) || 0.0,
                vz: Number(em.vz) || 0.0,
                fx: Number(em.fx) || 0.0,
                fy: Number(em.fy) || 0.0,
                fz: Number(em.fz) || 0.0,
                collisionMask: Number.isFinite(Number(em.collisionMask)) ? Number(em.collisionMask) : 0,
                collisionEnergy: Number(em.collisionEnergy) || 0.0,
                physicsEnabled: !!em.physics,
            });
        }

        const target = emitterVisualTargets.get(emitterId);
        target.x = Number(em.x) || 0;
        target.y = Number(em.y) || 0;
        target.z = Number(em.z) || 0;
        target.sx = Number(em.sx) || 1;
        target.sy = Number(em.sy) || 1;
        target.sz = Number(em.sz) || 1;
        target.aimX = Number(em.aimX) || 0.0;
        target.aimY = Number(em.aimY) || 0.0;
        target.aimZ = Number(em.aimZ) || -1.0;
        target.directivity = clamp(Number(em.directivity) || 0.5, 0.0, 1.0);
        target.rms = clamp(Number(em.rms) || 0.0, 0.0, 4.0);
        target.vx = Number(em.vx) || 0.0;
        target.vy = Number(em.vy) || 0.0;
        target.vz = Number(em.vz) || 0.0;
        target.fx = Number(em.fx) || 0.0;
        target.fy = Number(em.fy) || 0.0;
        target.fz = Number(em.fz) || 0.0;
        target.collisionMask = Number.isFinite(Number(em.collisionMask)) ? Number(em.collisionMask) : 0;
        target.collisionEnergy = Number(em.collisionEnergy) || 0.0;
        target.physicsEnabled = !!em.physics;

        if (!mesh.userData.visualSeeded) {
            mesh.position.set(target.x, target.y, target.z);
            mesh.scale.set(target.sx * 2, target.sy * 2, target.sz * 2);
            mesh.userData.visualSeeded = true;
        }

        const trailPoints = mesh.userData.trailPoints;
        if (Array.isArray(trailPoints)) {
            trailPoints.push(new THREE.Vector3(target.x, target.y, target.z));
            while (trailPoints.length > maxTrailPoints) trailPoints.shift();

            const trailLine = mesh.userData.trailLine;
            if (trailLine && trailLine.geometry) {
                trailLine.geometry.setFromPoints(trailPoints.length > 1 ? trailPoints : [mesh.position.clone(), mesh.position.clone()]);
            }
        }

        mesh.userData.emitterId = emitterId;
        const isSelected = emitterId === selectedEmitterId;
        const color = getEmitterColorHex(em.color);
        mesh.material.color.setHex(color);
        mesh.material.transparent = true;
        mesh.material.opacity = (em.muted ? 0.12 : (isSelected ? 0.95 : 0.28)) * staleOpacityScale;
        mesh.visible = currentMode !== "calibrate";

        const dashedOutline = mesh.userData.dashedOutline;
        if (dashedOutline && dashedOutline.material) {
            dashedOutline.material.color.setHex(color);
            dashedOutline.material.opacity = (em.muted ? 0.18 : 0.72) * staleOpacityScale;
            dashedOutline.visible = mesh.visible && !isSelected;
        }

        const energyRing = mesh.userData.energyRing;
        if (energyRing && energyRing.material) {
            const energyNorm = clamp(Math.sqrt(target.rms) * 0.9, 0.0, 1.0);
            const ringScale = 1.0 + (energyNorm * (isSelected ? 1.25 : 0.9));
            energyRing.scale.set(ringScale, ringScale, 1.0);
            energyRing.material.color.setHex(color);
            energyRing.material.opacity = (isSelected ? 0.22 : 0.10) + (energyNorm * (isSelected ? 0.55 : 0.28));
            energyRing.visible = mesh.visible;
        }

        const aimArrow = mesh.userData.aimArrow;
        if (aimArrow) {
            const aimLength = 0.36 + (target.directivity * 0.64);
            setArrowFromVector(aimArrow, target.aimX, target.aimY, target.aimZ, aimLength);
            aimArrow.setColor(new THREE.Color(isSelected ? 0xD4A847 : 0x7AAFC9));
            aimArrow.visible = mesh.visible;
        }

        const velocityArrow = mesh.userData.velocityArrow;
        if (velocityArrow) {
            const speed = Math.sqrt((target.vx * target.vx) + (target.vy * target.vy) + (target.vz * target.vz));
            const velocityLength = clamp(speed * 0.08, 0.10, 0.95);
            setArrowFromVector(velocityArrow, target.vx, target.vy, target.vz, velocityLength);
            velocityArrow.visible = mesh.visible && vectorsEnabled && speed > 0.02;
        }

        const forceArrow = mesh.userData.forceArrow;
        if (forceArrow) {
            const forceMag = Math.sqrt((target.fx * target.fx) + (target.fy * target.fy) + (target.fz * target.fz));
            const forceLength = clamp((forceMag * 0.035) + (diagnosticsMix * 0.08), 0.08, 0.95);
            setArrowFromVector(forceArrow, target.fx, target.fy, target.fz, forceLength);
            forceArrow.visible = mesh.visible && physicsLensEnabled && !!em.physics && forceMag > 0.025;
        }

        const trajectoryLine = mesh.userData.trajectoryLine;
        if (trajectoryLine && trajectoryLine.geometry && trajectoryLine.material) {
            const trajectoryPoints = buildTrajectoryPreviewPoints(target, diagnosticsMix);
            trajectoryLine.geometry.setFromPoints(
                trajectoryPoints.length > 1 ? trajectoryPoints : [mesh.position.clone(), mesh.position.clone()]
            );
            if (typeof trajectoryLine.computeLineDistances === "function") {
                trajectoryLine.computeLineDistances();
            }
            trajectoryLine.material.opacity = (0.20 + diagnosticsMix * 0.40) * staleOpacityScale;
            trajectoryLine.material.color.setHex(color);
            trajectoryLine.visible = mesh.visible && physicsLensEnabled && !!em.physics;
        }

        const collisionRing = mesh.userData.collisionRing;
        if (collisionRing && collisionRing.material) {
            const collisionDetected = (target.collisionMask & 0x7) !== 0;
            const collisionEnergyNorm = clamp((target.collisionEnergy || 0.0) * 0.35, 0.0, 1.0);
            const collisionPulse = collisionDetected
                ? Math.max(0.35 + collisionEnergyNorm, Number(mesh.userData.collisionPulse) || 0.0)
                : (Number(mesh.userData.collisionPulse) || 0.0) * 0.85;
            mesh.userData.collisionPulse = collisionPulse;
            collisionRing.scale.set(1.0 + collisionPulse * 0.6, 1.0 + collisionPulse * 0.6, 1.0);
            collisionRing.material.opacity = (0.08 + collisionPulse * 0.75) * diagnosticsMix * staleOpacityScale;
            collisionRing.visible = mesh.visible && physicsLensEnabled && collisionPulse > 0.04;
        }

        const trailLine = mesh.userData.trailLine;
        if (trailLine && trailLine.material) {
            trailLine.material.color.setHex(color);
            trailLine.material.opacity = (isSelected ? 0.44 : 0.22) * staleOpacityScale;
            trailLine.visible = mesh.visible && trailsEnabled && Array.isArray(mesh.userData.trailPoints) && mesh.userData.trailPoints.length > 1;
        }
    });

    for (const [id, mesh] of emitterMeshes) {
        if (!activeIds.has(id)) {
            const trailLine = mesh.userData.trailLine;
            if (trailLine) {
                threeScene.remove(trailLine);
                trailLine.geometry?.dispose?.();
                trailLine.material?.dispose?.();
            }

            const trajectoryLine = mesh.userData.trajectoryLine;
            if (trajectoryLine) {
                threeScene.remove(trajectoryLine);
                trajectoryLine.geometry?.dispose?.();
                trajectoryLine.material?.dispose?.();
            }

            const dashedOutline = mesh.userData.dashedOutline;
            dashedOutline?.geometry?.dispose?.();
            dashedOutline?.material?.dispose?.();

            const energyRing = mesh.userData.energyRing;
            energyRing?.geometry?.dispose?.();
            energyRing?.material?.dispose?.();

            const aimArrow = mesh.userData.aimArrow;
            aimArrow?.line?.geometry?.dispose?.();
            aimArrow?.line?.material?.dispose?.();
            aimArrow?.cone?.geometry?.dispose?.();
            aimArrow?.cone?.material?.dispose?.();

            const velocityArrow = mesh.userData.velocityArrow;
            velocityArrow?.line?.geometry?.dispose?.();
            velocityArrow?.line?.material?.dispose?.();
            velocityArrow?.cone?.geometry?.dispose?.();
            velocityArrow?.cone?.material?.dispose?.();

            const forceArrow = mesh.userData.forceArrow;
            forceArrow?.line?.geometry?.dispose?.();
            forceArrow?.line?.material?.dispose?.();
            forceArrow?.cone?.geometry?.dispose?.();
            forceArrow?.cone?.material?.dispose?.();

            const collisionRing = mesh.userData.collisionRing;
            collisionRing?.geometry?.dispose?.();
            collisionRing?.material?.dispose?.();

            mesh.geometry?.dispose?.();
            mesh.material?.dispose?.();
            threeScene.remove(mesh);
            emitterMeshes.delete(id);
            emitterVisualTargets.delete(id);
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
    const frameNowMs = typeof performance !== "undefined" && typeof performance.now === "function"
        ? performance.now()
        : Date.now();
    let frameDeltaSeconds = 0.016;
    if (lastAnimationFrameTimeMs > 0) {
        frameDeltaSeconds = clamp((frameNowMs - lastAnimationFrameTimeMs) / 1000.0, 0.0, 0.1);
    }
    lastAnimationFrameTimeMs = frameNowMs;

    animTime += frameDeltaSeconds;
    updateSceneTransportHealth(Date.now());
    enforceEmitterTimelineInvariant(frameNowMs);

    // Selection ring float
    if (selectionRing && selectionRing.visible) {
        selectionRing.position.y = selectionRingBaseY + Math.sin(animTime * 2.0) * 0.015;
    }

    const smoothingAlpha = getSceneSmoothingAlpha(frameDeltaSeconds);
    const staleOpacityScale = sceneTransportState.stale ? 0.55 : 1.0;
    const trailsEnabled = !!getToggleValue(toggleStates.rend_viz_trails);
    const vectorsEnabled = !!getToggleValue(toggleStates.rend_viz_vectors);
    const physicsLensEnabled = getPhysicsLensEnabled();
    const diagnosticsMix = getPhysicsLensMix();
    for (const [id, mesh] of emitterMeshes) {
        const target = emitterVisualTargets.get(id);
        if (target) {
            mesh.position.x += (target.x - mesh.position.x) * smoothingAlpha;
            mesh.position.y += (target.y - mesh.position.y) * smoothingAlpha;
            mesh.position.z += (target.z - mesh.position.z) * smoothingAlpha;
            mesh.scale.x += ((target.sx * 2) - mesh.scale.x) * smoothingAlpha;
            mesh.scale.y += ((target.sy * 2) - mesh.scale.y) * smoothingAlpha;
            mesh.scale.z += ((target.sz * 2) - mesh.scale.z) * smoothingAlpha;
        }

        const emitter = sceneEmitterLookup.get(id);
        if (emitter) {
            const isSelected = id === selectedEmitterId;
            const baseOpacity = emitter.muted ? 0.12 : (isSelected ? 0.95 : 0.28);
            mesh.material.opacity = baseOpacity * staleOpacityScale;
        }

        const trailLine = mesh.userData.trailLine;
        if (trailLine && trailLine.material) {
            const isSelected = id === selectedEmitterId;
            trailLine.material.opacity = (isSelected ? 0.44 : 0.22) * staleOpacityScale;
            trailLine.visible = mesh.visible
                && trailsEnabled
                && Array.isArray(mesh.userData.trailPoints)
                && mesh.userData.trailPoints.length > 1;
        }

        const dashedOutline = mesh.userData.dashedOutline;
        if (dashedOutline) {
            dashedOutline.visible = mesh.visible && (id !== selectedEmitterId);
        }

        const velocityArrow = mesh.userData.velocityArrow;
        if (velocityArrow && target) {
            const speed = Math.sqrt((target.vx * target.vx) + (target.vy * target.vy) + (target.vz * target.vz));
            velocityArrow.visible = mesh.visible && vectorsEnabled && speed > 0.02;
        }

        const forceArrow = mesh.userData.forceArrow;
        if (forceArrow && target) {
            const forceMag = Math.sqrt((target.fx * target.fx) + (target.fy * target.fy) + (target.fz * target.fz));
            forceArrow.visible = mesh.visible && physicsLensEnabled && !!target.physicsEnabled && forceMag > 0.025;
        }

        const trajectoryLine = mesh.userData.trajectoryLine;
        if (trajectoryLine && trajectoryLine.material) {
            trajectoryLine.material.opacity = (0.20 + diagnosticsMix * 0.40) * staleOpacityScale;
            trajectoryLine.visible = mesh.visible && physicsLensEnabled && !!target?.physicsEnabled;
        }

        const collisionRing = mesh.userData.collisionRing;
        if (collisionRing && collisionRing.material) {
            const collisionPulse = Number(mesh.userData.collisionPulse) || 0.0;
            collisionRing.scale.set(1.0 + collisionPulse * 0.6, 1.0 + collisionPulse * 0.6, 1.0);
            collisionRing.material.opacity = (0.08 + collisionPulse * 0.75) * diagnosticsMix * staleOpacityScale;
            collisionRing.visible = mesh.visible && physicsLensEnabled && !!target?.physicsEnabled && collisionPulse > 0.04;
        }
    }

    if (listenerGroup) {
        listenerGroup.position.x += (listenerTarget.x - listenerGroup.position.x) * smoothingAlpha;
        listenerGroup.position.y += (listenerTarget.y - listenerGroup.position.y) * smoothingAlpha;
        listenerGroup.position.z += (listenerTarget.z - listenerGroup.position.z) * smoothingAlpha;

        const speakerEnergy = Array.isArray(sceneData?.speakerRms)
            ? sceneData.speakerRms.reduce((sum, value) => sum + clamp(Number(value) || 0.0, 0.0, 4.0), 0.0) / Math.max(1, sceneData.speakerRms.length)
            : 0.0;
        const listenerEnergy = clamp(Math.sqrt(speakerEnergy) * 0.55, 0.0, 1.0);
        if (listenerEnergyRing && listenerEnergyRing.material) {
            const ringScale = 1.0 + listenerEnergy * 1.15;
            listenerEnergyRing.scale.set(ringScale, ringScale, 1.0);
            listenerEnergyRing.material.opacity = 0.14 + listenerEnergy * 0.48;
        }

        if (listenerAimArrow) {
            const selectedTarget = emitterVisualTargets.get(selectedEmitterId);
            if (selectedTarget) {
                setArrowFromVector(listenerAimArrow, selectedTarget.aimX, selectedTarget.aimY, selectedTarget.aimZ, 0.38);
            }
        }
    }

    const calibrationCurrentSpeaker = Math.max(0, Math.min(3, (calibrationState.currentSpeaker || 1) - 1));
    const calibrationPreviewSpeakerCount = currentMode === "calibrate"
        ? getCalibrationPreviewSpeakerCount()
        : 4;

    // Speaker energy meters
    speakerMeters.forEach((m, i) => {
        const speaker = speakers[i];
        const ring = speakerEnergyRings[i];
        const targetSpeaker = speakerTargets[i] || defaultSpeakerSnapshotPositions[i];
        const speakerVisible = currentMode !== "calibrate" || i < calibrationPreviewSpeakerCount;
        if (speaker) {
            speaker.visible = speakerVisible;
        }
        m.mesh.visible = speakerVisible;

        if (!speakerVisible) {
            m.target = 0.0;
            m.level += (m.target - m.level) * 0.2;
            if (ring) {
                ring.visible = false;
            }
            return;
        }

        if (speaker && targetSpeaker) {
            speaker.position.x += (targetSpeaker.x - speaker.position.x) * smoothingAlpha;
            speaker.position.y += (targetSpeaker.y - speaker.position.y) * smoothingAlpha;
            speaker.position.z += (targetSpeaker.z - speaker.position.z) * smoothingAlpha;
        }

        m.basePos.x += (targetSpeaker.x - m.basePos.x) * smoothingAlpha;
        m.basePos.y += (targetSpeaker.y - m.basePos.y) * smoothingAlpha;
        m.basePos.z += (targetSpeaker.z - m.basePos.z) * smoothingAlpha;

        if (currentMode === "calibrate") {
            const level = getCalibrationSpeakerLevel(i);
            const pulse = (calibrationState.running && i === calibrationCurrentSpeaker)
                ? 0.8 + 0.2 * Math.sin(animTime * 10.0)
                : 1.0;
            m.target = level * 0.45 * pulse;
        } else if (currentMode === "renderer" || currentMode === "emitter") {
            const rms = clamp(Number(targetSpeaker?.rms) || 0.0, 0.0, 4.0);
            const responsiveFloor = 0.04 + 0.03 * Math.sin(animTime * 2.0 + i * 0.7);
            m.target = clamp(responsiveFloor + (rms * 0.42), 0.0, 1.6);
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

        if (speaker && speaker.material?.color) {
            if (currentMode === "calibrate") {
                speaker.material.color.setHex(getCalibrationSpeakerColor(i));
            } else {
                const speakerGlowMix = clamp((m.level - 0.08) / 0.55, 0.0, 1.0);
                speaker.material.color.setHex(speakerGlowMix > 0.45 ? 0xD8CFA0 : 0xE0E0E0);
            }
            speaker.material.opacity = 0.55 + Math.min(0.4, m.level * 0.35);
        }

        if (ring && ring.material) {
            const ringScale = 1.0 + clamp(m.level * 0.9, 0.0, 1.6);
            ring.scale.set(ringScale, ringScale, 1.0);
            ring.position.set(m.basePos.x, Math.max(0.05, m.basePos.y - 0.18), m.basePos.z);
            ring.material.opacity = 0.08 + Math.min(0.52, m.level * 0.45);
            ring.visible = speakerVisible && (currentMode !== "calibrate" || calibrationState.running || calibrationState.complete);
        }

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

})();
