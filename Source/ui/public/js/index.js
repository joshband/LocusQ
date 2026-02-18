import * as Juce from "./juce/index.js";

// ===========================================================================
// LocusQ WebView – JUCE Parameter Integration & Three.js Viewport
// ===========================================================================

// ===== PARAMETER STATES =====
const sliderStates = {
    pos_azimuth:    Juce.getSliderState("pos_azimuth"),
    pos_elevation:  Juce.getSliderState("pos_elevation"),
    pos_distance:   Juce.getSliderState("pos_distance"),
    emit_gain:      Juce.getSliderState("emit_gain"),
    emit_spread:    Juce.getSliderState("emit_spread"),
    emit_directivity: Juce.getSliderState("emit_directivity"),
    rend_master_gain: Juce.getSliderState("rend_master_gain"),
};

const toggleStates = {
    bypass:      Juce.getToggleState("bypass"),
    emit_mute:   Juce.getToggleState("emit_mute"),
    emit_solo:   Juce.getToggleState("emit_solo"),
    phys_enable: Juce.getToggleState("phys_enable"),
};

const comboStates = {
    mode:         Juce.getComboBoxState("mode"),
    rend_quality: Juce.getComboBoxState("rend_quality"),
};

// ===== DESATURATED EMITTER PALETTE (v2) =====
const emitterPalette = [
    0xD4736F, 0x5BBAB3, 0x5AADC0, 0x8DBEA7, 0xD8CFA0, 0xBF9ABD, 0x8CC5B7, 0xCCBA6E,
    0xA487B5, 0x7AAFC9, 0xC9A07A, 0x7DC49A, 0xC98A84, 0x96BAD0, 0xB3A0BF, 0x8EC8BD
];

// ===== APP STATE =====
let currentMode = "emitter";
let selectedLane = "azimuth";
let sceneData = { emitters: [], emitterCount: 0, rendererActive: false };

// ===== THREE.JS SETUP =====
let threeScene, camera, rendererGL, canvas;
let roomLines, gridHelper, speakers = [], speakerMeters = [];
let emitterMeshes = new Map();
let selectionRing, trail;
let azArc, elArc, distRing;
let spherical = { theta: Math.PI / 4, phi: Math.PI / 4, radius: 8 };
let orbitTarget = new THREE.Vector3(0, 1, 0);
let isDragging = false, isRight = false, prevMouse = { x: 0, y: 0 };
let animTime = 0;

// Lane highlight state
let highlightTargets = { azimuth: 0.25, elevation: 0, distance: 0, size: 0 };
let highlightCurrent = { azimuth: 0, elevation: 0, distance: 0, size: 0 };

document.addEventListener("DOMContentLoaded", () => {
    initThreeJS();
    initUIBindings();
    initParameterListeners();
    animate();
    console.log("LocusQ WebView initialized");
});

