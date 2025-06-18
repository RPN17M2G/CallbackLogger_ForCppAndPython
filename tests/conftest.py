import pytest

import pycallbacklogger

@pytest.fixture
def logger(request):
    """
    Pytest fixture to create a CallbackLogger instance.
    To specify worker count: @pytest.mark.parametrize('logger', [{worker_count}], indirect=True)
    """
    if hasattr(request, 'param'):
        logger_instance = pycallbacklogger.CallbackLogger(request.param)
    else:
        logger_instance = pycallbacklogger.CallbackLogger()
    yield logger_instance
    del logger_instance
