from pathlib import Path

import logging
import pandas as pd

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(name)s - %(message)s"
)

logger = logging.getLogger(name=__name__)

# Constants
data_path = Path("../data/SP500.csv")

def load_data(filepath: Path) -> pd.DataFrame:
    df = pd.read_csv(filepath, parse_dates=["Date"])
    return df

def main() -> None:
    if not data_path.exists():
        logger.error(f"Data file not found {data_path}")
    
    df = load_data(data_path)
    logger.info(f"Data loaded:\n {df.tail()}")

if __name__ == "__main__":
    main()