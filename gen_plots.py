import matplotlib.pyplot as plt
import numpy as np
import matplotlib as mpl

# Update font sizes globally
mpl.rcParams.update({
    'font.size': 40,          # Overall font size
    'axes.titlesize': 40,     # Title font size
    'axes.labelsize': 40,     # Axis label font size
    'xtick.labelsize': 16,    # X-axis tick label size
    'ytick.labelsize': 16,    # Y-axis tick label size
})

# total population size

# ----------------------------
# 1) Setup: Parameter Values
# ----------------------------
params = np.array([10, 20, 30, 40, 50, 60, 70])

# ----------------------------
# 2) Dummy Data for Each Benchmark
#    (Replace these with your actual numbers)
# ----------------------------
# Unnormalized COST (example)
cost_ws1 = np.array([63.41, 62.85, 58.76, 56.7, 54.7, 54.1, 54.1])
cost_ws2 = np.array([132.56, 130.19, 126.34, 122.24, 121.29, 120.11, 120.11])
cost_adi1 = np.array([55.49, 54.11, 53.23, 51.12, 50.79, 50.79, 50.79])
cost_adi2 = np.array([53.31, 52.01, 51.14, 49.95, 49.02, 49.02, 49.02])

# Unnormalized RUNTIME (example)
runtime_ws1 = np.array([573, 613, 653, 703, 773,873, 1073])
runtime_ws2 = np.array([3659, 3969, 4119, 4759, 5759, 9759, 10759])
runtime_adi1 = np.array([16, 25, 22, 34, 19, 32, 40])
runtime_adi2 = np.array([20, 28, 27, 33, 22, 38, 49])

# ----------------------------
# 3) Normalization
# ----------------------------
# Cost normalization denominators (given in your question)
ref_ws1 = 71.08
ref_ws2 = 145.5
ref_adi1 = 53.14
ref_adi2 = 49.48

# Normalize cost
cost_ws1_norm = cost_ws1 / ref_ws1
cost_ws2_norm = cost_ws2 / ref_ws2
cost_adi1_norm = cost_adi1 / ref_adi1
cost_adi2_norm = cost_adi2 / ref_adi2

# Normalize runtime by 10000 seconds
runtime_ws1_norm = runtime_ws1 / 10000.0
runtime_ws2_norm = runtime_ws2 / 10000.0
runtime_adi1_norm = runtime_adi1 / 10000.0
runtime_adi2_norm = runtime_adi2 / 10000.0

# ----------------------------
# 4) Plot Setup
# ----------------------------
plt.style.use('ggplot')
fig, ax_cost = plt.subplots(figsize=(8, 5))

# ax_cost for cost (left y-axis)
# ax_runtime for runtime (right y-axis)
ax_runtime = ax_cost.twinx()

# ----------------------------
# 5) Plot COST (Left Axis)
# ----------------------------
ax_cost.plot(params, cost_ws1_norm, 'o-', color='blue',  label='WS1 Cost')
ax_cost.plot(params, cost_ws2_norm, 'o-', color='green', label='WS2 Cost')
ax_cost.plot(params, cost_adi1_norm,'o-', color='red',   label='TC1 Cost')
ax_cost.plot(params, cost_adi2_norm,'o-', color='purple',label='TC2 Cost')

# ----------------------------
# 6) Plot RUNTIME (Right Axis)
# ----------------------------
ax_runtime.plot(params, runtime_ws1_norm, 's--', color='blue',  label='WS1 Runtime')
ax_runtime.plot(params, runtime_ws2_norm, 's--', color='green', label='WS2 Runtime')
ax_runtime.plot(params, runtime_adi1_norm,'s--', color='red',   label='TC1 Runtime')
ax_runtime.plot(params, runtime_adi2_norm,'s--', color='purple',label='TC2 Runtime')

# ----------------------------
# 7) Labeling and Legends
# ----------------------------
ax_cost.set_xlabel('Total population size')
ax_cost.set_ylabel('Normalized Cost')
ax_runtime.set_ylabel('Normalized Runtime')

ax_cost.set_title('Effect of total population size')
# Create combined legend by merging handles
cost_lines, cost_labels = ax_cost.get_legend_handles_labels()
runtime_lines, runtime_labels = ax_runtime.get_legend_handles_labels()
ax_cost.legend(cost_lines + runtime_lines, cost_labels + runtime_labels, loc='upper left')

