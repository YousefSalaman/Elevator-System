
from .cli import nodes


class Device:
    """Data class to store information about individual devices on the MCUs."""

    def __init__(self):

        self.thread_id = None  # Thread id where device can be found
        self.device_id = None  # Id it has in the mcu

    @staticmethod
    def register_test(device_name, test):
        """Register a test for a specific device"""

        nodes.Node(test.__name__, test.__doc__, device_name, test)


class DeviceTracker:
    """A representation of a device in a set of MCUs connected to the main computer

    An MCU registers a device, any sensor, actuator, etc. attached to the
    MCUs, by passing its name through the scheduling system. After it does
    this, it registers an instance of that device, so to keep track of """

    _trackers = {}  # Registered devices in the system (to fetch these you need the devices id)
    _tracker_count = 0  # Count of the trackerices stored in the system (these are used as ids)
    _tracker_name_to_tracker_id = {}  # Dictionary that maps a registered device name to its device id

    def __init__(self):

        self.devices = {}
        self.device_count = 0

        # Set up dictionaries to retrieve computer dev instance ids
        self.device_attrs = set()  # Attributes this type of device instance must have
        self._mcu_info_to_cu_id = {}  # Map a unique mcu key to a computer dev instance id
        self._device_name_to_cu_id = {}  # Map a registered instance name to a computer dev instance id

        # Set up device instance attributes
        self._attr_id_to_attr_name = {}

    def add_device(self, device_id, device_name, thread_id):
        """Add a device to the device tracker"""

        # Create device instance and set up base attributes
        device = self.devices[self.device_count] = Device()
        device.thread_id = thread_id
        device.device_id = device_id
        for attr in self.device_attrs:
            setattr(device, attr, None)

        # Register device instance in device
        self._device_name_to_cu_id[device_name] = self.device_count
        self._mcu_info_to_cu_id[(thread_id, device_id)] = self.device_count
        self.device_count += 1

    def add_device_attr(self, attr_id, attr_name):
        """Add an attribute to the devices"""

        self.device_attrs.add(attr_name)
        self._attr_id_to_attr_name[attr_id] = attr_name

    @classmethod
    def add_tracker(cls, tracker_name):
        """Create and add a device tracker to the system"""

        tracker = cls()
        cls._trackers[cls._tracker_count] = tracker
        cls._tracker_name_to_tracker_id[tracker_name] = cls._tracker_count
        cls._tracker_count += 1

    @classmethod
    def get_tracker(cls, tracker_id=None, tracker_name=None):
        """Get the correct device with either its registered name or its trackerice number"""

        if tracker_id is None:
            tracker_id = cls._tracker_name_to_tracker_id.get(tracker_name)
        return cls._trackers.get(tracker_id)

    def get_device(self, cu_id=None, device_name=None):
        """Get a device instance by using its registered name or an mcu key"""

        # Find the computer unique id if it wasn't given
        if cu_id is None:
            cu_id = self._device_name_to_cu_id.get(device_name)

        # Pass device instance if available
        if cu_id is not None:
            return self.devices[cu_id]
        return None

    @classmethod
    def was_tracker_created(cls, tracker_name):
        """Verify if tracker was created and stored in the system"""

        return cls._tracker_name_to_tracker_id.get(tracker_name) is not None

    def update_device_attr(self, attr_id, device_id, thread_id, value):
        """Update the attribute of a device with a new value"""

        device_id = self._mcu_info_to_cu_id[(thread_id, device_id)]
        device = self.devices[device_id]
        attr_name = self._attr_id_to_attr_name[attr_id]
        setattr(device, attr_name, value)
