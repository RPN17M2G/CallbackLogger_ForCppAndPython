import pytest
import pycallbacklogger
import tempfile
import os
from enum import Enum

def test_register_function_callback_info_message_received(logger, PyComponent, log_entry_collector):
    # Arrange
    callback, received_entries = log_entry_collector
    MESSAGE = "test message"
    FILE_NAME = "file.cpp"
    LINE_NUMBER = 42

    logger.register_function_callback(callback, pycallbacklogger.Severity.Debug)

    # Act
    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, MESSAGE, FILE_NAME, LINE_NUMBER)

    # Assert
    assert len(received_entries) == 1
    assert received_entries[0].message == MESSAGE

def test_register_file_callback_info_message_written(logger, PyComponent, temp_log_file):
    # Arrange
    log_file_path = temp_log_file
    MESSAGE = "file log"
    FILE_NAME = "file.cpp"
    LINE_NUMBER = 1

    logger.register_file_callback(log_file_path, pycallbacklogger.Severity.Info)

    # Act
    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, MESSAGE, FILE_NAME, LINE_NUMBER)

    # Assert
    with open(log_file_path, "r") as f:
        content = f.read()
    assert MESSAGE in content

def test_register_function_callback_severity_filter_only_error_received(logger, PyComponent, log_entry_collector):
    # Arrange
    callback, received_entries = log_entry_collector
    MESSAGE_NOT = "should not appear"
    MESSAGE_YES = "should appear"
    FILE_NAME = "f.cpp"
    LINE_1 = 1
    LINE_2 = 2

    logger.register_function_callback(callback, pycallbacklogger.Severity.Warning)

    # Act
    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, MESSAGE_NOT, FILE_NAME, LINE_1)
    logger.log(pycallbacklogger.Severity.Error, PyComponent.S, MESSAGE_YES, FILE_NAME, LINE_2)

    # Assert
    assert len(received_entries) == 1
    assert received_entries[0].message == MESSAGE_YES

def test_register_function_callback_component_filter_only_M_received(logger, PyComponent, log_entry_collector):
    # Arrange
    callback, received_entries = log_entry_collector
    MESSAGE_M = "for M"
    MESSAGE_S = "not for M"
    FILE_NAME = "f.cpp"
    LINE_M = 2
    LINE_S = 1

    logger.register_function_callback(callback, [PyComponent.M])

    # Act
    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, MESSAGE_S, FILE_NAME, LINE_S)
    logger.log(pycallbacklogger.Severity.Info, PyComponent.M, MESSAGE_M, FILE_NAME, LINE_M)

    # Assert
    assert len(received_entries) == 1
    assert received_entries[0].component == PyComponent.M
    assert received_entries[0].message == MESSAGE_M

def test_register_multiple_callbacks_info_message_both_callbacks_receive(logger, PyComponent):
    # Arrange
    received_messages_callback1 = []
    received_messages_callback2 = []
    MESSAGE = "msg"
    FILE_NAME = "f.cpp"
    LINE_NUMBER = 1

    logger.register_function_callback(lambda entry: received_messages_callback1.append(entry.message), pycallbacklogger.Severity.Debug)
    logger.register_function_callback(lambda entry: received_messages_callback2.append(entry.message), pycallbacklogger.Severity.Info)

    # Act
    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, MESSAGE, FILE_NAME, LINE_NUMBER)

    # Assert
    assert received_messages_callback1 == [MESSAGE]
    assert received_messages_callback2 == [MESSAGE]

def test_log_async_processing_all_messages_received_in_order(logger, PyComponent, log_entry_collector):
    # Arrange
    callback, received_messages = log_entry_collector
    FILE_NAME = "f.cpp"
    MESSAGE_PREFIX = "msg"
    MESSAGE_COUNT = 10

    logger.register_function_callback(lambda entry: received_messages.append(entry.message), pycallbacklogger.Severity.Debug)

    # Act
    for i in range(MESSAGE_COUNT):
        logger.log(pycallbacklogger.Severity.Info, PyComponent.S, f"{MESSAGE_PREFIX}{i}", FILE_NAME, i + 1)

    # Assert
    assert len(received_messages) == MESSAGE_COUNT
    assert received_messages[0] == f"{MESSAGE_PREFIX}0"
    assert received_messages[-1] == f"{MESSAGE_PREFIX}{MESSAGE_COUNT-1}"