plt.tight_layout()
plt.show()
plt.savefig('total_pop_size_plot.png', dpi=300)

# clear plot 
plt.clf()

# total number of generations 

# ----------------------------
# 1) Setup: Parameter Values
# ----------------------------
params = np.array([10, 20, 30, 40, 50, 60, 70])

# ----------------------------
# 2) Dummy Data for Each Benchmark
#    (Replace these with your actual numbers)
# ----------------------------
# Unnormalized COST (example)
cost_ws1 = np.array([59.43, 58.85, 57.76, 56.7, 54.7, 54.1, 54.1])
cost_ws2 = np.array([131.25, 127.22, 124.19, 121.29, 121.29, 121.11, 120.22])
cost_adi1 = np.array([52.11, 52.11, 52.05, 51.12, 50.79, 50.79, 50.42])
cost_adi2 = np.array([50.31, 50.03, 49.95, 49.76, 49.02, 49.01, 49.01])

# Unnormalized RUNTIME (example)
runtime_ws1 = np.array([473, 513, 603, 653, 773, 973, 1273])
runtime_ws2 = np.array([2659, 3369, 3919, 4459, 4759, 8759, 11759])
runtime_adi1 = np.array([11, 11, 16, 19, 19, 32, 39])
runtime_adi2 = np.array([14, 19, 29, 33, 37, 48, 52])

# ----------------------------
# 3) Normalization
# ----------------------------
# Cost normalization denominators (given in your question)
ref_ws1 = 71.08
ref_ws2 = 145.5
ref_adi1 = 53.14
ref_adi2 = 49.48

# Normalize cost
cost_ws1_norm = cost_ws1 / ref_ws1
cost_ws2_norm = cost_ws2 / ref_ws2
cost_adi1_norm = cost_adi1 / ref_adi1
cost_adi2_norm = cost_adi2 / ref_adi2

# Normalize runtime by 10000 seconds
runtime_ws1_norm = runtime_ws1 / 10000.0
runtime_ws2_norm = runtime_ws2 / 10000.0
runtime_adi1_norm = runtime_adi1 / 10000.0
runtime_adi2_norm = runtime_adi2 / 10000.0

# ----------------------------
# 4) Plot Setup
# ----------------------------
plt.style.use('ggplot')
fig, ax_cost = plt.subplots(figsize=(8, 5))

# ax_cost for cost (left y-axis)
# ax_runtime for runtime (right y-axis)
ax_runtime = ax_cost.twinx()

# ----------------------------
# 5) Plot COST (Left Axis)
# ----------------------------
ax_cost.plot(params, cost_ws1_norm, 'o-', color='blue',  label='WS1 Cost')
ax_cost.plot(params, cost_ws2_norm, 'o-', color='green', label='WS2 Cost')
ax_cost.plot(params, cost_adi1_norm,'o-', color='red',   label='TC1 Cost')
ax_cost.plot(params, cost_adi2_norm,'o-', color='purple',label='TC2 Cost')

# ----------------------------
# 6) Plot RUNTIME (Right Axis)
# ----------------------------
ax_runtime.plot(params, runtime_ws1_norm, 's--', color='blue',  label='WS1 Runtime')
ax_runtime.plot(params, runtime_ws2_norm, 's--', color='green', label='WS2 Runtime')
ax_runtime.plot(params, runtime_adi1_norm,'s--', color='red',   label='TC1 Runtime')
ax_runtime.plot(params, runtime_adi2_norm,'s--', color='purple',label='TC2 Runtime')

# ----------------------------
# 7) Labeling and Legends
# ----------------------------
ax_cost.set_xlabel('Total number of generations')
ax_cost.set_ylabel('Normalized Cost')
ax_runtime.set_ylabel('Normalized Runtime')

ax_cost.set_title('Effect of #generations')
# Create combined legend by merging handles
cost_lines, cost_labels = ax_cost.get_legend_handles_labels()
runtime_lines, runtime_labels = ax_runtime.get_legend_handles_labels()
ax_cost.legend(cost_lines + runtime_lines, cost_labels + runtime_labels, loc='upper left')

plt.tight_layout()
plt.show()
plt.savefig('total_generations_plot.png', dpi=300)

# Number of genome pairs in tournament selection

# clear plot 
plt.clf()

# ----------------------------
# 1) Setup: Parameter Values
# ----------------------------
params = np.array([30, 35, 40, 45, 50, 55, 60])

