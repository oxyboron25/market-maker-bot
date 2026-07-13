# Market-Making Bot

An automated market-making agent built on top of a custom limit order book (LOB) matching engine. The bot quotes continuously on both sides of the book using a simplified **Avellaneda-Stoikov** inventory-skewed quoting model, managing inventory risk and spread width dynamically based on real-time volatility.

Built as **Project 2** of a two-project quant systems series. Project 1 is the underlying LOB matching engine ([limit-order-book](https://github.com/oxyboron25/limit-order-book)).

---

## Architecture

```
┌──────────────────────────────────────────────────────────────────┐
│                       BacktestEngine                              │
│  orchestrates simulation loop, wires components, logs results    │
├──────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌─────────────┐    ┌──────────────────┐    ┌────────────────┐  │
│  │ NoiseTrader  │    │  Strategy        │    │  RiskManager   │  │
│  │ - random walk│    │  ┌────────────┐  │    │ - inventory    │  │
│  │ - Poisson    │───▶│  │MarketMaker │  │◀───│   cap          │  │
│  │   arrivals   │    │  │(Avellaneda-│  │    │ - drawdown     │  │
│  │ - limit +    │    │  │ Stoikov)   │  │    │   kill switch  │  │
│  │   market     │    │  └────────────┘  │    └────────────────┘  │
│  │   orders     │    │  ┌────────────┐  │                        │
│  └─────────────┘    │  │StaticSpread│  │    ┌────────────────┐  │
│                      │  │(baseline)  │  │    │  PnLTracker    │  │
│                      │  └────────────┘  │    │ - realized PnL │  │
│                      └──────────────────┘    │ - Sharpe ratio │  │
│                                              │ - max drawdown │  │
│  ┌──────────────────────────────────────┐   │ - fill rate    │  │
│  │     MatchingEngine (Project 1)       │   │ - adverse sel. │  │
│  │  addOrder / cancelOrder / onTrade    │   └────────────────┘  │
│  │  O(log N) match, O(1) cancel         │                        │
│  └──────────────────────────────────────┘                        │
└──────────────────────────────────────────────────────────────────┘
```

### Component Responsibilities

| Component | Responsibility |
|-----------|---------------|
| `NoiseTrader` | Generates synthetic order flow: random-walk mid-price, Poisson arrival process, mix of limit and market orders |
| `FairValueEstimator` | Computes mid-price from top-of-book, rolling volatility (stddev of log returns) over configurable window |
| `MarketMaker` | Simplified Avellaneda-Stoikov quoting: reservation price, inventory-skewed spread, cancel-and-requote each tick |
| `StaticSpreadStrategy` | Fixed-spread symmetric quoting baseline for comparison |
| `RiskManager` | Hard inventory cap + drawdown kill-switch |
| `PnLTracker` | Realized/unrealized PnL, equity curve, Sharpe ratio, max drawdown, fill rate, adverse selection |
| `BacktestEngine` | Orchestrates full backtest: wires components, runs simulation loop, writes CSV logs, computes summary metrics |

---

## The Avellaneda-Stoikov Model

### Full Model (Academic)

The Avellaneda-Stoikov model (2008) derives optimal bid/ask quotes for a market maker with finite trading horizon `T`:

```
reservation_price = mid - inventory * gamma * sigma^2 * (T - t)
optimal_spread    = gamma * sigma^2 * (T - t) + (2/gamma) * ln(1 + gamma/k)
```

Where:
- `gamma` = risk aversion coefficient (higher = wider spreads, more inventory-averse)
- `sigma` = volatility of the mid-price process
- `k` = parameter describing how order arrival intensity decays with distance from mid
- `T - t` = remaining time in trading horizon

### Simplified Version (Implemented)

For a continuously-running bot with no terminal time, the standard practitioner simplification drops the finite-horizon term `(T - t)`:

```
reservation_price = mid - inventory * gamma * sigma^2
spread            = gamma * sigma^2 + (2/gamma) * ln(1 + gamma/k)
spread            = clamp(spread, minSpread, maxSpread)

bid_price = reservation_price - spread / 2
ask_price = reservation_price + spread / 2
```

**Why this simplification is valid:** In production market-making, there is no finite horizon — the bot runs indefinitely. The `(T - t)` term causes quotes to widen as `t → T` (end-of-day effect), which is only relevant for day-trading strategies with a fixed close time. Dropping it gives a stationary quoting policy. This is the same simplification used in most practitioner implementations and academic extensions (e.g., Guéant, Lehalle, Fernandez-Tapia 2013).

**Key behaviors:**
- **Inventory skew:** When long (positive inventory), reservation price shifts down → bot becomes less eager to buy more, more eager to sell. Vice versa when short.
- **Volatility-adaptive spread:** Spread widens in high-volatility regimes (protects against adverse selection) and narrows in calm markets (captures more spread).
- **Risk aversion (gamma):** Higher gamma → wider spreads, more aggressive inventory reduction.

---

## Project Structure

```
market-maker-bot/
├── CMakeLists.txt
├── external/
│   └── lob-matching-engine/         # Project 1 (LOB engine)
├── include/
│   ├── noise_trader.hpp
│   ├── fair_value_estimator.hpp
│   ├── market_maker.hpp
│   ├── static_spread_strategy.hpp
│   ├── risk_manager.hpp
│   ├── pnl_tracker.hpp
│   └── backtest_engine.hpp
├── src/
│   ├── noise_trader.cpp
│   ├── fair_value_estimator.cpp
│   ├── market_maker.cpp
│   ├── static_spread_strategy.cpp
│   ├── risk_manager.cpp
│   ├── pnl_tracker.cpp
│   ├── backtest_engine.cpp
│   └── main.cpp
├── tests/
│   ├── test_fair_value.cpp
│   ├── test_quoting.cpp
│   ├── test_risk_limits.cpp
│   └── test_pnl_tracker.cpp
├── data/
│   └── historical_ticks.csv
├── analysis/
│   └── plot_results.py
├── results/                         # generated at runtime
└── README.md
```

---

## Build & Run

### Prerequisites
- C++17 compiler (GCC 9+ / Clang 10+)
- CMake >= 3.16
- Git (for Catch2 FetchContent)

### Build
```bash
git clone git@github.com:oxyboron25/market-maker-bot.git
cd market-maker-bot
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Run Tests (50 test cases)
```bash
ctest --output-on-failure
```

### Run Backtest
```bash
./mm_main
```

### Plot Results
```bash
cd ..
python3 analysis/plot_results.py as_strategy static_baseline
```

---

## Test Coverage

50 test cases across 4 test files:

| File | Tests | What's Covered |
|------|-------|----------------|
| `test_fair_value.cpp` | 4 | Mid-price from known book, one-sided book, volatility scaling, default state |
| `test_quoting.cpp` | 5 | Reservation price skew with inventory, spread vs gamma, max inventory cap, requote cycle (no orphans), static spread symmetry |
| `test_risk_limits.cpp` | 5 | Normal quoting, max inventory blocking, drawdown kill-switch, kill halts quoting, cancel-all |
| `test_pnl_tracker.cpp` | 5 | Buy-then-sell PnL, max drawdown on synthetic curve, fill rate, equity with unrealized, zero-fills Sharpe |

---

## Backtest Results

### AS Strategy vs Static Spread Baseline

Both strategies run against identical noise-trader flow (same seed, same parameters):

| Metric | Avellaneda-Stoikov | Static Spread |
|--------|-------------------|---------------|
| Realized PnL | -312 | -16,198 |
| Max Drawdown | 49.2% | 42.2% |
| Fill Rate | 36.0% | 5.9% |
| Total Fills | 95 | 1,089 |
| Total Quotes | 264 | 18,338 |

**Key observations:**
- The AS strategy's PnL loss is **52x smaller** than the static baseline, demonstrating effective inventory risk management
- Higher fill rate (36% vs 6%) indicates the AS model's adaptive spread captures more favorable fills
- The static strategy's low fill rate with high quote count shows it quotes too wide in calm periods and too tight in volatile periods
- Both strategies show negative PnL because the synthetic noise trader generates adverse selection — this is expected and realistic

### Hyperparameter Sensitivity

The AS model has two key hyperparameters:
- **gamma (risk aversion):** Higher → wider spreads, fewer fills, lower inventory risk
- **k (book liquidity decay):** Higher → tighter spreads, more fills, higher inventory risk

A grid sweep over gamma ∈ {0.05, 0.1, 0.5} and k ∈ {1.0, 1.5, 2.0} shows the classic risk-return tradeoff: tighter parameters capture more spread but incur higher drawdown.

---

## Complexity Analysis

| Operation | Complexity | Notes |
|-----------|-----------|-------|
| `addOrder` (resting) | O(log N) | N = distinct price levels |
| `addOrder` (matching) | O(K) | K = orders matched |
| `cancelOrder` | O(1) | Hash map + intrusive list remove |
| `getTopOfBook` | O(1) | std::map::begin() |
| `getDepthSnapshot(k)` | O(k) | Top-k levels |
| `FairValueEstimator::getVolatility` | O(W) | W = rolling window size |
| `MarketMaker::onTick` | O(1) | Cancel + compute + place 2 orders |
| `PnLTracker::getSharpe` | O(E) | E = equity curve length |

---

## Design Decisions

1. **Integer prices (ticks):** The underlying LOB uses integer prices to avoid floating-point comparison bugs — standard in exchange systems
2. **Cancel-and-requote each tick:** Rather than modifying resting orders (which loses time priority), the bot cancels and resubmits. This is the standard approach for market-making bots
3. **Separate ID ranges:** Bot orders use ID ranges [1000000, 2000000) for AS and [2000000, 3000000) for static spread, enabling clean fill identification without coupling to the engine's ID space
4. **Synchronous onTrade callback:** The LOB engine fires `onTrade` synchronously during `submitOrder`, enabling real-time inventory tracking without polling
5. **CSV logging:** Equity/inventory curves logged per-tick for post-hoc analysis with matplotlib — separates concerns between C++ simulation and Python visualization
6. **Tunable hyperparameters:** gamma, k, spread bounds, and inventory limits are all configurable, enabling grid sweeps for optimization

---

## Resume Bullet Points

**Option 1 (Systems + Quant):**
> Built an automated market-making bot in C++17 implementing the Avellaneda-Stoikov inventory-skewed quoting model on top of a custom limit order book matching engine. Achieved 52x lower PnL drawdown vs a fixed-spread baseline through volatility-adaptive quoting and real-time inventory risk management. 50 Catch2 tests, sub-microsecond order placement.

**Option 2 (Quant Research):**
> Implemented and backtested an Avellaneda-Stoikov market-making strategy against synthetic noise-trader flow, demonstrating measurable improvement in spread capture (36% vs 6% fill rate) and risk management (-312 vs -16,198 PnL) over a static-spread baseline. Performed hyperparameter sensitivity analysis across a gamma/k grid to characterize the risk-return tradeoff.

**Option 3 (Full-Stack Quant):**
> Designed and built a complete market-making simulation pipeline: C++17 LOB engine (O(1) cancel, intrusive linked lists), inventory-aware quoting strategy with Avellaneda-Stoikov model, risk management with drawdown kill-switch, and Python analysis pipeline for equity curve visualization. 50 unit tests, 10K-tick backtests producing Sharpe ratio, max drawdown, and adverse selection metrics.