def test_log_no_callbacks_registered_no_error(logger, PyComponent):
    # Arrange
    MESSAGE = "no cb"
    FILE_NAME = "file.cpp"
    LINE_NUMBER = 1

    # Act
    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, MESSAGE, FILE_NAME, LINE_NUMBER)

    # Assert
    assert True

def test_unregister_callback_callback_not_called(logger, PyComponent, log_entry_collector):
    # Arrange
    callback, received_entries = log_entry_collector
    MESSAGE = "should not be received"
    FILE_NAME = "f.cpp"
    LINE_NUMBER = 1

    callback_handle = logger.register_function_callback(callback, pycallbacklogger.Severity.Debug)
    logger.unregister_function_callback(callback_handle)

    # Act
    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, MESSAGE, FILE_NAME, LINE_NUMBER)

    # Assert
    assert not received_entries

def test_log_empty_message_callback_receives_empty_string(logger, PyComponent, log_entry_collector):
    # Arrange
    callback, received_messages = log_entry_collector
    FILE_NAME = "f.cpp"
    LINE_NUMBER = 1

    logger.register_function_callback(lambda entry: received_messages.append(entry.message), pycallbacklogger.Severity.Debug)

    # Act & Assert
    with pytest.raises(RuntimeError, match="Cannot log an empty message"):
        logger.log(pycallbacklogger.Severity.Info, PyComponent.S, "", FILE_NAME, LINE_NUMBER)
    assert received_messages == []

def test_log_long_message_callback_receives_full_message(logger, PyComponent, log_entry_collector):
    # Arrange
    callback, received_messages = log_entry_collector
    FILE_NAME = "f.cpp"
    LINE_NUMBER = 1
    LONG_MESSAGE = "x" * 10000

    logger.register_function_callback(lambda entry: received_messages.append(entry.message), pycallbacklogger.Severity.Debug)

    # Act
    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, LONG_MESSAGE, FILE_NAME, LINE_NUMBER)

    # Assert
    assert received_messages == [LONG_MESSAGE]

def test_log_special_characters_callback_receives_message(logger, PyComponent, log_entry_collector):
    # Arrange
    callback, received_messages = log_entry_collector
    FILE_NAME = "f.cpp"
    LINE_NUMBER = 1
    SPECIAL_MESSAGE = "特殊字符!@#$%^&*()_+"

    logger.register_function_callback(lambda entry: received_messages.append(entry.message), pycallbacklogger.Severity.Debug)

    # Act
    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, SPECIAL_MESSAGE, FILE_NAME, LINE_NUMBER)

    # Assert
    assert received_messages == [SPECIAL_MESSAGE]

def test_log_all_severities_and_components_all_combinations_received(logger, PyComponent, log_entry_collector):
    # Arrange
    callback, received_pairs = log_entry_collector
    SEVERITIES = [
        pycallbacklogger.Severity.Debug,
        pycallbacklogger.Severity.Info,
        pycallbacklogger.Severity.Warning,
        pycallbacklogger.Severity.Error,
        pycallbacklogger.Severity.Fatal,
    ]
    COMPONENTS = [
        PyComponent.S,
        PyComponent.M,
        PyComponent.P,
    ]
    MESSAGE = "msg"
    FILE_NAME = "f.cpp"
    LINE = 1

    logger.register_function_callback(lambda entry: received_pairs.append((entry.severity, entry.component)), pycallbacklogger.Severity.Debug)

    # Act
    for severity in SEVERITIES:
        for component in COMPONENTS:
            logger.log(severity, component, MESSAGE, FILE_NAME, LINE)

    # Assert
    received_set = set((s.value, c.value) for s, c in received_pairs)
    for severity in SEVERITIES:
        for component in COMPONENTS:
            assert (severity.value, component.value) in received_set

def test_log_after_logger_deleted_no_crash(logger, PyComponent, log_entry_collector):
    # Arrange
    callback, received_messages = log_entry_collector
    FILE_NAME = "f.cpp"
    LINE_NUMBER = 1
    MESSAGE = "before del"

    logger.register_function_callback(callback, pycallbacklogger.Severity.Debug)
    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, MESSAGE, FILE_NAME, LINE_NUMBER)
    del logger
    assert True

