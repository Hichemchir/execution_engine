#include <pybind11/pybind11.h>
#include <pybind11/stl.h>   // std::vector, std::string
#include "execution_engine.hpp"

namespace py = pybind11;
using namespace execution;

PYBIND11_MODULE(_execution_cpp, m) {
    m.doc() = "C++ TWAP execution engine for low-latency trading";
    
    /**
     * Expose Order Struct
     */
    py::class_<Order>(m, "Order", "Trading order with size, direction, slices")
        // Constructor
        .def(py::init<double, std::string&, int>(),
             py::arg("size"),
             py::arg("direction"),
             py::arg("num_slices"),
             "Create a new order\n"
            )
        // Attributes (READ + WRITE), for Python, pointer to c++ attr, getter/setter
        .def_readwrite("size", &Order::size, "Order size in shares")
        .def_readwrite("direction", &Order::direction, "Order direction ('buy' or 'sell')")
        .def_readwrite("num_slices", &Order::num_slices, "Number of execution slices")

        // __repr__ method for print()
        .def("__repr__", [](const Order& o) {
            return "<Order size=" + std::to_string(o.size) +
                   " direction=" + o.direction +
                   " slices=" + std::to_string(o.num_slices) + ">";
        }
    );

    /**
     * Expose ExecutionSlice struct
     */ 
    py::class_<ExecutionSlice>(m, "ExecutionSlice", "Single execution slice")
        // Constructor not exposed, created by C++

        // Attributes (READ only, Python should not modify)
        .def_readonly("day", &ExecutionSlice::day, "Execution day number")
        .def_readonly("size", &ExecutionSlice::size, "Slice size in shares")
        .def_readonly("price", &ExecutionSlice::price, "Execution price")
        .def_readonly("cost", &ExecutionSlice::cost, "Total cost (size*price)")

        // __repr__ method for print()
        .def("__repr__", [](const ExecutionSlice& s) {
            return "<Slice day=" + std::to_string(s.day) +
                   " size=" + std::to_string(s.size) +
                   " price=" + std::to_string(s.price) + ">";
        }
    );
    
    /**
     * Expose ExecutionResult struct
     */ 
    py::class_<ExecutionResult>(m, "ExecutionREsult", "Execution result with metrics")
        // Constructor not exposed, created by C++

        // Attributes (READ only, Python should not modify)
        .def_readonly("slices", &ExecutionResult::slices, "List of execution slices")
        .def_readonly("total_cost", &ExecutionResult::total_cost, "Total execution cost")
        .def_readonly("avg_price", &ExecutionResult::avg_price, "Average execution price")
        .def_readonly("benchmark_price", &ExecutionResult::benchmark_price, "Benchmark price (start)")
        .def_readonly("slippage_bps", &ExecutionResult::slippage_bps, "Slippage in basis points")

        // __repr__ method for print()
        .def("__repr__", [](const ExecutionResult& r) {
            return "<ExecutionResult slices=" + std::to_string(r.slices.size()) +
                   " cost=" + std::to_string(r.total_cost) +
                   " slippage=" + std::to_string(r.slippage_bps) + "bps>";
        }
    );

    /**
     * Expose ExecutionEngine class
     */ 
    py::class_<ExecutionEngine>(m, "ExecutionEngine", "Low-latency execution engine")
        // Constructor
        .def(py::init<>(), "Create execution engine")

        // Method execution_twap
        .def("execution_twap", &ExecutionEngine::execute_twap,
            py::arg("prices"),
            py::arg("order"),
            py::arg("start_idx"),
            "Execute TWAP strategy\n"
        )
        
        // __repr__ method for print()
        .def("__repr__", [](const ExecutionEngine&) {
            return "<ExecutionEngine ready>";
        }
    );   

}