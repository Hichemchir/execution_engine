"""Benchmark for Python vs C++ performance execution."""

import statistics
import time

import execution_engine._execution_cpp as cpp

from execution_engine import DATA_PATH, execute_twap, load_data
from execution_engine import Order as PyOrder

BENCHMARK_DATA_SIZE = 1000


def benchmark_twap(iterations: int = 1000, warmup: int = 10) -> tuple[float, float, float]:
    """
    Bencharm for twap execution Py vs C++.
    Args:
        iterations: number of iterations to run
    Returns:
        (python_time, cpp_time, speedup)
    """
    # Load data
    df = load_data(DATA_PATH)
    df = df.tail(BENCHMARK_DATA_SIZE)
    prices_list = df["Close"].tolist()

    # setup
    py_order = PyOrder(10_000, "buy", 10)
    cpp_order = cpp.Order(10_000, "buy", 10)
    engine = cpp.ExecutionEngine()

    ########## WARMUP ##########
    # -> to remove bias, to load data & code into CPU caches L1/L2/L3
    # After warmup: data & code into caches
    # + Python uses JIT (code optimized after some runs)
    # + CPU branch prediction (cPU learns patterns)
    # + remove first execution overhead (buffer allocation, data structures init)

    if warmup > 0:
        for _ in range(warmup):
            execute_twap(df, py_order, 0)
            engine.execute_twap(prices_list, cpp_order, 0)

    ########## Benchmark PYTHON ##########
    start = time.perf_counter()
    for _ in range(iterations):
        execute_twap(df, py_order, 0)
    py_time = time.perf_counter() - start

    ########## Benchmark C++ ##########∑
    start = time.perf_counter()
    for _ in range(iterations):
        engine.execute_twap(prices_list, cpp_order, 0)
    cpp_time = time.perf_counter() - start

    return py_time, cpp_time, py_time / cpp_time


def benchmark_multiple_runs(runs: int = 10, iterations: int = 100, warmup: int = 5) -> None:
    """Run multiple benchmark runs and compute statistics."""
    print(f"\n{'=' * 70}")
    print("Multiple Runs Benchmark")
    print(f"\n{'=' * 70}")
    print(f"Runs: {runs} x {iterations} iterations\n")

    py_times = []
    cpp_times = []
    speedups = []

    for i in range(runs):
        print(f"Run {i + 1}/{runs}...", end=" ", flush=True)
        p, c, s = benchmark_twap(iterations=iterations, warmup=warmup)
        py_times.append(p)
        cpp_times.append(c)
        speedups.append(s)
        print(f"{s:.1f}x")

    # Statistics
    print(f"\n{'=' * 70}")
    print("Statistics")
    print(f"{'=' * 70}")

    print("\n🐍 Python:")
    print(f"   Mean:   {statistics.mean(py_times):.4f}s")
    print(f"   Median: {statistics.median(py_times):.4f}s")
    print(f"   Std:    {statistics.stdev(py_times):.4f}s")
    print(f"   Min:    {min(py_times):.4f}s")
    print(f"   Max:    {max(py_times):.4f}s")

    print("\n⚙️  C++:")
    print(f"   Mean:   {statistics.mean(cpp_times):.4f}s")
    print(f"   Median: {statistics.median(cpp_times):.4f}s")
    print(f"   Std:    {statistics.stdev(cpp_times):.4f}s")
    print(f"   Min:    {min(cpp_times):.4f}s")
    print(f"   Max:    {max(cpp_times):.4f}s")

    print("\n🚀 Speedup:")
    print(f"   Mean:   {statistics.mean(speedups):.1f}x")
    print(f"   Median: {statistics.median(speedups):.1f}x")
    print(f"   Std:    {statistics.stdev(speedups):.1f}x")
    print(f"   Min:    {min(speedups):.1f}x")
    print(f"   Max:    {max(speedups):.1f}x")

    print(f"\n{'=' * 70}")


def benchmark_varying_sizes() -> None:
    """Benchmark with different order sizes and slice counts."""
    print(f"\n{'=' * 70}")
    print("Varying Order Sizes")
    print(f"{'=' * 70}\n")

    df = load_data(DATA_PATH)
    df = df.tail(BENCHMARK_DATA_SIZE)
    prices_list = df["Close"].tolist()

    configs = [
        (1_000, 5),
        (10_000, 10),
        (100_000, 20),
        (1_000_000, 50),
        (1_000_000_000, 200),
    ]

    print(f"{'Size':>12} | {'Slices':>6} | {'Python':>10} | {'C++':>10} | {'Speedup':>8}")
    print(f"{'-' * 12}-+-{'-' * 6}-+-{'-' * 10}-+-{'-' * 10}-+-{'-' * 8}")

    for size, slices in configs:
        py_order = PyOrder(size, "buy", slices)
        cpp_order = cpp.Order(size, "buy", slices)
        engine = cpp.ExecutionEngine()

        # Benchmark
        iterations = 100

        start = time.perf_counter()
        for _ in range(iterations):
            execute_twap(df, py_order, 0)
        py_time = time.perf_counter() - start

        start = time.perf_counter()
        for _ in range(iterations):
            engine.execute_twap(prices_list, cpp_order, 0)
        cpp_time = time.perf_counter() - start

        speedup = py_time / cpp_time

        print(
            f"{size:>12,} | {slices:>6} | {py_time:>9.4f}s | {cpp_time:>9.4f}s | {speedup:>7.1f}x"
        )

    print()


def main() -> None:
    """Run benchmark."""
    print("""
╔════════════════════════════════════════════════════════════════════╗
║                   🚀 Execution Engine Benchmark                    ║
║                      Python vs C++ Performance                     ║
╚════════════════════════════════════════════════════════════════════╝
""")

    # Quick benchmark
    print(f"\n{'=' * 70}")
    print("⚡ Quick Benchmark (1000 iterations)")
    print(f"{'=' * 70}\n")

    py_time, cpp_time, speedup = benchmark_twap(iterations=1000, warmup=10)

    print("\nResults:")
    print(f"   🐍 Python: {py_time:.4f}s ({py_time / 1000 * 1000:.2f}ms/iter)")
    print(f"   ⚙️  C++:    {cpp_time:.4f}s ({cpp_time / 1000 * 1000:.2f}ms/iter)")
    print(f"   🚀 Speedup: {speedup:.1f}x faster")

    # Multiple runs
    benchmark_multiple_runs(runs=10, iterations=100)

    # Varying sizes
    benchmark_varying_sizes()

    print(f"\n{'=' * 70}")
    print("Benchmark complete!")
    print(f"{'=' * 70}\n")


if __name__ == "__main__":
    main()
