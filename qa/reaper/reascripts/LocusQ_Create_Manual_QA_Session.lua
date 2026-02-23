-- LocusQ manual QA bootstrap for REAPER.
-- Creates:
--   1) Synth source track with MIDI clip
--   2) LocusQ renderer track with synth send routing

local project = 0
local nonInteractive = os.getenv("LQ_REAPER_NONINTERACTIVE") == "1"
local statusJsonPath = os.getenv("LQ_REAPER_STATUS_JSON")
local requireLocusQ = os.getenv("LQ_REAPER_REQUIRE_LOCUSQ") == "1"

local function json_escape(value)
  if not value then
    return ""
  end
  value = value:gsub("\\", "\\\\")
  value = value:gsub('"', '\\"')
  value = value:gsub("\n", "\\n")
  value = value:gsub("\r", "\\r")
  return value
end

local function write_status_json(synthFx, locusqFx, sendOk, midiOk)
  if not statusJsonPath or statusJsonPath == "" then
    return
  end

  local file = io.open(statusJsonPath, "w")
  if not file then
    return
  end

  local synthValue = synthFx and ('"' .. json_escape(synthFx) .. '"') or "null"
  local locusqValue = locusqFx and ('"' .. json_escape(locusqFx) .. '"') or "null"
  file:write("{\n")
  file:write('  "status": "ok",\n')
  file:write('  "synthFxFound": ' .. tostring(synthFx ~= nil) .. ",\n")
  file:write('  "locusqFxFound": ' .. tostring(locusqFx ~= nil) .. ",\n")
  file:write('  "sendCreated": ' .. tostring(sendOk) .. ",\n")
  file:write('  "midiClipCreated": ' .. tostring(midiOk) .. ",\n")
  file:write('  "synthFxName": ' .. synthValue .. ",\n")
  file:write('  "locusqFxName": ' .. locusqValue .. "\n")
  file:write("}\n")
  file:close()
end

local function set_track_name(track, name)
  if track then
    reaper.GetSetMediaTrackInfo_String(track, "P_NAME", name, true)
  end
end

local function add_fx_from_candidates(track, candidates)
  for _, fxName in ipairs(candidates) do
    local fxIndex = reaper.TrackFX_AddByName(track, fxName, false, 1)
    if fxIndex >= 0 then
      return fxName, fxIndex
    end
  end
  return nil, -1
end

local function add_basic_midi_clip(track)
  local item = reaper.CreateNewMIDIItemInProj(track, 0.0, 8.0, false)
  if not item then return false end

  local take = reaper.GetActiveTake(item)
  if not take or not reaper.TakeIsMIDI(take) then return false end

  for beat = 0, 7 do
    local startTime = beat
    local endTime = beat + 0.5
    local startPPQ = reaper.MIDI_GetPPQPosFromProjTime(take, startTime)
    local endPPQ = reaper.MIDI_GetPPQPosFromProjTime(take, endTime)
    reaper.MIDI_InsertNote(take, false, false, startPPQ, endPPQ, 0, 60, 96, true)
  end

  reaper.MIDI_Sort(take)
  return true
end

local function quit_reaper_noninteractive()
  -- Save generated project changes so quit path is prompt-free in automation runs.
  reaper.Main_SaveProject(project, false)
  -- File: Quit REAPER
  reaper.Main_OnCommand(40004, 0)
end

reaper.Undo_BeginBlock()

local initialTrackCount = reaper.CountTracks(project)
reaper.InsertTrackAtIndex(initialTrackCount, true)
reaper.InsertTrackAtIndex(initialTrackCount + 1, true)

local synthTrack = reaper.GetTrack(project, initialTrackCount)
local locusqTrack = reaper.GetTrack(project, initialTrackCount + 1)

set_track_name(synthTrack, "LQ QA Synth Source")
set_track_name(locusqTrack, "LQ QA Spatial Renderer")

local synthFxName = nil
synthFxName = ({ add_fx_from_candidates(synthTrack, {
  "VSTi: ReaSynth (Cockos)",
  "ReaSynth (Cockos)",
  "AU: ReaSynth (Cockos)",
}) })[1]

local locusqFxName = nil
locusqFxName = ({ add_fx_from_candidates(locusqTrack, {
  "VST3: LocusQ",
  "AU: LocusQ",
  "LocusQ",
}) })[1]

local sendIndex = reaper.CreateTrackSend(synthTrack, locusqTrack)
if sendIndex >= 0 then
  reaper.SetTrackSendInfo_Value(synthTrack, 0, sendIndex, "I_SRCCHAN", 0)
  reaper.SetTrackSendInfo_Value(synthTrack, 0, sendIndex, "I_DSTCHAN", 0)
end

local midiClipCreated = add_basic_midi_clip(synthTrack)

reaper.SetMediaTrackInfo_Value(locusqTrack, "I_RECMON", 1)
reaper.SetEditCurPos(0.0, true, false)
reaper.TrackList_AdjustWindows(false)
reaper.UpdateArrange()

reaper.Undo_EndBlock("LocusQ manual QA session bootstrap", -1)

local sendCreated = sendIndex >= 0
write_status_json(synthFxName, locusqFxName, sendCreated, midiClipCreated)

if requireLocusQ and (locusqFxName == nil) then
  local msg = "ERROR: LocusQ FX was not found in this REAPER environment.\n"
  if nonInteractive then
    reaper.ShowConsoleMsg(msg)
  else
    reaper.ShowMessageBox(msg, "LocusQ QA Bootstrap", 0)
  end
end

local summary = "LocusQ manual QA session created.\n"
summary = summary .. "Synth FX: " .. (synthFxName or "NOT FOUND") .. "\n"
summary = summary .. "LocusQ FX: " .. (locusqFxName or "NOT FOUND") .. "\n"
summary = summary .. "Run the checklist in Documentation/testing/reaper-manual-qa-session.md"
if nonInteractive then
  reaper.ShowConsoleMsg(summary .. "\n")
  reaper.defer(quit_reaper_noninteractive)
else
  reaper.ShowMessageBox(summary, "LocusQ QA Bootstrap", 0)
end