// ===== THREE.JS INITIALIZATION =====
function initThreeJS() {
    canvas = document.getElementById("viewport-canvas");
    threeScene = new THREE.Scene();
    threeScene.background = new THREE.Color(0x0A0A0A);

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

    // Orbit controls
    canvas.addEventListener("mousedown", e => {
        isDragging = true;
        isRight = e.button === 2;
        prevMouse = { x: e.clientX, y: e.clientY };
    });
    canvas.addEventListener("mousemove", e => {
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
    canvas.addEventListener("mouseup", () => isDragging = false);
    canvas.addEventListener("contextmenu", e => e.preventDefault());
    canvas.addEventListener("wheel", e => {
        spherical.radius *= 1 + e.deltaY * 0.001;
        spherical.radius = Math.max(2, Math.min(20, spherical.radius));
        updateCamera();
    });

    updateCamera();
    resize();
    window.addEventListener("resize", resize);
}

function updateCamera() {
    camera.position.x = orbitTarget.x + spherical.radius * Math.sin(spherical.phi) * Math.cos(spherical.theta);
    camera.position.y = orbitTarget.y + spherical.radius * Math.cos(spherical.phi);
    camera.position.z = orbitTarget.z + spherical.radius * Math.sin(spherical.phi) * Math.sin(spherical.theta);
    camera.lookAt(orbitTarget);
}

function resize() {
    if (!canvas || !canvas.parentElement) return;
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
function initUIBindings() {
    // Mode tabs
    document.querySelectorAll(".mode-tab").forEach(tab => {
        tab.addEventListener("click", () => {
            const mode = tab.dataset.mode;
            const modeMap = { calibrate: 1, emitter: 2, renderer: 3 };
            comboStates.mode.setChosenItemIndex(modeMap[mode] || 2);
        });
    });

    // Quality badge
    document.getElementById("quality-badge").addEventListener("click", function() {
        const isCurrentlyDraft = this.classList.contains("draft");
        comboStates.rend_quality.setChosenItemIndex(isCurrentlyDraft ? 2 : 1);
    });

    // Generic toggle binding
    document.querySelectorAll(".toggle").forEach(t => {
        t.addEventListener("click", () => t.classList.toggle("on"));
    });

    // View buttons
    document.querySelectorAll(".view-btn").forEach(btn => {
        btn.addEventListener("click", () => {
            document.querySelectorAll(".view-btn").forEach(b => b.classList.remove("active"));
            btn.classList.add("active");
            const views = {
                perspective: { theta: Math.PI/4, phi: Math.PI/4, radius: 8 },
                top:   { theta: 0, phi: 0.1, radius: 8 },
                front: { theta: 0, phi: Math.PI/2, radius: 8 },
                side:  { theta: Math.PI/2, phi: Math.PI/2, radius: 8 }
            };
            Object.assign(spherical, views[btn.dataset.view]);
            updateCamera();
        });
    });

    // Physics preset
    document.getElementById("physics-preset").addEventListener("change", function() {
        const active = document.getElementById("physics-active");
        if (this.value === "off") {
            active.style.display = "none";
            toggleStates.phys_enable.setValue(false);
        } else {
            active.style.display = "block";
            toggleStates.phys_enable.setValue(true);
        }
    });

    // Physics advanced disclosure
    document.getElementById("physics-disclosure").addEventListener("click", () => {
        document.getElementById("physics-advanced").classList.toggle("open");
        document.getElementById("physics-arrow").classList.toggle("open");
    });

    // Timeline lane selection
    document.querySelectorAll(".timeline-lane").forEach(lane => {
        lane.addEventListener("click", () => {
            document.querySelectorAll(".timeline-lane").forEach(l => l.classList.remove("selected"));
            lane.classList.add("selected");
            selectedLane = lane.dataset.lane;
            setLaneHighlight(selectedLane);
        });
    });

    // Animation toggle
    document.getElementById("toggle-anim").addEventListener("click", function() {
        const controls = document.getElementById("anim-controls");
        const isOn = this.classList.contains("on");
        controls.style.opacity = isOn ? "0.4" : "1.0";
        document.getElementById("anim-source").disabled = isOn;
    });
}

// ===== PARAMETER LISTENERS =====
function initParameterListeners() {
    // Mode changes from DAW
    comboStates.mode.valueChangedEvent.addListener(() => {
        const idx = comboStates.mode.getChosenItemIndex();
        const modes = ["calibrate", "emitter", "renderer"];
        const mode = modes[idx - 1] || "emitter";
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
    sliderStates.emit_gain.valueChangedEvent.addListener(() => {
        updateValueDisplay("val-gain", sliderStates.emit_gain.getScaledValue().toFixed(1), "dB");
    });
    sliderStates.emit_spread.valueChangedEvent.addListener(() => {
        updateValueDisplay("val-spread", sliderStates.emit_spread.getScaledValue().toFixed(2), "");
    });
    sliderStates.emit_directivity.valueChangedEvent.addListener(() => {
        updateValueDisplay("val-directivity", sliderStates.emit_directivity.getScaledValue().toFixed(2), "");
    });
    sliderStates.rend_master_gain.valueChangedEvent.addListener(() => {
        updateValueDisplay("val-master-gain", sliderStates.rend_master_gain.getScaledValue().toFixed(1), "dB");
    });

    // Quality badge from DAW
    comboStates.rend_quality.valueChangedEvent.addListener(() => {
        const badge = document.getElementById("quality-badge");
        const isFinal = comboStates.rend_quality.getChosenItemIndex() === 2;
        badge.className = "quality-badge " + (isFinal ? "final" : "draft");
        badge.textContent = isFinal ? "FINAL" : "DRAFT";
    });
}

function updateValueDisplay(id, value, unit) {
    const el = document.getElementById(id);
    if (el) {
        el.innerHTML = value + (unit ? '<span class="control-unit">' + unit + '</span>' : '');
    }
}

// ===== MODE SWITCHING =====
function switchMode(mode) {
    if (currentMode === mode) return;
    currentMode = mode;

    document.querySelectorAll(".mode-tab").forEach(t => t.classList.remove("active"));
    const activeTab = document.querySelector(`[data-mode="${mode}"]`);
    if (activeTab) activeTab.classList.add("active");

    document.querySelectorAll(".rail-panel").forEach(p => p.classList.remove("active"));
    const activePanel = document.querySelector(`[data-panel="${mode}"]`);
    if (activePanel) activePanel.classList.add("active");

    const tl = document.getElementById("timeline");
    if (mode === "emitter") tl.classList.add("visible"); else tl.classList.remove("visible");

    // Scene status
    const ss = document.getElementById("scene-status");
    ss.className = "scene-status";
    if (mode === "calibrate") { ss.textContent = "NO PROFILE"; ss.classList.add("noprofile"); }
    else if (mode === "renderer") { ss.textContent = "READY"; ss.classList.add("ready"); }
    else { ss.textContent = "STABLE"; }

    // 3D viewport adjustments
    selectionRing.visible = mode === "emitter";
    if (mode !== "emitter") setLaneHighlight(null);
    else setLaneHighlight(selectedLane);

    speakers.forEach(s => s.material.color.setHex(mode === "calibrate" ? 0xD4A847 : 0xE0E0E0));
    roomLines.material.opacity = mode === "calibrate" ? 0.15 : 0.3;

    setTimeout(resize, 10);
}

function setLaneHighlight(lane) {
    highlightTargets = { azimuth: 0, elevation: 0, distance: 0, size: 0 };
    if (lane) highlightTargets[lane] = lane === "distance" ? 0.15 : 0.25;
}

// ===== SCENE STATE FROM C++ =====
// Called by C++ via evaluateJavascript
window.updateSceneState = function(data) {
    sceneData = data;
    updateEmitterMeshes(data.emitters);
    updateSceneList(data.emitters);

    const info = document.getElementById("viewport-info");
    if (currentMode === "emitter") {
        info.textContent = "Emitter Mode \u00B7 " + data.emitterCount + " objects";
    } else if (currentMode === "renderer") {
        info.textContent = "Renderer Mode \u00B7 " + data.emitterCount + " emitters \u00B7 " +
            (document.getElementById("quality-badge").classList.contains("final") ? "Final" : "Draft");
    } else {
        info.textContent = "Calibrate Mode \u00B7 Room Profile Setup";
    }
};

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
            threeScene.add(mesh);
            emitterMeshes.set(em.id, mesh);
        }

        // Update position and properties
        mesh.position.set(em.x, em.y, em.z);
        mesh.scale.set(em.sx * 2, em.sy * 2, em.sz * 2);
        mesh.material.color.setHex(emitterPalette[em.color % emitterPalette.length]);
        mesh.material.opacity = em.muted ? 0.15 : 0.7;
        mesh.visible = currentMode !== "calibrate";
    });

    // Remove meshes for emitters no longer active
    for (const [id, mesh] of emitterMeshes) {
        if (!activeIds.has(id)) {
            threeScene.remove(mesh);
            emitterMeshes.delete(id);
        }
    }

    // Update selection ring for first emitter (this instance's emitter)
    if (emitters.length > 0 && currentMode === "emitter") {
        const first = emitters[0];
        selectionRing.position.set(first.x, first.y, first.z);
        selectionRing.visible = true;
    }
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
        item.innerHTML = `<span class="scene-dot" style="background:${color};"></span>` +
            `<span class="scene-name">${em.label}</span>` +
            `<button class="scene-action-btn">S</button>` +
            `<button class="scene-action-btn">M</button>`;
        list.appendChild(item);
    });
}

// ===== ANIMATION LOOP =====
function animate() {
    requestAnimationFrame(animate);
    animTime += 0.016;

    // Selection ring float
    if (selectionRing.visible) {
        selectionRing.position.y += Math.sin(animTime * 2) * 0.001;
    }

    // Speaker energy meters (simulated until real data)
    speakerMeters.forEach((m, i) => {
        if (currentMode === "renderer" || currentMode === "emitter") {
            m.target = 0.15 + Math.sin(animTime * 3 + i * 1.7) * 0.12 + Math.random() * 0.03;
        } else {
            m.target = 0;
        }
        m.level += (m.target - m.level) * 0.15;
        m.mesh.scale.y = Math.max(0.5, m.level * 40);
        m.mesh.position.y = m.basePos.y + m.level * 0.2;
        const t = Math.min(1, Math.max(0, (m.level - 0.2) / 0.3));
        m.mesh.material.color.setHex(t > 0.5 ? 0xD4A847 : 0xE0E0E0);
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
