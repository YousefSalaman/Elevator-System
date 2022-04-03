
from ..tools import cli, devices, testers


# System task constants

REGISTER_PLATFORM = 255
REGISTER_TRACKER = 254
REGISTER_DEVICE = 253
REGISTER_TESTER = 252
ADD_DEVICE_ATTR = 251
ALERT_MCU_SETUP_COMPLETION = 250
UPDATE_DEVICE_ATTR_COMP = 249
UPDATE_DEVICE_ATTR_MCU = 248


# Innate tasks of the testbed

def register_platform(thread_id, scheduler, pkt):
    """Registers a platform in the computer

    Each time this task is called, a thread's scheduler is be named.
    The name, the corresponding thread id, is used to fetch the
    correct scheduler object. Furthermore, the thread id is stored
    in the _mcu_setup_status variable to verify if the mcu setup has
    been completed. Since this is the first task an mcu calls in its
    setup, this used to register the mcu in the variable and state
    it hasn't completed the setup.

    Now, when this task is first called, it will setup everything
    related to the relevant platform like its node and defining
    the device tasks and tests.
    """

    scheduler.name = thread_id
    _mcu_setup_status[thread_id] = False

    # Setup platform
    if cli.Node.get_root_node() is None:
        platform_name = str(pkt)
        cli.Node(platform_name)


def register_tracker(pkt):
    """Register a device with a given name if it wasn't registered"""

    tracker_name = str(pkt)
    if not devices.DeviceTracker.was_tracker_created(tracker_name):
        devices.DeviceTracker.add_tracker(tracker_name)

        # Register device as a node in the cli
        root_node = cli.Node.get_root_node()
        if root_node is None:
            raise cli.CLINodeError("No platform has been registered")
        root_node.register(tracker_name)


def register_device(thread_id, pkt):
    """Register an instance of the device using info of the mcu with a given name"""

    # Get pkt info
    tracker_id = pkt[0]
    device_count = pkt[1]

    tracker = devices.DeviceTracker.get_tracker(tracker_id=tracker_id)
    if tracker is not None:
        tracker.add_device(device_count, thread_id)


def register_tester(pkt):
    """Register a tester object to be used in the system"""

    # Get pkt info
    task_id = pkt[0]
    priority_type = pkt[1]
    task_name = pkt[2:]

    # Create and add a tester object in the system
    testers.Tester.add_tester(task_name, task_id, priority_type)


def add_device_attr(pkt):
    """Add a device instance attribute to store and retrieve values"""

    # Get pkt info
    tracker_id = pkt[0]
    attr_id = pkt[1]
    attr_name = str(pkt[2:])

    # Add the attribute to the attribute list
    tracker = devices.DeviceTracker.get_tracker(tracker_id=tracker_id)
    if tracker is not None:
        tracker.add_device_attr(attr_id, attr_name)


def alert_mcu_setup_completion(thread_id, _):
    """Alert the computer that the current MCU is done with its scheduler setup"""

    _mcu_setup_status[thread_id] = True


def update_device_attr_comp(thread_id, pkt):
    """Update a device instance's attribute in the computer"""

    # Get pkt info
    tracker_id = pkt[0]
    device_id = pkt[1]
    attr_id = pkt[2]
    attr_type = pkt[3]
    value_pkt = pkt[4:]

    devices.DeviceTracker.update_device_attr_comp(tracker_id, attr_id, device_id, thread_id,
                                                  attr_type, value_pkt)


def update_device_attr_mcu(tracker_name, device_name, attr_name, attr_type, value):
    """Update a device instance's attribute on an MCU"""

    update_info = devices.DeviceTracker.update_device_attr_mcu(tracker_name, device_name,
                                                               attr_name, attr_type, value)

    if update_info is not None:
        scheduler, pkt = update_info
        scheduler.schedule_normal_task(UPDATE_DEVICE_ATTR_MCU, pkt)


# Task helpers

_mcu_setup_status = {}  # Indicates if an mcu has finished with its setup


def is_mcu_setup_complete():
    """Verify if the MCUs have finished their setup"""

    if len(_mcu_setup_status) == 0:
        return False
    return all(_mcu_setup_status.items())
