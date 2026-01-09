from models import Order, ExecutionSlice, ExecutionResult
from utils.utils import load_data, DATA_PATH
from utils.logging import logger

import pandas as pd



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
 