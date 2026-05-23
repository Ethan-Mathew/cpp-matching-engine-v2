import json
import re
import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

target_file_base = Path(__file__).parents[2] / "results" / "raw" / "base" / "base.json"
target_file_optimized = Path(__file__).parents[2] / "results" / "raw" / "optimizations" / "session_end_callback_pruning.json"

with open(target_file_base, "r") as file:
    json_file_base = json.load(file)

with open(target_file_optimized, "r") as file:
    json_file_optimized = json.load(file)

REGEX_BM_Session_End = r"^BM_SessionEnd"
REGEX_benchmark_full_name = r"^(BM_SessionEnd)([A-Za-z]+)"
REGEX_order_quantity = r"\d+"

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
    if re.search(REGEX_BM_Session_End, benchmark["name"]):
        benchmark_names_optimized.append(
            re.search(REGEX_benchmark_full_name, benchmark["name"]).group()
        )
        order_quantity_optimized.append(
            int(re.search(REGEX_order_quantity, benchmark["name"]).group())
        )
        throughputs_optimized.append(int(benchmark["items_per_second"]))
        latencies_optimized.append(int(benchmark["real_time"]))

optimized_performance = pd.DataFrame({
    "Benchmark Name": benchmark_names_optimized,
    "Order Quantity": order_quantity_optimized,
    "Throughput (items/second)": throughputs_optimized,
    "Latency (ns)": latencies_optimized
})

output_dir = Path(__file__).parents[2] / "docs" / "plots" / "session_end_callback_pruning"

comparison = base_performance.merge(
    optimized_performance,
    on=["Benchmark Name", "Order Quantity"],
    suffixes=("_Base", "_Optimized")
)

comparison["Throughput Speedup"] = (
    comparison["Throughput (items/second)_Optimized"] /
    comparison["Throughput (items/second)_Base"]
)

comparison["Latency Reduction (%)"] = (
    1 -
    comparison["Latency (ns)_Optimized"] /
    comparison["Latency (ns)_Base"]
) * 100

for order_quantity in sorted(comparison["Order Quantity"].unique()):
    quantity_slice = comparison[
        comparison["Order Quantity"] == order_quantity
    ].copy()

    quantity_slice = quantity_slice.sort_values("Benchmark Name")

    benchmark_names = quantity_slice["Benchmark Name"]
    x_positions = range(len(benchmark_names))
    bar_width = 0.35

    fig, ax = plt.subplots(figsize=(12, 6))

    ax.bar(
        [x - bar_width / 2 for x in x_positions],
        quantity_slice["Throughput (items/second)_Base"],
        width=bar_width,
        label="Baseline"
    )

    ax.bar(
        [x + bar_width / 2 for x in x_positions],
        quantity_slice["Throughput (items/second)_Optimized"],
        width=bar_width,
        label="Callback Pruning"
    )

    ax.set_xticks(list(x_positions))
    ax.set_xticklabels(
        benchmark_names,
        rotation=30,
        ha="right"
    )

    ax.set_ylabel("Throughput (items/second)")
    ax.set_title(
        f"Session-End Throughput Comparison\n"
        f"Order Quantity = {order_quantity:,}"
    )
    ax.legend()

    fig.tight_layout()

    output_path = (
        output_dir /
        f"session_end_callback_pruning_throughput_{order_quantity}.png"
    )

    fig.savefig(output_path, dpi=200)
    plt.close(fig)

fig, ax = plt.subplots(figsize=(12, 6))

for benchmark_name in sorted(comparison["Benchmark Name"].unique()):
    benchmark_slice = comparison[
        comparison["Benchmark Name"] == benchmark_name
    ].sort_values("Order Quantity")

    ax.plot(
        benchmark_slice["Order Quantity"],
        benchmark_slice["Throughput Speedup"],
        marker="o",
        label=benchmark_name
    )

ax.axhline(1.0, linestyle="--", linewidth=1)

ax.set_xscale("log")
ax.set_xlabel("Order Quantity")
ax.set_ylabel("Throughput Speedup vs Baseline")
ax.set_title("Session-End Callback Pruning Throughput Speedup")
ax.legend()

fig.tight_layout()

output_path = (
    output_dir /
    "session_end_callback_pruning_speedup.png"
)

fig.savefig(output_path, dpi=200)
plt.close(fig)