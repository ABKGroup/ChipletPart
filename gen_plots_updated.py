import matplotlib.pyplot as plt
import numpy as np

# ----------------------------
# 1. Hyperparameter Settings
# ----------------------------
params_dict = {
    'Population Size': (np.array([10, 20, 30, 40, 50, 60, 70]), 50),
    'Generations':     (np.array([10, 20, 30, 40, 50, 60, 70]), 50),
    'Mutation Rate':   (np.array([0.05, 0.06, 0.07, 0.08, 0.09, 0.10, 0.11]), 0.07),
    'Crossover Prob.': (np.array([0.45, 0.5, 0.55, 0.6, 0.65, 0.7, 0.75]), 0.6)
}

benchmarks = ['WS1', 'WS2', 'TC1', 'TC2']
colors = ['blue', 'green', 'red', 'purple']
ref_costs = [71.08, 145.5, 53.14, 49.48]
runtime_norm = 10000.0

# ----------------------------
# 2. Cost and Runtime Data
# ----------------------------
cost_data = {
    'Population Size': [
        [56.41, 57.85, 57.76, 56.7, 55.7, 56.1, 56.1],
        [126.56, 129.19, 126.34, 122.24, 121.29, 120.11, 120.11],
        [55.49, 54.11, 53.23, 51.12, 50.79, 50.79, 50.79],
        [53.31, 52.01, 51.14, 49.95, 49.02, 49.02, 49.02]
    ],
    'Generations': [
        [59.43, 58.85, 57.76, 56.7, 54.7, 54.1, 54.1],
        [131.25, 127.22, 124.19, 121.29, 121.29, 121.11, 121.11],
        [51.11, 51.11, 51.05, 50.86, 50.79, 50.79, 50.79],
        [50.31, 50.03, 49.95, 49.76, 49.02, 49.01, 49.01]
    ],
    'Mutation Rate': [
        [55.64, 55.64, 54.1, 56.41, 56.7, 57.29, 59.14],
        [129.45, 128.63, 124.29, 125.29, 126.21, 129.11, 129.22],
        [53.73, 52.42, 52.79, 53.15, 53.79, 56.95, 54.96],
        [49.99, 49.21, 49.02, 50.41, 50.02, 54.16, 53.33]
    ],
    'Crossover Prob.': [
        [66.29, 63.64, 54.1, 53.1, 54.1, 57.1, 58.2],
        [124.13, 122.19, 122.29, 121.29, 121.29, 123.21, 124.21],
        [52.76, 52.19, 50.95, 50.79, 50.79, 51.21, 52.33],
        [53.26, 50.21, 49.12, 49.02, 49.02, 50.14, 53.92]
    ]
}

runtime_data = {
    'Population Size': [
        [573, 613, 653, 703, 773, 873, 1073],
        [3659, 3969, 4119, 4759, 5759, 9759, 10759],
        [16, 25, 22, 34, 19, 32, 40],
        [20, 28, 27, 33, 22, 38, 49]
    ],
    'Generations': [
        [473, 513, 603, 653, 773, 973, 1273],
        [2659, 3369, 3919, 4459, 4759, 8759, 11759],
        [11, 11, 16, 19, 19, 32, 39],
        [14, 19, 29, 33, 37, 48, 52]
    ],
    'Mutation Rate': [
        [673, 663, 671, 663, 653, 656, 662],
        [4219, 4269, 4219, 4219, 4229, 4239, 4219],
        [21, 21, 20, 20, 19, 19, 20],
        [34, 39, 37, 35, 37, 38, 38]
    ],
    'Crossover Prob.': [
        [654, 656, 656, 654, 670, 676, 682],
        [4216, 4339, 4459, 4414, 4589, 4569, 4572],
        [19, 21, 19, 18, 19, 19, 20],
        [30, 31, 33, 32, 34, 36, 37]
    ]
}

# ----------------------------
# 3. Generate Separate Plots
# ----------------------------
for idx, (hyperparam, (x_vals, default)) in enumerate(params_dict.items()):
    fig, ax = plt.subplots(figsize=(10, 7))
    for j, bench in enumerate(benchmarks):
        cost = np.array(cost_data[hyperparam][j]) / ref_costs[j]
        runtime = np.array(runtime_data[hyperparam][j]) / runtime_norm
        ax.plot(x_vals, cost, 'o-', color=colors[j], label=f"{bench} Cost", markersize=10)
        ax.plot(x_vals, runtime, 's--', color=colors[j], alpha=0.5, label=f"{bench} Runtime", markersize=10)

    ax.axvline(x=default, color='black', linestyle='--', linewidth=2, label='Default')
    ax.set_xlabel(hyperparam, fontsize=22, fontweight='bold')
    ax.set_ylabel("Normalized Value", fontsize=22, fontweight='bold')
    ax.tick_params(axis='both', labelsize=20)
    ax.grid(True)
    ax.legend(loc='upper right', fontsize=12)
    fig.tight_layout()
    fig.savefig(f"hyperparam_sweep_{hyperparam.replace(' ', '_')}.png", dpi=300)
    plt.close(fig)  # Close to avoid memory issues

