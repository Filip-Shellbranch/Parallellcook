import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Load CSV
df = pd.read_csv("Exercise_5/results.csv")

# Create grid
pivot = df.pivot(index="N", columns="T", values="Real_Time_Sec")

T = pivot.columns.values
N = pivot.index.values

T_grid, N_grid = np.meshgrid(T, N)
Z = pivot.values

# Create 3D figure
fig = plt.figure(figsize=(10, 7))
ax = fig.add_subplot(111, projection="3d")

# Surface
surface = ax.plot_surface(
    T_grid,
    N_grid,
    Z,
    cmap="viridis",
    edgecolor="k",
    linewidth=0.3
)

# Labels
ax.set_xlabel("T")
ax.set_ylabel("N")
ax.set_zlabel("Real Time (sec)")
ax.set_title("Benchmark Performance")

# Use logarithmic axes because T and N are powers of 2
ax.set_xscale("log", base=2)
ax.set_yscale("log", base=2)

# Color bar
fig.colorbar(
    surface,
    ax=ax,
    shrink=0.6,
    label="Real Time (sec)"
)

plt.tight_layout()

# Save instead of plt.show()
plt.savefig("Exercise_5/benchmark_3d.png", dpi=300, bbox_inches="tight")

print("Saved plot to benchmark_3d.png")
