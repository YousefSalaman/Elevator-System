
from . import scheduler


class Tester:
    """A shorthand callable version of a task in the scheduler

    This is meant to simplify the interaction with the internal
    scheduling system that the testbed uses to communicate with
    the MCUs connected with the computer.

    One can store a tester object in the class by providing the
    class its name through the "add_tester" method. Afterwards,
    one can retrieve this object by providing the class the
    name that was used to store it through the "get_tester".
    Now, one can just create the object and use it directly,
    but by using the methods mentioned before one can indirectly
    reference a tester object without it being created. This
    feature is used by the testbed system to define the testers
    from the MCU.
    """

    _testers = {}

    def __init__(self, task_id, is_priority=False):

        self.task_id = task_id
        self.is_priority = is_priority

    def __call__(self, device, pkt):

        thread_id = device.thread_id
        current_scheduler = scheduler.Scheduler.get_scheduler(thread_id)
        if self.is_priority:
            current_scheduler.schedule_priority_task(self.task_id, pkt)
        else:
            current_scheduler.schedule_normal_task(self.task_id, pkt)

    @classmethod
    def add_tester(cls, task_name, task_id, priority_type):

        cls._testers[task_name] = cls(task_id, priority_type)

    @classmethod
    def get_tester(cls, task_name):

        return cls._testers.get(task_name)
