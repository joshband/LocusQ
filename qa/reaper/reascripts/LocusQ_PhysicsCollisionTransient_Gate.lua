-- LocusQ collision transient host gate — headless REAPER acceptance test.
--
-- Proves, in a real host session, that:
--   A  param_registration       required params are visible on two LocusQ instances
--   B  gain_scale_response      higher Collision Gain Scale yields a bigger transient burst
--   C  decay_response           longer Collision Decay ms yields a larger late-window transient
--   D  transient_visible        collision transient becomes visible on the DAW mirror
--
-- Env vars (all optional):
--   LQ_REAPER_NONINTERACTIVE=1
--   LQ_REAPER_STATUS_JSON=<path>
--   LQ_REAPER_REQUIRE_LOCUSQ=1
--   LQ_REAPER_PROJECT_FILE=<path>

local NONINTERACTIVE = os.getenv("LQ_REAPER_NONINTERACTIVE") == "1"
local STATUS_JSON = os.getenv("LQ_REAPER_STATUS_JSON") or ""
local REQUIRE_LOCUSQ = os.getenv("LQ_REAPER_REQUIRE_LOCUSQ") ~= "0"

local SETTLE_DEFERS = 45
local COLLISION_DEFERS = 90
local THROW_PULSE_DEFERS = 2
local COLLISION_THRESHOLD = 0.005
local GAIN_DELTA_THRESHOLD = 0.05
local DECAY_DELTA_THRESHOLD = 0.03

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
  phys_collision_radius = "Collision Radius",
  phys_collide_emitters = "Collide Emitters",
  phys_collision_gain_scale = "Collision Gain Scale",
  phys_collision_decay_ms = "Collision Decay ms",
  phys_turbulence = "Turbulence",
  phys_spring_enable = "Spring Enable",
  phys_out_transient_0 = "Emitter 1 Physics Transient",
  phys_out_transient_1 = "Emitter 2 Physics Transient",
}

