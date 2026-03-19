-- LocusQ spring/turbulence-spread host gate — headless REAPER acceptance test.
--
-- Proves, in a real host session, that:
--   A  gate_a_spring_spread     spring oscillation produces visible spread modulation
--   B  gate_b_turbulence_spread turbulence produces visible spread modulation
--   C  gate_c_baseline          spread stays quiet with both spring and turbulence off
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
local SPRING_DEFERS = 60
local TURBULENCE_DEFERS = 60
local BASELINE_DEFERS = 30
local SPRING_THRESHOLD = 0.05
local TURBULENCE_THRESHOLD = 0.05
local BASELINE_MAX_THRESHOLD = 0.01

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
  phys_spring_enable = "Spring Enable",
  phys_spring_k = "Spring Stiffness",
  phys_spring_damp = "Spring Damping",
  phys_turbulence = "Turbulence",
  phys_turbulence_rate = "Turbulence Rate",
  phys_collide_emitters = "Collide Emitters",
  rend_phys_interact = "Interact",
  phys_out_spread_mod_0 = "Emitter 1 Physics Spread",
}

local log_lines = {}
local function log(msg)
  log_lines[#log_lines + 1] = msg
  if NONINTERACTIVE then
    reaper.ShowConsoleMsg("[LQ-spring-turbulence-gate] " .. msg .. "\n")
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
    gate_a_spring_spread = false,
    gate_b_turbulence_spread = false,
    gate_c_baseline = false,
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

local function configure_scene(ctx)
  set_bool(ctx, "phys_enable", true)
  set_linear(ctx, "phys_drag", 0.5, 0.0, 10.0)
  set_linear(ctx, "phys_gravity", 0.0, -20.0, 20.0)
  set_choice2(ctx, "pos_coord_mode", 1)
  set_linear(ctx, "pos_x", 0.0, -25.0, 25.0)
  set_linear(ctx, "pos_y", 0.0, -25.0, 25.0)
  set_linear(ctx, "pos_z", 1.2, -10.0, 10.0)
  set_linear(ctx, "phys_vel_x", 0.5, -50.0, 50.0)
  set_linear(ctx, "phys_vel_y", 0.0, -50.0, 50.0)
  set_linear(ctx, "phys_vel_z", 0.0, -50.0, 50.0)
  set_bool(ctx, "phys_throw", false)
  set_bool(ctx, "phys_reset", false)
  set_bool(ctx, "phys_spring_enable", false)
  set_linear(ctx, "phys_spring_k", 2.0, 0.5, 500.0)
  set_linear(ctx, "phys_spring_damp", 0.1, 0.0, 1.0)
  set_linear(ctx, "phys_turbulence", 0.0, 0.0, 1.0)
  set_linear(ctx, "phys_turbulence_rate", 0.0, 0.1, 20.0)
  set_bool(ctx, "phys_collide_emitters", false)
  set_bool(ctx, "rend_phys_interact", false)
end

local function apply_spring_config(ctx)
  set_bool(ctx, "phys_spring_enable", true)
  set_linear(ctx, "phys_spring_k", 2.0, 0.5, 500.0)
  set_linear(ctx, "phys_spring_damp", 0.1, 0.0, 1.0)
  set_linear(ctx, "phys_turbulence", 0.0, 0.0, 1.0)
  set_linear(ctx, "phys_turbulence_rate", 0.0, 0.1, 20.0)
end

local function apply_turbulence_config(ctx)
  set_bool(ctx, "phys_spring_enable", false)
  set_linear(ctx, "phys_turbulence", 0.7, 0.0, 1.0)
  set_linear(ctx, "phys_turbulence_rate", 4.0, 0.1, 20.0)
end

local function apply_baseline_config(ctx)
  set_bool(ctx, "phys_spring_enable", false)
  set_linear(ctx, "phys_turbulence", 0.0, 0.0, 1.0)
  set_linear(ctx, "phys_turbulence_rate", 0.0, 0.1, 20.0)
end

local gate_a_pass = false
local gate_b_pass = false
local gate_c_pass = false
local context = nil

local metrics = {
  spring_sum = 0.0,
  spring_count = 0,
  turbulence_sum = 0.0,
  turbulence_count = 0,
  baseline_max = 0.0,
}

local state = "SETUP"
local defer_count = 0

local function finish()
  reaper.Main_OnCommand(1016, 0)
  local overall = gate_a_pass and gate_b_pass and gate_c_pass
  local spring_mean = metrics.spring_count > 0 and (metrics.spring_sum / metrics.spring_count) or 0.0
  local turbulence_mean = metrics.turbulence_count > 0 and (metrics.turbulence_sum / metrics.turbulence_count) or 0.0
  local summary = table.concat(log_lines, "\n")

  write_status({
    status = overall and "pass" or "fail",
    gate_a_spring_spread = gate_a_pass,
    gate_b_turbulence_spread = gate_b_pass,
    gate_c_baseline = gate_c_pass,
    spring_mean_spread = spring_mean,
    turbulence_mean_spread = turbulence_mean,
    baseline_max_spread = metrics.baseline_max,
    summary = summary,
  })

  log(string.format("PASS=%s spring_mean=%.3f turbulence_mean=%.3f baseline_max=%.3f",
    tostring(overall), spring_mean, turbulence_mean, metrics.baseline_max))
  quit_reaper()
end

local function tick()
  if state == "SETUP" then
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

    configure_scene(context)
    apply_spring_config(context)
    log("Gate A: spring config applied; spring_k=2.0 spring_damp=0.1 turbulence=0")
    reaper.SetEditCurPos(0.0, false, false)
    reaper.Main_OnCommand(1007, 0)
    state = "SETTLE"
    defer_count = 0
    reaper.defer(tick)
    return
  end

  if state == "SETTLE" then
    defer_count = defer_count + 1
    if defer_count < SETTLE_DEFERS then
      reaper.defer(tick)
      return
    end
    state = "SPRING"
    defer_count = 0
    reaper.defer(tick)
    return
  end

  if state == "SPRING" then
    local spread = get_normalized(context, "phys_out_spread_mod_0")
    metrics.spring_sum = metrics.spring_sum + spread
    metrics.spring_count = metrics.spring_count + 1
    defer_count = defer_count + 1
    if defer_count < SPRING_DEFERS then
      reaper.defer(tick)
      return
    end
    local spring_mean = metrics.spring_count > 0 and (metrics.spring_sum / metrics.spring_count) or 0.0
    gate_a_pass = spring_mean > SPRING_THRESHOLD
    if gate_a_pass then
      log(string.format("Gate A PASS: spring spread mean=%.4f > threshold=%.4f", spring_mean, SPRING_THRESHOLD))
    else
      log(string.format("Gate A FAIL: spring spread mean=%.4f <= threshold=%.4f", spring_mean, SPRING_THRESHOLD))
    end
    apply_turbulence_config(context)
    log("Gate B: turbulence config applied; spring=off turbulence=0.7 turbulence_rate=4.0")
    state = "TURBULENCE"
    defer_count = 0
    reaper.defer(tick)
    return
  end

  if state == "TURBULENCE" then
    local spread = get_normalized(context, "phys_out_spread_mod_0")
    metrics.turbulence_sum = metrics.turbulence_sum + spread
    metrics.turbulence_count = metrics.turbulence_count + 1
    defer_count = defer_count + 1
    if defer_count < TURBULENCE_DEFERS then
      reaper.defer(tick)
      return
    end
    local turbulence_mean = metrics.turbulence_count > 0 and (metrics.turbulence_sum / metrics.turbulence_count) or 0.0
    gate_b_pass = turbulence_mean > TURBULENCE_THRESHOLD
    if gate_b_pass then
      log(string.format("Gate B PASS: turbulence spread mean=%.4f > threshold=%.4f", turbulence_mean, TURBULENCE_THRESHOLD))
    else
      log(string.format("Gate B FAIL: turbulence spread mean=%.4f <= threshold=%.4f", turbulence_mean, TURBULENCE_THRESHOLD))
    end
    apply_baseline_config(context)
    log("Gate C: baseline config applied; spring=off turbulence=0")
    state = "BASELINE"
    defer_count = 0
    reaper.defer(tick)
    return
  end

  if state == "BASELINE" then
    local spread = get_normalized(context, "phys_out_spread_mod_0")
    metrics.baseline_max = math.max(metrics.baseline_max, spread)
    defer_count = defer_count + 1
    if defer_count < BASELINE_DEFERS then
      reaper.defer(tick)
      return
    end
    gate_c_pass = metrics.baseline_max < BASELINE_MAX_THRESHOLD
    if gate_c_pass then
      log(string.format("Gate C PASS: baseline spread max=%.4f < threshold=%.4f", metrics.baseline_max, BASELINE_MAX_THRESHOLD))
    else
      log(string.format("Gate C FAIL: baseline spread max=%.4f >= threshold=%.4f", metrics.baseline_max, BASELINE_MAX_THRESHOLD))
    end
    finish()
    return
  end
end

log("LocusQ spring/turbulence-spread host gate starting")
if reaper.CountTracks(0) > 0 then
  log("Project pre-loaded by REAPER (" .. reaper.CountTracks(0) .. " track(s))")
  reaper.defer(tick)
elseif REQUIRE_LOCUSQ then
  fatal("Project not pre-loaded; use the wrapper so REAPER opens the LocusQ session first.", "project_not_loaded")
else
  write_status({ status = "skipped", reason = "project_not_loaded" })
  quit_reaper()
end