def test_register_function_callback_callback_raises_exception_callback_called(logger, PyComponent):
    # Arrange
    callback_called = []
    MESSAGE = "test"
    FILE_NAME = "f.cpp"
    LINE_NUMBER = 1

    def bad_callback(entry):
        callback_called.append(True)
        raise RuntimeError("fail")

    logger.register_function_callback(bad_callback, pycallbacklogger.Severity.Debug)

    # Act
    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, MESSAGE, FILE_NAME, LINE_NUMBER)

    # Assert
    assert callback_called

def test_register_function_callback_severity_filter_edge_cases_only_warning_and_error_received(logger, PyComponent, log_entry_collector):
    # Arrange
    callback, received_severities = log_entry_collector
    FILE_NAME = "f.cpp"
    MESSAGE_DEBUG = "debug"
    MESSAGE_INFO = "info"
    MESSAGE_WARN = "warn"
    MESSAGE_ERROR = "error"

    logger.register_function_callback(lambda entry: received_severities.append(entry.severity), pycallbacklogger.Severity.Warning)

    # Act
    logger.log(pycallbacklogger.Severity.Debug, PyComponent.S, MESSAGE_DEBUG, FILE_NAME, 1)
    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, MESSAGE_INFO, FILE_NAME, 2)
    logger.log(pycallbacklogger.Severity.Warning, PyComponent.S, MESSAGE_WARN, FILE_NAME, 3)
    logger.log(pycallbacklogger.Severity.Error, PyComponent.S, MESSAGE_ERROR, FILE_NAME, 4)

    # Assert
    assert pycallbacklogger.Severity.Warning in received_severities
    assert pycallbacklogger.Severity.Error in received_severities
    assert pycallbacklogger.Severity.Debug not in received_severities
    assert pycallbacklogger.Severity.Info not in received_severities

def test_register_function_callback_component_filter_multiple_only_selected_received(logger, PyComponent, log_entry_collector):
    # Arrange
    callback, received_components = log_entry_collector
    MESSAGE_S = "S"
    MESSAGE_M = "M"
    MESSAGE_P = "P"
    FILE_NAME = "f.cpp"
    LINE_S = 1
    LINE_M = 2
    LINE_P = 3

    logger.register_function_callback(lambda entry: received_components.append(entry.component), {PyComponent.S})
    logger.register_function_callback(lambda entry: received_components.append(entry.component), {PyComponent.M})

    # Act
    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, MESSAGE_S, FILE_NAME, LINE_S)
    logger.log(pycallbacklogger.Severity.Info, PyComponent.M, MESSAGE_M, FILE_NAME, LINE_M)
    logger.log(pycallbacklogger.Severity.Info, PyComponent.P, MESSAGE_P, FILE_NAME, LINE_P)

    # Assert
    received_values = set(comp.value for comp in received_components)
    assert PyComponent.S.value in received_values
    assert PyComponent.M.value in received_values
    assert PyComponent.P.value not in received_values

def test_register_function_callback_component_filter_empty_set_nothing_received(logger, PyComponent, log_entry_collector):
    # Arrange
    callback, received_components = log_entry_collector
    MESSAGE = "S"
    FILE_NAME = "f.cpp"
    LINE_NUMBER = 1

    # No registration for any component

    # Act
    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, MESSAGE, FILE_NAME, LINE_NUMBER)

    # Assert
    assert not received_components

def test_register_function_callback_combined_severity_component_filter_only_error_M_received(logger, PyComponent, log_entry_collector):
    # Arrange
    callback, received_pairs = log_entry_collector
    MESSAGE_APPEAR = "should appear"
    MESSAGE_NOT_APPEAR = "should not appear"
    FILE_NAME = "f.cpp"
    LINE_1 = 1
    LINE_2 = 2
    LINE_3 = 3

    logger.register_function_callback(callback, pycallbacklogger.Severity.Error)
    logger.register_function_callback(callback, [PyComponent.M])

    # Act
    logger.log(pycallbacklogger.Severity.Error, PyComponent.M, MESSAGE_APPEAR, FILE_NAME, LINE_1)
    logger.log(pycallbacklogger.Severity.Error, PyComponent.S, MESSAGE_NOT_APPEAR, FILE_NAME, LINE_2)
    logger.log(pycallbacklogger.Severity.Info, PyComponent.M, MESSAGE_NOT_APPEAR, FILE_NAME, LINE_3)

    # Assert
    assert any(
        entry.severity.value == pycallbacklogger.Severity.Error.value and entry.component.value == PyComponent.M.value and entry.message == MESSAGE_APPEAR
        for entry in received_pairs
    )

