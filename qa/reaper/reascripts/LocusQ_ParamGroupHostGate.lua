-- LocusQ BL-079 Parameter Group Host Gate — headless Reaper parameter-layout verification.
--
-- Gates:
--   A  param_count     total host-visible params >= MIN_PARAM_COUNT
--   B  group_names     all required group-boundary parameter names are present
--   C  flat_order      "Mode" is at index 0 (Global group first in flat layout)
--   D  section_order   "Master Gain" index > "Color" index (Renderer after Emitter/Identity)
--
-- Method:
--   1. Find LocusQ on any track (or insert it into a fresh project).
--   2. Start transport so prepareToPlay() fires and layout is finalised.
--   3. Enumerate all TrackFX parameters via TrackFX_GetParamName.
--   4. Run the four gates and write status.json.
--
-- Env vars:
--   LQ_REAPER_NONINTERACTIVE=1        suppress message boxes
--   LQ_REAPER_STATUS_JSON=<path>      where to write status.json (required for shell wrapper)
--   LQ_REAPER_REQUIRE_LOCUSQ=1        fail if LocusQ FX not found (default: 1)
--   LQ_REAPER_PROJECT_FILE=<path>     open an existing RPP instead of creating a blank project
--
-- Usage:
--   Launched automatically by scripts/reaper-param-group-host-gate-mac.sh
--   or via REAPER: Actions > Load ReaScript

local NONINTERACTIVE  = os.getenv("LQ_REAPER_NONINTERACTIVE") == "1"
local STATUS_JSON     = os.getenv("LQ_REAPER_STATUS_JSON") or ""
local REQUIRE_LOCUSQ  = os.getenv("LQ_REAPER_REQUIRE_LOCUSQ") ~= "0"
local PROJECT_FILE    = os.getenv("LQ_REAPER_PROJECT_FILE") or ""
local PREFERRED_FX    = os.getenv("LQ_REAPER_PREFERRED_FX") or ""

-- Minimum total param count (APVTS layout, confirmed by physics DAW gate).
local MIN_PARAM_COUNT = 90

-- All 4 top-level group entry points + 11 subgroup first-params must be present.
-- Name strings must match exactly what JUCE exports to VST3 hosts.
local REQUIRED_NAMES = {
  -- Global
  "Mode",
  "Bypass",
  -- Calibration
  "Speaker Config",
  "Topology Profile",
  -- Emitter > Position
  "Azimuth",
  -- Emitter > Size
  "Width",
  -- Emitter > Audio
  "Emitter Gain",
  -- Emitter > Physics
  "Physics Enable",
  "Spring Enable",
  "Angular Enable",
  "Flock Group",
  -- Emitter > Animation
  "Animation Enable",
  -- Emitter > Identity
  "Color",
  -- Renderer > Master
  "Master Gain",
  -- Renderer > Spatialization
  "Quality",
  -- Renderer > Room
  "Room Enable",
  -- Renderer > Physics
  "Physics Rate",
  -- Renderer > Scene Physics (attractors)
  "Attractor 1 Active",
  -- Renderer > Scene Physics (boids)
  "Flock 1 Enable",
  -- Renderer > Visualization
  "View Mode",
}

local INIT_DEFERS = 30  -- ~1 s at ~30 fps

-- ── JSON helpers ─────────────────────────────────────────────────────────────

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

-- ── Logging ──────────────────────────────────────────────────────────────────

