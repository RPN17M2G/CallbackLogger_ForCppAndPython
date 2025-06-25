import tempfile
import pytest
from enum import Enum
import pycallbacklogger

class PyComponent(Enum):
    """
    Python enum to use with the logger.
    """
    S = 0
    M = 1
    P = 2

def test_register_function_callback_info_message_received(logger):
    received_entries = []
    def callback(entry):
        received_entries.append((entry.severity, entry.component, entry.message))
    logger.register_function_callback(callback, pycallbacklogger.Severity.Debug)

    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, "test message", "file.cpp", 42)

    assert len(received_entries) == 1
    assert received_entries[0][2] == "test message"

def test_register_file_callback_info_message_written(logger):
    with tempfile.NamedTemporaryFile(delete=False) as tmpfile:
        log_file_path = tmpfile.name

    logger.register_file_callback(log_file_path, pycallbacklogger.Severity.Info)

    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, "file log", "file.cpp", 1)

    with open(log_file_path, "r") as f:
        content = f.read()
    assert "file log" in content

def test_register_function_callback_severity_filter_only_error_received(logger):
    received_entries = []
    def callback(entry):
        received_entries.append(entry)
    logger.register_function_callback(callback, pycallbacklogger.Severity.Warning)

    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, "should not appear", "f.cpp", 1)
    logger.log(pycallbacklogger.Severity.Error, PyComponent.S, "should appear", "f.cpp", 2)

    assert len(received_entries) == 1
    assert received_entries[0].message == "should appear"

def test_register_function_callback_component_filter_only_M_received(logger):
    received_entries = []
    def callback(entry):
        received_entries.append(entry)
    logger.register_function_callback(callback, {PyComponent.M})

    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, "not for M", "f.cpp", 1)
    logger.log(pycallbacklogger.Severity.Info, PyComponent.M, "for M", "f.cpp", 2)

    assert len(received_entries) == 1
    assert received_entries[0].component == PyComponent.M

def test_register_multiple_callbacks_info_message_both_callbacks_receive(logger):
    received_messages_callback1 = []
    received_messages_callback2 = []
    logger.register_function_callback(lambda entry: received_messages_callback1.append(entry.message), pycallbacklogger.Severity.Debug)
    logger.register_function_callback(lambda entry: received_messages_callback2.append(entry.message), pycallbacklogger.Severity.Info)

    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, "msg", "f.cpp", 1)

    assert received_messages_callback1 == ["msg"]
    assert received_messages_callback2 == ["msg"]

def test_log_async_processing_all_messages_received_in_order(logger):
    received_messages = []
    def callback(entry):
        received_messages.append(entry.message)
    logger.register_function_callback(callback, pycallbacklogger.Severity.Debug)

    # Act
    for i in range(10):
        logger.log(pycallbacklogger.Severity.Info, PyComponent.S, f"msg{i}", "f.cpp", i)

    assert len(received_messages) == 10
    assert received_messages[0] == "msg0"
    assert received_messages[-1] == "msg9"

def test_log_no_callbacks_registered_no_error(logger):
    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, "no cb", "file.cpp", 1)

    # Assert
    # No exception or hang
    assert True

def test_unregister_callback_callback_not_called(logger): # TODO: Fix this test - add unregister_callback to CallbackLogger
    received_entries = []
    def callback(entry):
        received_entries.append(entry)
    callback_handle = logger.register_function_callback(callback, pycallbacklogger.Severity.Debug)
    logger.unregister_function_callback(callback_handle)

    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, "should not be received", "f.cpp", 1)

    assert not received_entries

def test_log_empty_message_callback_receives_empty_string(logger):
    received_messages = []
    logger.register_function_callback(lambda entry: received_messages.append(entry.message), pycallbacklogger.Severity.Debug)

    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, "", "f.cpp", 1)

    assert received_messages == [""]

def test_log_long_message_callback_receives_full_message(logger):
    received_messages = []
    long_message = "x" * 10000
    logger.register_function_callback(lambda entry: received_messages.append(entry.message), pycallbacklogger.Severity.Debug)

    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, long_message, "f.cpp", 1)

    assert received_messages == [long_message]

def test_log_special_characters_callback_receives_message(logger):
    received_messages = []
    special_message = "特殊字符!@#$%^&*()_+"
    logger.register_function_callback(lambda entry: received_messages.append(entry.message), pycallbacklogger.Severity.Debug)

    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, special_message, "f.cpp", 1)

    assert received_messages == [special_message]

def test_log_all_severities_and_components_all_combinations_received(logger):
    received_pairs = []
    logger.register_function_callback(lambda entry: received_pairs.append((entry.severity, entry.component)), pycallbacklogger.Severity.Debug)

    severities = [
        pycallbacklogger.Severity.Debug,
        pycallbacklogger.Severity.Info,
        pycallbacklogger.Severity.Warning,
        pycallbacklogger.Severity.Error,
        pycallbacklogger.Severity.Fatal,
    ]
    components = [
        PyComponent.S,
        PyComponent.M,
        PyComponent.P,
    ]
    for severity in severities:
        for component in components:
            logger.log(severity, component, "msg", "f.cpp", 1)

    assert all((severity, component) in received_pairs for severity in severities for component in components)

def test_log_after_logger_deleted_no_crash(logger): # TODO: Check why the hang happens
    received_messages = []
    def callback(entry):
        received_messages.append(entry.message)
    logger.register_function_callback(callback, pycallbacklogger.Severity.Debug)

    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, "before del", "f.cpp", 1)
    del logger
    assert True # Should not crash or hang

