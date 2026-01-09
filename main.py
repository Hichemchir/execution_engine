"""Main entry point for the execution engine."""

import sys
from pathlib import Path

from src.execution_engine.data.loader import load_data, DATA_PATH
from src.execution_engine.models.order import Order
from src.execution_engine.strategies.twap import execute_twap
from src.execution_engine.utils.logging import get_logger

logger = get_logger(__name__)

def main() -> int:    
    if not DATA_PATH.exists():
        logger.error(f"Data file not found: {DATA_PATH}")
        return 1
    
    df = load_data(DATA_PATH)

    # Test
    order = Order(
        size=100,
        direction="buy",
        num_slices=10
    )
    
    logger.info(f"\nOrder Details:")
    logger.info(f"  Size: {order.size:,.0f} shares")
    logger.info(f"  Direction: {order.direction.upper()}")
    logger.info(f"  Slices: {order.num_slices}")
    
    start_idx = len(df) - 50
    result = execute_twap(df, order, start_idx)
    
    logger.info(f"{'='*60}")
    logger.info("EXECUTION SUMMARY")
    logger.info(f"{'='*60}")
    logger.info(f"  Benchmark price: ${result.benchmark_price:.2f}")
    logger.info(f"  Average price: ${result.avg_price:.2f}")
    logger.info(f"  Total cost: ${result.total_cost:,.2f}")
    logger.info(f"  Slippage: {result.slippage_bps:+.2f} bps")
    
    if result.slippage_bps > 0:
        extra_cost = result.total_cost - (result.benchmark_price * order.size)
        logger.info(f"  Extra cost: ${extra_cost:,.2f}")
    
    logger.info(f"{'='*60}")
    logger.info(f"Execution complete! {len(result.slices)} slices executed")
    
    return 0


if __name__ == "__main__":
    sys.exit(main())