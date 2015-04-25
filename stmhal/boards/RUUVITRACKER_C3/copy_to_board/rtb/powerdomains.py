class powerdomains_base:
    pin = None
    reservations = 0
    invert = False

    def __init__(self, pin, invert=False):
        self.pin = pin
        self.invert = invert
        # For some weird reason the pin was already up, make sure we track that...
        if self.pin.value() != self.invert:
            self.reservations += 1

    def request(self):
        """Requests a power domain, this will enable the domain if not already enabled, returns boolean indicating whether pin was actually changed."""
        if self.reservations >= 254:
            raise RuntimeError("Trying to request domain for the 255th time, something must be wrong")
        self.reservations += 1
        if self.reservations == 1:
            self.pin.value((not self.invert))
            return True
        return False

    def release(self):
        """Release a power domain, this will disable the domain if there are no other requests for it, returns boolean indicating whether pin was actually changed."""
        if self.reservations <= 0:
            raise RuntimeError("Trying to release a domain that is already fully released")
        self.reservations -= 1
        if self.reservations == 0:
            self.pin.value(self.invert)
            return True
        return False

    def status(self):
        return bool(self.pin.value()) != self.invert

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
            if d.status():
                #print("pin %s is enabled" % d.pin)
                return False
        return True

from .powerdomains_config import domains_list
powermanager_singleton = powermanager(domains_list)