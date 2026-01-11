import pandas as pd

from src.execution_engine.models.order import ExecutionResult, ExecutionSlice, Order
from src.execution_engine.utils.logging import get_logger

logger = get_logger(__name__)


def execute_vwap(df: pd.DataFrame, order: Order, start_idx: int) -> ExecutionResult:
    """Execute order using VWAP: slices proportional to volume.
    VWAP allocates more shares to days with higher volume,
    minimizing market impact.
    """
    # Get volume data for execution window
    end_idx = start_idx + order.num_slices
    if end_idx > len(df):
        logger.warning("Not enough data")

    window_df = df.iloc[start_idx:end_idx].copy()

    # Check if NaN
    if window_df["Volume"].isna().any():
        logger.warning(f"Found {window_df['Volume'].isna().sum()} NaN volumes. Filling with 0.")
        window_df["Volume"] = window_df["Volume"].fillna(0)

    # Calculate volume proportions
    total_volume = window_df["Volume"].sum()
    if total_volume == 0:
        window_df["volume_pct"] = 1.0 / len(window_df)
    else:
        window_df["volume_pct"] = window_df["Volume"] / total_volume

    # Allocate shares proportionnaly to volume
    window_df["slice_size"] = window_df["volume_pct"] * order.size

    slices: list[ExecutionSlice] = []
    total_cost = 0.0

    for i, (_, row) in enumerate(window_df.iterrows()):
        price = float(row["Close"])
        slice_size = float(row["slice_size"])
        slice_cost = price * slice_size
        total_cost += slice_cost

        exec = ExecutionSlice(
            day=i + 1, date=row["Date"], size=slice_size, price=price, cost=slice_cost
        )

        slices.append(exec)

    avg_price = total_cost / order.size
    benchmark_price = float(window_df.iloc[start_idx]["Close"])

    slippage = (avg_price - benchmark_price) / benchmark_price
    slippage_bps = slippage * 10000

    return ExecutionResult(
        slices=slices,
        total_cost=total_cost,
        avg_price=avg_price,
        benchmark_price=benchmark_price,
        slippage_bps=slippage_bps,
    )
