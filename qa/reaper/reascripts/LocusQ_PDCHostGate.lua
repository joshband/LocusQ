-- LocusQ PDC Host Gate — headless Reaper PDC truthfulness check.
--
-- Gate:
--   PDC_ZERO   LocusQ reports 0 samples of PDC to the host, for all
--              tap-count regimes (short and long FIR).
--
-- Method:
--   1. Create a fresh project and insert LocusQ.
--   2. Start the transport briefly so prepareToPlay() fires and the
--      plugin publishes its latency to Reaper.
--   3. Read the PDC value via TrackFX_GetNamedConfigParm(track, fx, "pdc").
--   4. Assert value == 0.
--   5. Write status.json and quit.
--
-- Env vars:
--   LQ_REAPER_NONINTERACTIVE=1     suppress message boxes
--   LQ_REAPER_STATUS_JSON=<path>   where to write status.json
--   LQ_REAPER_REQUIRE_LOCUSQ=1     fail if LocusQ FX not found (default: 1)
--
-- Usage:
--   Launched automatically by scripts/reaper-pdc-host-gate-mac.sh
--   or via REAPER: Actions > Load ReaScript

local NONINTERACTIVE  = os.getenv("LQ_REAPER_NONINTERACTIVE") == "1"
local STATUS_JSON     = os.getenv("LQ_REAPER_STATUS_JSON") or ""
local REQUIRE_LOCUSQ  = os.getenv("LQ_REAPER_REQUIRE_LOCUSQ") ~= "0"
local PROJECT_PATH    = os.getenv("LQ_REAPER_PROJECT") or ""  -- optional: open existing .rpp

-- Defers to wait after transport start before reading PDC.
-- prepareToPlay() fires synchronously when Reaper starts audio,
-- but allow a few defers for the plugin to fully initialize.
local INIT_DEFERS = 30  -- ~1 s at Reaper's ~30 fps defer rate

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
  reaper.ShowConsoleMsg("[LQ-pdc-gate] " .. msg .. "\n")
end

-- ── State ────────────────────────────────────────────────────────────────────

local state        = "SETUP"
local defer_count  = 0
local locusq_track = nil
local locusq_fxidx = -1

local function fatal(msg, code)
  log("FATAL: " .. msg)
  reaper.Main_OnCommand(1016, 0)  -- stop transport
  write_status({
    status        = "fail",
    error         = msg,
    error_code    = code or "fatal",
    gate_pdc_zero = false,
    pdc_samples   = -1,
  })
  if not NONINTERACTIVE then
    reaper.ShowMessageBox("LocusQ PDC Host Gate FAILED:\n" .. msg, "LQ PDC Gate", 0)
  end
  reaper.defer(function() reaper.Main_OnCommand(40004, 0) end)  -- quit
end

local function finish(pdc_samples, gate_pass)
  reaper.Main_OnCommand(1016, 0)  -- stop transport
  local summary = table.concat(log_lines, "\n")
  write_status({
    status        = gate_pass and "pass" or "fail",
    gate_pdc_zero = gate_pass,
    pdc_samples   = pdc_samples,
    summary       = summary,
  })
  local result = (gate_pass and "PASS" or "FAIL") ..
    " | pdc_samples=" .. tostring(pdc_samples)
  log(result)
  if not NONINTERACTIVE then
    reaper.ShowMessageBox(result, "LocusQ PDC Host Gate", 0)
  end
  reaper.defer(function() reaper.Main_OnCommand(40004, 0) end)  -- quit
end

-- ── Tick ─────────────────────────────────────────────────────────────────────

