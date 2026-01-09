from dataclasses import dataclass
from typing import Literal

import pandas as pd


@dataclass
class Order:
    """
    Represents a buy/Sell order
    """

    size: float  # number of shares
    direction: Literal["buy", "sell"]  # buy or sell
    num_slices: int = 10  # spit into how many pieces


@dataclass
class ExecutionSlice:
    """One piece of the order execution"""

    day: int
    date: pd.Timestamp
    size: float
    price: float
    cost: float


@dataclass
class ExecutionResult:
    slices: list[ExecutionSlice]
    total_cost: float
    avg_price: float
    benchmark_price: float
    slippage_bps: float
