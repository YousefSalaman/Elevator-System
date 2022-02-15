
from . import nodes


# Command line interface attributes

_run_program = True


def close():
    """Close the interface."""

    global _run_program

    _run_program = False


def reset():
    """Reset the interface"""

    global _run_program

    _run_program = True

    nodes.Node.reset_tree()
    # task_scheduler.close()
