"""This package has the behaves like programmable command line interface (cli)

More specifically, the whole package, including its classes and variables,
act as a single entity (i.e., the cli). If you want to add a command to the
cli, you can add one through the Command class. Similarly, if you want to add
a page in the interface, you can add one by creating a Node instance.
"""

from .commands import Command
from .nodes import Node, CLINodeError


# Command line interface attributes

_run_program = True


# Command line interface methods

def activate():
    """Turn on the interface"""

    global _run_program

    _run_program = True


def deactivate():
    """Close the interface"""

    global _run_program

    _run_program = False


def is_running():

    return _run_program
