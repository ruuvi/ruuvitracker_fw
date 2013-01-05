require('http')

local JSON = require('json')

module('tracker', package.seeall)

--[[ event data structure
event = {}
event.version = nil
event.tracker_code = nil
event.time = nil
event.session_code = nil
event.nonce = nil
event.latitude = nil
event.longitude = nil
event.accuracy = nil
event.vertical_accuracy = nil
event.heading = nil
event.satellite_count = nil
event.battery = nil
event.speed = nil
event.altitude = nil
event.temperature = nil
event.annotation = nil
event.mac = nil
event.password = nil
   --]]

local function base_string(event)
   local sorted_keys = {}
   for key in pairs(event) do 
      table.insert(sorted_keys, key) 
   end
   table.sort(sorted_keys)
   local s = ""
   for _, key in ipairs(sorted_keys) do 
      local value = event[key]
      s = s .. key .. ':' .. value .. '|'
   end
   return s
end

local function generate_mac(event, shared_secret) 
   local base = base_string(event)
   local mac = sha1.hmac(shared_secret, base)
   return mac
end


local function create_event_json(event, tracker_code, shared_secret)
   local event = event  -- Do not modify original
   event.version = "1"
   -- TODO if possible, set event.time field here or before
   event.tracker_code = tracker_code
   if shared_secret then
      local mac = generate_mac(event, shared_secret)
      event.mac = mac
   end
   return JSON:encode(event)
end


function send_event(event)
   local message = create_event_json(event, server.tracker_code, server.shared_secret)
   return http.post(server.url .. 'events', message, 'application/json')
end


--[[--
function ping_server()
   local code, data = http.get(server.url .. 'ping')
   if code ~= "200" then
      return nil
   end
   local success, response = pcall(function() return json.decode(data) end)
   if success then
      return response
   end
   return nil
end
   --]]

-- Unit tests, uncomment to enable
function create_test_event() 
   local event = {
      heading="90",
      time="2013-01-05T15:45:02.000Z",
      speed=7.2227376,
      latitude="6504.019739,N",
      session_code="test",
      longitude="02525.097090,E"
   }
   return event
end
