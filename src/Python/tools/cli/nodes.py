
from collections import deque


class Node:
    """A "page" or node in the command line interface (cli).

    Node objects register the pages and tests to the interface
    and are in charge of establishing a structure for the cli.
    """

    _tree = {}  # Tree where the nodes reside in
    _entries = deque()  # Nodes that were entered in the interface

    def __init__(self, name, description=None, parent_name=None, callback=None):

        self.name = name  # Name of the current node
        self.children = set()  # Name of the direct descendant nodes
        self.parent = parent_name  # Name of the node this node is registered to
        self.description = description  # Description for the node/page

        # Store the test
        if callback is not None:
            self.callback = callback

        self._link_with_parent()
        self._add_node_to_node_class()

    def __repr__(self):

        return self.name + "\n"

    @property
    def has_callback(self):
        """Check if the node has a callback function"""

        return getattr(self, "callback", None) is not None

    @property
    def path(self):
        """Gets the path from the root node to the current node."""

        if self.parent is None:
            path_to_node = []
        else:
            parent_node = self._tree[self.parent]
            path_to_node = parent_node.path

        path_to_node.append(self.name)
        return path_to_node

    @classmethod
    def get_current_node(cls):
        """Return the current node in the system."""

        return cls._tree[cls._entries[-1]]

    @classmethod
    def get_node(cls, name):

        return cls._tree.get(name)

    @classmethod
    def get_entries(cls):

        return cls._entries

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

    def register_test(self, test):
        """Alternate constructor for a node.

        You can register test node under a device node through this method.

        - For example,
          test_node = device.register_test(test_function)
        """

        return Node(test.__name__, test.__doc__, self.name, test)

    @classmethod
    def reset_tree(cls):
        """Resets the node tree to its initial state"""

        root_name = cls._entries[0]
        cls._entries.clear()
        cls._entries.append(root_name)

    def _add_node_to_node_class(self):
        """Save relevant node attributes to the node class attributes"""

        # Add node to the node storage
        if self._tree.get(self.name) is not None:
            raise KeyError("This name was already registered.")
        self._tree[self.name] = self  # Add node to storage of nodes

        # Save the root node of the interface
        if self.parent is None:
            if len(self._entries) != 0:
                raise KeyError("There can only be one root in the interface."
                               "{} is the root of the interface, ".format(self._entries[0]) +
                               "but the node '" + self.name + "' was being added as a root.")
            self._entries.append(self.name)

    def _link_with_parent(self):
        """Link the current node to its parent if it has one"""

        if self.parent is not None:
            parent_node = self._tree.get(self.parent)
            if parent_node is not None:
                parent_node.children.add(self.name)
