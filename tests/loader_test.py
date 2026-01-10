from execution_engine import load_data, DATA_PATH

# AAA

class TestLoad:

    def test_load_data(self):
        df = load_data(DATA_PATH)

        assert len(df) != 0