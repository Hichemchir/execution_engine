from execution_engine import DATA_PATH, ExecutionResult, Order, execute_vwap, load_data

df = load_data(DATA_PATH)


class TestVWAP:
    def test_basic_vwap(self):
        # AAA: arrange act assert
        order = Order(10000, "buy", 5)
        result = execute_vwap(df, order, 0)

        assert len(result.slices) == 5, "Should execute 5 slices"
        assert isinstance(result, ExecutionResult)
        assert result.total_cost > 0
