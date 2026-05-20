import json
import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

target_file_base = Path(__file__).parents[2] / "results" / "raw" / "base" / "base.json"
target_file_optimized = Path(__file__).parents[2] / "results" / "raw" / "optimizations" / "dense_price_ladders.json"

with open(target_file_base, "r") as file:
    json_file_base = json.load(file)

with open(target_file_optimized, "r") as file:
    json_file_optimized = json.load(file)

benchmark_names_base = []
order_quantity_base = []
throughputs_base = []
latencies_base = []

for benchmark in json_file_base["benchmarks"]:
    benchmark_names_base.append(benchmark["name"].split("/")[0])
    order_quantity_base.append(int(benchmark["name"].split("/")[-1]))
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
    benchmark_names_optimized.append(benchmark["name"].split("/")[0])
    order_quantity_optimized.append(int(benchmark["name"].split("/")[-1]))
    throughputs_optimized.append(int(benchmark["items_per_second"]))
    latencies_optimized.append(int(benchmark["real_time"]))

optimized_performance = pd.DataFrame({
    "Benchmark Name": benchmark_names_optimized,
    "Order Quantity": order_quantity_optimized,
    "Throughput (items/second)": throughputs_optimized,
    "Latency (ns)": latencies_optimized
})

output_dir = Path(__file__).parents[2] / "docs" / "plots" / "dense_price_ladders"

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

top_benchmark_count = 8
largest_speedup_count = 12

top_benchmarks = (
    comparison.groupby("Benchmark Name")["Throughput Speedup"]
    .max()
    .sort_values(ascending=False)
    .head(top_benchmark_count)
    .index
)

top_speedup_comparison = comparison[
    comparison["Benchmark Name"].isin(top_benchmarks)
].copy()

fig, ax = plt.subplots(figsize=(14, 7))

for benchmark_name in top_benchmarks:
    benchmark_slice = top_speedup_comparison[
        top_speedup_comparison["Benchmark Name"] == benchmark_name
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
ax.set_title(
    f"Dense Price Ladder Throughput Speedup\n"
    f"Top {top_benchmark_count} Benchmark Families by Maximum Observed Speedup"
)
ax.legend(
    bbox_to_anchor=(1.02, 1),
    loc="upper left",
    fontsize=8
)

fig.tight_layout()

output_path = output_dir / "dense_price_ladders_top_speedups.png"
fig.savefig(output_path, dpi=200, bbox_inches="tight")
plt.close(fig)


largest_speedups = (
    comparison.sort_values("Throughput Speedup", ascending=False)
    .head(largest_speedup_count)
    .copy()
)

largest_speedups["Benchmark Label"] = (
    largest_speedups["Benchmark Name"]
    + " / "
    + largest_speedups["Order Quantity"].map(lambda value: f"{value:,}")
)

fig, ax = plt.subplots(figsize=(14, 8))

ax.barh(
    largest_speedups["Benchmark Label"][::-1],
    largest_speedups["Throughput Speedup"][::-1]
)

ax.axvline(1.0, linestyle="--", linewidth=1)

ax.set_xlabel("Throughput Speedup vs Baseline")
ax.set_ylabel("Benchmark / Order Quantity")
ax.set_title("Largest Dense Price Ladder Throughput Improvements")

fig.tight_layout()

output_path = output_dir / "dense_price_ladders_largest_speedups.png"
fig.savefig(output_path, dpi=200, bbox_inches="tight")
plt.close(fig)