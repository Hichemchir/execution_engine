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