def test_register_function_callback_same_callback_multiple_times_unregister_one_only_one_receives(logger, PyComponent, log_entry_collector):
    # Arrange
    callback, received_messages = log_entry_collector
    MESSAGE_1 = "msg"
    MESSAGE_2 = "msg2"
    FILE_NAME = "f.cpp"
    LINE_1 = 1
    LINE_2 = 2

    handle1 = logger.register_function_callback(callback, pycallbacklogger.Severity.Debug)
    handle2 = logger.register_function_callback(callback, pycallbacklogger.Severity.Debug)

    # Act
    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, MESSAGE_1, FILE_NAME, LINE_1)
    logger.unregister_function_callback(handle1)
    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, MESSAGE_2, FILE_NAME, LINE_2)

    # Assert
    assert [entry.message for entry in received_messages].count(MESSAGE_1) == 2
    assert [entry.message for entry in received_messages].count(MESSAGE_2) == 1

def test_unregister_callback_twice_no_error(logger, PyComponent, log_entry_collector):
    # Arrange
    callback, received_messages = log_entry_collector
    MESSAGE = "msg"
    FILE_NAME = "f.cpp"
    LINE_NUMBER = 1

    handle = logger.register_function_callback(callback, pycallbacklogger.Severity.Debug)
    logger.unregister_function_callback(handle)

    # Act & Assert
    with pytest.raises(RuntimeError) as error:
        logger.unregister_function_callback(handle)
    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, MESSAGE, FILE_NAME, LINE_NUMBER)
    assert not received_messages
    assert "Callback handle not found" in str(error.value)

def test_register_function_callback_severity_as_list_only_selected_received(logger, PyComponent, log_entry_collector):
    # Arrange
    callback, received_messages = log_entry_collector
    MESSAGE_INFO = "info"
    MESSAGE_ERROR = "error"
    MESSAGE_DEBUG = "debug"
    FILE_NAME = "f.cpp"

    logger.register_function_callback(lambda entry: received_messages.append(entry.message), pycallbacklogger.Severity.Info)
    logger.register_function_callback(lambda entry: received_messages.append(entry.message), pycallbacklogger.Severity.Error)

    # Act
    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, MESSAGE_INFO, FILE_NAME, 1)
    logger.log(pycallbacklogger.Severity.Error, PyComponent.S, MESSAGE_ERROR, FILE_NAME, 2)
    logger.log(pycallbacklogger.Severity.Debug, PyComponent.S, MESSAGE_DEBUG, FILE_NAME, 3)

    # Assert
    assert MESSAGE_INFO in received_messages and MESSAGE_ERROR in received_messages and MESSAGE_DEBUG not in received_messages

def test_register_function_callback_component_as_list_only_selected_received(logger, PyComponent, log_entry_collector):
    # Arrange
    callback, received_messages = log_entry_collector
    MESSAGE_S = "S"
    MESSAGE_M = "M"
    MESSAGE_P = "P"
    FILE_NAME = "f.cpp"

    logger.register_function_callback(lambda entry: received_messages.append(entry.message), PyComponent.S)
    logger.register_function_callback(lambda entry: received_messages.append(entry.message), PyComponent.M)

    # Act
    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, MESSAGE_S, FILE_NAME, 1)
    logger.log(pycallbacklogger.Severity.Info, PyComponent.M, MESSAGE_M, FILE_NAME, 2)
    logger.log(pycallbacklogger.Severity.Info, PyComponent.P, MESSAGE_P, FILE_NAME, 3)

    # Assert
    assert MESSAGE_S in received_messages and MESSAGE_M in received_messages and MESSAGE_P not in received_messages

def test_register_function_callback_with_none_filter(logger, PyComponent, log_entry_collector):
    # Arrange
    callback, received = log_entry_collector
    MESSAGE = "should appear"
    FILE_NAME = "f.cpp"
    LINE_NUMBER = 1

    logger.register_function_callback(callback, None)

    # Act
    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, MESSAGE, FILE_NAME, LINE_NUMBER)

    # Assert
    assert received

class FakeComponent(Enum):
    X = 99
