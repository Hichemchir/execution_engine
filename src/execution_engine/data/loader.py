from pathlib import Path

import pandas as pd

# Constants
DATA_PATH = Path(__file__).parent.parent.parent.parent / "data" / "SP500.csv"


def load_data(filepath: Path) -> pd.DataFrame:
    df = pd.read_csv(filepath, parse_dates=["Date"])

    if df["Volume"].isna().any():
        df["Volume"] = df["Volume"].fillna()

    return df
