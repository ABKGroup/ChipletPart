import matplotlib.pyplot as plt
import numpy as np

# --------------------------
# 1. Sensitivity Data
# --------------------------
labels = [
    "Population Size", 
    "Generations", 
    "Tournament Size", 
    "Crossover Probability",
    "Mutation Rate"
]

cost_sensitivity = np.array([0.081, 0.065, 0.034, 0.141, 0.132])
runtime_sensitivity = np.array([0.15, 0.110, 0.004, 0.003, 0.003])

# --------------------------
# 2. Prepare Data for Radar Chart
# --------------------------
labels_radar = labels + [labels[0]]
cost_radar = np.append(cost_sensitivity, cost_sensitivity[0])
runtime_radar = np.append(runtime_sensitivity, runtime_sensitivity[0])
angles = np.linspace(0, 2 * np.pi, len(labels), endpoint=False).tolist()
angles += angles[:1]

# --------------------------
# 3. Create Radar Plot
# --------------------------
plt.style.use("ggplot")
fig, ax = plt.subplots(figsize=(14, 8), subplot_kw=dict(polar=True))

# Plot lines
ax.plot(angles, cost_radar, 'o-', linewidth=3, color='steelblue', label='Cost Sensitivity')
ax.fill(angles, cost_radar, color='steelblue', alpha=0.2)

ax.plot(angles, runtime_radar, 's--', linewidth=3, color='darkorange', label='Runtime Sensitivity')
ax.fill(angles, runtime_radar, color='darkorange', alpha=0.2)

# Axis labels
ax.set_thetagrids(np.degrees(angles[:-1]), labels, fontsize=30, fontweight='bold')

# Radial tick settings
ax.set_ylim(0, 0.15)
ax.set_yticks([0.05, 0.10, 0.15])
ax.set_yticklabels(["0.05", "0.10", "0.15"], fontsize=30, fontweight='bold')
ax.set_rlabel_position(200)

# Title and Legend
ax.legend(loc='upper center', bbox_to_anchor=(0.5, -0.1), ncol=2, fontsize=24)
plt.subplots_adjust(bottom=0.2)

# Clean layout
plt.tight_layout()

# --------------------------
# 4. Save and Show Plot
# --------------------------
output_file = 'hyperparameter_sensitivity_radar.png'
plt.savefig(output_file, dpi=300)
plt.show()

print(f"Saved radar chart to: {output_file}")
