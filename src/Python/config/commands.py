from __future__ import print_function

import sys

from ..tools import cli


# Define version specific functions
if sys.version_info[:2] <= (2, 7):  # If Python 2.7
    user_input = raw_input
else:
    user_input = input


# Commands for the interface

def _show_description():
    """Command to show description of a node."""

    current_node = cli.Node.get_current_node()
    if current_node.description is None:
        print("No description is available for this node.")
    else:
        print(current_node.description)


def _show_commands():
    """Command to show the commands for the layer"""

    cmds_to_display = cli.Command.get_commands()
    for name, command in cmds_to_display.items():
        print("- {}:\n".format(name))
        print("\t{}\n".format(command.__doc__))


def _navigate_down_node_tree():
    """Command to go down a level in the interface"""

    entries = cli.Node.get_entries()
    current_node = cli.Node.get_current_node()
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
        print("You have the following options to choose from: ", ', '.join(child_nodes), '\n')
        option = user_input("Choose one of the options or you can go back by typing 'back': ")
        if option in child_nodes:
            entries.append(option)
            break
        elif option == "back":
            break
        else:
            print("The entry '" + option + "' is not in the current options.\n")


def _navigate_up_node_tree():
    """Command to go up a level in the interface or return to the previous entry"""

    cli.Node.get_entries().pop()


def _run_node_callback():
    """Command to execute a callback function"""

    current_node = cli.Node.get_current_node()
    if hasattr(current_node, "callback"):
        current_node.callback()