def test_register_function_callback_with_invalid_enum_value(logger, PyComponent, log_entry_collector):
    # Arrange
    callback, received = log_entry_collector
    MESSAGE = "should not appear"
    FILE_NAME = "f.cpp"
    LINE_NUMBER = 1

    logger.register_function_callback(callback, FakeComponent.X)

    # Act
    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, MESSAGE, FILE_NAME, LINE_NUMBER)

    # Assert
    assert not received

def test_register_function_callback_with_empty_message_and_none(logger, PyComponent, log_entry_collector):
    # Arrange
    callback, received = log_entry_collector
    FILE_NAME = "f.cpp"
    LINE_1 = 1
    LINE_2 = 2

    logger.register_function_callback(lambda entry: received.append(entry.message), pycallbacklogger.Severity.Debug)

    # Act & Assert
    with pytest.raises(RuntimeError, match="Cannot log an empty message"):
        logger.log(pycallbacklogger.Severity.Info, PyComponent.S, "", FILE_NAME, LINE_1)
    with pytest.raises(TypeError):
        logger.log(pycallbacklogger.Severity.Info, PyComponent.S, None, FILE_NAME, LINE_2)
    assert not received

def test_register_function_callback_with_multiple_filters(logger, PyComponent, log_entry_collector):
    # Arrange
    callback, received = log_entry_collector
    MESSAGE = "should appear"
    FILE_NAME = "f.cpp"
    LINE_NUMBER = 1

    logger.register_function_callback(lambda entry: received.append(entry.message), pycallbacklogger.Severity.Info)
    logger.register_function_callback(lambda entry: received.append(entry.message), {PyComponent.S})

    # Act
    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, MESSAGE, FILE_NAME, LINE_NUMBER)

    # Assert
    assert received.count(MESSAGE) == 2

class BigComponent(Enum):
    A = 0
    B = 1
    C = 2
    D = 3
    E = 4
    F = 5
    G = 6
    H = 7
    I = 8
    J = 9
def test_register_function_callback_with_large_component_enum(logger, log_entry_collector):
    # Arrange
    callback, received = log_entry_collector
    FILE_NAME = "f.cpp"
    MESSAGE_TEMPLATE = "msg-{}"
    LINE = 1

    for comp in BigComponent:
        logger.register_function_callback(lambda entry, c=comp: received.append((c.value, entry.component.value)), [comp])

    # Act
    for comp in BigComponent:
        logger.log(pycallbacklogger.Severity.Info, comp, MESSAGE_TEMPLATE.format(comp), FILE_NAME, LINE)

    # Assert
    assert all((comp.value, comp.value) in received for comp in BigComponent)

def test_register_file_callback_with_large_number_of_files(logger, PyComponent):
    # Arrange
    FILE_NAME = "f.cpp"
    FILE_COUNT = 10
    files = []

    try:
        for i in range(FILE_COUNT):
            with tempfile.NamedTemporaryFile(delete=False) as tmpfile:
                files.append(tmpfile.name)
                logger.register_file_callback(tmpfile.name, pycallbacklogger.Severity.Info)
        for i, file_path in enumerate(files):
            logger.log(pycallbacklogger.Severity.Info, PyComponent.S, f"filemsg{i}", FILE_NAME, i + 1)
        for i, file_path in enumerate(files):
            with open(file_path, "r") as f:
                content = f.read()
            assert f"filemsg{i}" in content
    finally:
        for file_path in files:
            try:
                os.remove(file_path)
            except Exception:
                pass

def test_register_function_callback_with_large_filter_map_and_many_logs(logger, PyComponent, log_entry_collector):
    # Arrange
    callback, received = log_entry_collector
    FILE_NAME = "f.cpp"
    LOG_COUNT = 100
    filter_map = {PyComponent(i): pycallbacklogger.Severity.Info for i in range(3)}

    logger.register_function_callback(callback, filter_map)

    # Act
    for i in range(LOG_COUNT):
        logger.log(pycallbacklogger.Severity.Info, PyComponent.S, f"msg{i}", FILE_NAME, i + 1)

    # Assert
    assert any(entry.message == "msg0" for entry in received)

def test_log_empty_message_throws(logger, PyComponent):
    # Arrange
    COMPONENT_S = PyComponent.S
    FILE_NAME = "file.cpp"
    LINE_NUMBER = 1

    # Act & Assert
    with pytest.raises(RuntimeError, match="Cannot log an empty message"):
        logger.log(pycallbacklogger.Severity.Info, COMPONENT_S, "", FILE_NAME, LINE_NUMBER)
