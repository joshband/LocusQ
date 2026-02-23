local statusJsonPath = os.getenv("LQ_REAPER_STATUS_JSON")

local function json_escape(value)
  if not value then return "" end
  value = value:gsub("\\", "\\\\")
  value = value:gsub('"', '\\"')
  value = value:gsub("\n", "\\n")
  value = value:gsub("\r", "\\r")
  return value
end

local function write_status(ok, matchName, candidate, fxIndex)
  if not statusJsonPath or statusJsonPath == "" then return end
  local f = io.open(statusJsonPath, "w")
  if not f then return end
  f:write("{\n")
  f:write('  "status": "' .. (ok and 'pass' or 'fail') .. '",\n')
  f:write('  "clapFxFound": ' .. tostring(ok) .. ',\n')
  f:write('  "matchedFxName": "' .. json_escape(matchName or "") .. '",\n')
  f:write('  "matchedCandidate": "' .. json_escape(candidate or "") .. '",\n')
  f:write('  "fxIndex": ' .. tostring(fxIndex or -1) .. '\n')
  f:write("}\n")
  f:close()
end

local trackCount = reaper.CountTracks(0)
reaper.InsertTrackAtIndex(trackCount, true)
local track = reaper.GetTrack(0, trackCount)

local candidates = {
  "CLAP: LocusQ",
  "CLAPi: LocusQ",
  "CLAP: LocusQ (Noizefield)",
  "CLAPi: LocusQ (Noizefield)",
}

local found = false
local foundName = nil
local foundCandidate = nil
local foundIdx = -1

for _, cand in ipairs(candidates) do
  local idx = reaper.TrackFX_AddByName(track, cand, false, 1)
  if idx >= 0 then
    local ok, name = reaper.TrackFX_GetFXName(track, idx, "")
    local fxName = ok and name or ""
    if fxName:lower():find("clap") then
      found = true
      foundName = fxName
      foundCandidate = cand
      foundIdx = idx
      break
    end
  end
end

write_status(found, foundName, foundCandidate, foundIdx)
