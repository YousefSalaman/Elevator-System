from __future__ import print_function

import sys


# Define version specific functions
if sys.version_info[:2] <= (2, 7):  # If Python 2.7
    user_input = raw_input
else:
    user_input = input


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
                cls._cmds[cmd_name].cmd()  # Run one of the stored commands
                break
            else:
                print("The command '{}' is not in the current options.\n\n".format(cmd_name))

    @classmethod
    def get_commands(cls):
        """Get the commands that will run for the current node"""

        return {cmd_name: cmd for cmd_name, cmd in cls._cmds.items() if not hasattr(cmd, "cond") or cmd.cond()}