local log_lines = {}
local function log(msg)
  log_lines[#log_lines + 1] = msg
  reaper.ShowConsoleMsg("[LQ-param-group-gate] " .. msg .. "\n")
end

local function fx_matches_preference(fx_name)
  if PREFERRED_FX == "" then
    return fx_name:find("LocusQ", 1, true) ~= nil
  end

  return fx_name:find(PREFERRED_FX, 1, true) ~= nil
end

-- ── State ────────────────────────────────────────────────────────────────────

local state        = "SETUP"
local defer_count  = 0
local locusq_track = nil
local locusq_fxidx = -1

local function fatal(msg, code)
  log("FATAL: " .. msg)
  reaper.Main_OnCommand(1016, 0)
  write_status({
    status             = "fail",
    error              = msg,
    error_code         = code or "fatal",
    gate_a_count       = false,
    gate_b_names       = false,
    gate_c_order_first = false,
    gate_d_order_sect  = false,
  })
  if not NONINTERACTIVE then
    reaper.ShowMessageBox("LocusQ Param Group Gate FAILED:\n" .. msg, "LQ Param Group Gate", 0)
  end
  reaper.defer(function() reaper.Main_OnCommand(40004, 0) end)
end

local function finish(results)
  reaper.Main_OnCommand(1016, 0)
  local all_pass = results.gate_a_count and results.gate_b_names
                   and results.gate_c_order_first and results.gate_d_order_sect
  results.status  = all_pass and "pass" or "fail"
  results.summary = table.concat(log_lines, "\n")
  write_status(results)
  local verdict = all_pass and "PASS" or "FAIL"
  log(verdict .. " | count=" .. tostring(results.param_count)
      .. " gate_a=" .. tostring(results.gate_a_count)
      .. " gate_b=" .. tostring(results.gate_b_names)
      .. " gate_c=" .. tostring(results.gate_c_order_first)
      .. " gate_d=" .. tostring(results.gate_d_order_sect))
  if not NONINTERACTIVE then
    reaper.ShowMessageBox(verdict, "LocusQ Param Group Gate", 0)
  end
  reaper.defer(function() reaper.Main_OnCommand(40004, 0) end)
end

-- ── Tick ─────────────────────────────────────────────────────────────────────

local function tick()
  if state == "SETUP" then
    local track_count = reaper.CountTracks(0)
    for t = 0, track_count - 1 do
      local track = reaper.GetTrack(0, t)
      for fx = 0, reaper.TrackFX_GetCount(track) - 1 do
        local _, fx_name = reaper.TrackFX_GetFXName(track, fx, "")
        if fx_matches_preference(fx_name) then
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
        local missing_name = (PREFERRED_FX ~= "" and PREFERRED_FX or "LocusQ")
        fatal(missing_name .. " FX not found in project", "locusq_not_found")
      else
        log("SKIP: LocusQ not found and not required")
        write_status({ status = "skipped", reason = "locusq_not_found" })
        reaper.defer(function() reaper.Main_OnCommand(40004, 0) end)
      end
      return
    end

    reaper.SetEditCurPos(0.0, false, false)
    reaper.Main_OnCommand(1007, 0)  -- Play
    log(string.format("Transport started — waiting %d defers", INIT_DEFERS))
    defer_count = 0
    state = "WAIT_INIT"
    reaper.defer(tick)

  elseif state == "WAIT_INIT" then
    defer_count = defer_count + 1
    if defer_count < INIT_DEFERS then
      reaper.defer(tick)
      return
    end

    -- ── Enumerate all parameters ───────────────────────────────────────────
    local num_params = reaper.TrackFX_GetNumParams(locusq_track, locusq_fxidx)
    log("param_count=" .. tostring(num_params))

    -- Build name → index and index → name tables
    local name_to_idx = {}
    local idx_to_name = {}
    for i = 0, num_params - 1 do
      local _, pname = reaper.TrackFX_GetParamName(locusq_track, locusq_fxidx, i, "")
      name_to_idx[pname] = i
      idx_to_name[i] = pname
    end

    -- ── Gate A: param count ────────────────────────────────────────────────
    local gate_a = (num_params >= MIN_PARAM_COUNT)
    log(string.format("Gate A count: %s (%d >= %d)",
                      tostring(gate_a), num_params, MIN_PARAM_COUNT))

    -- ── Gate B: required names present ────────────────────────────────────
    local gate_b = true
    local missing = {}
    for _, req in ipairs(REQUIRED_NAMES) do
      if name_to_idx[req] == nil then
        gate_b = false
        missing[#missing + 1] = req
        log("MISSING: '" .. req .. "'")
      else
        log(string.format("  [ok] '%s' at idx %d", req, name_to_idx[req]))
      end
    end
    log("Gate B names: " .. tostring(gate_b)
        .. (gate_b and "" or (" — missing: " .. table.concat(missing, ", "))))

    -- ── Gate C: "Mode" is at index 0 (VST3) or index 1 (AU) ──────────────
    -- JUCE's AU wrapper inserts a host-side bypass parameter at index 0,
    -- shifting all user params by 1.  VST3 handles bypass via a separate
    -- native mechanism, so Mode lands at index 0 there.
    local mode_idx = name_to_idx["Mode"]
    local is_au = (PREFERRED_FX:sub(1, 2) == "AU")
    local expected_mode_idx = is_au and 1 or 0
    local gate_c = (mode_idx == expected_mode_idx)
    log(string.format("Gate C flat_order: %s (Mode idx=%s, expected %d, format=%s)",
                      tostring(gate_c), tostring(mode_idx),
                      expected_mode_idx, is_au and "AU" or "VST3"))

    -- ── Gate D: Renderer after Emitter/Identity ("Master Gain" > "Color") ─
    local color_idx       = name_to_idx["Color"]
    local master_gain_idx = name_to_idx["Master Gain"]
    local gate_d = false
    if color_idx and master_gain_idx then
      gate_d = (master_gain_idx > color_idx)
    end
    log(string.format("Gate D section_order: %s (Color idx=%s, MasterGain idx=%s)",
                      tostring(gate_d), tostring(color_idx), tostring(master_gain_idx)))

    -- ── Write TSV param list ───────────────────────────────────────────────
    if STATUS_JSON ~= "" then
      local tsv_path = STATUS_JSON:gsub("status.json$", "param_names.tsv")
      local tf = io.open(tsv_path, "w")
      if tf then
        tf:write("index\tname\n")
        for i = 0, num_params - 1 do
          tf:write(string.format("%d\t%s\n", i, idx_to_name[i] or ""))
        end
        tf:close()
        log("param_names.tsv written: " .. tsv_path)
      end
    end

    state = "DONE"
    finish({
      param_count         = num_params,
      min_param_count     = MIN_PARAM_COUNT,
      gate_a_count        = gate_a,
      gate_b_names        = gate_b,
      gate_c_order_first  = gate_c,
      gate_d_order_sect   = gate_d,
      missing_names       = table.concat(missing, ", "),
      mode_idx            = mode_idx or -1,
      color_idx           = color_idx or -1,
      master_gain_idx     = master_gain_idx or -1,
    })
  end
end

-- ── Entry point ───────────────────────────────────────────────────────────────

log("LocusQ Param Group Host Gate starting")
log("STATUS_JSON=" .. STATUS_JSON)
log("PROJECT_FILE=" .. (PROJECT_FILE ~= "" and PROJECT_FILE or "(none — will create new)"))

if PROJECT_FILE ~= "" then
  reaper.Main_openProject(PROJECT_FILE)
  log("Opened project: " .. PROJECT_FILE)
else
  reaper.Main_OnCommand(40023, 0)  -- new project
  log("New project created")

  if reaper.CountTracks(0) == 0 then
    log("No tracks — auto-inserting LocusQ track")
    reaper.InsertTrackAtIndex(0, true)
    local track = reaper.GetTrack(0, 0)
    reaper.GetSetMediaTrackInfo_String(track, "P_NAME", "LQ Param Group Gate", true)
    if PREFERRED_FX ~= "" then
      reaper.TrackFX_AddByName(track, PREFERRED_FX, false, 1)
    else
      reaper.TrackFX_AddByName(track, "VST3: LocusQ", false, 1)
      if reaper.TrackFX_GetCount(track) == 0 then
        reaper.TrackFX_AddByName(track, "AU: LocusQ", false, 1)
      end
      if reaper.TrackFX_GetCount(track) == 0 then
        reaper.TrackFX_AddByName(track, "LocusQ", false, 1)
      end
    end
    log("Track inserted, FX count=" .. reaper.TrackFX_GetCount(reaper.GetTrack(0, 0)))
  end
end

reaper.defer(tick)