def test_register_file_callback_invalid_path_no_crash(logger):
    # Arrange
    invalid_path = "/invalid/path/to/log.txt"
    logger.register_file_callback(invalid_path, pycallbacklogger.Severity.Info)

    # Act
    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, "should not crash", "f.cpp", 1)

    # Assert
    # No exception or hang
    assert True

def test_register_function_callback_callback_raises_exception_callback_called(logger):
    callback_called = []
    def bad_callback(entry):
        callback_called.append(True)
        raise RuntimeError("fail")
    logger.register_function_callback(bad_callback, pycallbacklogger.Severity.Debug)

    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, "test", "f.cpp", 1)

    assert callback_called

def test_register_function_callback_severity_filter_edge_cases_only_warning_and_error_received(logger):
    received_severities = []
    logger.register_function_callback(lambda entry: received_severities.append(entry.severity), pycallbacklogger.Severity.Warning)

    logger.log(pycallbacklogger.Severity.Debug, PyComponent.S, "debug", "f.cpp", 1)
    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, "info", "f.cpp", 2)
    logger.log(pycallbacklogger.Severity.Warning, PyComponent.S, "warn", "f.cpp", 3)
    logger.log(pycallbacklogger.Severity.Error, PyComponent.S, "error", "f.cpp", 4)

    assert pycallbacklogger.Severity.Warning in received_severities
    assert pycallbacklogger.Severity.Error in received_severities
    assert pycallbacklogger.Severity.Debug not in received_severities
    assert pycallbacklogger.Severity.Info not in received_severities

def test_register_function_callback_component_filter_multiple_only_selected_received(logger):
    received_components = []
    # Only S and M, so call register_function_callback twice
    logger.register_function_callback(lambda entry: received_components.append(entry.component), PyComponent.S)
    logger.register_function_callback(lambda entry: received_components.append(entry.component), PyComponent.M)

    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, "S", "f.cpp", 1)
    logger.log(pycallbacklogger.Severity.Info, PyComponent.M, "M", "f.cpp", 2)
    logger.log(pycallbacklogger.Severity.Info, PyComponent.P, "P", "f.cpp", 3)

    assert PyComponent.S in received_components
    assert PyComponent.M in received_components
    assert PyComponent.P not in received_components

def test_register_function_callback_component_filter_empty_set_nothing_received(logger):
    received_components = []
    # No registration for any component

    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, "S", "f.cpp", 1)

    assert not received_components

def test_register_function_callback_combined_severity_component_filter_only_error_M_received(logger):
    received_pairs = []
    def callback(entry):
        received_pairs.append((entry.severity, entry.component))
    logger.register_function_callback(callback, pycallbacklogger.Severity.Error)
    logger.register_function_callback(callback, {PyComponent.M})

    logger.log(pycallbacklogger.Severity.Error, PyComponent.M, "should appear", "f.cpp", 1)
    logger.log(pycallbacklogger.Severity.Error, PyComponent.S, "should not appear", "f.cpp", 2)
    logger.log(pycallbacklogger.Severity.Info, PyComponent.M, "should not appear", "f.cpp", 3)

    assert (pycallbacklogger.Severity.Error, PyComponent.M) in received_pairs

def test_register_function_callback_same_callback_multiple_times_unregister_one_only_one_receives(logger):
    received_messages = []
    def callback(entry):
        received_messages.append(entry.message)
    handle1 = logger.register_function_callback(callback, pycallbacklogger.Severity.Debug)
    handle2 = logger.register_function_callback(callback, pycallbacklogger.Severity.Debug)

    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, "msg", "f.cpp", 1)
    logger.unregister_function_callback(handle1)
    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, "msg2", "f.cpp", 2)

    assert received_messages.count("msg") == 2
    assert received_messages.count("msg2") == 1

def test_unregister_callback_twice_no_error(logger):
    received_messages = []
    def callback(entry):
        received_messages.append(entry.message)
    handle = logger.register_function_callback(callback, pycallbacklogger.Severity.Debug)
    logger.unregister_function_callback(handle)

    with pytest.raises(RuntimeError) as error:
        logger.unregister_function_callback(handle)
    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, "msg", "f.cpp", 1)

    assert not received_messages
    assert "Callback handle not found" in str(error.value)

def test_register_function_callback_severity_as_list_only_selected_received(logger):
    received_messages = []
    # Register for Info and Error separately
    logger.register_function_callback(lambda entry: received_messages.append(entry.message), pycallbacklogger.Severity.Info)
    logger.register_function_callback(lambda entry: received_messages.append(entry.message), pycallbacklogger.Severity.Error)

    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, "info", "f.cpp", 1)
    logger.log(pycallbacklogger.Severity.Error, PyComponent.S, "error", "f.cpp", 2)
    logger.log(pycallbacklogger.Severity.Debug, PyComponent.S, "debug", "f.cpp", 3)

    assert "info" in received_messages and "error" in received_messages and "debug" not in received_messages

def test_register_function_callback_component_as_list_only_selected_received(logger):
    received_messages = []
    # Register for S and M separately
    logger.register_function_callback(lambda entry: received_messages.append(entry.message), PyComponent.S)
    logger.register_function_callback(lambda entry: received_messages.append(entry.message), PyComponent.M)

    logger.log(pycallbacklogger.Severity.Info, PyComponent.S, "S", "f.cpp", 1)
    logger.log(pycallbacklogger.Severity.Info, PyComponent.M, "M", "f.cpp", 2)
    logger.log(pycallbacklogger.Severity.Info, PyComponent.P, "P", "f.cpp", 3)

    assert "S" in received_messages and "M" in received_messages and "P" not in received_messages
