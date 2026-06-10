# General Usage Instructions

This project is organized around several independently buildable features:

- [General Information](#general-information)
- [Core Limit Order Book Library and Tests](#build-and-run-unit-tests)
- [Throughput Benchmarks](#build-and-run-throughput-benchmarks)
- [Latency Benchmarks](#build-and-run-latency-benchmarks)
- [NASDAQ ITCH 5.0 Replay Tool](#build-and-run-the-nasdaq-itch-50-replay-tool)

All commands below assume they are run from the repository root.

## General Information

This order book implementation uses fixed-point arithmetic instead of decimal numbers for pricing. I use a **multiplier of 10000**, meaning prices have up to **4 points of decimal precision** (e.g. $1.2345 -> 12345). This is important for library and replay usage as all prices, including order book price ranges and order submission requests, will be specified in this manner.

## Build and Run Unit Tests

For quick test builds, use the following set of commands:

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DLOB_BUILD_TESTS=ON

cmake --build build -j

ctest --test-dir build --output-on-failure
```

Use this configuration to build the unit tests with **AddressSanitizer** and **UndefinedBehaviorSanitizer** enabled.

```
cmake -S . -B build-sanitize \
  -DCMAKE_BUILD_TYPE=Debug \
  -DLOB_BUILD_TESTS=ON \
  -DLOB_ENABLE_SANITIZERS=ON

cmake --build build-sanitize -j

ctest --test-dir build-sanitize --output-on-failure
```

## Build and Run Throughput Benchmarks

The following command is used to build the throughput benchmarks:

```
cmake -S . -B build-bench \
  -DCMAKE_BUILD_TYPE=Release \
  -DLOB_BUILD_TESTS=OFF \
  -DLOB_BUILD_BENCHMARKS=ON

cmake --build build-bench -j
```

Also, ideally before benchmarks are run, the following should be run as well:

```
sudo cpupower frequency-set -g performance
```

### Run Benchmarks Normally

This method of running the throughput benchmarks does not output new files and pushes all GoogleBenchmark artifacts to the terminal.

```
./build-bench/bench/individual_behaviours/Bench
```

### Run Benchmarks and Record Performance Output

This method of running the throughput benchmarks will output a summary file. If the user chooses, they can tweak the output file/format.

```
taskset -c 2 nice -n -20 \
  ./build-bench/bench/individual_behaviours/Bench \
  --benchmark_min_time=1s \
  --benchmark_min_warmup_time=0.5s \
  --benchmark_repetitions=5 \
  --benchmark_report_aggregates_only=true \
  --benchmark_out=results/throughput/sample_run.json \
  --benchmark_out_format=json
```

## Build and Run Latency Benchmarks

Latency benchmark data is written to a simple CSV in `results/latency/resting_limit_submission_latencies.csv`

```
cmake -S . -B build-latency \
  -DCMAKE_BUILD_TYPE=Release \
  -DLOB_BUILD_TESTS=OFF \
  -DLOB_BUILD_LATENCY=ON

cmake --build build-latency -j

taskset -c 2 nice -n -20 ./build-latency/bench/latency/Latency
```

## Build and Run the NASDAQ ITCH 5.0 Replay Tool

To use this tool, the relevant data must be downloaded. The following script to import and extract the data **must** be run:

```
./scripts/replay/download_itch_sample.sh
```

It outputs to a new directory, `data/full/03272019.NASDAQ_ITCH50.bin`

To build the replay tool, use the following commands:

```
cmake -S . -B build-replay \
  -DCMAKE_BUILD_TYPE=Release \
  -DLOB_BUILD_TESTS=OFF \
  -DLOB_BUILD_REPLAY=ON

cmake --build build-replay -j
```

### Replay a Full Trading Day

Runs the full day of trading data through the order book. Along with the input, provide a valid NASDAQ stock symbol (e.g. AAPL), an initial pool size, minimum and maximum price brounds.

```
./build-replay/replay/Replay \
  data/full/03272019.NASDAQ_ITCH50.bin \
  AAPL \
  2000000 \
  1 \
  10000000
```

Note that running for a full day's data may leave the book empty. End of day cleanup removes remaining visible orders.

### Capture an Intraday Snapshot

As a final input parameter, input a stop time in HH:MM:SS to halt replay at the specified time.

```
./build-replay/replay/Replay \
  data/full/03272019.NASDAQ_ITCH50.bin \
  AAPL \
  2000000 \
  1 \
  10000000 \
  --stop-at 10:00:00
```

Likely, a richer output will be displayed, including populated L1 and L2 depth reports.