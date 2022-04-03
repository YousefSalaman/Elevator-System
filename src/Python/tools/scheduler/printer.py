"""Printer system of the scheduler library.

Its purpose is to provide a way to display messages on the main
computer side based on events that are happening in the MCUs while
reducing the amount of information being transferred through the
choosen method of communication. This is done by registering the
messages in the computer and using the scheduling system to trigger
the what message to print.
"""

from __future__ import print_function

import struct
import warnings
from numbers import Integral

from . import constants


class SchedulerPrinterError(Exception):
    pass


class SchedulerPrinterWarning(Warning):
    pass


class SchedulerPrinter:

    _struct_format_chars = {'c', 'b', 'B', '?', 'h', 'H', 'i', 'I', 'e', 'f',  # Allowed format character for unpacking
                            'q', 'Q', 'n', 'N', 'd'}

    def __init__(self, endianness, skip_set_up=False):

        self._task_printers = {}  # Dictionary to store task printers

        # Set endianness to interpret incoming task printer variables
        if endianness:  # If it's little endian
            self._endianness = '<'
        else:  # If it's big endian
            self._endianness = '>'

        # Create internal task printers and messages
        if not skip_set_up:
            self._set_up_internal_task_printers()

    @property
    def endianness_format(self):
        """Retrieves the format character used for the endianness"""

        return self._endianness

    @property
    def is_little_endian(self):
        """Notifies if the printer interprets incoming tasks variables """

        return self._endianness == '<'

    def print_task_msg(self, print_buf, dev_name=None):

        task_id, task_type, msg_num = print_buf
        self._task_printer_exec(task_type, task_id, "print_msg", dev_name, task_id, msg_num)

    def modify_task_printer_var(self, var_buf):

        # Extract variable info from buffer
        task_id, task_type, var_id = var_buf[0:3]
        var_type = struct.unpack("c", var_buf[3:4])

        try:
            if var_type in self._struct_format_chars:
                var_value = struct.unpack("{}{}".format(self._endianness, var_type), var_buf[4:])
                self._task_printer_exec(task_type, task_id, "update_task_var", var_id, var_value)
            else:
                warnings.warn("An incorrect unpacking key was sent to change variable {} ".format(var_id) +
                              "in task {} in the printer".format(task_id), SchedulerPrinterWarning)
        except struct.error:
            warnings.warn("An incorrect amount of bytes were sent for the unpacking key {} ".format(var_type) +
                          "for variable {} in task ()".format(var_id, task_id), SchedulerPrinterWarning)

    def register_msg(self, task_id, task_msg, task_msg_num, verbose=True):

        self._register_msg(constants.EXTERNAL_TASK, task_id, task_msg, task_msg_num, verbose)

    def register_task(self, task_name, task_id, *task_vars):

        self._register_task(task_name, constants.EXTERNAL_TASK, task_id, *task_vars)

    def set_task_msg_silent_status(self, task_id, msg_num, silent_status):

        self._task_printer_exec(constants.EXTERNAL_TASK, task_id, "set_msg_silent_status", msg_num, silent_status)

    def set_task_silent_status(self, task_id, silent_status):

        self._task_printer_exec(constants.EXTERNAL_TASK, task_id, "set_task_silent_status", silent_status)

    def _register_msg(self, task_type, task_id, msg, msg_number, verbose=True):

        printer_entry = (task_type, task_id)
        task_printer = self._task_printers.get(printer_entry)

        # Verify if task printer was created to register the message in
        if task_printer is None:
            if task_type == constants.INTERNAL_TASK:
                task_type = "internal"
            else:
                task_type = "external"
            raise SchedulerPrinterError("No {} task printer entry was found with ".format(task_type) +
                                        "number {} has been registered, so message was not registered".format(task_id))

        task_printer.register_msg(msg_number, msg, verbose)

    def _register_task(self, task_name, task_type, task_id, *task_vars):

        # Verify if valid task printer attributes were entered
        if not isinstance(task_id, Integral):
            raise SchedulerPrinterError("Task number must be an integer")

        if not all(isinstance(task_var, str) for task_var in task_vars):
            raise SchedulerPrinterError("Not all task variable names are strings")

        if not isinstance(task_name, str):
            raise SchedulerPrinterError("Task name must be a string")

        # Register task in printer
        printer_entry = (task_type, task_id)
        if self._task_printers.get(printer_entry) is not None:
            raise SchedulerPrinterError("A task with number {} has been registered in the printer".format(task_id))
        self._task_printers[printer_entry] = _TaskPrinter(task_name, *task_vars)

    def _set_up_internal_task_printers(self):
        """Set up the task printers and their respective messages for the internal tasks"""

        self._register_task("ALERT SYSTEM", constants.INTERNAL_TASK, constants.ALERT_SYSTEM)
        self._register_task("PRINT MESSAGE", constants.INTERNAL_TASK, constants.PRINT_MESSAGE)
        self._register_task("UNSCHEDULE TASK", constants.INTERNAL_TASK, constants.UNSCHEDULE_TASK)
        self._register_task("MODIFY PRINTER VARS", constants.INTERNAL_TASK, constants.MODIFY_PRINTER_VARS)
        self._register_task("PKT DECODE", constants.INTERNAL_TASK, constants.INTERNAL_TASK,
                            "expected pkt size", "received pkt size", "task number")
        self._register_task("PKT ENCODE", constants.INTERNAL_TASK, constants.PKT_ENCODE)
        self._register_task("TASK LOOKUP", constants.INTERNAL_TASK, constants.TASK_LOOKUP)
        self._register_task("TASK REGISTER", constants.INTERNAL_TASK, constants.TASK_REGISTER)

        # Register internal task messages in printer

        self._register_msg(constants.INTERNAL_TASK, constants.PKT_DECODE,
                           "short pkt header size", constants.SHORT_PKT_HDR_SIZE)
        self._register_msg(constants.INTERNAL_TASK, constants.PKT_DECODE,
                           "crc16 checksum fail", constants.CRC_CHECKSUM_FAIL)
        self._register_msg(constants.INTERNAL_TASK, constants.PKT_DECODE,
                           "task with number {task number} was not registered", constants.CRC_CHECKSUM_FAIL)
        self._register_msg(constants.INTERNAL_TASK, constants.PKT_DECODE,
                           "Expected {expected pkt size} byte(s) from the packet payload "
                           "but received {received pkt size} byte(s) for task {task number}",
                           constants.INCORRECT_PAYLOAD_SIZE)

    def _task_printer_exec(self, task_type, task_id, f_name, *args):
        """Fetch corresponding task printer and run a task printer method with it"""

        entry = (task_type, task_id)
        task_printer = self._task_printers.get(entry)
        if task_printer is None:
            warnings.warn("No task with the number {} has been registered in the printer".format(entry[1]),
                          SchedulerPrinterWarning)
        else:
            task_printer_method = getattr(task_printer, f_name)
            task_printer_method(*args)


