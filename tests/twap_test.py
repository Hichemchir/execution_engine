from execution_engine import DATA_PATH, ExecutionResult, Order, execute_twap, load_data

df = load_data(DATA_PATH)


# AAA: Arrange (setup), Act (execute func), Assert (verify)
class TestTWAP:
    def test_executes_all_slices(self) -> None:
        order = Order(size=10000, direction="buy", num_slices=5)
        result = execute_twap(df, order, start_idx=0)

        assert len(result.slices) == order.num_slices, "Should execute 5 slices"
        assert isinstance(result, ExecutionResult)
