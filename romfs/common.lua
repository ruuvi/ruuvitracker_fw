-- Common functionality and constants 

firmware = {}
firmware.version = "0.1"

options = {
   pin_code = '0000',
   tracking_intervall = 0  -- Interval in secods, 0=real time
}

server = {
   url = "http://dev-server.ruuvitracker.fi/api/v1-dev/",
   tracker_code = "sepeto",
   shared_secret = "sepeto"
}

