from __future__ import print_function

import struct

from . import cli
from . import scheduler


class Device:
    """Data class to store information about individual devices on the MCUs."""

    def __init__(self):

        self.name = None  # Name of the device
        self.thread_id = None  # Thread id where device can be found
        self.device_id = None  # Id it has in the mcu

    @staticmethod
    def register_test(device_name, test):
        """Register a test for a specific device"""

        cli.Node(test.__name__, test.__doc__, device_name, test)


class DeviceTracker:
    """A representation of a device in a set of MCUs connected to the main computer

    An MCU registers a device, any sensor, actuator, etc. attached to the
    MCUs, by passing its name through the scheduling system. After it does
    this, it registers an instance of that device, so to keep track of """

    _trackers = {}  # Registered devices in the system (to fetch these you need the devices id)
    _tracker_count = 0  # Count of the trackerices stored in the system (these are used as ids)
    _tracker_name_to_tracker_id = {}  # Dictionary that maps a registered device name to its device id
    _VALID_ATTR_TYPES = {  # Valid attribute types
        "bool": '?',
        "char": 'c',
        "int8_t": 'b',
        "uint8_t": 'B',
        "int16_t": 'h',
        "uint16_t": 'H',
        "int32_t": 'i',
        "uint32_t": 'I',
        "int64_t": 'q',
        "uint64_t": 'Q',
        "float16_t": 'e',
        "float32_t": 'f',
        "float64_t": 'd',
        "size_t": 'N',
        "ssize_t": 'n'
    }

    def __init__(self, name):

        self.name = name
        self.devices = {}
        self.device_count = 0

        # Set up dictionaries to retrieve computer dev instance ids
        self.device_attrs = set()  # Attributes this type of device instance must have
        self._mcu_info_to_cu_id = {}  # Map a unique mcu key to a computer dev instance id
        self._device_name_to_cu_id = {}  # Map a registered instance name to a computer dev instance id

        # Set up device instance attribute ids
        self._attr_id_to_attr_name = {}
        self._attr_name_to_attr_id = {}

    def add_devices(self, device_count, thread_id):
        """Add a device to the device tracker"""

        for device_id in range(device_count):

            # Create device instance and set up base attributes
            self.device_count += 1
            device = self.devices[self.device_count] = Device()
            device_name = self.name + "_" + str(self.device_count)

            # Setup device attributes
            device.name = device_name
            device.device_id = device_id
            device.thread_id = thread_id
            for attr in self.device_attrs:
                setattr(device, attr, None)

            # Register device instance in device
            self._device_name_to_cu_id[device_name] = self.device_count
            self._mcu_info_to_cu_id[(thread_id, device_id)] = self.device_count

    def add_device_attr(self, attr_id, attr_name):
        """Add an attribute to the devices"""

        self.device_attrs.add(attr_name)
        self._attr_id_to_attr_name[attr_id] = attr_name
        self._attr_name_to_attr_id[attr_name] = attr_id

    @classmethod
    def add_tracker(cls, tracker_name):
        """Create and add a device tracker to the system"""

        tracker = cls(tracker_name)
        cls._trackers[cls._tracker_count] = tracker
        cls._tracker_name_to_tracker_id[tracker_name] = cls._tracker_count
        cls._tracker_count += 1

    @classmethod
    def get_attr_type_format(cls, attr_type):
        """Retrieve format character of a C-native attribute type"""

        return cls._VALID_ATTR_TYPES.get(attr_type)

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
    def get_tracker(cls, tracker_id=None, tracker_name=None):
        """Get the correct device with either its registered name or its trackerice number"""

        if tracker_id is None:
            tracker_id = cls._tracker_name_to_tracker_id.get(tracker_name)
        return cls._trackers.get(tracker_id)

    @classmethod
    def was_tracker_created(cls, tracker_name):
        """Verify if tracker was created and stored in the system"""

        return cls._tracker_name_to_tracker_id.get(tracker_name) is not None

    @classmethod
    def update_device_attr_comp(cls, tracker_id, attr_id, device_id, thread_id, attr_type, value_pkt):
        """Update the attribute of a device in the computer

        This is mostly meant to update simple device attributes such
        as numeric values. For more complex data structures like
        arrays, tables, etc., you need to make and register a separate
        task that can handle this.
        """

        tracker = cls._trackers.get(tracker_id)
        if tracker is not None:
            # Fetch relevant information to update the attribute
            device_id = tracker._mcu_info_to_cu_id[(thread_id, device_id)]
            device = tracker.devices[device_id]
            attr_name = tracker._attr_id_to_attr_name[attr_id]

            # Interpret value and update attribute
            try:
                value = struct.unpack(attr_type, value_pkt)
                setattr(device, attr_name, value)
            except struct.error:
                print("Error in unpacking value for attribute '{}' for device '{}'".format(attr_name, device.name))

    @classmethod
    def update_device_attr_mcu(cls, tracker_name, device_name, attr_name, attr_type, value):
        """Update the attribute of a device on an MCU

        Similar to the 'update_device_attr_mcu', this is meant to
        update simple numeric values. For more complex data structures,
        like arrays, tables, etc., you need to make and register a
        separate task that can handle this.
        """

        # Retrieve the tracker
        tracker_id = cls._tracker_name_to_tracker_id.get(tracker_name)
        tracker = cls._trackers[tracker_id]

        if tracker is not None:
            # Fetch relevant information to update the attribute in MCU
            thread_id, device_id = tracker._device_name_to_cu_id[device_name]
            current_scheduler = scheduler.Scheduler.get_scheduler(thread_id)

            # Fetch format characters to packetize value
            attr_id = tracker._attr_name_to_attr_id.get(attr_name)
            endian_format = current_scheduler.printer.endianness_format
            attr_type_format = tracker._VALID_ATTR_TYPES.get(attr_type)

            # Build packet if valid information was gathered
            if not (attr_id is None and attr_type_format is None):
                pkt_format = endian_format + "BBB" + attr_type_format
                pkt = struct.pack(pkt_format, tracker_id, device_id, attr_id, value)
                return scheduler, pkt