# ----------------------------
# 2) Dummy Data for Each Benchmark
#    (Replace these with your actual numbers)
# ----------------------------
# Unnormalized COST (example)
cost_ws1 = np.array([63.64, 63.64, 56.41, 54.1, 54.7, 57.29, 59.14])
cost_ws2 = np.array([139.45, 138.63, 125.29, 121.29, 120.21, 129.11, 129.22])
cost_adi1 = np.array([58.73, 56.42, 53.15, 50.79, 51.79, 52.95, 52.96])
cost_adi2 = np.array([55.99, 53.21, 50.41, 49.02, 49.02, 54.16, 53.33])

# Unnormalized RUNTIME (example)
runtime_ws1 = np.array([673, 663, 671, 653, 673, 666, 692])
runtime_ws2 = np.array([4319, 4369, 4219, 4459, 4509, 4759, 4469])
runtime_adi1 = np.array([21, 21, 20, 19, 19, 19, 20])
runtime_adi2 = np.array([34, 39, 39, 33, 37, 38, 32])

# ----------------------------
# 3) Normalization
# ----------------------------
# Cost normalization denominators (given in your question)
ref_ws1 = 71.08
ref_ws2 = 145.5
ref_adi1 = 53.14
ref_adi2 = 49.48

# Normalize cost
cost_ws1_norm = cost_ws1 / ref_ws1
cost_ws2_norm = cost_ws2 / ref_ws2
cost_adi1_norm = cost_adi1 / ref_adi1
cost_adi2_norm = cost_adi2 / ref_adi2

# Normalize runtime by 10000 seconds
runtime_ws1_norm = runtime_ws1 / 10000.0
runtime_ws2_norm = runtime_ws2 / 10000.0
runtime_adi1_norm = runtime_adi1 / 10000.0
runtime_adi2_norm = runtime_adi2 / 10000.0

# ----------------------------
# 4) Plot Setup
# ----------------------------
plt.style.use('ggplot')
fig, ax_cost = plt.subplots(figsize=(8, 5))

# ax_cost for cost (left y-axis)
# ax_runtime for runtime (right y-axis)
ax_runtime = ax_cost.twinx()

# ----------------------------
# 5) Plot COST (Left Axis)
# ----------------------------
ax_cost.plot(params, cost_ws1_norm, 'o-', color='blue',  label='WS1 Cost')
ax_cost.plot(params, cost_ws2_norm, 'o-', color='green', label='WS2 Cost')
ax_cost.plot(params, cost_adi1_norm,'o-', color='red',   label='TC1 Cost')
ax_cost.plot(params, cost_adi2_norm,'o-', color='purple',label='TC2 Cost')

# ----------------------------
# 6) Plot RUNTIME (Right Axis)
# ----------------------------
ax_runtime.plot(params, runtime_ws1_norm, 's--', color='blue',  label='WS1 Runtime')
ax_runtime.plot(params, runtime_ws2_norm, 's--', color='green', label='WS2 Runtime')
ax_runtime.plot(params, runtime_adi1_norm,'s--', color='red',   label='TC1 Runtime')
ax_runtime.plot(params, runtime_adi2_norm,'s--', color='purple',label='TC2 Runtime')

# ----------------------------
# 7) Labeling and Legends
# ----------------------------
ax_cost.set_xlabel('Number of genome pairs in tournament selection')
ax_cost.set_ylabel('Normalized Cost')
ax_runtime.set_ylabel('Normalized Runtime')

ax_cost.set_title('Effect of #genome pairs in selection')
# Create combined legend by merging handles
cost_lines, cost_labels = ax_cost.get_legend_handles_labels()
runtime_lines, runtime_labels = ax_runtime.get_legend_handles_labels()
ax_cost.legend(cost_lines + runtime_lines, cost_labels + runtime_labels, loc='upper left')

plt.tight_layout()
plt.show()
plt.savefig('total_genome_pairs_plot.png', dpi=300)

# Crossover probability

# clear plot 
plt.clf()

# ----------------------------
# 1) Setup: Parameter Values
# ----------------------------
params = np.array([0.45, 0.5, 0.55, 0.6, 0.65, 0.7, 0.75])

