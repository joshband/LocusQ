-- LocusQ boids-spread host gate — headless REAPER acceptance test.
--
-- Proves, in a real host session, that:
--   A  param_registration       required params are visible on two LocusQ instances
--   B  quiet_baseline          spread mirrors stay quiet with flocking disabled
--   C  boids_visible           enabling flocking produces visible spread modulation
--   D  decay_observed          disabling flocking returns spread close to baseline
--
-- Env vars (all optional):
--   LQ_REAPER_NONINTERACTIVE=1
--   LQ_REAPER_STATUS_JSON=<path>
--   LQ_REAPER_REQUIRE_LOCUSQ=1
--   LQ_REAPER_PROJECT_FILE=<path>

local NONINTERACTIVE = os.getenv("LQ_REAPER_NONINTERACTIVE") == "1"
local STATUS_JSON = os.getenv("LQ_REAPER_STATUS_JSON") or ""
local REQUIRE_LOCUSQ = os.getenv("LQ_REAPER_REQUIRE_LOCUSQ") ~= "0"
local PREBUILT_DUAL = os.getenv("LQ_REAPER_PREBUILT_DUAL") == "1"
local RESET_TRACKS = os.getenv("LQ_REAPER_RESET_TRACKS") == "1"
local EXPECTED_PROJECT_FILE = os.getenv("LQ_REAPER_PROJECT_FILE") or ""

local SETTLE_DEFERS = 45
local RESET_PULSE_DEFERS = 4
local BASELINE_DEFERS = 24
local ACTIVE_DEFERS = 70
local OFF_DEFERS = 40
local BASELINE_THRESHOLD = 0.01
local ACTIVE_THRESHOLD = 0.20
local OFF_THRESHOLD = 0.05
local OFF_RATIO_THRESHOLD = 0.35

local PARAM_NAMES = {
  phys_enable = "Physics Enable",
  mode = "Mode",
  phys_drag = "Drag",
  phys_gravity = "Gravity",
  rend_phys_rate = "Physics Rate",
  rend_phys_walls = "Wall Collision",
  rend_phys_pause = "Pause Physics",
  phys_boundary_mode = "Boundary Mode",
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
  phys_turbulence = "Turbulence",
  phys_collide_emitters = "Collide Emitters",
  rend_phys_interact = "Interact",
  phys_flock_group = "Flock Group",
  phys_flock_0_enable = "Flock 1 Enable",
  phys_flock_0_sep_weight = "Flock 1 Sep Weight",
  phys_flock_0_align_weight = "Flock 1 Align Weight",
  phys_flock_0_coh_weight = "Flock 1 Coh Weight",
  phys_flock_0_sep_radius = "Flock 1 Sep Radius",
  phys_flock_0_align_radius = "Flock 1 Align Radius",
  phys_flock_0_coh_radius = "Flock 1 Coh Radius",
  phys_flock_0_max_speed = "Flock 1 Max Speed",
  phys_out_spread_mod_0 = "Emitter 1 Physics Spread",
  phys_out_spread_mod_1 = "Emitter 2 Physics Spread",
  phys_out_spread_mod_2 = "Emitter 3 Physics Spread",
  phys_out_spread_mod_3 = "Emitter 4 Physics Spread",
  phys_out_spread_mod_4 = "Emitter 5 Physics Spread",
  phys_out_spread_mod_5 = "Emitter 6 Physics Spread",
  phys_out_spread_mod_6 = "Emitter 7 Physics Spread",
  phys_out_spread_mod_7 = "Emitter 8 Physics Spread",
}

local log_lines = {}
local function log(msg)
  log_lines[#log_lines + 1] = msg
  if NONINTERACTIVE then
    reaper.ShowConsoleMsg("[LQ-boids-gate] " .. msg .. "\n")
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
    gate_c_boids_visible = false,
    gate_d_decay_observed = false,
    summary = table.concat(log_lines, "\n"),
  })
  quit_reaper()
end

local function current_project_path()
  local _, path = reaper.EnumProjects(-1, "")
  return path or ""
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

