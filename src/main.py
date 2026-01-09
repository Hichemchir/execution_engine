from models import Order, ExecutionSlice, ExecutionResult
from utils import load_data, DATA_PATH

import logging
import pandas as pd
import numpy as np

# Logging def
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(name)s - %(message)s"
)
logger = logging.getLogger(name=__name__)


def execute_twap(df: pd.DataFrame, order: Order, start_idx: int) -> ExecutionSlice:
    """Execute order using TWAP: equal slices over time.
    TWAP = time weighted average price
    Split order into equal pieces, one per day
    """
    slice_size = order.size / order.num_slices
    slices = []
    total_cost = 0.0

    for i in range(order.num_slices):
        day_idx = start_idx + i
        if day_idx >= len(df):
            logger.warning("Not enough data")
        
        row = df.iloc[day_idx]
        # We exec at close price (simple)
        price = row['Close']

        slice_cost = price * slice_size
        total_cost += slice_cost

        exec = ExecutionSlice(
            day=i + 1,
            date=row['Date'],
            size=slice_size,
            price=price,
            cost=slice_cost
        )
        
        slices.append(exec)
    
    avg_price = total_cost / order.size
    benchmark_price = df.iloc[start_idx]["Close"]

    # Slippage: how much more we paid VS initial price
    slippage = (avg_price - benchmark_price) / benchmark_price
    slippage_bps = slippage * 10000 # convert to basis points

    return ExecutionResult(
        slices=slices,
        total_cost=total_cost,
        avg_price=avg_price,
        benchmark_price=benchmark_price,
        slippage_bps=slippage_bps
    )



def main() -> None:
    if not DATA_PATH.exists():
        logger.error(f"Data file not found {DATA_PATH}")
    
    df = load_data(DATA_PATH)
    # logger.info(f"Data loaded:\n {df.tail()}")

    # Test
    order = Order(
        size=100,
        direction="buy",
        num_slices=2
    )

    start_idx = len(df) - 50
    result = execute_twap(df=df, order=order, start_idx=start_idx)

    logger.info(f"\nExecution Summary:")
    logger.info(f"  Benchmark price (day 1): ${result.benchmark_price:.2f}")
    logger.info(f"  Average execution price: ${result.avg_price:.2f}")
    logger.info(f"  Total cost: ${result.total_cost:,.2f}")
    logger.info(f"  Slippage: {result.slippage_bps:+.2f} bps")
    
    if result.slippage_bps > 0:
        extra_cost = result.total_cost - (result.benchmark_price * order.size)
        logger.info(f"  Extra cost vs benchmark: ${extra_cost:,.2f}")

if __name__ == "__main__":
    main()