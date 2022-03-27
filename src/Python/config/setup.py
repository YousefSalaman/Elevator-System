"""General setup module for the testbed system

This module combines the different parts of the system to define
the setup methods. These are used to define the main function,
which runs the entire testbed system.
"""

import os
import importlib
from glob import glob
import serial.tools.list_ports as list_ports

from . import tasks
from . import commands
from ..tools import cli, messengers


# Configuration constants

_DEV_INFO = {  # A dict to map specific devices to its info (baudrate, endianness)
    None: (115200, False)  # Default device info
}

_TASK_COUNT = 10  # Available tasks to be scheduled for the schedulers


# Configuraion functions

def define_interface_cmds():
    """Define the default interface commands in the system"""

    cli.Command("run", commands._run_node_callback, lambda: hasattr(cli.Node.get_current_node(), "callback"))
    cli.Command("end", cli.deactivate)
    cli.Command("help", commands._show_commands)
    cli.Command("desc", commands._show_description)
    cli.Command("back", commands._navigate_up_node_tree, lambda: cli.Node.get_current_node().parent is not None)
    cli.Command("go", commands._navigate_down_node_tree, lambda: not hasattr(cli.Node.get_current_node(), "callback"))


def define_scheduler_tasks(scheduler):
    """Define the default scheduler tasks"""

    scheduler.register_task(tasks.REGISTER_PLATFORM, -1, tasks.register_platform)
    scheduler.register_task(tasks.REGISTER_TRACKER, -1, tasks.register_tracker)
    scheduler.register_task(tasks.REGISTER_DEVICE, -1, tasks.register_device)
    scheduler.register_task(tasks.REGISTER_TESTER, -1, tasks.register_tester)
    scheduler.register_task(tasks.ADD_DEVICE_ATTR, -1, tasks.add_device_attr)
    scheduler.register_task(tasks.ALERT_MCU_SETUP_COMPLETION, -1, tasks.alert_mcu_setup_completion)


def import_platform(platform_name):
    """Decides what platform to import based on the platform name"""

    os.chdir("platforms")
    _modules = glob(os.path.join(os.getcwd(), "*"))  # Files and directories in tests directory

    for module in _modules:
        if platform_name in module:
            importlib.import_module("platforms." + os.path.basename(module))
            return
    raise ImportError("A platform with the name '{}' was not found".format(platform_name))


def setup_mcu_channels():
    """Find devices conneceted to ports and create messengers to interact with them"""

    mcu_list = list_ports.comports()
    for mcu in mcu_list:
        dev_info = _get_dev_info(mcu.name)
        messengers.SerialMessenger(mcu.device, dev_info[0], _TASK_COUNT, dev_info[1])


def _get_dev_info(port_name):
    """Get device information from port name"""

    dev_info = _DEV_INFO.get(port_name)
    if dev_info is not None:
        return dev_info
    return _DEV_INFO[None]
