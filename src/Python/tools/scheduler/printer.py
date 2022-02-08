"""
The printer system of the scheduler library.

It assumes that for a given process containing a set of scheduler
objects, the schedulers share the same task table.
"""

from __future__ import print_function

import warnings
from numbers import Integral

from . import constants


_task_print_dict = {}  # Dictionary to store task printers


def register_msg(task_num, task_msg, task_msg_num, verbose=True):

    _register_msg(constants.EXTERNAL_TASK, task_num, task_msg, task_msg_num, verbose)


def register_task(task_name, task_num, *task_vars):

    _register_task(task_name, constants.EXTERNAL_TASK, task_num, *task_vars)


def print_task_msg(dev_name, print_bytes):

    msg_print_info = _print_decode_bytes(print_bytes)
    entry = msg_print_info[:2]
    task_msg_num = msg_print_info[2]

    task_printer = _task_print_dict.get(entry)
    if task_printer is None:
        warnings.warn("No task with the number {} has been registered in the printer".format(entry[1]),
                      SchedulerPrinterWarning)
    else:
        task_printer.print_msg(dev_name, task_msg_num)


def _print_decode_bytes(print_bytes):

    # Extract values from incoming bytes
    task_type = None
    task_num = None
    task_msg_num = None

    return task_type, task_num, task_msg_num


def _register_msg(task_type, task_num, msg, msg_number, verbose=True):

    printer_entry = (task_type, task_num)
    task_printer = _task_print_dict.get(printer_entry)

    # Verify if task printer was created to register the message in
    if task_printer is None:
        if task_type == constants.INTERNAL_TASK:
            task_type = "internal"
        else:
            task_type = "external"
        raise SchedulerPrinterError("No {} task printer entry was found with ".format(task_type) +
                                    "number {} has been registered, so message was not registered".format(task_num))

    task_printer.register_msg(msg_number, msg, verbose)


def _register_task(task_name, task_type, task_num, *task_vars):

    # Verify if valid task printer attributes were entered
    if not isinstance(task_num, Integral):
        raise SchedulerPrinterError("Task number must be an integer")

    if not all(isinstance(task_var, str) for task_var in task_vars):
        raise SchedulerPrinterError("Not all task variable names are strings")

    if not isinstance(task_name, str):
        raise SchedulerPrinterError("Task name must be a string")

    # Register task in printer
    printer_entry = (task_type, task_num)
    if _task_print_dict.get(printer_entry) is not None:
        raise SchedulerPrinterError("A task with number {} has been registered in the printer".format(task_num))
    _task_print_dict[printer_entry] = _TaskPrinter(task_name, *task_vars)


class SchedulerPrinterError(Exception):
    pass


class SchedulerPrinterWarning(Warning):
    pass


class _TaskPrinter:

    def __init__(self, name, task_vars):

        self.msgs = {}
        self.vars = task_vars
        self.name = name.strip()

    def print_msg(self, dev_name, task_num, task_msg_num):

        msg = self.msgs.get(task_msg_num)
        if msg is None:
            warnings.warn("Scheduler printer tried to print an unregistered message with "
                          " number {} for task {}".format(task_msg_num, task_num), SchedulerPrinterWarning)
        print("[" + dev_name + "] " + msg)

    def register_msg(self, msg_num, base_msg, task_num, verbose):

        msg_str = self._build_msg_str(base_msg, task_num, verbose)
        if self.msgs.get(msg_num) is not None:
            raise SchedulerPrinterError("A message with message number {} already "
                                        "has been registered for task {}".format(msg_num, task_num))
        self.msgs[msg_num] = msg_str

    def _build_msg_str(self, base_msg, task_num, verbose):
        """Build the message string that is going to be printed when invoked"""

        msg_str = "Task " + str(task_num)
        if verbose:
            msg_str += " ({})".format(self.name)

        try:
            msg_str += " - " + base_msg
            for task_var_pos, task_var in enumerate(self.vars):
                msg_str.replace("{" + task_var + "}", "{" + str(task_var_pos) + "}")
        except TypeError:
            raise SchedulerPrinterError("Message that was trying to be registered is not string")

        return msg_str
