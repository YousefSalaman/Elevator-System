from ...config import teardown, tasks
from ...tools import cli, messengers, devices
from . import manager, commands, constants, tests
from .tasks import (pass_elevator_floor_name,
                    alert_floor_arrival,
                    alert_person_removal,
                    alert_person_addition,
                    remove_elevator_from_floor)


# Command display conditions

def _in_elevator_page():
    """Check if the current page is the elevator page"""

    return cli.Node.get_current_node().name == "elevator"


def _in_system_page():
    """Check if the current page is the system page"""

    return cli.Node.get_current_node().name == "elevator_system"


# Register commmands
cli.Command("request", commands.request, _in_elevator_page)
cli.Command("enter", commands.enter, _in_elevator_page)
cli.Command("display_stats", commands.elevator_statuses, _in_system_page)

# Register tasks
messengers.SerialMessenger.register_task_to_all_mcus(
    constants.PASS_ELEVATOR_FLOOR_NAME, -1, pass_elevator_floor_name)
messengers.SerialMessenger.register_task_to_all_mcus(
    constants.ALERT_FLOOR_ARRIVAL, 2, alert_floor_arrival)
messengers.SerialMessenger.register_task_to_all_mcus(
    constants.ALERT_PERSON_ADDITION, 2, alert_person_addition)
messengers.SerialMessenger.register_task_to_all_mcus(
    constants.REMOVE_CAR_FROM_FLOOR, 2, remove_elevator_from_floor)
messengers.SerialMessenger.register_task_to_all_mcus(
    constants.ALERT_PERSON_REMOVAL, 2, alert_person_removal)

print messengers.SerialMessenger._messengers

# Wait for the mcus to finish their setup
while not tasks.is_mcu_setup_complete():
    pass

# Register tests
elevator_tracker = devices.DeviceTracker.get_tracker(constants.ELEVATOR_TRACKER)
elevator_tracker.register_test(tests.manual_set)

# Setup the manager
manager.get_all_elevator_floors()
teardown.atexit(manager.close)

manager.start()  # Start the elevator manager
