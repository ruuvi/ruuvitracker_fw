-- This file holds configuration values for RuuciTracker
-- 

-- User modifiable configuration values
config = {

   -- Settings for GSM modem
   gsm = {
      pin_code = '0000',
      apn = 'internet.saunalahti'
   },

   -- Settings for Tracking
   tracker = {
      tracking_intervall = 5,  -- Interval in secods, 0=real time
      url = "http://dev-server.ruuvitracker.fi/api/v1-dev/",
      tracker_code = "sepeto",
      shared_secret = "sepeto"
   }
}



-- Static values. Do not change
firmware = {}
firmware.version = "0.2"
firmware.timers = {
   tracker_timer = 1
}
