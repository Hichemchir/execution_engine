import pytest
import pandas as pd
import numpy as np

from execution_engine import (
    load_data, 
    DATA_PATH, 
    Order, 
    execute_twap, 
    get_logger,
    ExecutionResult
)

df = load_data(DATA_PATH)

# AAA: Arrange (setup), Act (execute func), Assert (verify)
class TestTWAP:
    def test_executes_all_slices(self, sample_market_data: pd.DataFrame) -> None:
        order = Order(size=10000, direction="BUY", num_slices=5)
        result = execute_twap(df, order, start_idx=0)

        assert len(result.size) == order.num_slices, "Should execute 5 slices"
        assert isinstance(result, ExecutionResult)