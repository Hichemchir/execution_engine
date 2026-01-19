"""
Simple market impact prediction model.
Demonstrates ML engineering rigor without over-complexity.

Key for CFM interview:
1. Clean data pipeline
2. Proper train/test split (temporal)
3. Feature importance analysis
4. Residual diagnostics
"""

import pandas as pd
import numpy as np
import lightgbm as lgb
from sklearn.model_selection import TimeSeriesSplit
from sklearn.metrics import mean_absolute_error, r2_score
import matplotlib.pyplot as plt
import seaborn as sns
from pathlib import Path
from typing import Tuple, Dict
from dataclasses import dataclass
import sys

sys.path.insert(0, str(Path(__file__).parent.parent / "src"))

from execution_engine import DATA_PATH, load_data


# ============================================================================
# Configuration
# ============================================================================

@dataclass
class ModelConfig:
    """Simple configuration for reproducibility"""
    target_column: str = 'market_impact_bps'
    test_size: float = 0.2
    random_state: int = 42
    n_estimators: int = 100
    learning_rate: float = 0.05
    max_depth: int = 5


# ============================================================================
# Feature Engineering
# ============================================================================

def create_features(df: pd.DataFrame, order_size: int = 100000) -> pd.DataFrame:
    """
    Create simple but effective features for impact prediction.
    
    Focus: Market microstructure + order characteristics
    """
    features = pd.DataFrame(index=df.index)
    
    # 1. Spread (High-Low as proxy for bid-ask)
    features['spread'] = (df['High'] - df['Low']) / df['Close']
    
    # 2. Relative volume
    features['volume_rel'] = df['Volume'] / df['Volume'].rolling(20).mean()
    
    # 3. Realized volatility (20-day)
    returns = np.log(df['Close'] / df['Close'].shift(1))
    features['volatility'] = returns.rolling(20).std() * np.sqrt(252)
    
    # 4. Momentum (5-day return)
    features['momentum_5d'] = df['Close'].pct_change(5)
    
    # 10-day momentum (longer term)
    features['momentum_10d'] = df['Close'].pct_change(10)
    
    # 5. Volume imbalance proxy (price change weighted by volume)
    features['volume_imbalance'] = (
        returns * df['Volume'] / df['Volume'].rolling(20).mean()
    )
    
    # 6. Order size (normalized)
    features['order_size'] = order_size / 1e6  # In millions
    
    # 7. Participation rate (order size / daily volume)
    features['participation_rate'] = order_size / df['Volume']
    
    # Log participation rate (handle extreme values)
    features['log_participation'] = np.log1p(features['participation_rate'])
    
    # 8. Volatility regime (binary: high/low)
    vol_median = features['volatility'].median()
    features['high_vol_regime'] = (features['volatility'] > vol_median).astype(int)
    
    # 9. Trend (simple: up/down based on 20-day MA)
    ma_20 = df['Close'].rolling(20).mean()
    features['uptrend'] = (df['Close'] > ma_20).astype(int)
    
    # Trend strength
    features['trend_strength'] = (df['Close'] - ma_20) / ma_20
    
    # Recent return (proxy for momentum continuation)
    features['return_1d'] = df['Close'].pct_change(1)
    
    # Volume trend
    ma_volume = df['Volume'].rolling(20).mean()
    features['volume_trend'] = (df['Volume'] - ma_volume) / ma_volume
    
    # 10. Hour of day (if intraday data available, else dummy)
    features['hour'] = 10  # Placeholder (assume mid-day execution)
    
    # Price level (normalized by recent max)
    features['price_level'] = df['Close'] / df['Close'].rolling(252).max()
    
    return features

def create_target(
    df: pd.DataFrame,
    order_size: int = 100000,
    execution_horizon: int = 10
) -> pd.Series:
    """
    Create target: market impact in basis points.
    
    Impact = (VWAP_execution - Price_start) / Price_start * 10000
    """
    impacts = []
    
    for i in range(len(df) - execution_horizon):
        start_price = df['Close'].iloc[i]
        
        # Compute VWAP over execution window
        window = df.iloc[i:i+execution_horizon]
        total_volume = window['Volume'].sum()
        if total_volume == 0 or start_price == 0:
            impacts.append(np.nan)
            continue
        vwap = (window['Close'] * window['Volume']).sum() / window['Volume'].sum()
        
        # Impact in bps
        impact_bps = (vwap - start_price) / start_price * 10000
        impacts.append(impact_bps)
    
    # Pad with NaN
    impacts.extend([np.nan] * execution_horizon)
    
    return pd.Series(impacts, index=df.index, name='market_impact_bps')


