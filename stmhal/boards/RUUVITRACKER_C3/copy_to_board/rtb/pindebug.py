import pyb
PINS = [
    'GSM_RI',
    'GSM_DCD',
    'GSM_RI',
    'GSM_RTS',
    'GSM_CTS',
    'GSM_DTR',
    'GSM_PWRKEY',
    'GPS_WAKEUP',
    'GPS_1PPS_SIGNAL',
    'SD_CARD_INSERTED',
    'CHARGER_STATUS',
    'GSM_STATUS',
]

for pinname in PINS:
    pyb.Pin(pinname).debug(True)
