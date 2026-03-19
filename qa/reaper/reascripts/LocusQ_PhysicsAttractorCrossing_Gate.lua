-- LocusQ attractor-crossing transient host gate — headless REAPER acceptance test.
--
-- Proves, in a real host session, that:
--   A  param_registration       required params are visible on one LocusQ instance
--   B  quiet_baseline          transient mirror stays quiet before the throw
--   C  crossing_visible        attractor radius crossing produces a visible transient burst
--   D  decay_observed          the burst decays back down over a late observation window
--
-- Env vars (all optional):
--   LQ_REAPER_NONINTERACTIVE=1
--   LQ_REAPER_STATUS_JSON=<path>
--   LQ_REAPER_REQUIRE_LOCUSQ=1
--   LQ_REAPER_PROJECT_FILE=<path>

local NONINTERACTIVE = os.getenv("LQ_REAPER_NONINTERACTIVE") == "1"
local STATUS_JSON = os.getenv("LQ_REAPER_STATUS_JSON") or ""
local REQUIRE_LOCUSQ = os.getenv("LQ_REAPER_REQUIRE_LOCUSQ") ~= "0"

local SETTLE_DEFERS = 60
local CROSSING_DEFERS = 120
local RESET_PULSE_DEFERS = 6
local THROW_PULSE_DEFERS = 12
local BASELINE_WINDOW_END = 20
local TRANSIENT_THRESHOLD = 0.03
local BASELINE_THRESHOLD = 0.01
local LATE_WINDOW_START = 18
local LATE_WINDOW_END = 36
local DECAY_RATIO_THRESHOLD = 0.35

local PARAM_NAMES = {
  phys_enable = "Physics Enable",
  phys_drag = "Drag",
  phys_gravity = "Gravity",
  pos_coord_mode = "Coord Mode",
  pos_x = "Position X",
  pos_y = "Position Y",
  pos_z = "Position Z",
  phys_vel_x = "Init Vel X",
  phys_vel_y = "Init Vel Y",
  phys_vel_z = "Init Vel Z",
  phys_throw = "Throw",
  phys_reset = "Reset Position",
  phys_turbulence = "Turbulence",
  phys_spring_enable = "Spring Enable",
  phys_collide_emitters = "Collide Emitters",
  rend_phys_interact = "Interact",
  attractor_0_active = "Attractor 1 Active",
  attractor_0_pos_x = "Attractor 1 X",
  attractor_0_pos_y = "Attractor 1 Y",
  attractor_0_pos_z = "Attractor 1 Z",
  attractor_0_strength = "Attractor 1 Strength",
  attractor_0_falloff = "Attractor 1 Falloff",
  attractor_0_radius = "Attractor 1 Radius",
  attractor_0_orbit_stabilize = "Attractor 1 Orbit Stabilize",
  phys_out_transient_0 = "Emitter 1 Physics Transient",
  phys_out_transient_1 = "Emitter 2 Physics Transient",
  phys_out_transient_2 = "Emitter 3 Physics Transient",
  phys_out_transient_3 = "Emitter 4 Physics Transient",
  phys_out_transient_4 = "Emitter 5 Physics Transient",
  phys_out_transient_5 = "Emitter 6 Physics Transient",
  phys_out_transient_6 = "Emitter 7 Physics Transient",
  phys_out_transient_7 = "Emitter 8 Physics Transient",
}

