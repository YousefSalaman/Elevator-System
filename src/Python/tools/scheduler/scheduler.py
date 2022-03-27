
import warnings
from time import time
from collections import deque

from . import printer
from . import constants
from . import pkt_handling as pkt_handler


class _TaskQueueEntry(object):
    """An object that holds the information of an outgiong task packet in
    the scheduling queues.
    """

    __slots__ = ("id", "pkt", "rescheduled")

    def __init__(self):

        self.pkt = pkt_handler.SchedulerPacket()

    def __eq__(self, other):

        if hasattr(other, "id"):
            return other.id == self.id
        return other == self.id

    def __repr__(self):

        return "Task {}".format(self.id)


class _TaskTableEntry(object):
    """An object that holds the information of an item in the task table"""

    __slots__ = ("id", "task", "size")

    def __init__(self, task, size):

        if type(size) != int:
            raise TypeError("Size of task must be an integer")

        if not callable(task):
            raise TypeError("The task provided is not callable")

        self.size = size
        self.task = task


class _SchedulerQueue(deque):
    """Modified deque class with convenient methods.

    This was mostly added to make the code more readable while
    mimicking the code in the microcontrollers.
    """

    @property
    def peek(self):
        """Get the head of the queue"""

        return self[0]

    def reschedule(self):
        """Put the current task in the back of the queue"""

        entry = self[0]
        self.append(self.popleft())

        entry.rescheduled = True


class _SchedulingLists:
    """Lists for scheduling task entries"""

    def __init__(self, task_count):

        self.normal = _SchedulerQueue()
        self.priority = _SchedulerQueue()
        self.unscheduled = deque(_TaskQueueEntry() for _ in range(task_count))

        self._current_q = None

    @property
    def queue_type(self):
        """Indicates which queue should be processed.

        If true, then the queue to be processed is the normal queue.
        Otherwise, then the queue to be processed is the priority
        one.
        """

        self._current_q = len(self.priority) == 0
        return self._current_q

    @property
    def current_queue(self):

        if self._current_q:
            return self.normal
        return self.priority

    def in_queues(self, task_id):

        return task_id in self.normal or task_id in self.priority

    def queues_are_empty(self):

        return len(self.normal) == 0 and len(self.priority) == 0

    def queues_are_full(self):

        return len(self.unscheduled) == 0

    def is_priority_queue_empty(self):

        return len(self.priority) == 0

    def prioritize_normal_task(self):

        self.priority.append(self.normal.popleft())

    def push_task(self, task_id, task_type, pkt, is_priority, is_fast):

        entry = self._prepare_unscheduled_task(task_id, task_type, pkt)

        # If there are no unscheduled tasks, then tell the system
        # there are no more tasks that can be scheduled at the
        # moment
        if entry is None:
            return False

        # Get the queue to push task to
        if is_priority:
            target_queue = self.priority
        else:
            target_queue = self.normal

        # Place unscheduled task in corresponding queue and place
        if is_fast:
            target_queue.appendleft(entry)
        else:
            target_queue.append(entry)

        return True

    def _prepare_unscheduled_task(self, task_id, task_type, payload_pkt):

        # If no task is available for scheduling, then return nothing
        if self.queues_are_full():
            return None

        new_task = self.unscheduled.popleft()

        # Process and verify if the outgoing packet was processed correctly
        if not new_task.pkt.process_outgoing_pkt(task_id, task_type, payload_pkt):
            return None

        new_task.id = task_id  # Pass the task id to the unscheduled task

        return new_task


class SchedulerError(Exception):
    pass


class SchedulerWarning(Warning):
    pass