local function get_formatted(ctx, logical)
  local idx = ctx.param_idx[logical]
  if not idx or idx < 0 then return "" end
  local _, value = reaper.TrackFX_GetFormattedParamValue(ctx.track, ctx.fxidx, idx, "")
  return value or ""
end

local function set_bool(ctx, logical, enabled)
  set_normalized(ctx, logical, enabled and 1.0 or 0.0)
end

local function set_choice2(ctx, logical, index)
  set_normalized(ctx, logical, index <= 0 and 0.0 or 1.0)
end

local function set_choice3(ctx, logical, index)
  set_normalized(ctx, logical, index / 2.0)
end

local function set_choice5(ctx, logical, index)
  set_normalized(ctx, logical, index / 4.0)
end

local function set_linear(ctx, logical, actual, min_value, max_value)
  local normalized = (actual - min_value) / (max_value - min_value)
  set_normalized(ctx, logical, normalized)
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

local function verify_expected_project()
  if EXPECTED_PROJECT_FILE == "" then
    return true
  end
  local active = current_project_path()
  if active == EXPECTED_PROJECT_FILE then
    log("Project path verified: " .. active)
    return true
  end
  fatal("Loaded project path mismatch. expected=" .. EXPECTED_PROJECT_FILE .. " active=" .. active, "project_path_mismatch")
  return false
end

