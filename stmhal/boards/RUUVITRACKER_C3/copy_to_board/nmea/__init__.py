# Simplified NMEA parser, implements only what we need

class FormatError(Exception):
    pass

def checksum(s):
    """This can be used to calculate and/or verify the checksum on an NMEA sentence"""
    ss = s.find(b'$')
    if ss < 0:
        raise FormatError("Start of sentence ($) not found")
    se = s.find(b'*')
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
        checksum ^= c

    checkstr = b'*%02X' % checksum

    # Verify mode, check the result and return accordingly
    if verify:
        # Make sure the checksum to be verified is in upper case and that it's a string, not bytearray
        if verify.upper().format() == checkstr:
            return True
        return False

    # checksum mode, append the checksum and return
    return s + checkstr