# ============================================================================
# Model Training
# ============================================================================

def split_data(
    X: pd.DataFrame,
    y: pd.Series,
    test_size: float = 0.2
) -> Tuple[pd.DataFrame, pd.DataFrame, pd.Series, pd.Series]:
    """
    Temporal train/test split (crucial for time series).
    """
    # Remove NaN
    valid_idx = ~(X.isna().any(axis=1) | y.isna())
    X = X[valid_idx]
    y = y[valid_idx]
    
    # Temporal split
    split_idx = int(len(X) * (1 - test_size))
    
    X_train = X.iloc[:split_idx]
    X_test = X.iloc[split_idx:]
    y_train = y.iloc[:split_idx]
    y_test = y.iloc[split_idx:]
    
    return X_train, X_test, y_train, y_test


def train_model(
    X_train: pd.DataFrame,
    y_train: pd.Series,
    config: ModelConfig
) -> lgb.LGBMRegressor:
    """
    Train LightGBM model with sensible defaults.
    """
    model = lgb.LGBMRegressor(
        n_estimators=config.n_estimators,
        learning_rate=config.learning_rate,
        max_depth=config.max_depth,
        random_state=config.random_state,
        verbose=-1
    )
    
    model.fit(X_train, y_train)
    
    return model


# ============================================================================
# Evaluation
# ============================================================================

def evaluate_model(
    model: lgb.LGBMRegressor,
    X_test: pd.DataFrame,
    y_test: pd.Series
) -> Dict[str, float]:
    """
    Compute key metrics.
    """
    y_pred = model.predict(X_test)
    
    mae = mean_absolute_error(y_test, y_pred)
    r2 = r2_score(y_test, y_pred)
    
    # Directional accuracy (important for trading)
    direction_correct = (np.sign(y_test) == np.sign(y_pred)).mean()
    
    return {
        'mae': mae,
        'r2': r2,
        'direction_accuracy': direction_correct
    }


def plot_diagnostics(
    model: lgb.LGBMRegressor,
    X_train: pd.DataFrame,
    X_test: pd.DataFrame,
    y_train: pd.Series,
    y_test: pd.Series,
    save_dir: Path
):
    """
    Create diagnostic plots (crucial for CFM interview).
    """
    save_dir.mkdir(parents=True, exist_ok=True)
    
    # 1. Feature importance
    plt.figure(figsize=(10, 6))
    importance = pd.DataFrame({
        'feature': X_train.columns,
        'importance': model.feature_importances_
    }).sort_values('importance', ascending=False)
    
    sns.barplot(data=importance, x='importance', y='feature')
    plt.title('Feature Importance')
    plt.tight_layout()
    plt.savefig(save_dir / 'feature_importance.png', dpi=300)
    plt.close()
    
    # 2. Predicted vs Actual (test set)
    y_pred_test = model.predict(X_test)
    
    plt.figure(figsize=(10, 6))
    plt.scatter(y_test, y_pred_test, alpha=0.5)
    plt.plot([y_test.min(), y_test.max()], [y_test.min(), y_test.max()], 
             'r--', lw=2, label='Perfect prediction')
    plt.xlabel('Actual Impact (bps)')
    plt.ylabel('Predicted Impact (bps)')
    plt.title('Predicted vs Actual Market Impact')
    plt.legend()
    plt.tight_layout()
    plt.savefig(save_dir / 'pred_vs_actual.png', dpi=300)
    plt.close()
    
    # 3. Residual analysis (KEY for CFM)
    residuals = y_test - y_pred_test
    
    fig, axes = plt.subplots(1, 2, figsize=(14, 5))
    
    # Residual histogram
    axes[0].hist(residuals, bins=50, edgecolor='black')
    axes[0].set_xlabel('Residuals (bps)')
    axes[0].set_title('Residual Distribution')
    axes[0].axvline(0, color='r', linestyle='--')
    
    # Residuals vs predicted
    axes[1].scatter(y_pred_test, residuals, alpha=0.5)
    axes[1].axhline(0, color='r', linestyle='--')
    axes[1].set_xlabel('Predicted Impact (bps)')
    axes[1].set_ylabel('Residuals (bps)')
    axes[1].set_title('Residuals vs Predicted')
    
    plt.tight_layout()
    plt.savefig(save_dir / 'residual_analysis.png', dpi=300)
    plt.close()
    
    # 4. Time series of errors (check for autocorrelation)
    plt.figure(figsize=(12, 4))
    plt.plot(residuals.values, alpha=0.7)
    plt.axhline(0, color='r', linestyle='--')
    plt.xlabel('Time')
    plt.ylabel('Residuals (bps)')
    plt.title('Residuals Over Time (Check for Patterns)')
    plt.tight_layout()
    plt.savefig(save_dir / 'residuals_time_series.png', dpi=300)
    plt.close()


