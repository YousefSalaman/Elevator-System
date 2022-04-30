from __future__ import print_function

import sys

from . import constants
from ...config import tasks
from ...tools import devices, messengers


# Define version specific functions
if sys.version_info[:2] <= (2, 7):  # If Python 2.7
    user_input = raw_input
else:
    user_input = input


def manual_set():
    """Manually set up the values of the elevators in the MCUs"""

    # Get information from the user

    print("The attributes that can be modified are: " + ", ".join(constants.SET_TASKS))
    attr_name = user_input("Which attribute do you want to modify? ")
    attr_type = constants.SET_TASKS.get(attr_name)

    elevator_tracker = devices.DeviceTracker.get_tracker(constants.ELEVATOR_TRACKER)
    elevator_names = [elevator.name for elevator in elevator_tracker.devices.values()]
    print("Here are the following elevators that can be motified: " + ", ".join(elevator_names))
    elevator_name = user_input("Which elevator do you want to modify? ")

    # Verify values
    value = user_input("Enter the value for the attribute: ")
    if attr_type in {"uint8_t", "uint16_t"}:
        if not ((attr_type == "uint8_t" and 0 <= value <= 230) and
                (attr_type == "uint16_t" and 0 <= value <= 2000)):
            print("Values for uint8_t types have to be within [0, 230] and"
                  "for uin16_t have to be within [0, 2000]")

    # Update value in the MCU if its valid
    if attr_name in constants.SET_TASKS and elevator_name in elevator_names and attr_type in {"uint8_t", "uint16_t"}:
        tasks.update_device_attr_mcu(constants.ELEVATOR_TRACKER, elevator_name, attr_name, attr_type, value)
    else:
        print("Not all the inputs where valid")
