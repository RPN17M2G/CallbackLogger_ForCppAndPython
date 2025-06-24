import pytest

import pycallbacklogger

@pytest.fixture
def logger():
    """
    Pytest fixture to create a CallbackLogger instance.
    """
    logger_instance = pycallbacklogger.CallbackLogger()
    yield logger_instance
    del logger_instance
