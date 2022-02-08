from __future__ import print_function

import sys

from . import nodes
from . import interface


# Define version specific functions
if sys.version_info[:2] <= (2, 7):  # If Python 2.7
    user_input = raw_input
else:
    user_input = input


# Commands for the interface

def _show_description():
    """Command to show description of a node."""

    current_node = nodes.Node.get_current_node()
    if current_node.description is None:
        print("No description is available for this node.")
    else:
        print(current_node.description)


def _show_commands():
    """Command to show the commands for the layer."""

    cmds_to_display = Command.get_commands()
    for name, command in cmds_to_display.items():
        print("- {}:\n".format(name))
        print("\t{}\n".format(command.__doc__))


def _navigate_down_node_tree():
    """Command to go down a level in the interface."""

    entries = nodes.Node.get_entries()
    current_node = nodes.Node.get_current_node()
    node_count = len(current_node.children)
    if node_count != 0:
        if node_count == 1:
            entries.append(list(current_node.children)[0])  # Go to the only option available
        else:
            _give_node_options(current_node.children, entries)
    else:
        print("There are currently no options to choose from.")


def _give_node_options(child_nodes, entries):

    while True:
        print("You have the following options to choose from:", ', '.join(child_nodes), '\n')
        option = user_input("Choose one of the options or you can go back by typing 'back': ")
        if option in child_nodes:
            entries.append(option)
            break
        elif option == "back":
            break
        else:
            print("The entry '" + option + "' is not in the current options.\n")


def _navigate_up_node_tree():
    """Command to go up a level in the interface or return to the previous entry."""

    nodes.Node.get_entries().pop()


def _run_node_callback():
    """Command to execute a callback function."""

    current_node = nodes.Node.get_current_node()
    if hasattr(current_node, "callback"):
        current_node.callback()


class Command:
    """Define what commands are available for each node in the cli."""

    _cmds = {}  # All the commands in the cli.

    def __init__(self, name, cmd, cond=None):

        self.cmd = cmd
        self.name = name

        self._cmds[name] = self  # Register command in cmd storage

        # Store the condition that displays the command in the cli
        if cond is not None:
            self.cond = cond

    @classmethod
    def run_command(cls):

        cmds_to_display = cls.get_commands()
        while True:
            print("You have the following commands to choose from: ", ', '.join(cmds_to_display))
            cmd_name = user_input("Type one of the commands to proceed: ")
            if cmd_name in cmds_to_display:
                cls._cmds[cmd_name]()  # Run one of the stored commands
                break
            else:
                print("The command '{}' is not in the current options.\n\n".format(cmd_name))

    @classmethod
    def get_commands(cls):
        """Get the commands that will run for the current node"""

        return {cmd_name: cmd for cmd_name, cmd in cls._cmds.items() if not hasattr(cmd, "cond") or cmd.cond()}


# Register the commands that will appear in the cli

Command("run", _run_node_callback, lambda: hasattr(nodes.Node.get_current_node(), "callback"))
Command("end", interface.close)
Command("help", _show_commands)
Command("desc", _show_description)
Command("back", _navigate_up_node_tree, lambda: nodes.Node.get_current_node().parent is not None)
Command("go", _navigate_down_node_tree, lambda: not hasattr(nodes.Node.get_current_node(), "callback"))