# ============================================================================
# Main Pipeline
# ============================================================================
def run_simple_pipeline(
    data_path: str = "data/SP500.csv",
    order_size: int = 100000,
    execution_horizon: int = 10
):
    """
    Complete pipeline: data → features → model → evaluation.
    """
    print("=" * 60)
    print("SIMPLE MARKET IMPACT PREDICTION MODEL")
    print("=" * 60)
    
    config = ModelConfig()
    
    # 1. Load data
    print("\n1. Loading data...")
    df = load_data(DATA_PATH)
    
    # ✅ FIX: Ensure index is datetime
    if not isinstance(df.index, pd.DatetimeIndex):
        df.index = pd.to_datetime(df.index)
    
    # ✅ FIX: Now filter by date (using pd.Timestamp for safety)
    df = df[df.index >= pd.Timestamp('2015-01-01')]
    df = df[df['Volume'] > 0]
    
    print(f"   Loaded {len(df)} days of data (2015+ with volume > 0)")
    
    # 2. Create features & target
    print("\n2. Engineering features...")
    X = create_features(df, order_size=order_size)
    y = create_target(df, order_size=order_size, execution_horizon=execution_horizon)
    print(f"   Created {len(X.columns)} features")
    print(f"   Features: {list(X.columns)}")
    
    # 3. Split data (temporal)
    print("\n3. Splitting data (temporal)...")
    X_train, X_test, y_train, y_test = split_data(X, y, test_size=config.test_size)
    print(f"   Train: {len(X_train)} samples ({X_train.index.min().date()} to {X_train.index.max().date()})")
    print(f"   Test:  {len(X_test)} samples ({X_test.index.min().date()} to {X_test.index.max().date()})")
    
    # 4. Train model
    print("\n4. Training LightGBM model...")
    model = train_model(X_train, y_train, config)
    print("   Model trained ✓")
    
    # 5. Evaluate
    print("\n5. Evaluating model...")
    metrics = evaluate_model(model, X_test, y_test)
    print(f"   MAE:                 {metrics['mae']:.2f} bps")
    print(f"   R²:                  {metrics['r2']:.4f}")
    print(f"   Direction Accuracy:  {metrics['direction_accuracy']:.2%}")
    
    # ✅ ADD: Interpretation
    print("\n   📊 Interpretation:")
    if metrics['r2'] > 0.3:
        print("   ✅ Good predictive power")
    elif metrics['r2'] > 0.1:
        print("   ⚠️  Moderate predictive power")
    else:
        print("   ❌ Low predictive power - need better features")
    
    if metrics['direction_accuracy'] > 0.55:
        print("   ✅ Good directional prediction")
    elif metrics['direction_accuracy'] > 0.52:
        print("   ⚠️  Slight edge on direction")
    else:
        print("   ❌ No directional edge")
    
    # 6. Diagnostics
    print("\n6. Creating diagnostic plots...")
    plot_diagnostics(model, X_train, X_test, y_train, y_test, Path("ml/results"))
    print("   Plots saved to ml/results/")
    
    # 7. Feature importance
    print("\n7. Top 10 Most Important Features:")
    importance = pd.DataFrame({
        'feature': X_train.columns,
        'importance': model.feature_importances_
    }).sort_values('importance', ascending=False)
    
    for idx, row in importance.head(10).iterrows():
        print(f"   {row['feature']:25s} {row['importance']:.1f}")
    
    print("\n" + "=" * 60)
    print("PIPELINE COMPLETE ✓")
    print("=" * 60)
    
    return model, X_train, X_test, y_train, y_test, metrics


# ============================================================================
# Entry Point
# ============================================================================

if __name__ == "__main__":
    model, X_train, X_test, y_train, y_test, metrics = run_simple_pipeline()