class Scheduler:
    """Scheduler that manages communications with other devices

    These are the main parts of the scheduler:

    - Rx callback function

    After a packet has been processed, this callback will be called with the
    task function, task id, and its associated payload. The callback must
    define a way to run the task with the given information.

    - Tx callback function

    When a task needs to be sent to another destination, this callback will
    be called with the packet and the amount of bytes to sent. The callback
    must define a way to send the packet to a microcontroller.

    - Build incoming packet

    Similar to the tx callback, one must define a function that receives the
    incoming bytes and pass those bytes to a scheduler through the build
    incoming packet method.

    - Registering tasks

    For a task to be callable from a microcontroller, one must register a task
    with its task id and a function that represents the task to be performed.

    - Scheduling tasks

    To make an MCU perform a task, one must schedule it through a scheduler. If
    you need to perform a task quickly, one can schedule it with priority. This
    will skip the alert task completion verification from the MCU and remove it
    from the priority queue, the place where it's scheduled, as soon as it's sent
    to the MCU. Otherwise, it will schedule a normal task, which will reschedule
    a task if the computer didn't receive the alert task completion flag from the
    MCU.
    """

    _schedulers = {}

    def __init__(self, task_count, name=None, is_little_endian=True, no_internal_setup=False):

        self._name = name
        self.task_table = {}
        self.prev_task = None
        self.start_time = None
        self.printer = printer.SchedulerPrinter(is_little_endian, no_internal_setup)

        self._rx_pkt = pkt_handler.SchedulerPacket()
        self._schedule_qs = _SchedulingLists(task_count)

        self.rx_callback = None
        self.tx_callback = None

        if name is not None:
            self._schedulers[name] = self

    def build_incoming_pkt(self, byte):
        """Form and detect the incoming packet reading byte-by-byte"""

        if self._rx_pkt.process_incoming_byte(byte):
            self._perform_task()

    def copy(self, copy_callbacks=False):
        """Create a scheduler copy with the given scheduler"""

        task_count = len(self._schedule_qs.unscheduled)
        scheduler = Scheduler(task_count, self.printer.is_little_endian, True)
        scheduler.printer = self.printer
        scheduler.task_table = self.task_table

        if copy_callbacks:
            scheduler.tx_callback = self.tx_callback
            scheduler.rx_callback = self.rx_callback

        return scheduler

    @classmethod
    def create_task_handler(cls, task_id):

        return lambda scheduler_name, pkt: cls.perform_task(scheduler_name, task_id, pkt)

    @classmethod
    def get_scheduler(cls, name):

        scheduler = cls._schedulers.get(name)
        if scheduler is None:
            warnings.warn("Scheduler for {} was not found".format(name), SchedulerWarning)
        return scheduler

    @property
    def name(self):

        return self._name

    @name.setter
    def name(self, name):

        named_scheduler = self._schedulers.get(name)
        if named_scheduler is None or named_scheduler is self:
            self._name = name
            self._schedulers[name] = self

    @classmethod
    def perform_task(cls, scheduler_name, task_id, pkt):
        """Schedule a normal task by using its scheduler name"""

        scheduler = cls._schedulers.get(scheduler_name)
        if scheduler is not None:
            scheduler._schedule_general_task(task_id, constants.EXTERNAL_TASK, pkt, False, False)

    def register_task(self, task_id, task_size, callback):
        """Register a task in the scheduler, so it can be called by another system."""

        # If task with the same number has already been registered, raise error
        if self.task_table.get(task_id):
            raise KeyError("Task number '{}' has already been registered.".format(task_id))

        self.task_table[task_id] = _TaskTableEntry(callback, task_size)

    def schedule_normal_task(self, task_id, pkt):
        """Schedule a normal task to be performed by an MCU"""

        self._schedule_general_task(task_id, constants.EXTERNAL_TASK, pkt, False, False)

    def schedule_priority_task(self, task_id, pkt):
        """Schedule a priority task to be performed by an MCU"""

        self._schedule_general_task(task_id, constants.EXTERNAL_TASK, pkt, True, False)

    def send_task(self):
        """Send a task to be performed by another device connected to this one.

        This method is biased towards sending priority tasks when there is one
        present in the priority queue. When that queue is empty, it will read
        tasks from the normal task queue.
        """

        if not self._schedule_qs.queues_are_empty():

            queue_type = self._schedule_qs.queue_type
            entry = self._schedule_qs.current_queue[0]

            if queue_type:  # Send normal task (i.e. priority queue is empty)

                # Check if the task has been sent already
                if self.prev_task != entry.id:
                    self.prev_task = entry.id
                    self.start_time = time()
                    self.tx_callback(entry.pkt.buf)

                # Check if elapsed task time passed the reply window
                if entry.rescheduled:
                    reply_time_passed = time() - self.start_time >= constants.LONG_TIMER
                else:
                    reply_time_passed = time() - self.start_time >= constants.SHORT_TIMER

                # Handler if the allowed reply time passed
                if reply_time_passed:
                    if entry.rescheduled:
                        self._schedule_qs.normal.popleft()
                    else:
                        entry.rescheduled = True
                        self._schedule_qs.normal.reschedule()

            else:  # Send priority task
                self.tx_callback(entry.pkt.buf)
                self._schedule_qs.priority.popleft()

    def _perform_task(self):
        """Process the stored scheduler rx packet

        In here, the packet that was built with process_incoming byte
        will be processed in its entirety.
        """

        entry = self._rx_pkt.process_incoming_pkt(self.task_table)

        if entry is not None:
            ret_code = self.rx_callback(entry.id, entry.task, self._rx_pkt[constants.PAYLOAD_OFFSET:])
            alert_system_pkt = bytearray([entry.id, ret_code])
            self._schedule_general_task(constants.ALERT_SYSTEM, constants.INTERNAL_TASK,
                                        alert_system_pkt, True, True)

        elif self._rx_pkt.buf[constants.TASK_TYPE_OFFSET] == constants.INTERNAL_TASK:
            self._process_internal_task()

        self._rx_pkt.buf = bytearray()  # Reset rx packet buffer

    def _process_internal_task(self):
        """This method basically acts as an internal task table"""

        internal_task_id = self._rx_pkt.buf[constants.TASK_ID_OFFSET]
        if internal_task_id == constants.ALERT_SYSTEM:
            self._process_current_task()
        elif internal_task_id == constants.PRINT_MESSAGE:
            self.printer.print_task_msg(self._rx_pkt.buf[constants.PAYLOAD_OFFSET:], self.name)
        elif internal_task_id == constants.MODIFY_PRINTER_VARS:
            self.printer.modify_task_printer_var(self._rx_pkt.buf[constants.PAYLOAD_OFFSET:])

    def _process_current_task(self):
        """Decides what action for the task to take based on the other system's reply"""

        if len(self._schedule_qs.normal) != 0:

            entry = self._schedule_qs.normal[0]
            if self._rx_pkt.buf[constants.PAYLOAD_OFFSET] == entry.id:
                if self._rx_pkt.buf[constants.PAYLOAD_OFFSET + 1] and not entry.rescheduled:
                    entry.rescheduled = True
                    self._schedule_qs.normal.reschedule()
                else:
                    self._schedule_qs.normal.popleft()

    def _schedule_general_task(self, task_id, task_type, pkt, is_priority=False, is_fast=False):
        """Schedule a general task to be performed by another system"""

        if not self._schedule_qs.in_queues(task_id):

            # Send a task to free up space in queues (if they are full)
            if self._schedule_qs.queues_are_full():

                # Prioritize a normal task to make space
                if self._schedule_qs.queue_type:
                    self._schedule_qs.prioritize_normal_task()
                self.send_task()

            self._schedule_qs.push_task(task_id, task_type, pkt, is_priority, is_fast)

            # Send task immediately if it's a fast task
            if is_fast:
                self.send_task()
