from ..src import models, utils
import logging

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(name)s - %(message)s"
)
logger = logging.getLogger(name=__name__)

def kpi_ohlcv(df : pd.DataFrame) -> None:
    logger.info(f"\nColumns: {df.columns.tolist()}")

    sample = df.iloc[-5:]
    logger.info(f"5 most recent days:\n{sample}")

    last_day = df.iloc[-1]
    daily_move = last_day['High'] - last_day['Low']
    pct_move = (daily_move / last_day['Open']) * 100
    logger.info(f"Daily range: ${daily_move:.2f} ({pct_move:.2f})")

def metrics(df: pd.DataFrame) -> pd.DataFrame:
    result = df.copy()

    # Daily return: did price go up or down
    result['daily_return'] = result['Close'].pct_change()

    result['avg_price'] = (result['High'] + result['Low']) / 2
    result['daily_volatility'] = (result['High'] - result['Low']) / result['Open']
    
    return result