-- Geofence

-- Limitations: 
-- - doesn't handle correctly polygons that cross 180 degree meridian

-- Location: latitude/longitude coordinates
Location = {}
Location.__index = Location

local EARTH_RADIUS = 6371009

function Location:new(lat, lon)
   return setmetatable({lat = lat, lon = lon}, Location)
end

-- TODO doesn't calculate correct result
function Location:ellipsoidal_distance_to(location)
   -- http://en.wikipedia.org/wiki/Geographical_distance
   local delta_lat = self.lat - location.lat
   local delta_lon = self.lon - location.lon
   local mean_lat = math.rad(self.lat) + math.rad(location.lat) / 2

   local k1 = 111.13209 - 0.56605 * math.cos(2*mean_lat) + 0.00120 * math.cos(4 * mean_lat)
   local k2 = 111.41513 * math.cos(mean_lat) - 0.09455 * math.cos(3 * mean_lat) + 0.00012 * math.cos(5 * mean_lat)

   local k1_term = k1 * delta_lat
   local k2_term = k2 * delta_lon
   local dist = math.sqrt(k1 * k1 + k2 * k2) * 1000
   return dist
end

-- Returns distance in meters
-- TODO doesn't calculate correct result
function Location:flat_surface_distance_to(location)
   -- http://en.wikipedia.org/wiki/Geographical_distance
   local delta_lat = math.rad(self.lat) - math.rad(location.lat)
   local delta_lon = math.rad(self.lon) - math.rad(location.lon)
   local mean_lat = math.rad(self.lat) + math.rad(location.lat) / 2

   local term = math.cos(mean_lat) * delta_lon
   local dist = EARTH_RADIUS * math.sqrt( delta_lat*delta_lat + term * term)
   return dist
end

Circle = {}
Circle.__index = Circle

function Circle:new(location, radius)
   return setmetatable({location = location, radius = radius}, Circle)
end

function Circle:is_inside(location)
   self.location:flat_surface_distance_to(location)
   if( self.location:ellipsoidal_distance_to(location) <= self.radius) then
      return true
   end
   return false
end

-- Polygon is list of locations that describe start and 
-- end points of edges the first and last location are
-- considered to be linked
Polygon = {}
Polygon.__index = Polygon

-- Creates a new polygon
function Polygon:new(locations) 
   return setmetatable({locations = locations or {}}, Polygon)
end

function Polygon:add_location(location)
   table.insert(self.locations, location)
end

-- Checks if location is inside the polygon
function Polygon:is_inside(location)
   local point_inside = false
   local lat = location.lat
   local lon = location.lon
   local prev_loc = self.locations[#self.locations]
   for i, loc in ipairs(self.locations) do
      -- if longitude of point is inside of 
      -- longitudes of end points of current edge 
      if(loc.lon < lon and prev_loc.lon >= lon 
         or prev_loc.lon < lon and loc.lon >= lon) then
         if(loc.lat + (lon - loc.lon) /
            (prev_loc.lon - loc.lon) * (prev_loc.lat - loc.lat) < lat) then
            point_inside = not point_inside
         end

      end
      prev_loc = loc
   end
   return point_inside
end

-- GeoFence is a group of areas (e.g. Circles or Polygons)
GeoFence = {}
GeoFence.__index = GeoFence

function GeoFence:new(areas)
   return setmetatable({areas = areas or {}}, GeoFence)  
end

function GeoFence:add_area(area)
   table.insert(self.areas, area)
end

-- Checks if location is inside any of the areas.
function GeoFence:is_inside(location)
   for i, shape in ipairs(self.areas) do
      if(shape:is_inside(location)) then
         return true
      end
   end
   return false
end

--[[--
-- testing
a = Location:new(10, 10)
b = Location:new(20, 10)
c = Location:new(20, 20)
d = Location:new(10, 20)
e = Location:new(10, 15)
outside = Location:new(10,10)
inside = Location:new(10.1,14)
far = Location:new(30.1, 30.1)

locs = {a, b, c}
p = Polygon:new(locs)
p:add_location(e)

print("outside", p:is_inside(outside))
print("inside", p:is_inside(inside))

circle = Circle:new(Location:new(30, 30), 15)

fence = GeoFence:new({p})
fence:add_area(circle)
print("fence outside", fence:is_inside(outside))
print("fence inside", fence:is_inside(inside))
print("fence far", fence:is_inside(far))
--]]--