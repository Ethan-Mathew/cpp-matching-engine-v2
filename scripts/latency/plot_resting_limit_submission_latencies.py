import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

input_file = Path(__file__).parents[2] / "results" / "latency" / "resting_limit_submission_latencies.csv"
output_dir = Path(__file__).parents[2] / "docs" / "plots" / "latency"

output_dir.mkdir(parents=True, exist_ok=True)

latencies = pd.read_csv(input_file)["latency_ns"]

p50 = latencies.quantile(0.50)
p90 = latencies.quantile(0.90)
p99 = latencies.quantile(0.99)
p999 = latencies.quantile(0.999)
p9999 = latencies.quantile(0.9999)
mean = latencies.mean()
max_latency = latencies.max()

print("Resting Limit Submission Latency Summary")
print("----------------------------------------")
print(f"Samples: {len(latencies):,}")
print(f"Mean:    {mean:.2f} ns")
print(f"p50:     {p50:.0f} ns")
print(f"p90:     {p90:.0f} ns")
print(f"p99:     {p99:.0f} ns")
print(f"p99.9:   {p999:.0f} ns")
print(f"p99.99:  {p9999:.0f} ns")
print(f"Max:     {max_latency:.0f} ns")

bucket_edges = [
    0,
    50,
    75,
    100,
    250,
    500,
    1_000,
    5_000,
    10_000,
    50_000,
    100_000,
    1_000_000,
    10_000_000,
    np.inf,
]

bucket_labels = [
    "≤50 ns",
    "51–75 ns",
    "76–100 ns",
    "101–250 ns",
    "251–500 ns",
    "501 ns–1 µs",
    "1–5 µs",
    "5–10 µs",
    "10–50 µs",
    "50–100 µs",
    "100 µs–1 ms",
    "1–10 ms",
    ">10 ms",
]

latency_buckets = pd.cut(
    latencies,
    bins=bucket_edges,
    labels=bucket_labels,
    right=True,
    include_lowest=True,
)

bucket_counts = latency_buckets.value_counts(sort=False)
bucket_percentages = bucket_counts / len(latencies) * 100

bucket_summary = pd.DataFrame({
    "Bucket": bucket_labels,
    "Count": bucket_counts.values,
    "Percentage": bucket_percentages.values,
})

fig, ax = plt.subplots(figsize=(14, 7))

ax.bar(
    bucket_summary["Bucket"],
    bucket_summary["Count"]
)

ax.set_yscale("log")
ax.set_xlabel("Latency Bucket")
ax.set_ylabel("Operation Count, log scale")
ax.set_title("Resting Limit Submission Latency by Bucket")
ax.tick_params(axis="x", rotation=35)

for index, row in bucket_summary.iterrows():
    if row["Count"] > 0:
        ax.text(
            index,
            row["Count"],
            f'{row["Percentage"]:.3f}%',
            ha="center",
            va="bottom",
            fontsize=8,
            rotation=90
        )

fig.tight_layout()
fig.savefig(
    output_dir / "resting_limit_submission_latency_buckets.png",
    dpi=200
)
plt.close(fig)


# ---------------------------------------------------------------------
# 2. CCDF tail plot
# ---------------------------------------------------------------------

value_counts = latencies.value_counts().sort_index()

tail_counts = value_counts.iloc[::-1].cumsum().iloc[::-1]
tail_probability = tail_counts / len(latencies)

fig, ax = plt.subplots(figsize=(12, 6))

ax.step(
    value_counts.index,
    tail_probability,
    where="post"
)

ax.axvline(p50, linestyle="--", linewidth=1, label=f"p50 = {p50:.0f} ns")
ax.axvline(p99, linestyle="--", linewidth=1, label=f"p99 = {p99:.0f} ns")
ax.axvline(p999, linestyle="--", linewidth=1, label=f"p99.9 = {p999:.0f} ns")
ax.axvline(p9999, linestyle="--", linewidth=1, label=f"p99.99 = {p9999:.0f} ns")

ax.axhline(0.01, linestyle=":", linewidth=1)
ax.axhline(0.001, linestyle=":", linewidth=1)
ax.axhline(0.0001, linestyle=":", linewidth=1)

ax.set_xscale("log")
ax.set_yscale("log")

ax.set_xlabel("Latency Threshold (ns, log scale)")
ax.set_ylabel("P(latency ≥ threshold), log scale")
ax.set_title("Resting Limit Submission Latency Tail Distribution")
ax.legend()

fig.tight_layout()
fig.savefig(
    output_dir / "resting_limit_submission_latency_ccdf.png",
    dpi=200
)
plt.close(fig)

body_cutoff = p99
body_latencies = latencies[latencies <= body_cutoff]

fig, ax = plt.subplots(figsize=(12, 6))

ax.hist(
    body_latencies,
    bins=100
)

ax.axvline(p50, linestyle="--", linewidth=1, label=f"p50 = {p50:.0f} ns")
ax.axvline(p90, linestyle="--", linewidth=1, label=f"p90 = {p90:.0f} ns")
ax.axvline(p99, linestyle="--", linewidth=1, label=f"p99 = {p99:.0f} ns")

ax.set_xlabel("Latency (ns)")
ax.set_ylabel("Operation Count")
ax.set_title("Resting Limit Submission Latency Distribution Through p99")
ax.legend()

fig.tight_layout()
fig.savefig(
    output_dir / "resting_limit_submission_latency_body_p99.png",
    dpi=200
)
plt.close(fig)