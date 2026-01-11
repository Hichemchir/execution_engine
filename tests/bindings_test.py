"""Tests for C++ bindings."""

import pytest 

try:
    import src.execution_engine._execution_cpp as cpp
    CPP_AVAILABLE = True
except ImportError:
    CPP_AVAILABLE = False
    pytestmark = pytest.mark.skip("C++ module not built")

@pytest.mark.skipif(not CPP_AVAILABLE, reason="C++ module not available")
class TestCppBindings:
    
    def test_import_module(self):
        assert cpp is not None
    
    def test_order_creation(self):
        order = cpp.Order(10_000, "buy", 5)

        assert order.size == 10_000
        assert order.direction == "buy"
        assert order.num_slices == 5
    
    def test_order_read_write(self):
        order = cpp.Order(1_000, "buy", 10)

        order.size = 5_000
        assert order.size == 5_000

        order.direction = "sell"
        assert order.direction == "sell"
    
    def test_order_repr(self):
        order = cpp.Order(1_000, "buy", 5)
        repr_str = repr(order)

        assert "Order" in repr_str
        assert "1000" in repr_str or "1000.0" in repr_str


class TestCppExecutionEngine:
    
    def test_engine_creation(self):
        engine = cpp.ExecutionEngine()
        
        assert engine is not None
    
    def test_basic_exec(self):
        engine = cpp.ExecutionEngine()
        prices = [100.0, 101.0, 102.0, 103.0, 104.0]
        order = cpp.Order(1_000, "buy", 5)

        result = engine.execute_twap(prices, order, 0)

        assert hasattr(result, "slices")
        assert len(result.slices) == 5