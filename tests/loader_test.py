from execution_engine import DATA_PATH, load_data

# AAA


class TestLoad:
    def test_load_data(self):
        df = load_data(DATA_PATH)

        assert len(df) != 0
