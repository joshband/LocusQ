(() => {
  function createListenerList() {
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
          } catch (_) {}
        }
      },
    };
  }

  function createJuceBridgeDirect() {
    const backend = window.__JUCE__ && window.__JUCE__.backend ? window.__JUCE__.backend : null;
    if (!backend) {
      return null;
    }

    const sliderStates = new Map();
    const toggleStates = new Map();
    const comboStates = new Map();

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

    const emit = (identifier, payload) => {
      backend.emitEvent(identifier, payload);
    };

    const attachBackendListener = (identifier, onEvent) => {
      backend.addEventListener(identifier, event => onEvent(event || {}));
      emit(identifier, { eventType: "requestInitialUpdate" });
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
        valueChangedEvent: createListenerList(),
        propertiesChangedEvent: createListenerList(),
        getScaledValue() {
          return Number(this.scaledValue) || 0;
        },
        getNormalisedValue() {
          return toNormalised(this.scaledValue, this.properties.start, this.properties.end, this.properties.skew);
        },
        setNormalisedValue(newValue) {
          this.scaledValue = toScaled(newValue, this.properties.start, this.properties.end, this.properties.skew);
          emit(this.identifier, { eventType: "valueChanged", value: this.scaledValue });
          this.valueChangedEvent.callListeners();
        },
        sliderDragStarted() {
          emit(this.identifier, { eventType: "sliderDragStarted" });
        },
        sliderDragEnded() {
          emit(this.identifier, { eventType: "sliderDragEnded" });
        },
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
        valueChangedEvent: createListenerList(),
        propertiesChangedEvent: createListenerList(),
        getValue() {
          return !!this.value;
        },
        setValue(nextValue) {
          this.value = !!nextValue;
          emit(this.identifier, { eventType: "valueChanged", value: this.value });
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
        valueChangedEvent: createListenerList(),
        propertiesChangedEvent: createListenerList(),
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
          emit(this.identifier, { eventType: "valueChanged", value: this.value });
          this.valueChangedEvent.callListeners();
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
        if (name === "locusqGetChoiceItems") {
          return async function fallbackChoiceItems() {
            return [];
          };
        }

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
    };
  }

  const counts = {
    domPointer: 0,
    domClick: 0,
    domChange: 0,
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
  };

  const dom = {
    toggle: document.getElementById("toggle-size-link"),
    choice: document.getElementById("choice-quality"),
    slider: document.getElementById("slider-size-uniform"),
    sliderReadout: document.getElementById("slider-readout"),
    bridgeState: document.getElementById("bridge-state"),
    countDom: document.getElementById("count-dom"),
    countSet: document.getElementById("count-set"),
    countValue: document.getElementById("count-value"),
    countProps: document.getElementById("count-props"),
    countLive: document.getElementById("count-live"),
    countHeartbeat: document.getElementById("count-heartbeat"),
    diagnostics: document.getElementById("diagnostics"),
  };

  const snapshot = {
    toggleValue: null,
    choiceIndex: null,
    sliderNorm: null,
    toggleKnown: false,
    choiceKnown: false,
    sliderKnown: false,
    choiceSource: "unknown",
  };

  function updateCounters() {
    dom.countDom.textContent = `${counts.domPointer} / ${counts.domClick} / ${counts.domChange}`;
    dom.countSet.textContent = `${counts.setToggle} / ${counts.setChoice} / ${counts.setSlider}`;
    dom.countValue.textContent = `${counts.valueToggle} / ${counts.valueChoice} / ${counts.valueSlider}`;
    dom.countProps.textContent = `${counts.propsToggle} / ${counts.propsChoice} / ${counts.propsSlider}`;
    const liveToggle = snapshot.toggleValue === null ? "?" : snapshot.toggleValue ? "1" : "0";
    const liveChoice = snapshot.choiceIndex === null ? "?" : String(snapshot.choiceIndex);
    const liveSlider = snapshot.sliderNorm === null ? "?" : Number(snapshot.sliderNorm).toFixed(3);
    dom.countLive.textContent = `${liveToggle} / ${liveChoice} / ${liveSlider}`;
    dom.countHeartbeat.textContent = String(counts.heartbeat);
  }

  function setBridgeState(ok, text) {
    dom.bridgeState.textContent = text;
    dom.bridgeState.className = `val ${ok ? "bridge-ok" : "bridge-warn"}`;
  }

  function writeDiagnostics(extra = {}) {
    const payload = {
      timestamp: new Date().toISOString(),
      hasWindowJuce: typeof window.Juce !== "undefined",
      hasBackend: !!(window.__JUCE__ && window.__JUCE__.backend),
      togglesListed: window.__JUCE__?.initialisationData?.__juce__toggles ?? [],
      combosListed: window.__JUCE__?.initialisationData?.__juce__comboBoxes ?? [],
      slidersListed: window.__JUCE__?.initialisationData?.__juce__sliders ?? [],
      snapshot,
      counts,
      ...extra,
    };
    dom.diagnostics.textContent = JSON.stringify(payload, null, 2);
  }

  function bindDomProbe(el) {
    const bumpPointer = () => {
      counts.domPointer += 1;
      updateCounters();
    };
    const bumpClick = () => {
      counts.domClick += 1;
      updateCounters();
    };

    el.addEventListener("pointerdown", bumpPointer);
    el.addEventListener("mousedown", bumpPointer);
    el.addEventListener("click", bumpClick);
  }

  function applyToggleToUi(state) {
    snapshot.toggleValue = !!state.getValue();
    dom.toggle.checked = snapshot.toggleValue;
  }

  function rebuildChoiceOptions(state) {
    const choices = Array.isArray(state.properties?.choices) ? state.properties.choices : [];
    return rebuildChoiceOptionsFromArray(choices, "relay");
  }

  function rebuildChoiceOptionsFromArray(choices, source) {
    dom.choice.innerHTML = "";

    if (choices.length === 0) {
      const option = document.createElement("option");
      option.value = "";
      option.textContent = "(no choices from backend yet)";
      dom.choice.appendChild(option);
      snapshot.choiceKnown = false;
      snapshot.choiceSource = source;
      return false;
    }

    choices.forEach((item, index) => {
      const option = document.createElement("option");
      option.value = String(index);
      option.textContent = String(item);
      dom.choice.appendChild(option);
    });
    snapshot.choiceKnown = true;
    snapshot.choiceSource = source;
    return true;
  }

  function applyChoiceToUi(state) {
    const index = Math.max(0, Number(state.getChoiceIndex()) || 0);
    snapshot.choiceIndex = index;
    dom.choice.selectedIndex = index;
  }

  function applySliderToUi(state) {
    const normalised = Math.max(0, Math.min(1, Number(state.getNormalisedValue()) || 0));
    snapshot.sliderNorm = normalised;
    dom.slider.value = normalised.toFixed(3);
    dom.sliderReadout.textContent = `normalized=${normalised.toFixed(3)} scaled=${Number(state.getScaledValue()).toFixed(3)}`;
  }

  function run() {
    const Juce = createJuceBridgeDirect();
    if (!Juce || typeof Juce.getToggleState !== "function") {
      setBridgeState(false, "bridge missing");
      writeDiagnostics({ error: "JUCE bridge unavailable" });
      return;
    }

    window.Juce = Juce;

    const toggleState = Juce.getToggleState("size_link");
    const choiceState = Juce.getComboBoxState("rend_quality");
    const sliderState = Juce.getSliderState("size_uniform");
    const getChoiceItemsNative = typeof Juce.getNativeFunction === "function"
      ? Juce.getNativeFunction("locusqGetChoiceItems")
      : null;
    let fallbackChoiceFetchInFlight = false;

    snapshot.toggleKnown = true;
    snapshot.choiceKnown = true;
    snapshot.sliderKnown = true;

    bindDomProbe(dom.toggle);
    bindDomProbe(dom.choice);
    bindDomProbe(dom.slider);

    dom.toggle.addEventListener("change", () => {
      counts.domChange += 1;
      counts.setToggle += 1;
      toggleState.setValue(dom.toggle.checked);
      updateCounters();
      writeDiagnostics();
    });

    dom.choice.addEventListener("change", () => {
      counts.domChange += 1;
      counts.setChoice += 1;
      const selected = Math.max(0, dom.choice.selectedIndex);
      choiceState.setChoiceIndex(selected);
      updateCounters();
      writeDiagnostics();
    });

    const onSliderDragStart = () => {
      if (typeof sliderState.sliderDragStarted === "function") {
        sliderState.sliderDragStarted();
      }
    };
    const onSliderDragEnd = () => {
      if (typeof sliderState.sliderDragEnded === "function") {
        sliderState.sliderDragEnded();
      }
    };

    dom.slider.addEventListener("pointerdown", onSliderDragStart);
    dom.slider.addEventListener("mousedown", onSliderDragStart);
    dom.slider.addEventListener("pointerup", onSliderDragEnd);
    dom.slider.addEventListener("mouseup", onSliderDragEnd);
    dom.slider.addEventListener("blur", onSliderDragEnd);

    dom.slider.addEventListener("input", () => {
      counts.domChange += 1;
      counts.setSlider += 1;
      sliderState.setNormalisedValue(Number(dom.slider.value));
      updateCounters();
      writeDiagnostics();
    });

    toggleState.valueChangedEvent.addListener(() => {
      counts.valueToggle += 1;
      applyToggleToUi(toggleState);
      updateCounters();
      writeDiagnostics();
    });

    toggleState.propertiesChangedEvent.addListener(() => {
      counts.propsToggle += 1;
      updateCounters();
      writeDiagnostics();
    });

    choiceState.valueChangedEvent.addListener(() => {
      counts.valueChoice += 1;
      applyChoiceToUi(choiceState);
      updateCounters();
      writeDiagnostics();
    });

    choiceState.propertiesChangedEvent.addListener(() => {
      counts.propsChoice += 1;
      rebuildChoiceOptions(choiceState);
      applyChoiceToUi(choiceState);
      updateCounters();
      writeDiagnostics();
    });

    sliderState.valueChangedEvent.addListener(() => {
      counts.valueSlider += 1;
      applySliderToUi(sliderState);
      updateCounters();
      writeDiagnostics();
    });

    sliderState.propertiesChangedEvent.addListener(() => {
      counts.propsSlider += 1;
      applySliderToUi(sliderState);
      updateCounters();
      writeDiagnostics();
    });

    applyToggleToUi(toggleState);
    rebuildChoiceOptions(choiceState);
    applyChoiceToUi(choiceState);
    applySliderToUi(sliderState);

    const ensureChoiceOptionsFromNative = async () => {
      if (!getChoiceItemsNative || fallbackChoiceFetchInFlight) {
        return;
      }

      if (snapshot.choiceKnown) {
        return;
      }

      fallbackChoiceFetchInFlight = true;
      try {
        const result = await getChoiceItemsNative("rend_quality");
        let nativeChoices = [];

        if (Array.isArray(result)) {
          nativeChoices = result;
        } else if (result && Array.isArray(result.items)) {
          nativeChoices = result.items;
        } else if (typeof result === "string" && result.length > 0) {
          nativeChoices = [result];
        }

        if (nativeChoices.length > 0) {
          rebuildChoiceOptionsFromArray(nativeChoices, "native");
          applyChoiceToUi(choiceState);
        }
      } catch (error) {
        writeDiagnostics({ choiceFallbackError: String(error) });
      } finally {
        fallbackChoiceFetchInFlight = false;
        updateCounters();
        writeDiagnostics();
      }
    };

    const heartbeat = () => {
      counts.heartbeat += 1;
      applyToggleToUi(toggleState);
      applyChoiceToUi(choiceState);
      applySliderToUi(sliderState);
      updateCounters();
      writeDiagnostics();
      void ensureChoiceOptionsFromNative();
    };

    setBridgeState(true, "connected");
    updateCounters();
    writeDiagnostics({ message: "POC loaded" });
    window.setInterval(heartbeat, 350);
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", run, { once: true });
  } else {
    run();
  }
})();