local log_lines = {}
local function log(msg)
  log_lines[#log_lines + 1] = msg
  if NONINTERACTIVE then
    reaper.ShowConsoleMsg("[LQ-collision-gate] " .. msg .. "\n")
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
    gate_b_gain_scale = false,
    gate_c_decay = false,
    gate_d_visible = false,
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

local function set_choice2(ctx, logical, index)
  set_normalized(ctx, logical, index <= 0 and 0.0 or 1.0)
end

local function set_linear(ctx, logical, actual, min_value, max_value)
  local normalized = (actual - min_value) / (max_value - min_value)
  set_normalized(ctx, logical, normalized)
end

local function get_transient_peak(contexts)
  local peak = 0.0
  for _, ctx in ipairs(contexts) do
    peak = math.max(peak, get_normalized(ctx, "phys_out_transient_0"))
    peak = math.max(peak, get_normalized(ctx, "phys_out_transient_1"))
  end
  return peak
end

local function gather_locusq_contexts()
  local found = {}
  local track_count = reaper.CountTracks(0)
  for t = 0, track_count - 1 do
    local track = reaper.GetTrack(0, t)
    local fx_count = reaper.TrackFX_GetCount(track)
    for fx = 0, fx_count - 1 do
      local _, fx_name = reaper.TrackFX_GetFXName(track, fx, "")
      if fx_name:find("LocusQ", 1, true) then
        found[#found + 1] = { track = track, fxidx = fx, fx_name = fx_name, track_index = t }
        break
      end
    end
  end
  return found
end

local function duplicate_first_locusq_track()
  local contexts = gather_locusq_contexts()
  if #contexts < 1 then
    return false
  end
  reaper.SetOnlyTrackSelected(contexts[1].track)
  reaper.Main_OnCommand(40062, 0) -- Track: Duplicate tracks
  reaper.TrackList_AdjustWindows(false)
  reaper.UpdateArrange()
  return true
end

local function build_param_indices(contexts)
  for _, ctx in ipairs(contexts) do
    ctx.param_idx = {}
    for logical, display in pairs(PARAM_NAMES) do
      local idx = find_param(ctx.track, ctx.fxidx, display)
      ctx.param_idx[logical] = idx
      if idx < 0 then
        return false, string.format("track %d missing param: %s", ctx.track_index + 1, display)
      end
    end
  end
  return true, ""
end

local function configure_emitter(ctx, pos_x, vel_x, gain_scale, decay_ms)
  set_bool(ctx, "phys_enable", true)
  set_linear(ctx, "phys_drag", 0.0, 0.0, 10.0)
  set_linear(ctx, "phys_gravity", 0.0, -20.0, 20.0)
  set_choice2(ctx, "pos_coord_mode", 1)
  set_linear(ctx, "pos_x", pos_x, -25.0, 25.0)
  set_linear(ctx, "pos_y", 0.0, -25.0, 25.0)
  set_linear(ctx, "pos_z", 1.2, -10.0, 10.0)
  set_linear(ctx, "phys_vel_x", vel_x, -50.0, 50.0)
  set_linear(ctx, "phys_vel_y", 0.0, -50.0, 50.0)
  set_linear(ctx, "phys_vel_z", 0.0, -50.0, 50.0)
  set_linear(ctx, "phys_collision_radius", 1.0, 0.05, 5.0)
  set_bool(ctx, "phys_collide_emitters", true)
  set_linear(ctx, "phys_collision_gain_scale", gain_scale, 0.0, 10.0)
  set_linear(ctx, "phys_collision_decay_ms", decay_ms, 1.0, 500.0)
  set_linear(ctx, "phys_turbulence", 0.0, 0.0, 10.0)
  set_bool(ctx, "phys_spring_enable", false)
  set_bool(ctx, "phys_throw", false)
  set_bool(ctx, "phys_reset", false)
end

local function pulse_toggle(contexts, logical)
  for _, ctx in ipairs(contexts) do set_bool(ctx, logical, true) end
  reaper.defer(function()
    for _, ctx in ipairs(contexts) do set_bool(ctx, logical, false) end
  end)
end

local function run_collision_scenario(contexts, gain_scale, decay_ms, done)
  configure_emitter(contexts[1], -0.25, 8.0, gain_scale, decay_ms)
  configure_emitter(contexts[2],  0.25, -8.0, gain_scale, decay_ms)

  local settle = 0
  local throw_clear = -1
  local tick = 0
  local collision_tick = -1
  local peak_transient = 0.0
  local late_sum = 0.0
  local late_count = 0

  local function step()
    if settle < SETTLE_DEFERS then
      if settle == 0 then
        for _, ctx in ipairs(contexts) do set_bool(ctx, "phys_reset", true) end
      elseif settle == 1 then
        for _, ctx in ipairs(contexts) do set_bool(ctx, "phys_reset", false) end
      end
      settle = settle + 1
      reaper.defer(step)
      return
    end

    if throw_clear < 0 then
      for _, ctx in ipairs(contexts) do set_bool(ctx, "phys_throw", true) end
      throw_clear = tick + THROW_PULSE_DEFERS
    elseif tick == throw_clear then
      for _, ctx in ipairs(contexts) do set_bool(ctx, "phys_throw", false) end
    end

    local transient = get_transient_peak(contexts)
    peak_transient = math.max(peak_transient, transient)
    if collision_tick < 0 and transient >= COLLISION_THRESHOLD then
      collision_tick = tick
    end

    if collision_tick >= 0 and tick >= collision_tick + 3 and tick <= collision_tick + 10 then
      late_sum = late_sum + transient
      late_count = late_count + 1
    end

    tick = tick + 1
    if tick < COLLISION_DEFERS then
      reaper.defer(step)
      return
    end

    done({
      peak_transient = peak_transient,
      late_mean = late_count > 0 and (late_sum / late_count) or 0.0,
      transient_visible = collision_tick >= 0,
      collision_tick = collision_tick,
      late_count = late_count,
    })
  end

  reaper.defer(step)
end

local gate_a_pass = false
local gate_b_pass = false
local gate_c_pass = false
local gate_d_pass = false

local contexts = {}
local low_metrics = nil
local high_metrics = nil
local short_metrics = nil
local long_metrics = nil

local function finish()
  reaper.Main_OnCommand(1016, 0)

  local gain_delta = (high_metrics and low_metrics) and (high_metrics.peak_transient - low_metrics.peak_transient) or 0.0
  local decay_delta = (long_metrics and short_metrics) and (long_metrics.late_mean - short_metrics.late_mean) or 0.0
  local overall = gate_a_pass and gate_b_pass and gate_c_pass and gate_d_pass
  local summary = table.concat(log_lines, "\n")

  write_status({
    status = overall and "pass" or "fail",
    gate_a_param_reg = gate_a_pass,
    gate_b_gain_scale = gate_b_pass,
    gate_b_gain_delta = gain_delta,
    gate_c_decay = gate_c_pass,
    gate_c_decay_delta = decay_delta,
    gate_d_visible = gate_d_pass,
    low_peak_transient = low_metrics and low_metrics.peak_transient or 0.0,
    high_peak_transient = high_metrics and high_metrics.peak_transient or 0.0,
    short_late_mean = short_metrics and short_metrics.late_mean or 0.0,
    long_late_mean = long_metrics and long_metrics.late_mean or 0.0,
    summary = summary,
  })

  log(string.format("PASS=%s gain_delta=%.3f decay_delta=%.3f", tostring(overall), gain_delta, decay_delta))
  quit_reaper()
end

local function run_scenarios()
  log("Running low-gain scenario")
  run_collision_scenario(contexts, 1.0, 50.0, function(metrics_low)
    low_metrics = metrics_low
    log(string.format("low: peak=%.3f late=%.3f visible=%s tick=%d",
      low_metrics.peak_transient, low_metrics.late_mean, tostring(low_metrics.transient_visible), low_metrics.collision_tick))

    log("Running high-gain scenario")
    run_collision_scenario(contexts, 10.0, 50.0, function(metrics_high)
      high_metrics = metrics_high
      log(string.format("high: peak=%.3f late=%.3f visible=%s tick=%d",
        high_metrics.peak_transient, high_metrics.late_mean, tostring(high_metrics.transient_visible), high_metrics.collision_tick))

      log("Running short-decay scenario")
      run_collision_scenario(contexts, 10.0, 25.0, function(metrics_short)
        short_metrics = metrics_short
        log(string.format("short: peak=%.3f late=%.3f visible=%s tick=%d",
          short_metrics.peak_transient, short_metrics.late_mean, tostring(short_metrics.transient_visible), short_metrics.collision_tick))

        log("Running long-decay scenario")
        run_collision_scenario(contexts, 10.0, 200.0, function(metrics_long)
          long_metrics = metrics_long
          log(string.format("long: peak=%.3f late=%.3f visible=%s tick=%d",
            long_metrics.peak_transient, long_metrics.late_mean, tostring(long_metrics.transient_visible), long_metrics.collision_tick))

          gate_d_pass = low_metrics.transient_visible and high_metrics.transient_visible
            and short_metrics.transient_visible and long_metrics.transient_visible
          gate_b_pass = gate_d_pass and (high_metrics.peak_transient - low_metrics.peak_transient) >= GAIN_DELTA_THRESHOLD
          gate_c_pass = gate_d_pass and (long_metrics.late_mean - short_metrics.late_mean) >= DECAY_DELTA_THRESHOLD
          if gate_d_pass then
            log("Gate D PASS: host-visible transient observed in every scenario")
          else
            log("Gate D FAIL: transient never surfaced on the DAW mirror in one or more scenarios")
          end
          if gate_b_pass then
            log("Gate B PASS: higher gain scale produced a larger transient burst")
          else
            log("Gate B FAIL: gain-scale response below threshold")
          end
          if gate_c_pass then
            log("Gate C PASS: longer decay kept the transient alive longer")
          else
            log("Gate C FAIL: decay response below threshold")
          end
          finish()
        end)
      end)
    end)
  end)
end

local function setup()
  log("LocusQ collision transient host gate starting")
  local current_contexts = gather_locusq_contexts()
  if #current_contexts < 2 then
    log("Only one LocusQ instance found; duplicating track for shared-worker collision lane")
    if not duplicate_first_locusq_track() then
      fatal("LocusQ FX not found in project. Load LocusQ on Track 1 first.", "locusq_not_found")
      return
    end
    local wait_count = 0
    local function wait_for_duplicate()
      wait_count = wait_count + 1
      current_contexts = gather_locusq_contexts()
      if #current_contexts >= 2 then
        contexts = { current_contexts[1], current_contexts[2] }
        local ok, err = build_param_indices(contexts)
        if not ok then
          fatal(err, "param_registration_fail")
          return
        end
        gate_a_pass = true
        log("Gate A PASS: required params found on both LocusQ instances")
        reaper.SetEditCurPos(0.0, false, false)
        reaper.Main_OnCommand(1007, 0)
        run_scenarios()
        return
      end
      if wait_count >= 30 then
        fatal("Failed to create a second LocusQ instance for collision testing", "duplicate_track_fail")
        return
      end
      reaper.defer(wait_for_duplicate)
    end
    reaper.defer(wait_for_duplicate)
    return
  end

  contexts = { current_contexts[1], current_contexts[2] }
  local ok, err = build_param_indices(contexts)
  if not ok then
    fatal(err, "param_registration_fail")
    return
  end

  gate_a_pass = true
  log("Gate A PASS: required params found on both LocusQ instances")
  reaper.SetEditCurPos(0.0, false, false)
  reaper.Main_OnCommand(1007, 0)
  run_scenarios()
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
