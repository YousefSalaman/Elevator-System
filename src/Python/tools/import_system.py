import os
import importlib
from glob import glob

from .cli import nodes


_modules = glob(os.path.join(os.getcwd(), "EmbeddedTestbed", "tests", "*"))  # Files and directories in tests directory


def define_interface_tests():

    platform_node = nodes.Node.get_root_node()


