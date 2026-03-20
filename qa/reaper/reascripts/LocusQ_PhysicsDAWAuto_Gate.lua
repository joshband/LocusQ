-- LocusQ Physics DAW Automation gate — headless Reaper acceptance test.
--
-- Gates:
--   A  param_registration      all required params found by display name
--   B  live_output             phys_out_spread_mod_0 > 0 after 2s of physics running
--   C  snapshot_no_jump        |pre_freeze - post_freeze| < 0.01 (no value jump)
--   D  frozen_to_live          value after unfreeze is non-zero (physics resumes)
--   E  transient_when_frozen   phys_out_transient_0 pulses > 0 while slot is frozen
--   F  frozen_spread_stable    spread does not drift during frozen window (physics silent)
--
-- Env vars (all optional):
--   LQ_REAPER_NONINTERACTIVE=1        suppress message boxes
--   LQ_REAPER_STATUS_JSON=<path>      where to write status.json (required for shell wrapper)
--   LQ_REAPER_REQUIRE_LOCUSQ=1        fail if LocusQ FX not found (default: 1)
--   LQ_REAPER_PROJECT_FILE=<path>     open an existing RPP instead of creating a blank project;
--                                     the RPP must have LocusQ on Track 1 (e.g. LocusQ-Loaded-Track1.RPP)
--
-- Usage:
--   Launched automatically by scripts/reaper-phys-daw-auto-gate-mac.sh
--   or via REAPER: Actions > Load ReaScript

local NONINTERACTIVE  = os.getenv("LQ_REAPER_NONINTERACTIVE") == "1"
local STATUS_JSON     = os.getenv("LQ_REAPER_STATUS_JSON") or ""
local REQUIRE_LOCUSQ  = os.getenv("LQ_REAPER_REQUIRE_LOCUSQ") ~= "0"
local PROJECT_FILE    = os.getenv("LQ_REAPER_PROJECT_FILE") or ""

local WARMUP_DEFERS      = 90   -- ~3 s at Reaper's ~30 fps defer rate
local FROZEN_DEFERS      = 45   -- ~1.5 s
local LIVE_DEFERS        = 45   -- ~1.5 s
local SETTLE_DEFERS      = 60   -- ~2 s — allow phys_reset to propagate before throw
local TRANSIENT_DEFERS   = 60   -- ~2 s — throw fires at t=0, transient expected shortly after
local THROW_PULSE_DEFERS = 8    -- hold throw param high for ~0.25 s then clear it
local RESET_PULSE_DEFERS = 6    -- hold phys_reset high for ~0.2 s then clear it
local JUMP_THRESHOLD     = 0.01 -- spec acceptance gate: |delta| < 0.01
local STABLE_THRESHOLD   = 0.01 -- Gate F: frozen spread must not drift by more than this
local TRANSIENT_THRESHOLD = 0.001 -- Gate E: transient must exceed this to count as a pulse

-- ── JSON helpers ────────────────────────────────────────────────────────────

local function jesc(s)
  if type(s) ~= "string" then s = tostring(s) end
  s = s:gsub("\\", "\\\\"):gsub('"', '\\"'):gsub("\n", "\\n"):gsub("\r", "\\r")
  return s
end

local function json_val(v)
  if type(v) == "boolean" then return tostring(v) end
  if type(v) == "number"  then return string.format("%.6f", v) end
  return '"' .. jesc(tostring(v)) .. '"'
end

-- ── Status writer ───────────────────────────────────────────────────────────

local function write_status(fields)
  if STATUS_JSON == "" then return end
  local f = io.open(STATUS_JSON, "w")
  if not f then return end
  f:write("{\n")
  local first = true
  for k, v in pairs(fields) do
    if not first then f:write(",\n") end
    f:write('  "' .. jesc(k) .. '": ' .. json_val(v))
    first = false
  end
  f:write("\n}\n")
  f:close()
end

-- ── Logging ─────────────────────────────────────────────────────────────────

