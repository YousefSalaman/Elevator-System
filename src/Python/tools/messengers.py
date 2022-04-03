"""Place to create different types of messenger objects

In here, you will find messenger classes. These are meant to
simplify the interaction between the computer and multiple
MCUs connected to the computer.

By default, the messengers will register a group of scheduler
tasks that have IDs between 250-255. These are used by the
system to setup and create the different elements of command
line interface that is integrated in the testbed.
"""

import abc
import serial
import threading

from . import scheduler
from ..config import setup, tasks


class BaseMessenger(metaclass=abc.ABCMeta):
    """Helper object that listens and writes to the MCUs

    It establishes a connection between the main computer and the MCUs
    connected to it. There's a thread and scheduler for each messenger
    object that is created, where each thread runs the listening
    portion of the scheduler, so information can be processed in
    parallel for multiple MCUs. One can write into a specific MCU by
    accessing a messenger's scheduler and scheduling a task through it.
    """

    _messengers = {}
    _main_scheduler = None
    _rx_lock = threading.Lock()

    def __init__(self, task_count, is_little_endian=False, no_internal_set_up=False):

        self._set_scheduler(task_count, is_little_endian, no_internal_set_up)
        self.thread = threading.Thread(target=self._receive_pkt)
        self._messengers[self.thread.ident] = self

    @abc.abstractmethod
    def _receive_pkt(self):
        """Listens for incoming bytes from the schedulers

        Using the "build_incoming_pkt" method provided by the
        scheduler object, you must override this method with
        a way to read the incoming bytes from an MCU.
        """

    @abc.abstractmethod
    def _tx_scheduler_cb(self, pkt):
        """This callback passes an outgoing packet to an MCU

        You must override this method with a way to write the
        outgoing bytes into an MCU.
        """

    @classmethod
    def normal_schedule_to_all_mcus(cls, task_id, pkt):
        """Schedule a common normal task for all the MCUs"""

        for messenger in cls._messengers:
            messenger.scheduler.schedule_normal_task(task_id, pkt)

    @classmethod
    def priority_schedule_to_all_mcus(cls, task_id, pkt):
        """Schedule a common priority task for all the MCUs"""

        for messenger in cls._messengers:
            messenger.scheduler.priority_priority_task(task_id, pkt)

    @classmethod
    def register_task_to_all_mcus(cls, task_id, task_size, callback):
        """Register a common task in all the schedulers"""

        for messenger in cls._messengers:
            messenger.scheduler.register_task(task_id, task_size, callback)

    @classmethod
    def get_messenger(cls, thread_id):
        """Retrieves a messenger with the given thread id"""

        return cls._messengers[thread_id]

    def _set_scheduler(self, task_count, is_little_endian, no_internal_set_up):
        """Helper method to setup a messenger's scheduler"""

        # Create a scheduler instance
        if self._main_scheduler is None:
            self.scheduler = scheduler.Scheduler(task_count, is_little_endian, no_internal_set_up)
            self._main_scheduler = self.scheduler
        else:
            self.scheduler = self._main_scheduler.copy()

        # Set up callbacks for the scheduler
        self.scheduler.tx_callback = self._tx_scheduler_cb
        self.scheduler.rx_callback = self._rx_scheduler_cb

    def _rx_scheduler_cb(self, task_id, task, pkt):
        """This callback runs the task was requested with the packet that was sent

        This is mostly meant to post-process an incoming task. There are
        some cases, like this one, that one needs to perform certain
        tasks differently, and this callback provides a venue to this.
        In this case, some tasks require the thread_id and/or scheduler
        to perform the task correctly, so these objects are passed down
        to the task in those cases.
        """

        self._rx_lock.acquire()

        if task_id == tasks.REGISTER_PLATFORM:
            current_scheduler = self._messengers[self.thread.ident].scheduler
            task(self.thread.ident, current_scheduler, pkt)
        elif task_id in {tasks.REGISTER_DEVICE, tasks.ALERT_MCU_SETUP_COMPLETION}:
            task(self.thread.ident, pkt)
        else:
            task(pkt)

        self._rx_lock.release()

        setup.define_scheduler_tasks(scheduler)


class SerialMessenger(BaseMessenger, object):
    """Helper object that listens and writes to the MCUs

    This class uses serial channels to communicate with the MCUs that
    are connected to the computer.
    """

    def __init__(self, port, baudrate, task_count, is_little_endian=False, no_internal_set_up=False):

        self.serial_ch = serial.Serial(port=port, baudrate=baudrate)
        super(SerialMessenger, self).__init__(task_count, is_little_endian, no_internal_set_up)

    @classmethod
    def close_channels(cls):
        """Closes all the serial communication channels with the MCUs connected to the computer"""

        for messenger in cls._messengers.items():
            messenger.serial_ch.close()

    def _receive_pkt(self):
        """Listens for incoming bytes from schedulers through a serial channel"""

        while self.serial_ch.in_waiting:
            byte = bytearray(self.serial_ch.read())
            self.scheduler.build_incoming_pkt(byte)

    def _tx_scheduler_cb(self, pkt):
        """Writes the outgoing bytes from a scheduler through a serial channel"""

        bytes_to_send = len(pkt)

        while bytes_to_send:
            bytes_to_send -= self.serial_ch.write(pkt)
            if bytes_to_send < 0:
                break
