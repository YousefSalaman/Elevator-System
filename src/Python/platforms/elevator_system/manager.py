import threading

from . import constants
from ...tools import devices, testers


# Manager variables

_positions = {}  # Current positions for the elevators
_requests = set()  # Storage for requests
_manager_thread = None  # Thread that runs the elevator manager

_MANAGER_LOCK = threading.Lock()


def add_elevator_to_floor(elevator, floor_name):
    """Add an elevator to the elevator positioning listing"""

    if floor_name in _positions:
        _positions[floor_name].add(elevator)


def close():
    """Close manager thread"""

    if _manager_thread is not None:
        _manager_thread.join()


def remove_elevator_from_floor(elevator, floor_name):
    """Remove an elevator from the given floor"""

    elevators = _positions.get(floor_name)
    if elevators is not None and elevator in elevators:
        _positions[floor_name].pop(elevator)


def get_all_elevator_floors():
    """Create a registry of all the floors available in the system"""

    global _positions

    all_floors = set()
    for elevator in devices.DeviceTracker.get_tracker(constants.ELEVATOR_TRACKER).devices.values():
        all_floors = all_floors.union(set(elevator.floors))

    _positions = dict.fromkeys(all_floors, set())


def get_elevator_in_floor(floor):
    if len(_positions[floor]) != 0:
        return _positions[floor].pop()
    return None


def get_floors():
    """Get the available floors in the system"""

    return _positions.keys()


def mutate_requests(floor, add_floor=True):
    """Add or remove requests from the request pool"""

    _MANAGER_LOCK.acquire()

    if add_floor:
        _requests.add(floor)
    else:
        _requests.remove(floor)

    _MANAGER_LOCK.release()


def start():
    global _manager_thread

    _manager_thread = threading.Thread(target=run)
    _manager_thread.start()


def run():
    """Main function that reads the requests of the system"""

    while True:

        for request_floor in _requests.copy():
            elevator = _find_elevator(request_floor)
            if elevator is not None:
                request_elevator = testers.Tester.get_tester("request_elevator")
                request_elevator(elevator, bytearray([elevator.device_id, elevator.floors[request_floor]]))
                mutate_requests(request_floor, False)


def _find_elevator(request_floor):
    """Find an elevator that can be called to current location"""

    chosen_elevator = None
    chosen_floor_distance = None

    elevators = devices.DeviceTracker.get_tracker(constants.ELEVATOR_TRACKER).devices.values()
    for elevator in elevators:
        if _valid_elevator(elevator, request_floor):
            floor_distance = abs(elevator.current_floor - elevator.floors[request_floor])
            if chosen_elevator is None or chosen_floor_distance > floor_distance:
                chosen_elevator = elevator
                chosen_floor_distance = floor_distance

    return chosen_elevator


def _valid_elevator(elevator, request_floor):
    """Check if the elevator is a possible candidate to transport the person"""

    # This checks if the requested floor is along the current path the elevator is taking
    in_elevator_path = False
    if request_floor in elevator.floors:
        request_floor_num = elevator.floors[request_floor]
        in_elevator_path = elevator.next_floor == constants.NULL_FLOOR \
                           or \
                           (elevator.current_floor > elevator.next_floor and request_floor_num > elevator.next_floor) \
                           or \
                           (elevator.current_floor < elevator.next_floor and request_floor_num < elevator.next_floor)

    return elevator.capacity and not (elevator.maintanence_state or elevator.emergency_state) and in_elevator_path
