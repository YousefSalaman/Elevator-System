
from numbers import Number

from .cli import nodes, commands


class Device:

    _devs = {}  # Registered devices in the system (to fetch these you need the devices id)
    _dev_count = 0  # Count of the devices stored in the system (these are used as ids)
    _dev_name_to_dev_id = {}  # Dictionary that maps a registered device name to its device id

    def __init__(self):

        self.instances = {}
        self.instance_count = 0

        # Set up dictionaries to retrieve computer dev instance ids
        self._mcu_info_to_cu_id = {}  # Map a unique mcu key to a computer dev instance id
        self._instance_name_to_cu_id = {}  # Map a registered instance name to a computer dev instance id

        # Set up device instance attributes
        self._attrs_id_to_attr_name = {}
        self._attrs = {"thread_id", "mcu_dev_id"}  # Attributes this type of device instance must have

    def add_instance_attrs(self, attr_name, attr_id):
        """Callable task from an mcu. Add a device instance attribute to store and retrieve values."""

        self._attrs.add(attr_name)
        self._attrs_id_to_attr_name[attr_id] = attr_name

    @classmethod
    def get_device(cls, dev_name):
        """Get the correct device with either its registered name or its device number"""

        dev_id = cls._dev_name_to_dev_id.get(dev_name)

        # Pass device if available
        if dev_id is not None:
            return cls._devs.get(dev_id)

    def get_instance(self, instance_name):
        """Get a device instance by using its registered name or an mcu key"""

        cu_id = self._instance_name_to_cu_id.get(instance_name)

        # Pass device instance if available
        if cu_id is not None:
            return self.instances[cu_id]

    def register_instance(self, instance_name, thread_id, mcu_dev_id):
        """A callable task from an mcu. Register an instance of the device using info of the mcu with a given name"""

        # Create device instance and set up base attributes
        self.instances[self.instance_count] = dict.fromkeys(self._attrs)
        self.instances[self.instance_count].update({"thread_id": thread_id, "mcu_dev_id": mcu_dev_id})

        # Register device instance in device
        self._instance_name_to_cu_id[instance_name] = self.instance_count
        self._mcu_info_to_cu_id[(thread_id, mcu_dev_id)] = self.instance_count

        self.instance_count += 1  # Update device instance count

    @classmethod
    def register_device(cls, dev_name):
        """A callbable task from an mcu. Register a device with a given name if it wasn't registered"""

        dev = None

        if cls._dev_name_to_dev_id.get(dev_name) is None:

            dev = cls()

            # Register new device to the class
            cls._devs[cls._dev_count] = dev
            cls._dev_name_to_dev_id[dev_name] = cls._dev_count

            cls._dev_count += 1  # Update device count

            # Register device as a node in the cli
            root_node = nodes.Node.get_root_node()
            if root_node is None:
                raise nodes.CLINodeError("No platform has been registered")
            root_node.register(dev_name)

        return dev

    #TODO: Add a task that can reject a platform if it's different from a platform
    # has been registered (i.e disable those devices in that platform by stopping
    # their creation by sending a task)
    @classmethod
    def register_platform(cls, platform_name):
        """A callable task from an mcu. Register the root node for devices in the cli."""

        if nodes.Node.get_root_node() is None:
            nodes.Node(platform_name)

    @classmethod
    def register_test(cls, dev_name, test):
        """Register a test for a specific device"""

        nodes.Node(test.__name__, test.__doc__, dev_name, test)

    @classmethod
    def update_instance_attr(cls, dev_id, thread_id, mcu_dev_id, attr_id, value):
        """A callable task from an mcu. It's used to update a device instance's attribute."""

        # Get the correct device instance
        dev = cls._devs[dev_id]
        dev_instance_id = dev._mcu_info_to_cu_id[(thread_id, mcu_dev_id)]
        dev_instance = dev.instances[dev_instance_id]

        attr_name = dev._attrs_id_to_attr_name[attr_id]
        dev_instance[attr_name] = value  # Update value for the given attr


commands.Command()

