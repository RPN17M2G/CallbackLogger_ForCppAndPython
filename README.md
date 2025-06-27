# CallbackLogger

A high-performance, flexible callback based logging infrastructure for Cpp and Python, supporting:
- Asynchronous and synchronous logging
- Custom log components (enums)
- Advanced filtering by severity and component
- Multiple callback types (function, file)
- Python bindings via [pybind11](https://github.com/pybind/pybind11)
- Thread-safe, scalable design

---

## Features

- **Register multiple callbacks**: Log to files, functions, or both.
- **Filter by severity and component**: Only receive the logs you care about.
- **Custom components**: Use your own enums for log sources.  
  The logger automatically maps and tracks any enum type and value you use as a component, so you can freely use multiple different enums (even at the same time) without any registration or configuration. Each enum's type and value are stored and distinguished internally, ensuring correct filtering and routing for all your log sources.
- **Async or single-threaded**: Choose your performance model.
- **Python and Cpp support**: Use the same logger in both languages - Perfect for multi-languages systems.

---

## Installation

### Python

```bash
pip install .
```

or (for editable/development mode):

```bash
pip install -e .
```

### Cpp

Add the library to your CMake project:

```cmake
add_subdirectory(CallbackLogger_ForCppAndPython)
target_link_libraries(your_target PRIVATE CallbackLogger)
```

---

## Usage

### Python Example

```python
import pycallbacklogger
from enum import Enum

class MyComponent(Enum):
    NETWORK = 0
    DATABASE = 1
    UI = 2

def print_callback(entry):
    print(f"[{entry.timestamp}] [{entry.severity.name}] {entry.component.name}: {entry.message}")

logger = pycallbacklogger.CallbackLogger()

# Register callbacks
logger.register_function_callback(print_callback, pycallbacklogger.Severity.Info)
logger.register_file_callback('all_logs.log')
logger.register_function_callback(print_callback, {MyComponent.DATABASE: pycallbacklogger.Severity.Error})

# Log messages
logger.log(pycallbacklogger.Severity.Info, MyComponent.NETWORK, "Network initialized", __file__, 10)
logger.log(pycallbacklogger.Severity.Error, MyComponent.DATABASE, "Database error!", __file__, 12)
```

### Cpp Example

```cpp
#include "CallbackLogger.hpp"
#include <iostream>

enum class MyComponent { NETWORK, DATABASE, UI };

int main() {
    CallbackLogger logger(1);

    logger.register_function_callback(
        [](const LogEntry& entry) {
            std::cout << "[" << to_string(entry.severity) << "] "
                      << entry.component.to_string() << ": "
                      << entry.message << std::endl;
        },
        Severity::Info
    );

    logger.register_file_callback("all_logs_cpp.log", Severity::Debug);

    logger.log(Severity::Info, make_component_entry(MyComponent::NETWORK), "Network initialized", __FILE__, __LINE__);
    logger.log(Severity::Error, make_component_entry(MyComponent::DATABASE), "Database error!", __FILE__, __LINE__);
}
```

---

## API Overview

### Python

- `CallbackLogger()`: Create a logger instance.
- `register_function_callback(callback, filter)`: Register a Python function as a log callback. `filter` can be a severity, set/list of components, or a dict mapping components to severities.
- `register_file_callback(filename, filter)`: Log to a file. `filter` as above.
- `unregister_function_callback(handle)`: Remove a function callback.
- `unregister_file_callback(handle)`: Remove a file callback.
- `log(severity, component, message, file, line)`: Log a message.

### Cpp

- `CallbackLogger(size_t thread_count)`: Create a logger (0 = single-threaded).
- `register_function_callback(function, filter)`: Register a function callback.
- `register_file_callback(filename, filter)`: Register a file callback.
- `unregister_function_callback(handle)`, `unregister_file_callback(handle)`: Remove callbacks.
- `log(severity, component, message, file, line)`: Log a message.

---

## Technology

Python integration: Pybind11
Cpp version: 17
Python version: 3.7+
Python testing: Pytest
Cpp testing: Google Test (gtest)
Build system: CMake

### Component System Architecture
Log components are abstracted via the ComponentEnumEntry class, which encapsulates both the enum type (using std::type_index for Cpp enums or std::string for dynamic types) and its value. 
This enables type-safe, runtime-agnostic component identification without explicit registration or compile-time knowledge of all possible enums. 
The logger leverages this abstraction to support heterogeneous component enums in a single logging instance.


### Multi-Enum Handling
The logger's core is built around the generic ComponentEnumEntry, allowing seamless integration of multiple, unrelated enum types.
At runtime, the logger introspects the enum type via typeid and stores it alongside the value, ensuring that log entries are always associated with their precise component type.
This mechanism supports advanced use cases such as dynamic extension of component sets without code changes to the logger itself.

### Thread Safety and Concurrency
The logger is designed for high-concurrency environments.
It uses mutexes and atomic operations to synchronize access to internal data structures, such as callback registries and log queues.
Log entries are processed asynchronously, enabling non-blocking logging from multiple threads.
This architecture ensures deterministic delivery order and prevents race conditions, even under heavy parallel workloads.


### Additional Features
Dynamic Callback Registration: Supports runtime registration and deregistration of function and file callbacks, each with customizable severity and component filters (including per-component severity maps).
Asynchronous Processing: Log entries are queued and processed by a configurable thread pool, minimizing logging overhead on application threads.
Flexible Filtering: Callbacks can be filtered by severity, component, a set of components, or a map of component-to-severity, enabling fine-grained control over log routing.
Exception Safety: All callback invocations are exception-safe; exceptions thrown by user callbacks are caught and do not disrupt the logging pipeline.
Extensible Component Model: New component enums can be introduced at any time without modifying the logger, thanks to the type-erased ComponentEnumEntry abstraction.
File Logging: File callbacks append log entries to disk with severity-based formatting, and handle file I/O errors.

## Testing

There are system tests for both Python and Cpp.

### Test Map

#### Python Tests

focuses on functionality and integration of the Python API, ensuring that all features work as expected, including:
- Function and file callbacks
- Custom component handling
- Filtering by severity and component

#### Cpp Tests

focuses on the core Cpp functionality and especially on the asynchronous logging capabilities, ensuring that:
- The Cpp API behaves correctly
- Asynchronous logging works as expected
- Thread safety is maintained
- Custom components are handled correctly
- Filtering and routing of log messages are correct



### Running Tests

- **Python:**  
  Run all tests with:
  ```bash
  pytest
  ```

- **Cpp:**  
  Enable tests in CMake (`-DBUILD_TESTING=ON`) and run with your test runner.

---

## License

MIT License

---

## Author

Omer Gindi

---