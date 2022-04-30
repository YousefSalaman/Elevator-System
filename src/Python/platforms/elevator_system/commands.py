from __future__ import print_function

import sys
import random
import struct

from . import manager, constants
from ...tools import testers, devices


# Define version specific functions
if sys.version_info[:2] <= (2, 7):  # If Python 2.7
    user_input = raw_input
else:
    user_input = input


def request():
    """Requests an elevator from the elevator system"""

    answer = user_input("Would you like to request an elevator? (Y/N) ").upper()
    if answer in {"Y", "N"}:
        if answer == "Y":
            print("These are the available floors: [" + ", ".join(manager.get_floors()) + "]")
            floor = user_input("Which floor are currently you on? ")
            if floor in manager.get_floors():
                manager.mutate_requests(floor)
                print("An elevator will be heading your way shortly.")
            else:
                print("{} is not a floor option.".format(floor))
    else:
        print("The entry {} is not a valid reply. Please reply Y for yes and N for no".format(answer))


def enter():
    """Enter the elevator if it's in a specific floor"""

    print("These are the available floors: [" + ", ".join(manager.get_floors()) + "]")
    floor = user_input("Which floor do you wish to enter? ")
    if floor in manager.get_floors():
        elevator = manager.get_elevator_in_floor(floor)
        if elevator is None:
            print("There are currently no elevators on this floor")
        else:
            enter_elevator = testers.Tester.get_tester("enter_elevator")
            enter_elevator(elevator, _generate_person_pkt(elevator))


def elevator_statuses():
    """Display the status for each elevator in the elevator system"""

    elevator_tracker = devices.DeviceTracker.get_tracker(constants.ELEVATOR_TRACKER)
    elevators = elevator_tracker.devices.values()
    attr_list = list(elevator_tracker.device_attrs)
    for elevator in elevators:
        print(elevator.name)
        for attr in attr_list:
            print("\t{}:".format(attr) + "\t" + str(getattr(elevator, attr)))
        print("\n")


# Helpers for the elevator cli commands

def _generate_person_pkt(elevator):
    """Generate random person info"""

    weight = random.randint(115, 230)  # Possible weight range
    temperature = random.randint(0, 2)  # Possible temperature differential

    return struct.pack("BBB", elevator.device_id, temperature, weight)
