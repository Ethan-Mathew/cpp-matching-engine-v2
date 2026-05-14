import json
import re
import pandas as pd
from pathlib import Path

target_file_base = Path(__file__).parents[2] / "results" / "raw" / "base" / "all_individual_behaviours_benchmark.json"
target_file_optimized = Path(__file__).parents[2] / "results" / "raw" / "optimizations" / "session_end_callback_pruning.json"

with open(target_file_base, "r") as file:
    json_file_base = json.load(file)

with open(target_file_optimized, "r") as file:
    json_file_optimized = json.load(file)

REGEX_BM_Session_End = "^BM_SessionEnd"
REGEX_benchmark_full_name = "^(BM_SessionEnd)([A-Za-z]+)"
REGEX_order_quantity = "\d+"

benchmark_names_base = []
order_quantity_base = []
throughputs_base = []
latencies_base = []

for benchmark in json_file_base["benchmarks"]:
    if re.search(REGEX_BM_Session_End, benchmark["name"]):
        benchmark_names_base.append(re.search(REGEX_benchmark_full_name, benchmark["name"]).group())
        order_quantity_base.append(int(re.search(REGEX_order_quantity, benchmark["name"]).group()))
        throughputs_base.append(int(benchmark["items_per_second"]))
        latencies_base.append(int(benchmark["real_time"]))

base_performance = pd.DataFrame({
    "Benchmark Name": benchmark_names_base,
    "Order Quantity": order_quantity_base,
    "Throughput (items/second)": throughputs_base,
    "Latency (ns)": latencies_base
})

benchmark_names_optimized = []
order_quantity_optimized = []
throughputs_optimized = []
latencies_optimized = []

for benchmark in json_file_optimized["benchmarks"]:
    benchmark_names_optimized.append(re.search(REGEX_benchmark_full_name, benchmark["name"]).group())
    order_quantity_optimized.append(int(re.search(REGEX_order_quantity, benchmark["name"]).group()))
    throughputs_optimized.append(int(benchmark["items_per_second"]))
    latencies_optimized.append(int(benchmark["real_time"]))

optimized_performance = pd.DataFrame({
    "Benchmark Name": benchmark_names_optimized,
    "Order Quantity": order_quantity_optimized,
    "Throughput (items/second)": throughputs_optimized,
    "Latency (ns)": latencies_optimized
})