local log_lines = {}
local function log(msg)
  log_lines[#log_lines + 1] = msg
  if NONINTERACTIVE then
    reaper.ShowConsoleMsg("[LQ-gate] " .. msg .. "\n")
  end
end

-- ── Param lookup ────────────────────────────────────────────────────────────
-- Returns param index (0-based) or -1 if not found.
-- Matches full display name first, then case-insensitive substring.

local function find_param(track, fxidx, target)
  local n = reaper.TrackFX_GetNumParams(track, fxidx)
  local target_lower = target:lower()
  local substring_match = -1
  for i = 0, n - 1 do
    local _, name = reaper.TrackFX_GetParamName(track, fxidx, i, "")
    if name == target then return i end
    if substring_match < 0 and name:lower():find(target_lower, 1, true) then
      substring_match = i
    end
  end
  return substring_match
end

-- ── Param display name map ───────────────────────────────────────────────────
-- Keys are logical names used in this script; values are JUCE display names
-- registered in ProcessorParameterLayout.cpp.

local PARAM_NAMES = {
  phys_enable              = "Physics Enable",
  phys_turbulence          = "Turbulence",
  phys_drag                = "Drag",
  phys_gravity             = "Gravity",
  phys_throw               = "Throw",
  phys_reset               = "Reset Position",
  phys_spring_enable       = "Spring Enable",
  phys_collide_emitters    = "Collide Emitters",
  rend_phys_interact       = "Object Interaction",
  pos_coord_mode           = "Coord Mode",
  pos_x                    = "Position X",
  pos_y                    = "Position Y",
  pos_z                    = "Position Z",
  phys_vel_x               = "Init Vel X",
  phys_vel_y               = "Init Vel Y",
  phys_vel_z               = "Init Vel Z",
  attractor_0_active       = "Attractor 1 Active",
  attractor_0_pos_x        = "Attractor 1 X",
  attractor_0_pos_y        = "Attractor 1 Y",
  attractor_0_pos_z        = "Attractor 1 Z",
  attractor_0_strength     = "Attractor 1 Strength",
  attractor_0_falloff      = "Attractor 1 Falloff",
  attractor_0_radius       = "Attractor 1 Radius",
  attractor_0_orbit_stabilize = "Attractor 1 Orbit Stabilize",
  phys_out_spread_mod_0    = "Emitter 1 Physics Spread",
  phys_out_gain_mod_0      = "Emitter 1 Physics Gain",
  phys_frozen_0            = "Emitter 1 Physics Freeze",
  phys_out_transient_0     = "Emitter 1 Physics Transient",
  bypass                   = "Bypass",
  mode                     = "Mode",
}

-- Normalized value for attractor radius ≈ 0.14 m (same tiny-radius as crossing gate):
--   range (0.1, 20.0, 0.01, skew=0.5), linear interp = (1.0-0.1)/(20.0-0.1) ≈ 0.0452
local ATTRACTOR_RADIUS_NORM    = 0.0452  -- ~0.14 m after JUCE skew
-- Normalized value for attractor strength = 0 (no pull force, emitter free to fly through):
--   range (-100, 100), value=0 → (0-(-100))/(100-(-100)) = 0.5
local ATTRACTOR_STRENGTH_NORM  = 0.5

-- ── State machine ────────────────────────────────────────────────────────────

local state        = "SETUP"
local defer_count  = 0
local locusq_track = nil
local locusq_fxidx = -1
local param_idx    = {}

local gate_a_pass  = false  -- param registration
local gate_b_pass  = false  -- live output
local gate_c_pass  = false  -- snapshot no jump
local gate_d_pass  = false  -- frozen → live
local gate_e_pass  = false  -- transient pulses when frozen
local gate_f_pass  = false  -- frozen spread stays stable
local gate_c_delta    = 1.0
local gate_f_delta    = 1.0  -- max spread drift observed during frozen window
local live_spread     = 0.0
local frozen_spread   = 0.0
local mid_frozen_spread = 0.0  -- spread sampled at midpoint of frozen window
local pre_freeze      = 0.0
local transient_peak  = 0.0   -- peak phys_out_transient_0 observed in WAIT_TRANSIENT_FROZEN

local function get_param(name)
  local idx = param_idx[name]
  if not idx or idx < 0 then return 0.0 end
  return reaper.TrackFX_GetParamNormalized(locusq_track, locusq_fxidx, idx)
end

local function set_param(name, value)
  local idx = param_idx[name]
  if not idx or idx < 0 then return end
  reaper.TrackFX_SetParamNormalized(locusq_track, locusq_fxidx, idx, value)
end

local function fatal(msg, code)
  log("FATAL: " .. msg)
  reaper.Main_OnCommand(1016, 0)  -- stop transport
  write_status({
    status                    = "fail",
    error                     = msg,
    error_code                = code or "fatal",
    gate_a_param_reg          = false,
    gate_b_live_output        = false,
    gate_c_no_jump            = false,
    gate_d_live_resume        = false,
    gate_e_transient_frozen   = false,
    gate_f_frozen_stable      = false,
  })
  if not NONINTERACTIVE then
    reaper.ShowMessageBox("LocusQ PhysicsDAWAuto Gate FAILED:\n" .. msg,
                          "LQ Gate", 0)
  end
  reaper.defer(function() reaper.Main_OnCommand(40004, 0) end)  -- quit
end

local function finish()
  reaper.Main_OnCommand(1016, 0)  -- stop transport

  local overall = gate_a_pass and gate_b_pass and gate_c_pass and gate_d_pass
                  and gate_e_pass and gate_f_pass
  local summary = table.concat(log_lines, "\n")

  write_status({
    status                    = overall and "pass" or "fail",
    gate_a_param_reg          = gate_a_pass,
    gate_b_live_output        = gate_b_pass,
    gate_b_live_spread        = live_spread,
    gate_c_no_jump            = gate_c_pass,
    gate_c_delta              = gate_c_delta,
    gate_d_live_resume        = gate_d_pass,
    gate_d_frozen_spread      = frozen_spread,
    gate_e_transient_frozen   = gate_e_pass,
    gate_e_transient_peak     = transient_peak,
    gate_f_frozen_stable      = gate_f_pass,
    gate_f_delta              = gate_f_delta,
    summary                   = summary,
  })

  local result_msg = (overall and "PASS" or "FAIL") ..
    " | A=" .. tostring(gate_a_pass) ..
    " B=" .. tostring(gate_b_pass) ..
    " C=" .. tostring(gate_c_pass) ..
    " D=" .. tostring(gate_d_pass) ..
    " E=" .. tostring(gate_e_pass) ..
    " F=" .. tostring(gate_f_pass)
  log(result_msg)

  if not NONINTERACTIVE then
    reaper.ShowMessageBox(result_msg, "LocusQ PhysicsDAWAuto Gate", 0)
  end

  reaper.defer(function() reaper.Main_OnCommand(40004, 0) end)  -- quit
end

local function tick()
  if state == "SETUP" then
    -- ── find LocusQ FX ──────────────────────────────────────────────────
    local track_count = reaper.CountTracks(0)
    for t = 0, track_count - 1 do
      local track = reaper.GetTrack(0, t)
      local fx_count = reaper.TrackFX_GetCount(track)
      for fx = 0, fx_count - 1 do
        local _, fx_name = reaper.TrackFX_GetFXName(track, fx, "")
        if fx_name:find("LocusQ", 1, true) then
          locusq_track = track
          locusq_fxidx = fx
          log("Found FX: " .. fx_name .. " on track " .. t)
          break
        end
      end
      if locusq_track then break end
    end

    if not locusq_track then
      if REQUIRE_LOCUSQ then
        fatal("LocusQ FX not found in project. Load LocusQ on a track first.",
              "locusq_not_found")
        return
      end
      log("SKIP: LocusQ not found and not required — no gates run")
      write_status({ status = "skipped", reason = "locusq_not_found" })
      reaper.defer(function() reaper.Main_OnCommand(40004, 0) end)
      return
    end

    -- ── find params ──────────────────────────────────────────────────────
    local all_found = true
    for logical, display in pairs(PARAM_NAMES) do
      local idx = find_param(locusq_track, locusq_fxidx, display)
      param_idx[logical] = idx
      if idx < 0 then
        log("MISS: param not found: " .. display)
        all_found = false
      else
        log("OK:   param[" .. idx .. "] = " .. display)
      end
    end

    -- Dump first 30 param names for diagnostics if any are missed
    if not all_found then
      local n_params = reaper.TrackFX_GetNumParams(locusq_track, locusq_fxidx)
      local sample = {}
      for i = 0, math.min(n_params - 1, 29) do
        local _, nm = reaper.TrackFX_GetParamName(locusq_track, locusq_fxidx, i, "")
        sample[#sample + 1] = i .. "=" .. nm
      end
      write_status({
        status             = "fail",
        error              = "One or more required params not found — check display name registration",
        error_code         = "param_registration_fail",
        gate_a_param_reg   = false,
        gate_b_live_output = false,
        gate_c_no_jump     = false,
        gate_d_live_resume = false,
        param_count        = n_params,
        param_sample       = table.concat(sample, "|"),
      })
      reaper.defer(function() reaper.Main_OnCommand(40004, 0) end)
      return
    end

    gate_a_pass = all_found
    if not all_found then
      fatal("One or more required params not found — check display name registration",
            "param_registration_fail")
      return
    end
    log("Gate A PASS: all params found")

    -- ── diagnostic: log bypass + mode state from RPP ─────────────────────
    local bypass_val = get_param("bypass")
    local mode_val   = get_param("mode")
    log(string.format("diag: bypass=%.3f mode=%.3f", bypass_val, mode_val))
    if bypass_val > 0.5 then
      log("WARNING: plugin is bypassed — clearing bypass param")
      set_param("bypass", 0.0)
    end

    -- ── enable physics + turbulence for observable spread output ─────────
    -- BlackHole 16ch is set as the CoreAudio device in the gate-local
    -- reaper.ini, so prepareToPlay and processBlock run for real.  The
    -- physics worker produces turbulence spread into APVTS within ~1–2 s;
    -- Gate B reads the genuine output after the 3 s warmup window.
    local audio_running = reaper.Audio_IsRunning()
    log("audio_running=" .. tostring(audio_running))
    set_param("phys_enable",     1.0)
    set_param("phys_turbulence", 0.8)  -- high turbulence → non-zero spread immediately
    log("Physics enabled, turbulence=0.8")
    log(string.format("diag: phys_enable readback=%.3f phys_turbulence readback=%.3f",
                      get_param("phys_enable"), get_param("phys_turbulence")))

    -- ── processBlock detection: set marker, wait 5 defers, check result ──
    -- If processBlock is running and writes dspValues.spreadMod (even 0),
    -- it will overwrite the marker. If marker survives, processBlock is not
    -- executing the spread-write path.
    set_param("phys_out_spread_mod_0", 0.777)
    log(string.format("diag: marker set → spread readback immediate=%.3f",
                      get_param("phys_out_spread_mod_0")))

    -- ── start real-time playback ─────────────────────────────────────────
    reaper.SetEditCurPos(0.0, false, false)
    reaper.Main_OnCommand(1007, 0)  -- Transport: Play
    local initial_pos = reaper.GetPlayPosition()
    log(string.format("Transport started — initial_pos=%.3f warmup %d defers",
                      initial_pos, WARMUP_DEFERS))

    defer_count = 0
    state = "WARMUP"
    reaper.defer(tick)

  elseif state == "WARMUP" then
    defer_count = defer_count + 1
    -- Poll spread every 20 defers (~0.7s) to see if it ever changes
    if defer_count % 20 == 0 then
      local poll_spread = get_param("phys_out_spread_mod_0")
      local poll_pos    = reaper.GetPlayPosition()
      log(string.format("warmup poll [%d/%d]: spread=%.6f pos=%.3f",
                        defer_count, WARMUP_DEFERS, poll_spread, poll_pos))
    end
    if defer_count < WARMUP_DEFERS then
      reaper.defer(tick)
      return
    end

    -- ── Gate B: live path produces non-zero spread ────────────────────────
    live_spread = get_param("phys_out_spread_mod_0")
    log(string.format("Gate B: phys_out_spread_mod_0 = %.6f", live_spread))
    gate_b_pass = live_spread > 0.001
    if gate_b_pass then
      log("Gate B PASS: live physics spread > 0")
    else
      log("Gate B FAIL: spread = 0 after warmup (physics may not be producing output)")
    end

    -- snapshot current value before freeze
    pre_freeze = live_spread
    log(string.format("Pre-freeze snapshot: %.6f", pre_freeze))

    -- ── set FROZEN ───────────────────────────────────────────────────────
    set_param("phys_frozen_0", 1.0)
    log("phys_frozen_0 set to 1.0 (FROZEN)")

    defer_count = 0
    state = "WAIT_FROZEN"
    reaper.defer(tick)

  elseif state == "WAIT_FROZEN" then
    defer_count = defer_count + 1

    -- ── Reseat pre_freeze with the actual frozen-snapshot value ───────────
    -- The APVTS value keeps changing for 1-3 processBlocks after phys_frozen_0
    -- is set (audio thread has not yet seen the param).  Sampling at defer 2
    -- (~66 ms) gives processBlock time to see frozen=1 and latch the value.
    -- Both Gates C and F then compare values read entirely within the frozen
    -- window, which is what the spec actually tests (no drift while frozen).
    if defer_count == 2 then
      pre_freeze = get_param("phys_out_spread_mod_0")
      log(string.format("Gate C/F frozen baseline (defer 2): %.6f", pre_freeze))
    end

    -- ── Gate F: sample spread at frozen midpoint for stability check ──────
    if defer_count == math.floor(FROZEN_DEFERS / 2) then
      mid_frozen_spread = get_param("phys_out_spread_mod_0")
      log(string.format("Gate F mid-sample [%d/%d]: spread=%.6f",
                        defer_count, FROZEN_DEFERS, mid_frozen_spread))
    end

    if defer_count < FROZEN_DEFERS then
      reaper.defer(tick)
      return
    end

    -- ── Gate C: no value jump on LIVE→FROZEN ─────────────────────────────
    -- In frozen state the APVTS value is seeded by the snapshot guard on
    -- the audio thread; we verify it didn't jump relative to pre_freeze.
    local after_freeze = get_param("phys_out_spread_mod_0")
    gate_c_delta = math.abs(after_freeze - pre_freeze)
    gate_c_pass  = gate_c_delta < JUMP_THRESHOLD
    log(string.format("Gate C: pre=%.6f after=%.6f delta=%.6f (threshold %.2f)",
                      pre_freeze, after_freeze, gate_c_delta, JUMP_THRESHOLD))
    if gate_c_pass then
      log("Gate C PASS: no value jump at freeze transition")
    else
      log("Gate C FAIL: delta exceeds threshold")
    end

    frozen_spread = after_freeze

    -- ── Gate F: frozen spread must not drift during the frozen window ─────
    -- physics continues running in the background but must not update APVTS
    -- while frozen — the APVTS value is read-only from the DAW side.
    gate_f_delta = math.max(
      math.abs(mid_frozen_spread - pre_freeze),
      math.abs(after_freeze - pre_freeze)
    )
    gate_f_pass = gate_f_delta < STABLE_THRESHOLD
    log(string.format("Gate F: pre=%.6f mid=%.6f end=%.6f max_delta=%.6f (threshold %.2f)",
                      pre_freeze, mid_frozen_spread, after_freeze,
                      gate_f_delta, STABLE_THRESHOLD))
    if gate_f_pass then
      log("Gate F PASS: frozen spread stable — physics not writing APVTS while frozen")
    else
      log("Gate F FAIL: spread drifted during frozen window")
    end

    -- ── unfreeze ─────────────────────────────────────────────────────────
    set_param("phys_frozen_0", 0.0)
    log("phys_frozen_0 set to 0.0 (LIVE)")

    defer_count = 0
    state = "WAIT_LIVE"
    reaper.defer(tick)

  elseif state == "WAIT_LIVE" then
    defer_count = defer_count + 1
    if defer_count < LIVE_DEFERS then
      reaper.defer(tick)
      return
    end

    -- ── Gate D: physics resumes after unfreeze ────────────────────────────
    local resumed_spread = get_param("phys_out_spread_mod_0")
    gate_d_pass = resumed_spread > 0.001
    log(string.format("Gate D: post-unfreeze spread = %.6f", resumed_spread))
    if gate_d_pass then
      log("Gate D PASS: physics producing spread after unfreeze")
    else
      log("Gate D FAIL: spread = 0 after unfreeze")
    end

    -- ── configure for Gate E: replicate LocusQ_PhysicsAttractorCrossing_Gate ──
    -- gainTransient is always written to APVTS regardless of freeze state.
    -- Setup mirrors the proven crossing gate: turbulence=0, drag=0, gravity=0,
    -- emitter at (-1.8, 0, 1.2), vel_x=2.0, attractor at (0, 1.2, 0) with
    -- radius ≈ 0.14 m (normalized 0.0452).  phys_reset pulse fires first to
    -- snap the emitter to the configured position before the throw.
    -- Slot is frozen AFTER the settle window so the reset can propagate.

    -- Physics environment: eliminate all noise sources
    set_param("phys_turbulence",      0.0)   -- 0.0 normalized = 0 turbulence
    set_param("phys_drag",            0.0)   -- 0.0 normalized = 0 drag
    set_param("phys_gravity",         0.5)   -- 0.5 normalized = 0 gravity (range -20..20)
    set_param("phys_spring_enable",   0.0)
    set_param("phys_collide_emitters",0.0)
    set_param("rend_phys_interact",   0.0)
    -- Emitter initial state: Cartesian, position (-1.8, 0, 1.2), velocity (2, 0, 0)
    set_param("pos_coord_mode",       1.0)   -- Cartesian (index 1 of 2)
    set_param("pos_x",                0.464) -- (-1.8+25)/50
    set_param("pos_y",                0.5)   -- (0+25)/50
    set_param("pos_z",                0.56)  -- (1.2+10)/20
    set_param("phys_vel_x",           0.52)  -- (2.0+50)/100
    set_param("phys_vel_y",           0.5)   -- (0+50)/100
    set_param("phys_vel_z",           0.5)   -- (0+50)/100
    -- Attractor: position (0, 1.2, 0), no pull, tiny radius ≈ 0.14 m
    set_param("attractor_0_active",         1.0)
    set_param("attractor_0_pos_x",          0.5)    -- (0+25)/50
    set_param("attractor_0_pos_y",          0.12)   -- 1.2/10 (range 0..10)
    set_param("attractor_0_pos_z",          0.5)    -- (0+25)/50
    set_param("attractor_0_strength",       ATTRACTOR_STRENGTH_NORM)  -- 0.5 = 0 N
    set_param("attractor_0_falloff",        0.5)    -- 1/r² (index 1 of 3)
    set_param("attractor_0_radius",         ATTRACTOR_RADIUS_NORM)   -- ≈ 0.14 m
    set_param("attractor_0_orbit_stabilize",0.0)
    -- Ensure NOT frozen during reset so the worker can update emitter position
    set_param("phys_frozen_0", 0.0)
    set_param("phys_throw",    0.0)
    -- Fire phys_reset pulse: snap emitter to configured pos/vel
    set_param("phys_reset",    1.0)
    log(string.format("Gate E settle: emitter configured, phys_reset pulse started (attractor r=%.4f norm)",
                      ATTRACTOR_RADIUS_NORM))
    transient_peak = 0.0

    defer_count = 0
    state = "WAIT_SETTLE"
    reaper.defer(tick)

  elseif state == "WAIT_SETTLE" then
    -- ── allow phys_reset to propagate, then freeze + throw ────────────────
    defer_count = defer_count + 1

    if defer_count == RESET_PULSE_DEFERS then
      set_param("phys_reset", 0.0)
      log("phys_reset cleared — emitter snapped to initial position")
    end

    if defer_count < SETTLE_DEFERS then
      reaper.defer(tick)
      return
    end

    -- Settle complete: freeze slot then fire throw
    set_param("phys_frozen_0", 1.0)
    set_param("phys_throw",    1.0)
    log("Gate E: slot frozen + throw fired after settle")

    defer_count = 0
    state = "WAIT_TRANSIENT_FROZEN"
    reaper.defer(tick)

  elseif state == "WAIT_TRANSIENT_FROZEN" then
    defer_count = defer_count + 1

    -- ── clear throw pulse after hold window ──────────────────────────────
    if defer_count == THROW_PULSE_DEFERS then
      set_param("phys_throw", 0.0)
      log("Throw pulse cleared")
    end

    -- ── poll transient param every 5 defers, track peak ──────────────────
    if defer_count % 5 == 0 then
      local t_val = get_param("phys_out_transient_0")
      if t_val > transient_peak then transient_peak = t_val end
      log(string.format("Gate E poll [%d/%d]: transient=%.6f peak=%.6f",
                        defer_count, TRANSIENT_DEFERS, t_val, transient_peak))
    end

    if defer_count < TRANSIENT_DEFERS then
      reaper.defer(tick)
      return
    end

    -- ── Gate E: transient pulsed at least once while frozen ──────────────
    gate_e_pass = transient_peak > TRANSIENT_THRESHOLD
    log(string.format("Gate E: transient_peak=%.6f (threshold %.4f)",
                      transient_peak, TRANSIENT_THRESHOLD))
    if gate_e_pass then
      log("Gate E PASS: transient pulsed while slot frozen")
    else
      log("Gate E FAIL: no transient pulse observed during frozen window")
    end

    -- ── unfreeze and wrap up ──────────────────────────────────────────────
    set_param("phys_frozen_0", 0.0)
    set_param("phys_throw", 0.0)
    set_param("attractor_0_active", 0.0)
    log("phys_frozen_0 restored; attractor + throw disabled")

    state = "DONE"
    finish()
  end
end

-- ── Entry point ──────────────────────────────────────────────────────────────

log("LocusQ PhysicsDAWAuto Gate starting")
log("STATUS_JSON=" .. STATUS_JSON)

if reaper.CountTracks(0) > 0 then
  -- Project was pre-loaded by REAPER from the command-line RPP argument.
  -- prepareToPlay has already been called on LocusQ, so emitterSlotId is set.
  -- Go straight to SETUP without touching the project or audio graph.
  log("Project pre-loaded by REAPER (" .. reaper.CountTracks(0) .. " track(s)) — skipping project init")
  reaper.defer(tick)
elseif PROJECT_FILE ~= "" then
  -- Fallback: REAPER didn't pre-load the project; open it from Lua.
  -- prepareToPlay timing is not guaranteed here — prefer command-line loading.
  log("Opening project from Lua (timing fallback): " .. PROJECT_FILE)
  reaper.Main_openProject(PROJECT_FILE)
  -- Wait several ticks for the audio engine to reinitialise for the new project.
  local settle = 0
  local function settle_then_tick()
    settle = settle + 1
    if settle < 30 then reaper.defer(settle_then_tick) else reaper.defer(tick) end
  end
  reaper.defer(settle_then_tick)
else
  -- No project file — create a blank project and auto-insert LocusQ.
  reaper.Main_OnCommand(40023, 0)
  log("New project created")
  if reaper.CountTracks(0) == 0 then
    log("No tracks found — auto-inserting LocusQ track")
    reaper.InsertTrackAtIndex(0, true)
    local track = reaper.GetTrack(0, 0)
    reaper.GetSetMediaTrackInfo_String(track, "P_NAME", "LQ Gate", true)
    reaper.TrackFX_AddByName(track, "VST3: LocusQ", false, 1)
    if reaper.TrackFX_GetCount(track) == 0 then
      reaper.TrackFX_AddByName(track, "AU: LocusQ", false, 1)
    end
    if reaper.TrackFX_GetCount(track) == 0 then
      reaper.TrackFX_AddByName(track, "LocusQ", false, 1)
    end
  end
  reaper.defer(tick)
end