class _TaskPrinter:
    """Printer for an individual task

    Task printer objects hold the messages that are going to be
    printed along with other convinient attributes to print useful
    messages for a given task. Only messages that are related to a
    task are stored within the object. As of now, the following
    is what is stored within a message entry:

    - msg_str: The message to be printed. The complete message is
      built when it is registered in the object.

    - silence_status: It indicates if a specific message will be
      printer or not. This is meant to be controlled within the
      main computer itself when one needs to print a message or
      not.
    """

    def __init__(self, name, task_vars):

        self.msgs = {}
        self.vars = []
        self.name = name.strip()

        # Add task variables to printer
        for task_var in task_vars:
            task_var_strip = task_var.strip()
            if ' ' in task_var_strip:
                raise SchedulerPrinterError("Task printer variables cannot contain spaces. Use a "
                                            "valid python attribute name as the task variable name.")
            self.vars.append(task_var_strip)

        self._silent = False

    def print_msg(self, dev_name, task_id, msg_num):

        msg = self.msgs.get(msg_num)
        if msg is None:
            warnings.warn("Scheduler printer tried to print an unregistered message with "
                          "number {} for task {}".format(msg_num, task_id), SchedulerPrinterWarning)

        silent_msg = msg[1]
        if not (self._silent or silent_msg):
            task_vars_values = [getattr(self, task_var) for task_var in self.vars]
            print_msg = msg[0].format(*task_vars_values)
            if dev_name is not None:
                print_msg = "[{}]".format(dev_name) + print_msg
            print(print_msg)

    def register_msg(self, msg_num, base_msg, task_id, verbose):

        msg_str = self._build_msg_str(base_msg, task_id, verbose)
        if self.msgs.get(msg_num) is not None:
            raise SchedulerPrinterError("A message with message number {} already "
                                        "has been registered for task {}".format(msg_num, task_id))
        self.msgs[msg_num] = [msg_str, False]

    def set_task_silent_status(self, silent_status):

        self._silent = silent_status

    def set_msg_silent_status(self, msg_num, silent_status):

        if self.msgs.get(msg_num) is None:
            warnings.warn("A message with number {} has not been registered ".format(msg_num) +
                          "in task {}".format(self.name), SchedulerPrinterWarning)
        else:
            self.msgs[msg_num][1] = silent_status

    def update_task_var(self, var_id, value):

        if var_id < len(self.vars):
            setattr(self, self.vars[var_id], value)
        else:
            if len(self.vars) > 0:
                num_of_vars = len(self.vars) - 1
            else:
                num_of_vars = 0
            warnings.warn("The variable id for the task {} must be ".format(self.name) +
                          "a value from 0 to {}, but the one given was {}".format(num_of_vars, var_id),
                          SchedulerPrinterWarning)

    def _build_msg_str(self, base_msg, task_id, verbose):
        """Build the message string that is going to be printed when invoked"""

        msg_str = "Task " + str(task_id)
        if verbose:
            msg_str += " ({})".format(self.name)

        try:
            msg_str += " - " + base_msg
            for task_var_pos, task_var in enumerate(self.vars):
                msg_str.replace("{" + task_var + "}", "{" + str(task_var_pos) + "}")
        except TypeError:
            raise SchedulerPrinterError("Message that was trying to be registered is not a string")

        return msg_str
