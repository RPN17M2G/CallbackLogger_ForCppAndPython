import pytest
from enum import Enum
import os
import tempfile

import pycallbacklogger

@pytest.fixture
def logger():
    logger_instance = pycallbacklogger.CallbackLogger()
    yield logger_instance
    del logger_instance

@pytest.fixture
def PyComponent():
    class _PyComponent(Enum):
        S = 0
        M = 1
        P = 2
    return _PyComponent

@pytest.fixture
def temp_log_file():
    fd, path = tempfile.mkstemp(suffix=".log")
    os.close(fd)
    yield path
    try:
        os.remove(path)
    except OSError:
        pass

@pytest.fixture
def log_entry_collector():
    entries = []
    def callback(entry):
        entries.append(entry)
    return callback, entries
