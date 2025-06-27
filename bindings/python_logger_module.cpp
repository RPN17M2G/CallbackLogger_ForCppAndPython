#include "python_logger_module.hpp"
#include "python_logger_types.hpp"
#include "python_logger_utils.hpp"

namespace py = pybind11;

PYBIND11_MODULE(pycallbacklogger, m)
{
    register_python_logger_types(m);
    register_python_logger_utils(m);
}