# ----------------------------
# 2) Dummy Data for Each Benchmark
#    (Replace these with your actual numbers)
# ----------------------------
# Unnormalized COST (example)
cost_ws1 = np.array([66.29, 63.64, 54.1, 54.1, 54.1, 57.1, 58.2])
cost_ws2 = np.array([124.13, 122.19, 122.29, 124.29, 121.29, 123.21, 124.21])
cost_adi1 = np.array([52.76, 52.19, 50.95, 51.79, 50.79, 51.21, 52.33])
cost_adi2 = np.array([53.26, 50.21, 49.12, 49.62, 49.02, 50.14, 53.92])

# Unnormalized RUNTIME (example)
runtime_ws1 = np.array([654, 656, 656, 661, 670, 676, 682])
runtime_ws2 = np.array([4216, 4339, 4459, 4559, 4589, 4569, 4572])
runtime_adi1 = np.array([19, 21, 19, 19, 19, 19, 20])
runtime_adi2 = np.array([30, 31, 33, 33, 34, 36, 37])

# ----------------------------
# 3) Normalization
# ----------------------------
# Cost normalization denominators (given in your question)
ref_ws1 = 71.08
ref_ws2 = 145.5
ref_adi1 = 53.14
ref_adi2 = 49.48

# Normalize cost
cost_ws1_norm = cost_ws1 / ref_ws1
cost_ws2_norm = cost_ws2 / ref_ws2
cost_adi1_norm = cost_adi1 / ref_adi1
cost_adi2_norm = cost_adi2 / ref_adi2

# Normalize runtime by 10000 seconds
runtime_ws1_norm = runtime_ws1 / 10000.0
runtime_ws2_norm = runtime_ws2 / 10000.0
runtime_adi1_norm = runtime_adi1 / 10000.0
runtime_adi2_norm = runtime_adi2 / 10000.0

# ----------------------------
# 4) Plot Setup
# ----------------------------
plt.style.use('ggplot')
fig, ax_cost = plt.subplots(figsize=(8, 5))

# ax_cost for cost (left y-axis)
# ax_runtime for runtime (right y-axis)
ax_runtime = ax_cost.twinx()

# ----------------------------
# 5) Plot COST (Left Axis)
# ----------------------------
ax_cost.plot(params, cost_ws1_norm, 'o-', color='blue',  label='WS1 Cost')
ax_cost.plot(params, cost_ws2_norm, 'o-', color='green', label='WS2 Cost')
ax_cost.plot(params, cost_adi1_norm,'o-', color='red',   label='TC1 Cost')
ax_cost.plot(params, cost_adi2_norm,'o-', color='purple',label='TC2 Cost')

# ----------------------------
# 6) Plot RUNTIME (Right Axis)
# ----------------------------
ax_runtime.plot(params, runtime_ws1_norm, 's--', color='blue',  label='WS1 Runtime')
ax_runtime.plot(params, runtime_ws2_norm, 's--', color='green', label='WS2 Runtime')
ax_runtime.plot(params, runtime_adi1_norm,'s--', color='red',   label='TC1 Runtime')
ax_runtime.plot(params, runtime_adi2_norm,'s--', color='purple',label='TC2 Runtime')

# ----------------------------
# 7) Labeling and Legends
# ----------------------------
ax_cost.set_xlabel('Crossover probability')
ax_cost.set_ylabel('Normalized Cost')
ax_runtime.set_ylabel('Normalized Runtime')

ax_cost.set_title('Effect of crossover probability')
# Create combined legend by merging handles
cost_lines, cost_labels = ax_cost.get_legend_handles_labels()
runtime_lines, runtime_labels = ax_runtime.get_legend_handles_labels()
ax_cost.legend(cost_lines + runtime_lines, cost_labels + runtime_labels, loc='upper left')

plt.tight_layout()
plt.show()
plt.savefig('crossover_prob_plot.png', dpi=300)

plt.clf()

# Data
labels = [
    'SA-based floorplanner',
    'Chiplets cost model',
    'Initial partitioning',
    'Partitioning pruning',
    'I/O'
]
sizes = [65, 30, 3, 1, 1]  # Corresponding time percentages

# Create the pie chart
plt.pie(
    sizes,
    labels=labels,
    autopct='%1.1f%%',      # Label each slice with X.X%
    startangle=140          # Rotate chart for a better view (optional)
)

plt.title('Runtime Breakdown')
plt.axis('equal')  # Make sure pie is drawn as a circle
plt.tight_layout()

# Show and/or save the figure
plt.show()
plt.savefig('runtime_breakdown.png', dpi=300)