local function tick()
  if state == "SETUP" then
    -- Find LocusQ FX across all tracks.
    local track_count = reaper.CountTracks(0)
    for t = 0, track_count - 1 do
      local track = reaper.GetTrack(0, t)
      for fx = 0, reaper.TrackFX_GetCount(track) - 1 do
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
        fatal("LocusQ FX not found in project", "locusq_not_found")
      else
        log("SKIP: LocusQ not found and not required")
        write_status({ status = "skipped", reason = "locusq_not_found" })
        reaper.defer(function() reaper.Main_OnCommand(40004, 0) end)
      end
      return
    end

    -- Start transport so prepareToPlay() fires and PDC is published.
    reaper.SetEditCurPos(0.0, false, false)
    reaper.Main_OnCommand(1007, 0)  -- Transport: Play
    log(string.format("Transport started — waiting %d defers for audio init", INIT_DEFERS))

    defer_count = 0
    state = "WAIT_INIT"
    reaper.defer(tick)

  elseif state == "WAIT_INIT" then
    defer_count = defer_count + 1
    if defer_count < INIT_DEFERS then
      reaper.defer(tick)
      return
    end

    -- Read PDC via named config param.
    -- "pdc" returns the plugin's reported PDC latency in samples as a string.
    local retval, pdc_str = reaper.TrackFX_GetNamedConfigParm(
      locusq_track, locusq_fxidx, "pdc")

    log(string.format("TrackFX_GetNamedConfigParm('pdc'): retval=%s value='%s'",
                      tostring(retval), tostring(pdc_str)))

    if not retval or pdc_str == nil or pdc_str == "" then
      -- API returned nothing — try "PDC" (capitalized) as fallback.
      retval, pdc_str = reaper.TrackFX_GetNamedConfigParm(
        locusq_track, locusq_fxidx, "PDC")
      log(string.format("Fallback TrackFX_GetNamedConfigParm('PDC'): retval=%s value='%s'",
                        tostring(retval), tostring(pdc_str)))
    end

    if not retval or pdc_str == nil or pdc_str == "" then
      fatal("TrackFX_GetNamedConfigParm('pdc') returned no value — "
            .. "Reaper version may not support PDC API query",
            "pdc_api_unsupported")
      return
    end

    local pdc_samples = tonumber(pdc_str)
    if pdc_samples == nil then
      fatal("PDC value is not numeric: '" .. tostring(pdc_str) .. "'",
            "pdc_not_numeric")
      return
    end

    -- Round to nearest integer (Reaper may return a float like 0.0).
    pdc_samples = math.floor(pdc_samples + 0.5)
    local gate_pass = (pdc_samples == 0)

    if gate_pass then
      log("Gate PDC_ZERO PASS: pdc_samples=" .. pdc_samples)
    else
      log("Gate PDC_ZERO FAIL: pdc_samples=" .. pdc_samples
          .. " (expected 0 — host would over-compensate by " .. pdc_samples .. " samples)")
    end

    state = "DONE"
    finish(pdc_samples, gate_pass)
  end
end

-- ── Entry point ───────────────────────────────────────────────────────────────

log("LocusQ PDC Host Gate starting")
log("STATUS_JSON=" .. STATUS_JSON)
log("PROJECT_PATH=" .. (PROJECT_PATH ~= "" and PROJECT_PATH or "(none — will create new)"))

if PROJECT_PATH ~= "" then
  -- Open the caller-supplied project instead of creating a blank one.
  reaper.Main_openProject(PROJECT_PATH)
  log("Opened project: " .. PROJECT_PATH)
else
  -- Fresh blank project.
  reaper.Main_OnCommand(40023, 0)
  log("New project created")

  -- Auto-insert LocusQ if no tracks present.
  if reaper.CountTracks(0) == 0 then
    log("No tracks — auto-inserting LocusQ track")
    reaper.InsertTrackAtIndex(0, true)
    local track = reaper.GetTrack(0, 0)
    reaper.GetSetMediaTrackInfo_String(track, "P_NAME", "LQ PDC Gate", true)
    reaper.TrackFX_AddByName(track, "VST3: LocusQ", false, 1)
    if reaper.TrackFX_GetCount(track) == 0 then
      reaper.TrackFX_AddByName(track, "AU: LocusQ", false, 1)
    end
    if reaper.TrackFX_GetCount(track) == 0 then
      reaper.TrackFX_AddByName(track, "LocusQ", false, 1)
    end
    log("Track inserted, FX count=" .. reaper.TrackFX_GetCount(reaper.GetTrack(0, 0)))
  end
end

reaper.defer(tick)