local log_lines = {}
local function log(msg)
  log_lines[#log_lines + 1] = msg
  if NONINTERACTIVE then
    reaper.ShowConsoleMsg("[LQ-attractor-gate] " .. msg .. "\n")
  end
end

local function jesc(s)
  if type(s) ~= "string" then s = tostring(s) end
  s = s:gsub("\\", "\\\\"):gsub('"', '\\"'):gsub("\n", "\\n"):gsub("\r", "\\r")
  return s
end

local function json_val(v)
  if type(v) == "boolean" then return tostring(v) end
  if type(v) == "number" then return string.format("%.6f", v) end
  return '"' .. jesc(tostring(v)) .. '"'
end

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

local function quit_reaper()
  reaper.defer(function() reaper.Main_OnCommand(40004, 0) end)
end

local function fatal(msg, code)
  log("FATAL: " .. msg)
  reaper.Main_OnCommand(1016, 0)
  write_status({
    status = "fail",
    error = msg,
    error_code = code or "fatal",
    gate_a_param_reg = false,
    gate_b_quiet_baseline = false,
    gate_c_crossing_visible = false,
    gate_d_decay_observed = false,
    summary = table.concat(log_lines, "\n"),
  })
  quit_reaper()
end

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

local function set_normalized(ctx, logical, normalized)
  local idx = ctx.param_idx[logical]
  if not idx or idx < 0 then return end
  reaper.TrackFX_SetParamNormalized(ctx.track, ctx.fxidx, idx, math.max(0.0, math.min(1.0, normalized)))
end

local function get_normalized(ctx, logical)
  local idx = ctx.param_idx[logical]
  if not idx or idx < 0 then return 0.0 end
  return reaper.TrackFX_GetParamNormalized(ctx.track, ctx.fxidx, idx)
end

local function set_bool(ctx, logical, enabled)
  set_normalized(ctx, logical, enabled and 1.0 or 0.0)
end

local function set_choice3(ctx, logical, index)
  set_normalized(ctx, logical, index / 2.0)
end

local function set_choice2(ctx, logical, index)
  set_normalized(ctx, logical, index <= 0 and 0.0 or 1.0)
end

local function set_linear(ctx, logical, actual, min_value, max_value)
  local normalized = (actual - min_value) / (max_value - min_value)
  set_normalized(ctx, logical, normalized)
end

local function get_transient_peak(ctx)
  local peak = 0.0
  for slot = 0, 7 do
    peak = math.max(peak, get_normalized(ctx, "phys_out_transient_" .. slot))
  end
  return peak
end

local function gather_locusq_context()
  local track_count = reaper.CountTracks(0)
  for t = 0, track_count - 1 do
    local track = reaper.GetTrack(0, t)
    local fx_count = reaper.TrackFX_GetCount(track)
    for fx = 0, fx_count - 1 do
      local _, fx_name = reaper.TrackFX_GetFXName(track, fx, "")
      if fx_name:find("LocusQ", 1, true) then
        return { track = track, fxidx = fx, fx_name = fx_name, track_index = t }
      end
    end
  end
  return nil
end

local function build_param_indices(ctx)
  ctx.param_idx = {}
  for logical, display in pairs(PARAM_NAMES) do
    local idx = find_param(ctx.track, ctx.fxidx, display)
    ctx.param_idx[logical] = idx
    if idx < 0 then
      return false, string.format("track %d missing param: %s", ctx.track_index + 1, display)
    end
  end
  return true, ""
end

local function configure_emitter(ctx)
  set_bool(ctx, "phys_enable", true)
  set_linear(ctx, "phys_drag", 0.0, 0.0, 10.0)
  set_linear(ctx, "phys_gravity", 0.0, -20.0, 20.0)
  set_choice2(ctx, "pos_coord_mode", 1)
  set_linear(ctx, "pos_x", -1.8, -25.0, 25.0)
  set_linear(ctx, "pos_y", 0.0, -25.0, 25.0)
  set_linear(ctx, "pos_z", 1.2, -10.0, 10.0)
  set_linear(ctx, "phys_vel_x", 2.0, -50.0, 50.0)
  set_linear(ctx, "phys_vel_y", 0.0, -50.0, 50.0)
  set_linear(ctx, "phys_vel_z", 0.0, -50.0, 50.0)
  set_bool(ctx, "phys_spring_enable", false)
  set_linear(ctx, "phys_turbulence", 0.0, 0.0, 10.0)
  set_bool(ctx, "phys_collide_emitters", false)
  set_bool(ctx, "rend_phys_interact", false)
  set_bool(ctx, "attractor_0_active", true)
  set_linear(ctx, "attractor_0_pos_x", 0.0, -25.0, 25.0)
  set_linear(ctx, "attractor_0_pos_y", 1.2, 0.0, 10.0)
  set_linear(ctx, "attractor_0_pos_z", 0.0, -25.0, 25.0)
  set_linear(ctx, "attractor_0_strength", 0.0, -100.0, 100.0)
  set_choice3(ctx, "attractor_0_falloff", 1)
  set_linear(ctx, "attractor_0_radius", 1.0, 0.1, 20.0)
  set_bool(ctx, "attractor_0_orbit_stabilize", false)
  set_bool(ctx, "phys_throw", false)
  set_bool(ctx, "phys_reset", false)
end

local function run_crossing_scenario(ctx, done)
  configure_emitter(ctx)

  local settle = 0
  local throw_clear = -1
  local tick = 0
  local crossing_tick = -1
  local baseline_peak = 0.0
  local peak_transient = 0.0
  local late_sum = 0.0
  local late_count = 0

  local function step()
    if settle < SETTLE_DEFERS then
      if settle == 0 then
        set_bool(ctx, "phys_reset", true)
      elseif settle == RESET_PULSE_DEFERS then
        set_bool(ctx, "phys_reset", false)
      end
      settle = settle + 1
      reaper.defer(step)
      return
    end

    if throw_clear < 0 then
      set_bool(ctx, "phys_throw", true)
      throw_clear = tick + THROW_PULSE_DEFERS
    elseif tick == throw_clear then
      set_bool(ctx, "phys_throw", false)
    end

    local transient = get_transient_peak(ctx)
    peak_transient = math.max(peak_transient, transient)
    if tick <= BASELINE_WINDOW_END then
      baseline_peak = math.max(baseline_peak, transient)
    end
    if crossing_tick < 0 and transient >= TRANSIENT_THRESHOLD then
      crossing_tick = tick
    end
    if crossing_tick >= 0 and tick >= crossing_tick + LATE_WINDOW_START and tick <= crossing_tick + LATE_WINDOW_END then
      late_sum = late_sum + transient
      late_count = late_count + 1
    end

    tick = tick + 1
    if tick < CROSSING_DEFERS then
      reaper.defer(step)
      return
    end

    done({
      baseline_peak = baseline_peak,
      peak_transient = peak_transient,
      late_mean = late_count > 0 and (late_sum / late_count) or 0.0,
      transient_visible = crossing_tick >= 0,
      crossing_tick = crossing_tick,
      late_count = late_count,
    })
  end

  reaper.defer(step)
end

local gate_a_pass = false
local gate_b_pass = false
local gate_c_pass = false
local gate_d_pass = false
local metrics = nil
local context = nil

local function finish()
  reaper.Main_OnCommand(1016, 0)

  local overall = gate_a_pass and gate_b_pass and gate_c_pass and gate_d_pass
  local summary = table.concat(log_lines, "\n")
  local decay_ratio = (metrics and metrics.peak_transient > 0.0) and (metrics.late_mean / metrics.peak_transient) or 1.0

  write_status({
    status = overall and "pass" or "fail",
    gate_a_param_reg = gate_a_pass,
    gate_b_quiet_baseline = gate_b_pass,
    gate_c_crossing_visible = gate_c_pass,
    gate_d_decay_observed = gate_d_pass,
    baseline_peak_transient = metrics and metrics.baseline_peak or 0.0,
    peak_transient = metrics and metrics.peak_transient or 0.0,
    late_mean = metrics and metrics.late_mean or 0.0,
    crossing_tick = metrics and metrics.crossing_tick or -1,
    decay_ratio = decay_ratio,
    summary = summary,
  })

  log(string.format("PASS=%s baseline=%.3f peak=%.3f late=%.3f decay_ratio=%.3f",
    tostring(overall),
    metrics and metrics.baseline_peak or 0.0,
    metrics and metrics.peak_transient or 0.0,
    metrics and metrics.late_mean or 0.0,
    decay_ratio))
  quit_reaper()
end

local function run_scenario()
  log("Running attractor-crossing scenario")
  run_crossing_scenario(context, function(run_metrics)
    metrics = run_metrics
    gate_b_pass = metrics.baseline_peak <= BASELINE_THRESHOLD
    gate_c_pass = metrics.transient_visible and metrics.peak_transient >= TRANSIENT_THRESHOLD
    gate_d_pass = gate_c_pass
      and metrics.late_count > 0
      and metrics.late_mean < metrics.peak_transient
      and (metrics.late_mean / metrics.peak_transient) <= DECAY_RATIO_THRESHOLD

    log(string.format("scenario: baseline=%.3f peak=%.3f late=%.3f visible=%s tick=%d",
      metrics.baseline_peak,
      metrics.peak_transient,
      metrics.late_mean,
      tostring(metrics.transient_visible),
      metrics.crossing_tick))

    if gate_b_pass then
      log("Gate B PASS: baseline stayed quiet before the crossing throw")
    else
      log("Gate B FAIL: baseline transient was already active before the crossing")
    end
    if gate_c_pass then
      log("Gate C PASS: attractor crossing produced a visible host transient")
    else
      log("Gate C FAIL: no visible host transient after the attractor crossing")
    end
    if gate_d_pass then
      log("Gate D PASS: host-visible transient decayed after the crossing burst")
    else
      log("Gate D FAIL: late-window host transient did not decay enough")
    end
    finish()
  end)
end

local function setup()
  log("LocusQ attractor-crossing host gate starting")
  context = gather_locusq_context()
  if not context then
    fatal("LocusQ FX not found in project. Load LocusQ on Track 1 first.", "locusq_not_found")
    return
  end

  local ok, err = build_param_indices(context)
  if not ok then
    fatal(err, "param_registration_fail")
    return
  end

  gate_a_pass = true
  log("Gate A PASS: required params found on the LocusQ instance")
  reaper.SetEditCurPos(0.0, false, false)
  reaper.Main_OnCommand(1007, 0)
  run_scenario()
end

if reaper.CountTracks(0) > 0 then
  log("Project pre-loaded by REAPER (" .. reaper.CountTracks(0) .. " track(s))")
  reaper.defer(setup)
elseif REQUIRE_LOCUSQ then
  fatal("Project not pre-loaded; use the wrapper so REAPER opens the LocusQ session first.", "project_not_loaded")
else
  write_status({ status = "skipped", reason = "project_not_loaded" })
  quit_reaper()
end
