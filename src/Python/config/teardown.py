"""Teardown module to unintialize different parts of the platform

This is meant for uninitializing objects that are created
in the platform since these may vary depending on the
situation.
"""

_deinits = []  # List to store different teardown methods


def atexit(deinit_cb):
    """Register a teardown for when the program finishes running"""

    _deinits.append(deinit_cb)


def platform_exit():
    """Run teardowns that were registered for a platform"""

    for deinit in _deinits:
        deinit()
