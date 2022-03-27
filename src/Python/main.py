"""Main script for the testbed system

In the initial setup phase, the system will setup everything related to
the scheduling system of the testbed system. First, the system will setup
the serial channels, so the setup tasks can be performed. During this
phase, the system will be dynamically creating the pages/nodes in the
command line interface (cli) by registering these through the MCU. This
phase will not progress until each MCU sends a flag that they finished
with their scheduler setups.

In the final setup phase, the system will start setting up the command
line interface (cli), which is used to fully define the cli and link up
all the unlinked nodes that were in the system after registering them in
the system. This will import one of the platforms in the platform directory
and define all the tests so one can access these in the cli.

The cli will run indefinitely until the users shuts down the program through
one of the commands in the cli. After this is done, the system will close off
all of the communication channels that were created to interact with the MCUs,
and it will delete every node in the cli.
"""

from __future__ import print_function

from config import setup, tasks
from tools import cli, messengers


if __name__ == "__main__":

    # Initial setup phase
    setup.setup_mcu_channels()
    while not tasks.is_mcu_setup_complete():
        pass

    # Final setup phase
    cli.activate()
    setup.define_interface_cmds()
    setup.import_platform(cli.Node.get_root_node().name)
    cli.Node.link_unlinked_nodes()

    # Run cli program
    while cli.is_running():
        print(cli.Node.get_current_node())
        cli.Command.run_command()

    # Teardown phase
    messengers.SerialMessenger.close_channels()
    cli.Node.reset_tree()
