# Simplified NMEA parser, implements only what we need
import ure as re

# Errors
class FormatError(RuntimeError):
    pass

class ChecksumError(RuntimeError):
    pass


# Constants
FIX_TYPE_NONE = 1
FIX_TYPE_2D = 2
FIX_TYPE_3D = 3

MSG_GPRMC = (1<<0)
MSG_GPGSA = (1<<1)
MSG_GPGGA = (1<<2)


# Classes
class Datetime:
    hh = None
    mm = None
    sec = None
    day = None
    month = None
    year = None

    def __repr__(self):
        return {
            'year': self.year,
            'month': self.month,
            'day': self.day,
            'hour': self.hh,
            'minute': self.mm,
            'second': self.sec,
        }

    def update_rtc(self):
        import pyb, math
        subsec, sec = math.modf(self.sec)
        rtcsubsec = int((1000 - int(subsec*1000)) / 3.9176) # RTC subseconds count down from 255
        pyb.RTC().datetime((self.year, self.month, self.day, None, self.hh, self.mm, int(sec), rtcsubsec))


class Fix:
    fix_type = FIX_TYPE_NONE
    n_satellites = 0
    lat = None
    lon = None
    speed = None
    heading = None
    altitude = None
    pdop = None
    hdop = None
    vdop = None
    dt = None
    received_messages = 0
    last_update = None

    # TODO: We might want to provide __repr__ for this too


# Functions
def checksum(s):
    """This can be used to calculate and/or verify the checksum on an NMEA sentence"""
    ss = s.find('$')
    if ss < 0:
        raise FormatError("Start of sentence ($) not found")
    se = s.find('*')
    if se < 0:
        # Just calculate the checksum
        verify = False
        sc = s[ss+1:]
    else:
        # Verify the checksum
        verify = s[se:]
        sc = s[ss+1:se]

    checksum = 0
    for c in sc:
        checksum ^= ord(c)

    checkstr = '*%02X' % checksum

    # Verify mode, check the result and return accordingly
    if verify:
        # Make sure the checksum to be verified is in upper case and that it's a string, not bytearray
        if verify.upper().format() == checkstr:
            return True
        return False

    # checksum mode, append the checksum and return
    return s + checkstr


def parse_time(dtstr, dt=None):
    """Parses time (ms accurate) from NMEA time format into Datetime object"""
    if not dt:
        dt = Datetime()

    dt.hh = int(dtstr[0:2])
    dt.mm = int(dtstr[2:4])
    dt.sec = float(dtstr[4:].format()) # Using .format to cast into str so we can parse a float

    return dt

def parse_date(dtstr, dt=None):
    """Parses date from NMEA date format into Datetime object"""
    if not dt:
        dt = Datetime()

    dt.day = int(dtstr[0:2])
    dt.month = int(dtstr[2:4])
    dt.year = 2000 + int(dtstr[4:])

    return dt

# Adapted from https://github.com/Knio/pynmea2/blob/master/pynmea2/nmea_utils.py
dm_to_sd_re = re.compile(r'^(\d+)(\d\d\.\d+)$')
def dm_to_sd(dm):
    """Converts the NMEA dddmm.mmmm format to the more commonly used decimal format"""
    if (   not dm
        or dm == '0'):
        return 0.0
    match = dm_to_sd_re.match(dm)
    d = match.group(1)
    m = match.group(2)
    return float(d) + float(m) / 60


def parse_gprmc(line, fix=None):
    """Parses G[PLN]RMC sentence, returns a Fix object. NOTE: The line must be a string, not bytearray. Cast before passing"""
    if not fix:
        fix = Fix()

    if not checksum(line):
        raise ChecksumError()

    # We skip the GP/GL/GN part since we want to avoid regexes
    start = line.find('RMC')
    if start < 0:
        raise FormatError("RMC not found")

    parts = line[start+4:-3].split(',')
    if parts[1] != 'A':
        fix.fix_type = FIX_TYPE_NONE
        return fix

    # Parse time
    fix.dt = parse_time(parts[0])
    parse_date(parts[8], fix.dt)

    # Parse lat/lon
    fix.lat = dm_to_sd(parts[2])
    if parts[3] == 'S':
        fix.lat = -fix.lat
    fix.lon = dm_to_sd(parts[4])
    if parts[5] == 'W':
        fix.lon = -fix.lon

    # Parse speed & heading
    fix.speed = float(parts[6]) # Using .format to cast into str so we can parse a float
    fix.heading = float(parts[7]) # Using .format to cast into str so we can parse a float

    # We ignore "magnetic variation" and it's direction

    fix.received_messages |= MSG_GPRMC
    return fix

def parse_gpgga(line, fix=None):
    """Parses G[PLN]RMC sentence, returns a Fix object. NOTE: The line must be a string, not bytearray. Cast before passing"""
    if not fix:
        fix = Fix()

    if not checksum(line):
        raise ChecksumError()

    # We skip the GP/GL/GN part since we want to avoid regexes
    start = line.find('GGA')
    if start < 0:
        raise FormatError("GGA not found")

    parts = line[start+4:-3].split(',')

    # If fix does not have RMC fields already take what we can get
    if not (fix.received_messages & MSG_GPRMC):
        # We have only time, no date :(
        fix.dt = parse_time(parts[0])

        # Parse lat/lon
        fix.lat = dm_to_sd(parts[1])
        if parts[2] == 'S':
            fix.lat = -fix.lat
        fix.lon = dm_to_sd(parts[3])
        if parts[4] == 'W':
            fix.lon = -fix.lon

    fix.n_satellites = int(parts[6], 10)
    # TODO: is it possible for this unit to be something else ? in that case add conversions to meters
    if parts[9] == 'M':
        fix.altitude = float(parts[8])

    fix.received_messages |= MSG_GPGGA
    return fix