local function reset_project_tracks(target_instances)
  local current_contexts = gather_locusq_contexts()
  if #current_contexts < 1 then
    fatal("Cannot reset project because no LocusQ track is present in the loaded session", "reset_no_locusq_track")
    return false
  end

  local keep_by_track = {}
  for i = 1, math.min(target_instances, #current_contexts) do
    keep_by_track[current_contexts[i].track] = true
  end

  for idx = reaper.CountTracks(0) - 1, 0, -1 do
    local track = reaper.GetTrack(0, idx)
    if not keep_by_track[track] then
      reaper.DeleteTrack(track)
    end
  end

  reaper.TrackList_AdjustWindows(false)
  reaper.UpdateArrange()
  log("Project tracks reset for boids gate target_instances=" .. tostring(target_instances) .. " remaining_tracks=" .. tostring(reaper.CountTracks(0)))
  return true
end

local function duplicate_first_locusq_track()
  local contexts = gather_locusq_contexts()
  if #contexts < 1 then
    return false
  end
  reaper.SetOnlyTrackSelected(contexts[1].track)
  reaper.Main_OnCommand(40062, 0)
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

local function configure_emitter(ctx, pos_x)
  set_choice2(ctx, "mode", 1)
  set_bool(ctx, "phys_enable", true)
  set_linear(ctx, "phys_drag", 0.0, 0.0, 10.0)
  set_linear(ctx, "phys_gravity", 0.0, -20.0, 20.0)
  set_choice2(ctx, "rend_phys_rate", 1)
  set_bool(ctx, "rend_phys_walls", true)
  set_bool(ctx, "rend_phys_pause", false)
  set_choice3(ctx, "phys_boundary_mode", 0)
  set_choice2(ctx, "pos_coord_mode", 1)
  set_linear(ctx, "pos_x", pos_x, -25.0, 25.0)
  set_linear(ctx, "pos_y", 0.0, -25.0, 25.0)
  set_linear(ctx, "pos_z", 1.2, -10.0, 10.0)
  set_linear(ctx, "phys_vel_x", 0.0, -50.0, 50.0)
  set_linear(ctx, "phys_vel_y", 0.0, -50.0, 50.0)
  set_linear(ctx, "phys_vel_z", 0.0, -50.0, 50.0)
  set_bool(ctx, "phys_throw", false)
  set_bool(ctx, "phys_spring_enable", false)
  set_linear(ctx, "phys_turbulence", 0.0, 0.0, 10.0)
  set_bool(ctx, "phys_collide_emitters", false)
  set_bool(ctx, "rend_phys_interact", false)
  set_choice5(ctx, "phys_flock_group", 0)
  set_bool(ctx, "phys_flock_0_enable", false)
  set_linear(ctx, "phys_flock_0_sep_weight", 0.10, 0.0, 1.0)
  set_linear(ctx, "phys_flock_0_align_weight", 0.35, 0.0, 1.0)
  set_linear(ctx, "phys_flock_0_coh_weight", 0.90, 0.0, 1.0)
  set_linear(ctx, "phys_flock_0_sep_radius", 0.75, 0.1, 20.0)
  set_linear(ctx, "phys_flock_0_align_radius", 4.0, 0.1, 20.0)
  set_linear(ctx, "phys_flock_0_coh_radius", 8.0, 0.1, 50.0)
  set_linear(ctx, "phys_flock_0_max_speed", 1.5, 0.1, 50.0)
  set_bool(ctx, "phys_reset", false)
end

local function set_boids_enabled(contexts, enabled)
  for _, ctx in ipairs(contexts) do
    set_choice5(ctx, "phys_flock_group", enabled and 1 or 0)
    set_bool(ctx, "phys_flock_0_enable", enabled)
  end
end

local function get_spread_peak(contexts)
  local peak = 0.0
  for _, ctx in ipairs(contexts) do
    for slot = 0, 7 do
      peak = math.max(peak, get_normalized(ctx, "phys_out_spread_mod_" .. slot))
    end
  end
  return peak
end

local function get_context_spread_peak(ctx)
  local peak = 0.0
  for slot = 0, 7 do
    peak = math.max(peak, get_normalized(ctx, "phys_out_spread_mod_" .. slot))
  end
  return peak
end

local gate_a_pass = false
local gate_b_pass = false
local gate_c_pass = false
local gate_d_pass = false
local contexts = {}
local metrics = {
  baseline_peak = 0.0,
  active_peak = 0.0,
  off_mean = 0.0,
  off_count = 0,
  active_ctx0_mode = -1.0,
  active_ctx1_mode = -1.0,
  active_ctx0_group = -1.0,
  active_ctx1_group = -1.0,
  active_ctx0_enable = -1.0,
  active_ctx1_enable = -1.0,
  active_ctx0_group_text = "",
  active_ctx1_group_text = "",
  active_ctx0_enable_text = "",
  active_ctx1_enable_text = "",
}

local state = "SETUP"
local defer_count = 0

local function finish()
  reaper.Main_OnCommand(1016, 0)
  local off_ratio = metrics.active_peak > 0.0 and (metrics.off_mean / metrics.active_peak) or 1.0
  local overall = gate_a_pass and gate_b_pass and gate_c_pass and gate_d_pass
  local summary = table.concat(log_lines, "\n")
  local ctx0 = contexts[1]
  local ctx1 = contexts[2]

  write_status({
    status = overall and "pass" or "fail",
    session_mode = PREBUILT_DUAL and "prepared_dual" or "duplicate_live",
    gate_a_param_reg = gate_a_pass,
    gate_b_quiet_baseline = gate_b_pass,
    gate_c_boids_visible = gate_c_pass,
    gate_d_decay_observed = gate_d_pass,
    baseline_peak_spread = metrics.baseline_peak,
    active_peak_spread = metrics.active_peak,
    off_mean_spread = metrics.off_mean,
    off_ratio = off_ratio,
    ctx0_mode = ctx0 and get_normalized(ctx0, "mode") or -1.0,
    ctx1_mode = ctx1 and get_normalized(ctx1, "mode") or -1.0,
    ctx0_flock_group = ctx0 and get_normalized(ctx0, "phys_flock_group") or -1.0,
    ctx1_flock_group = ctx1 and get_normalized(ctx1, "phys_flock_group") or -1.0,
    ctx0_flock_enable = ctx0 and get_normalized(ctx0, "phys_flock_0_enable") or -1.0,
    ctx1_flock_enable = ctx1 and get_normalized(ctx1, "phys_flock_0_enable") or -1.0,
    active_ctx0_mode = metrics.active_ctx0_mode,
    active_ctx1_mode = metrics.active_ctx1_mode,
    active_ctx0_flock_group = metrics.active_ctx0_group,
    active_ctx1_flock_group = metrics.active_ctx1_group,
    active_ctx0_flock_enable = metrics.active_ctx0_enable,
    active_ctx1_flock_enable = metrics.active_ctx1_enable,
    active_ctx0_flock_group_text = metrics.active_ctx0_group_text,
    active_ctx1_flock_group_text = metrics.active_ctx1_group_text,
    active_ctx0_flock_enable_text = metrics.active_ctx0_enable_text,
    active_ctx1_flock_enable_text = metrics.active_ctx1_enable_text,
    ctx0_peak_spread = ctx0 and get_context_spread_peak(ctx0) or 0.0,
    ctx1_peak_spread = ctx1 and get_context_spread_peak(ctx1) or 0.0,
    summary = summary,
  })

  log(string.format("PASS=%s baseline=%.3f active=%.3f off_mean=%.3f off_ratio=%.3f",
    tostring(overall), metrics.baseline_peak, metrics.active_peak, metrics.off_mean, off_ratio))
  quit_reaper()
end

local function tick()
  if state == "SETUP" then
    local current_contexts = gather_locusq_contexts()
    if #current_contexts < 2 then
      if PREBUILT_DUAL then
        fatal("Prepared dual-instance project did not expose two LocusQ instances", "prepared_dual_missing_instance")
        return
      end
      log("Only one LocusQ instance found; duplicating track for shared-worker boids lane")
      if not duplicate_first_locusq_track() then
        fatal("LocusQ FX not found in project. Load LocusQ on Track 1 first.", "locusq_not_found")
        return
      end
      state = "WAIT_DUPLICATE"
      defer_count = 0
      reaper.defer(tick)
      return
    end

    contexts = { current_contexts[1], current_contexts[2] }
    local ok, err = build_param_indices(contexts)
    if not ok then
      fatal(err, "param_registration_fail")
      return
    end

    configure_emitter(contexts[1], -1.5)
    configure_emitter(contexts[2], 1.5)
    set_boids_enabled(contexts, false)
    gate_a_pass = true
    log("Gate A PASS: required params found on both LocusQ instances")
    reaper.SetEditCurPos(0.0, false, false)
    reaper.Main_OnCommand(1007, 0)
    state = "SETTLE"
    defer_count = 0
    reaper.defer(tick)
    return
  end

  if state == "WAIT_DUPLICATE" then
    defer_count = defer_count + 1
    local current_contexts = gather_locusq_contexts()
    if #current_contexts >= 2 then
      contexts = { current_contexts[1], current_contexts[2] }
      local ok, err = build_param_indices(contexts)
      if not ok then
        fatal(err, "param_registration_fail")
        return
      end
      configure_emitter(contexts[1], -1.5)
      configure_emitter(contexts[2], 1.5)
      set_boids_enabled(contexts, false)
      gate_a_pass = true
      log("Gate A PASS: required params found on both LocusQ instances")
      reaper.SetEditCurPos(0.0, false, false)
      reaper.Main_OnCommand(1007, 0)
      state = "SETTLE"
      defer_count = 0
      reaper.defer(tick)
      return
    end
    if defer_count >= 30 then
      fatal("Failed to create a second LocusQ instance for boids testing", "duplicate_track_fail")
      return
    end
    reaper.defer(tick)
    return
  end

  if state == "SETTLE" then
    if defer_count == 0 then
      for _, ctx in ipairs(contexts) do set_bool(ctx, "phys_reset", true) end
    elseif defer_count == RESET_PULSE_DEFERS then
      for _, ctx in ipairs(contexts) do set_bool(ctx, "phys_reset", false) end
    end
    defer_count = defer_count + 1
    if defer_count < SETTLE_DEFERS then
      reaper.defer(tick)
      return
    end
    state = "BASELINE"
    defer_count = 0
    reaper.defer(tick)
    return
  end

  if state == "BASELINE" then
    metrics.baseline_peak = math.max(metrics.baseline_peak, get_spread_peak(contexts))
    defer_count = defer_count + 1
    if defer_count < BASELINE_DEFERS then
      reaper.defer(tick)
      return
    end
    gate_b_pass = metrics.baseline_peak <= BASELINE_THRESHOLD
    if gate_b_pass then
      log("Gate B PASS: baseline spread stayed quiet with boids disabled")
    else
      log("Gate B FAIL: baseline spread was already active before boids enable")
    end
    set_boids_enabled(contexts, true)
    state = "ACTIVE"
    defer_count = 0
    reaper.defer(tick)
    return
  end

  if state == "ACTIVE" then
    metrics.active_peak = math.max(metrics.active_peak, get_spread_peak(contexts))
    metrics.active_ctx0_mode = get_normalized(contexts[1], "mode")
    metrics.active_ctx1_mode = get_normalized(contexts[2], "mode")
    metrics.active_ctx0_group = get_normalized(contexts[1], "phys_flock_group")
    metrics.active_ctx1_group = get_normalized(contexts[2], "phys_flock_group")
    metrics.active_ctx0_enable = get_normalized(contexts[1], "phys_flock_0_enable")
    metrics.active_ctx1_enable = get_normalized(contexts[2], "phys_flock_0_enable")
    metrics.active_ctx0_group_text = get_formatted(contexts[1], "phys_flock_group")
    metrics.active_ctx1_group_text = get_formatted(contexts[2], "phys_flock_group")
    metrics.active_ctx0_enable_text = get_formatted(contexts[1], "phys_flock_0_enable")
    metrics.active_ctx1_enable_text = get_formatted(contexts[2], "phys_flock_0_enable")
    defer_count = defer_count + 1
    if defer_count < ACTIVE_DEFERS then
      reaper.defer(tick)
      return
    end
    gate_c_pass = metrics.active_peak >= ACTIVE_THRESHOLD
    if gate_c_pass then
      log("Gate C PASS: enabling boids produced visible spread modulation")
    else
      log("Gate C FAIL: spread stayed too low after boids enable")
    end
    set_boids_enabled(contexts, false)
    state = "OFF"
    defer_count = 0
    reaper.defer(tick)
    return
  end

  if state == "OFF" then
    metrics.off_mean = metrics.off_mean + get_spread_peak(contexts)
    metrics.off_count = metrics.off_count + 1
    defer_count = defer_count + 1
    if defer_count < OFF_DEFERS then
      reaper.defer(tick)
      return
    end
    metrics.off_mean = metrics.off_count > 0 and (metrics.off_mean / metrics.off_count) or 0.0
    local off_ratio = metrics.active_peak > 0.0 and (metrics.off_mean / metrics.active_peak) or 1.0
    gate_d_pass = metrics.off_mean <= OFF_THRESHOLD and off_ratio <= OFF_RATIO_THRESHOLD
    if gate_d_pass then
      log("Gate D PASS: spread decayed back down after boids disable")
    else
      log("Gate D FAIL: spread stayed too hot after boids disable")
    end
    finish()
    return
  end
end

log("LocusQ boids-spread host gate starting (" .. (PREBUILT_DUAL and "prepared_dual" or "duplicate_live") .. ")")
if reaper.CountTracks(0) > 0 then
  log("Project pre-loaded by REAPER (" .. reaper.CountTracks(0) .. " track(s))")
  if not verify_expected_project() then
    return
  end
  if RESET_TRACKS then
    local target_instances = PREBUILT_DUAL and 2 or 1
    if not reset_project_tracks(target_instances) then
      return
    end
  end
  reaper.defer(tick)
elseif REQUIRE_LOCUSQ then
  fatal("Project not pre-loaded; use the wrapper so REAPER opens the LocusQ session first.", "project_not_loaded")
else
  write_status({ status = "skipped", reason = "project_not_loaded" })
  quit_reaper()
end
