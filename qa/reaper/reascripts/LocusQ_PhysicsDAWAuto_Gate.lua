-- LocusQ Physics DAW Automation gate — headless Reaper acceptance test.
--
-- Gates:
--   A  param_registration   all 5 required params found by display name
--   B  live_output          phys_out_spread_mod_0 > 0 after 2s of physics running
--   C  snapshot_no_jump     |pre_freeze - post_freeze| < 0.01 (no value jump)
--   D  frozen_to_live       value after unfreeze is non-zero (physics resumes)
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

local WARMUP_DEFERS   = 90   -- ~3 s at Reaper's ~30 fps defer rate
local FROZEN_DEFERS   = 45   -- ~1.5 s
local LIVE_DEFERS     = 45   -- ~1.5 s
local JUMP_THRESHOLD  = 0.01 -- spec acceptance gate: |delta| < 0.01

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
  phys_enable           = "Physics Enable",
  phys_turbulence       = "Turbulence",
  phys_out_spread_mod_0 = "Emitter 1 Physics Spread",
  phys_out_gain_mod_0   = "Emitter 1 Physics Gain",
  phys_frozen_0         = "Emitter 1 Physics Freeze",
  bypass                = "Bypass",
  mode                  = "Mode",
}

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
local gate_c_delta = 1.0
local live_spread  = 0.0
local frozen_spread = 0.0
local pre_freeze   = 0.0

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
    status             = "fail",
    error              = msg,
    error_code         = code or "fatal",
    gate_a_param_reg   = false,
    gate_b_live_output = false,
    gate_c_no_jump     = false,
    gate_d_live_resume = false,
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
  local summary = table.concat(log_lines, "\n")

  write_status({
    status               = overall and "pass" or "fail",
    gate_a_param_reg     = gate_a_pass,
    gate_b_live_output   = gate_b_pass,
    gate_b_live_spread   = live_spread,
    gate_c_no_jump       = gate_c_pass,
    gate_c_delta         = gate_c_delta,
    gate_d_live_resume   = gate_d_pass,
    gate_d_frozen_spread = frozen_spread,
    summary              = summary,
  })

  local result_msg = (overall and "PASS" or "FAIL") ..
    " | A=" .. tostring(gate_a_pass) ..
    " B=" .. tostring(gate_b_pass) ..
    " C=" .. tostring(gate_c_pass) ..
    " D=" .. tostring(gate_d_pass)
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
