# Execution Engine

A high-performance order execution engine for algorithmic trading, combining Python's flexibility with C++'s speed through seamless bindings.

## Overview

This project implements TWAP (Time-Weighted Average Price) and VWAP (Volume-Weighted Average Price) execution strategies with both Python and C++ backends. The C++ implementation provides significant performance improvements for large orders while maintaining a clean Python API for strategy development and backtesting.

## Architecture

```
execution_engine/
├── src/execution_engine/          # Python package
│   ├── data/                      # Data loading utilities
│   ├── models/                    # Order models
│   ├── strategies/                # TWAP & VWAP implementations
│   └── utils/                     # Logging & helpers
├── cpp/                           # C++ engine
│   ├── include/                   # Headers
│   ├── src/                       # Implementation
│   ├── bindings/                  # Python bindings (pybind11)
│   └── test/                      # C++ unit tests
├── tests/                         # Python & binding tests
└── benchmark.py                   # Performance benchmarks
```

## Features

### Implemented

- **TWAP Strategy**: Equal-size order splitting across time periods
- **VWAP Strategy**: Volume-weighted order execution
- **Dual Implementation**: Python and C++
- **Python Bindings**: Seamless C++ integration via pybind11
- **Comprehensive Testing**: Unit tests for both Python and C++ components
- **Performance Benchmarks**: Automated comparative analysis

### Performance

Real-world benchmark results on Apple Silicon M4 (1000 days of S&P 500 data):

| Order Size | Slices | Python | C++ | Speedup |
|------------|--------|--------|-----|---------|
| 1,000 | 5 | 0.0073s | 0.0047s | 1.6x |
| 10,000 | 10 | 0.0137s | 0.0048s | 2.9x |
| 100,000 | 20 | 0.0258s | 0.0046s | 5.6x |
| 1,000,000 | 50 | 0.0616s | 0.0047s | 13.1x |
| 1,000,000,000 | 200 | 0.2426s | 0.0053s | **45.7x** |

**Insight**: C++ provides 5-45x speedup for institutional-scale orders (100K+ shares), with performance gains increasing as order size grows.

## Installation

### Requirements

- Python 3.13+
- C++20 compiler (clang/gcc)
- CMake 3.15+
- pybind11

### Setup

```bash
# Install Python dependencies
uv sync

# Build C++ engine
cd cpp
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
cmake --install .
```

## Usage

### Python API

```python
from execution_engine import Order, execute_twap, execute_vwap, load_data, DATA_PATH

# Load market data
df = load_data(DATA_PATH)

# Create order
order = Order(size=100_000, direction="buy", num_slices=10)

# Execute TWAP
result = execute_twap(df, order, start_idx=0)

print(f"Average price: ${result.avg_price:.2f}")
print(f"Slippage: {result.slippage_bps:.2f} bps")
```

### C++ API (via Python bindings)

```python
import execution_engine._execution_cpp as cpp

# Create C++ engine
engine = cpp.ExecutionEngine()
order = cpp.Order(100_000, "buy", 10)

# Execute with C++ backend
prices = df["Close"].tolist()
result = engine.execute_twap(prices, order, 0)

print(f"Total cost: ${result.total_cost:,.2f}")
print(f"Execution slices: {len(result.slices)}")
```

## Testing

```bash
# Run all tests
make test

# Python tests only
make test-python

# C++ tests only
make test-cpp

# Binding tests
make test-bindings
```

## Benchmarking

```bash
# Full benchmark suite
make benchmark

# Quick validation benchmark
make benchmark-quick
```

The benchmark suite includes:
- Quick benchmark (1000 iterations)
- Multiple runs with statistics (10 runs × 100 iterations)
- Varying order sizes (1K to 1B shares)

## Project Structure

### Python Package (src/execution_engine)

- `data/loader.py`: CSV data loading utilities
- `models/order.py`: Order data structures
- `strategies/twap.py`: TWAP implementation
- `strategies/vwap.py`: VWAP implementation
- `utils/logging.py`: Logging configuration

### C++ Engine (cpp)

- `include/execution_engine.hpp`: Core engine interface
- `src/execution_engine.cpp`: TWAP/VWAP implementations
- `bindings/bindings.cpp`: Python bindings via pybind11
- `test/test_twap.cpp`: Google Test unit tests

## Development

### Code Quality

```bash
# Lint
make lint

# Format
make format

# Type check
make typecheck

# Full check
make check
```

## Roadmap

### Next Steps

1. **Market Microstructure**
   - Real-time market impact modeling
   - Bid-ask spread analysis
   - Liquidity-aware execution

2. **Advanced Execution Logic**
   - Dynamic slice sizing based on market conditions
   - Adaptive participation rates
   - Smart order routing across venues

3. **Performance Optimization**
   - Multi-threaded execution engine
   - Lock-free data structures
   - SIMD vectorization for calculations

4. **Production Features**
   - Low-latency market data feed handlers
   - Real-time risk checks
   - Order management system (OMS) integration
   - FIX protocol support.

5. **Infrastructure**
   - Distributed backtesting framework
   - Real-time monitoring and alerting
   - Performance profiling tools
   
6. CI/CD, Docker, ML model

## Technical Details

### Why C++?

For institutional trading, latency is critical:
- **Target latency budget**: <10ms per order decision
- **Throughput requirements**: 10K-100K orders/second
- **Cost impact**: Faster execution reduces slippage (2-5 bps difference = millions annually)

### Design Choices

- **pybind11**: Zero-overhead Python/C++ interop
- **Google Test**: Industry-standard C++ testing
- **CMake**: Cross-platform build system
- **Modern C++20**: Safe, expressive, performant

### Performance Characteristics

The C++ engine shows superlinear speedup for large orders due to:
1. **Memory layout**: Contiguous arrays vs. pandas DataFrame overhead
2. **No GIL**: True parallel execution potential
3. **Cache efficiency**: Better CPU cache utilization
4. **Allocation**: Stack allocation vs. Python heap

## License

MIT License