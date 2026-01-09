from execution_engine import DATA_PATH, ExecutionResult, Order, execute_twap, load_data, execute_vwap

df = load_data(DATA_PATH)

class TestVWAP:

    def test_basic_vwap(self):
        # AAA: appply assert
        order = Order(10000, "buy", 5)
        result = execute_vwap(df, order, 0)

        assert result.total_cost >