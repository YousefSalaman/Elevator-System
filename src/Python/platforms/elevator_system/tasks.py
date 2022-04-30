from __future__ import print_function

from ...tools import devices
from . import constants, manager


# Elevator tasks

def pass_elevator_floor_name(thread_id, pkt):
    """Add a floor name to an elevator's listing"""

    device_id = pkt[0]
    floor_id = pkt[1]
    floor_name = str(pkt[2:])
    elevator_tracker = devices.DeviceTracker.get_tracker(constants.ELEVATOR_TRACKER)
    elevator = elevator_tracker.get_device((thread_id, device_id))

    if elevator.floors is None:
        elevator.floors = {}
    elevator.floors[floor_name] = floor_id


def alert_floor_arrival(thread_id, pkt):
    """Alerts the user that an elevator has reached a requested floor"""

    elevator, elevator_num, floor_name = _proc_elevator_pkt(thread_id, pkt)

    print("Elevator {} has reached floor {}".format(floor_name, elevator_num))
    manager.add_elevator_to_floor(elevator, floor_name)


def remove_elevator_from_floor(thread_id, pkt):
    """Removes an elevator from the position listing"""

    elevator, elevator_num, floor_name = _proc_elevator_pkt(thread_id, pkt)

    print("Elevator {} has left floor {}".format(floor_name, elevator_num))
    manager.remove_elevator_from_floor(elevator, floor_name)


def alert_person_addition(thread_id, pkt):
    """Alert that a person is entering the elevator"""

    _, elevator_num, floor_name = _proc_elevator_pkt(thread_id, pkt)

    print("A person has entered elevator {} in floor {}".format(floor_name, elevator_num))


def alert_person_removal(thread_id, pkt):
    """Alert that a person is entering the elevator"""

    _, elevator_num, floor_name = _proc_elevator_pkt(thread_id, pkt)

    print("A person has exited elevator {} in floor {}".format(floor_name, elevator_num))


# Task helpers

def _proc_elevator_pkt(thread_id, pkt):
    """Process the incoming elevator packet"""

    device_id = pkt[0]
    floor_id = pkt[1]
    elevator_tracker = devices.DeviceTracker.get_tracker(constants.ELEVATOR_TRACKER)
    elevator = elevator_tracker.get_device((thread_id, device_id))

    print("floor id: {}".format(floor_id))

    floor_name = "NULL_FLOOR"
    for name, floor_num in elevator.floors.items():
        if floor_id == floor_num:
            floor_name = name
            break

    elevator_num = elevator.name.split("_")[1]

    return elevator, elevator_num, floor_name
