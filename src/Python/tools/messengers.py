
import serial
import threading

from . import scheduler
from ..config import config, tasks


class SerialMessenger:
    """Helper object that listens and writes to the MCUs

    It establishes a connection between the main computer and the MCUs
    connected to it. There's a thead and scheduler for each messenger
    object that is created, where each thread runs the listening
    portion of the scheduler, so information can be processed in
    parallel for multiple MCUs. One can write into a specific MCU by
    accessing a messenger's scheduler and scheduling a task through it.
    """

    _messengers = {}
    _main_scheduler = None
    _rx_lock = threading.Lock()

    def __init__(self, port, baudrate, task_count, is_little_endian=False, no_internal_set_up=False):

        self.serial_ch = serial.Serial(port=port, baudrate=baudrate)
        self._set_scheduler(task_count, is_little_endian, no_internal_set_up)

        self.thread = threading.Thread(target=self._receive_serial_pkt)
        self._messengers[self.thread.ident] = self

    @classmethod
    def close_channels(cls):
        """Closes all the serial communication channels with the MCUs connected to the computer"""

        for messenger in cls._messengers.items():
            messenger.serial_ch.close()

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

    def _receive_serial_pkt(self):
        """Listens for incoming bytes from the schedulers"""

        while self.serial_ch.in_waiting:
            byte = bytearray(self.serial_ch.read())
            self.scheduler.build_incoming_pkt(byte)

    def _serial_rx_cb(self, task_id, task, pkt):

        self._rx_lock.acquire()

        if task_id == tasks.REGISTER_PLATFORM:
            current_scheduler = self._messengers[self.thread.ident].scheduler
            task(self.thread.ident, current_scheduler, pkt)
        elif task_id in {tasks.REGISTER_DEVICE, tasks.UPDATE_DEVICE_ATTR}:
            task(self.thread.ident, pkt)
        else:
            task(pkt)

        self._rx_lock.release()

    def _serial_tx_cb(self, pkt):

        bytes_to_send = len(pkt)

        while bytes_to_send:
            bytes_to_send -= self.serial_ch.write(pkt)
            if bytes_to_send < 0:
                break

    def _set_scheduler(self, task_count, is_little_endian, no_internal_set_up):

        # Create a scheduler instance
        if self._main_scheduler is None:
            self.scheduler = scheduler.Scheduler(task_count, is_little_endian, no_internal_set_up)
            self._main_scheduler = self.scheduler
        else:
            self.scheduler = self._main_scheduler.copy()

        # Set up callbacks for the scheduler
        self.scheduler.tx_callback = self._serial_tx_cb
        self.scheduler.rx_callback = self._serial_rx_cb

        config.define_scheduler_tasks(scheduler)
