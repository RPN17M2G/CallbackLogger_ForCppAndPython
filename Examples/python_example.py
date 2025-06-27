import pycallbacklogger
from colorama import Fore, Style, init
from enum import Enum
from inspect import currentframe, getframeinfo

# Initialize colorama for colored output in the terminal
init(autoreset=True)

# Define a custom component enum for demonstration
class MyComponent(Enum):
    NETWORK = 0
    DATABASE = 1
    UI = 2

# --- Callback functions for demonstration ---

def print_callback(entry):
    print(f"[{entry.timestamp}] [{entry.severity.name}] {entry.component.name}: {entry.message} ({entry.file}:{entry.line})")

def warning_callback(entry):
    print(f"{Fore.YELLOW}WARNING: {entry.message} in {entry.component.name}{Style.RESET_ALL}")

def error_callback(entry):
    print(f"{Fore.RED}ERROR: {entry.message} ({entry.file}:{entry.line}){Style.RESET_ALL}")

def debug_callback(entry):
    print(f"{Fore.CYAN}DEBUG: {entry.message}{Style.RESET_ALL}")

def all_callback(entry):
    print(f"{Fore.MAGENTA}ALL: {entry.message} in {entry.component.name} at {entry.file}:{entry.line}{Style.RESET_ALL}")

# --- Example usage ---

def main():
    logger = pycallbacklogger.CallbackLogger()

    # Register callbacks for different severities and components
    logger.register_function_callback(print_callback, pycallbacklogger.Severity.Info)
    logger.register_function_callback(warning_callback, pycallbacklogger.Severity.Warning)
    logger.register_function_callback(error_callback, {MyComponent.DATABASE: pycallbacklogger.Severity.Error})
    logger.register_function_callback(debug_callback, [MyComponent.UI])
    logger.register_function_callback(all_callback)  # No filter: receives all

    # Register file callbacks
    logger.register_file_callback('all_logs.log')
    logger.register_file_callback('db_warnings.log', {MyComponent.DATABASE: pycallbacklogger.Severity.Warning})

    # Helper to log with file/line info
    def log_with_location(severity, component, message):
        frameinfo = getframeinfo(currentframe())
        logger.log(severity, component, message, frameinfo.filename, frameinfo.lineno)

    # Log messages with various severities and components
    log_with_location(pycallbacklogger.Severity.Info, MyComponent.NETWORK, "Network initialized")
    log_with_location(pycallbacklogger.Severity.Warning, MyComponent.UI, "UI lag detected")
    log_with_location(pycallbacklogger.Severity.Error, MyComponent.DATABASE, "Database connection failed")
    log_with_location(pycallbacklogger.Severity.Debug, MyComponent.UI, "UI redraw event")
    log_with_location(pycallbacklogger.Severity.Info, MyComponent.DATABASE, "Database query executed")
    log_with_location(pycallbacklogger.Severity.Info, MyComponent.NETWORK, "Network packet sent")

    print("\nCheck 'all_logs.log' and 'db_warnings.log' for file output.")

if __name__ == "__main__":
    main()
