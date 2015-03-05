class powerdomains_base:
    pin = None
    reservations = 0

    def __init__(self, pin):
        self.pin = pin

    def request(self):
        if self.reservations >= 254:
            raise RuntimeError("Trying to request domain for the 255th time, something must be wrong")
        if self.reservations == 0:
            self.pin.high()
        self.reservations += 1
        return True

    def release(self):
        if self.reservations <= 0:
            raise RuntimeError("Trying to release a domain that is already fully released")
        self.reservations -= 1
        if self.reservations == 0:
            self.pin.low()
        return True

    def status(self):
        return bool(self.pin.value())

class powermanager:
    domains = []

    def __init__(self, domains_list):
        self.domains = domains_list
        pass

    def request(self, domain):
        """Proxy to the domains request method, for completenes' sake"""
        return domain.request()

    def release(self, domain):
        """Proxy to the domains release method, for completenes' sake"""
        return domain.request()

    def all_released(self):
        """Checks all known domains, returns True if all are released, False otherwise"""
        for d in self.domains:
            if not d.status:
                return False
        return True

from .powerdomains_config import domains_list
powermanager_singleton = powermanager(domains_list)