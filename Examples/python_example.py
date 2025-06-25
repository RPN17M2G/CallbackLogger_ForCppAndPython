import pycallbacklogger
from colorama import Fore, Style, init
from inspect import currentframe, getframeinfo
from enum import Enum

init(autoreset=True)

# Define a Python enum to use with the logger
class PyComponent(Enum):
    S = 0
    M = 1
    P = 2

def console_callback(entry):
    print(f"[{entry.timestamp}] [{(entry.severity)}] {entry.component}: {entry.message} ({entry.file}:{entry.line})")

def warning_callback(entry):
    print(f"{Fore.YELLOW}WARNING: {entry.message} in {entry.component}{Style.RESET_ALL}")

def error_callback(entry):
    print(f"{Fore.RED}ERROR: {entry.message} ({entry.file}:{entry.line}){Style.RESET_ALL}")

def debug_callback(entry):
    print(f"{Fore.CYAN}DEBUG: {entry.message}{Style.RESET_ALL}")

def all_callback(entry):
    print(f"{Fore.MAGENTA}ALL: {entry.message} in {entry.component} at {entry.file}:{entry.line}{Style.RESET_ALL}")

if __name__ == "__main__":
    logger = pycallbacklogger.CallbackLogger()

    # Register callbacks for different severities and components using the Python enum
    logger.register_function_callback(console_callback, pycallbacklogger.Severity.Info)
    logger.register_function_callback(warning_callback, pycallbacklogger.Severity.Warning)
    logger.register_function_callback(error_callback, {PyComponent.S: pycallbacklogger.Severity.Error})
    logger.register_function_callback(debug_callback, {PyComponent.P: pycallbacklogger.Severity.Debug})
    logger.register_function_callback(all_callback)

    # Register file callbacks using the Python enum
    logger.register_file_callback('all_logs.log')
    logger.register_file_callback('component_m_warnings.log', {PyComponent.M: pycallbacklogger.Severity.Warning})

    # Log various messages using the Python enum
    logger.log(pycallbacklogger.Severity.Info, PyComponent.P, "First info message", getframeinfo(currentframe()).filename, getframeinfo(currentframe()).lineno)
    logger.log(pycallbacklogger.Severity.Warning, PyComponent.P, "Second warning", getframeinfo(currentframe()).filename, getframeinfo(currentframe()).lineno)
    logger.log(pycallbacklogger.Severity.Error, PyComponent.P, "Third error", getframeinfo(currentframe()).filename, getframeinfo(currentframe()).lineno)
    logger.log(pycallbacklogger.Severity.Debug, PyComponent.P, "4 debug", getframeinfo(currentframe()).filename, getframeinfo(currentframe()).lineno)

    logger.log(pycallbacklogger.Severity.Info, PyComponent.P, "fifth info", getframeinfo(currentframe()).filename, getframeinfo(currentframe()).lineno)

    logger.register_function_callback(all_callback)

    logger.log(pycallbacklogger.Severity.Info, PyComponent.P, "sixth info", getframeinfo(currentframe()).filename, getframeinfo(currentframe()).lineno)

