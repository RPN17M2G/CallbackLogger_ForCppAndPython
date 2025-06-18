import pycallbacklogger
from colorama import Fore, Style, init
from inspect import currentframe, getframeinfo

init(autoreset=True)

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

logger = pycallbacklogger.CallbackLogger()

# Register callbacks for different severities and components
logger.register_function_callback(console_callback, pycallbacklogger.Severity.Info)
logger.register_function_callback(warning_callback, pycallbacklogger.Severity.Warning)
logger.register_function_callback(error_callback, {pycallbacklogger.Component.S: pycallbacklogger.Severity.Error})
logger.register_function_callback(debug_callback, {pycallbacklogger.Component.P: pycallbacklogger.Severity.Debug})
logger.register_function_callback(all_callback)

# Register file callbacks
logger.register_file_callback('all_logs.log')
logger.register_file_callback('component_m_warnings.log', {pycallbacklogger.Component.M: pycallbacklogger.Severity.Warning})

# Log various messages
logger.log(pycallbacklogger.Severity.Info, pycallbacklogger.Component.P, "First info message", getframeinfo(currentframe()).filename, getframeinfo(currentframe()).lineno)
logger.log(pycallbacklogger.Severity.Warning, pycallbacklogger.Component.P, "Second warning", getframeinfo(currentframe()).filename, getframeinfo(currentframe()).lineno)
logger.log(pycallbacklogger.Severity.Error, pycallbacklogger.Component.P, "Third error", getframeinfo(currentframe()).filename, getframeinfo(currentframe()).lineno)
logger.log(pycallbacklogger.Severity.Debug, pycallbacklogger.Component.P, "4 debug", getframeinfo(currentframe()).filename, getframeinfo(currentframe()).lineno)

# From cpp
pycallbacklogger.startup_function_that_logs(logger)
pycallbacklogger.function_that_logs(logger)

logger.log(pycallbacklogger.Severity.Info, pycallbacklogger.Component.P, "fifth into", getframeinfo(currentframe()).filename, getframeinfo(currentframe()).lineno)



def all_callback(entry):
    print(f"{Fore.MAGENTA}ALL: {entry.message} in {entry.component} at {entry.file}:{entry.line}{Style.RESET_ALL}")

logger.register_function_callback(all_callback)

logger.log(pycallbacklogger.Severity.Info, pycallbacklogger.Component.P, "fifth into", getframeinfo(currentframe()).filename, getframeinfo(currentframe()).lineno)
