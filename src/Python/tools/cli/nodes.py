from copy import copy
from collections import deque


class CLINodeError(Exception):
    """An error regarding node objects in the cli.

    Must be used indicating that a node was not defined or
    that a node cannot be defined in the cli.
    """


class Node:
    """A "page" or node in the command line interface (cli).

    Node objects register the pages and tests to the interface
    and are in charge of establishing a structure for the cli.
    """

    _nodes = {}  # Tree where the nodes reside in
    _entries = deque()  # Nodes that were entered in the interface
    _unlinked_nodes = set()  # Set of nodes that were not linked to its parent node

    def __init__(self, name, description=None, parent_name=None, callback=None):

        self.name = name  # Name of the current node
        self.children = set()  # Names of the direct descendant nodes
        self.parent = parent_name  # Name of the node this node is registered to
        self.description = description  # Description for the node/page

        # Store the test
        if callback is not None:
            self.callback = callback

        self.link_with_parent()
        self._add_node_to_node_class()

    def __repr__(self):

        return "Current page: " + self.name + "\n"

    @classmethod
    def get_current_node(cls):
        """Return the current node in the cli."""

        return cls._nodes[cls._entries[-1]]

    @classmethod
    def get_root_node(cls):
        """Return the root node in the cli."""

        if len(cls._entries) != 0:
            return cls._nodes.get(cls._entries[0])

    @classmethod
    def get_node(cls, name):
        """Return the node with the given name"""

        return cls._nodes.get(name)

    @classmethod
    def get_entries(cls):
        """Get the entries of the cli"""

        return cls._entries

    def is_leaf(self):
        """Verifies if the current node does not have any children"""

        return len(self.children) == 0

    @classmethod
    def link_unlinked_nodes(cls):
        """Attempt to link the listed unlinked nodes to their respective parents"""

        for node_name in copy(cls._unlinked_nodes):
            node = cls._nodes[node_name]
            if cls._nodes.get(node.parent) is not None:
                node.link_with_parent()
                cls._unlinked_nodes.remove(node.name)

    def link_with_parent(self):
        """Link the current node to its parent if it has one"""

        if self.parent is not None:
            parent_node = self._nodes.get(self.parent)
            if parent_node is None:  # Add name to the list of unlinked nodes if parent node is not present
                self._unlinked_nodes.add(self.name)
            else:  # Otherwise, link to parent node
                parent_node.children.add(self.name)

    def register(self, node_name, description=None):
        """Alternate constructor for a node.

        You can register and create a node under an already registered
        node. It serves as a short hand for creating the new node using
        the node that called this method as the parent's name.

        These are the types of nodes that can be registered through
        this method:

        - Platform: A platform would be one of the AUVs, and one
          uses the root node with this method to register them.

          - For example, this is how you would create a platform node
            under a root node:

            platform = root.register(platform_name, platform_description)

        - Device: A device would be one of the devices attached to AUV
          that goes through the Arduino in some way. One must register
          a device under a platform using this method.

          - For example, this is how you would create a device node under
            a platform node:

            device = platform.register(device_name, device_description)
        """

        return Node(node_name, description, self.name)

    @classmethod
    def reset_tree(cls):
        """Resets the node tree to its initial state"""

        root_name = cls._entries[0]
        cls._entries.clear()
        cls._entries.append(root_name)

    def _add_node_to_node_class(self):
        """Save relevant node attributes to the node class attributes"""

        # Add node to the node storage
        if self._nodes.get(self.name) is not None:
            raise CLINodeError("This name was already registered.")
        self._nodes[self.name] = self  # Add node to node tree

        # Save the root node of the interface
        if self.parent is None:
            if len(self._entries) != 0:
                raise CLINodeError("There can only be one root in the interface."
                                   "{} is the root of the interface, ".format(self._entries[0]) +
                                   "but the node '" + self.name + "' was being added as a root.")
            self._entries.append(self.name)
