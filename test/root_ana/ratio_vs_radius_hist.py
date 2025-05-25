# ratio_vs_radius_hist.py
# This script processes the RatioVsRadius.root file to create histograms
# showing mean ratio values within distance bins.
# It generates 4 plots, one for each deadtime, with 4 lines per plot representing different grid sizes.
# Also creates a combined 2x2 plot with all deadtimes.
# Usage: python ratio_vs_radius_hist.py

import uproot
import numpy as np
import matplotlib.pyplot as plt
from collections import defaultdict
import os

input_file = "RatioVsRadius.root"
n_bins = 20
distance_range = (0.0015, 0.0415)
output_dir = "RatioVsRadius_Histograms"

if not os.path.exists(output_dir):
    os.makedirs(output_dir)

file = uproot.open(input_file)

graph_names = [key for key in file.keys() if "RatioVsRadius_Deadtime" in key and "c1" not in key]

deadtime_graphs = defaultdict(list)

for graph_name in graph_names:
    parts = graph_name.split(';')[0].split('_')
    deadtime_part = parts[1]
    grid_part = parts[2]
    
    deadtime = float(deadtime_part.replace('Deadtime', '').replace('ns', ''))
    grid_size = int(grid_part.split('x')[0])
    
    graph = file[graph_name]
    distances = graph.member('fX')
    ratios = graph.member('fY')
    
    hist_bins = np.linspace(distance_range[0], distance_range[1], n_bins + 1)
    bin_centers = (hist_bins[:-1] + hist_bins[1:]) / 2
    
    bin_means = np.zeros(n_bins)
    bin_errors = np.zeros(n_bins)
    bin_counts = np.zeros(n_bins)
    
    for i in range(n_bins):
        bin_min = hist_bins[i]
        bin_max = hist_bins[i + 1]
        
        mask = (distances >= bin_min) & (distances < bin_max)
        bin_values = ratios[mask]
        
        if len(bin_values) > 0:
            bin_means[i] = np.mean(bin_values)
            bin_errors[i] = np.std(bin_values) / np.sqrt(len(bin_values)) if len(bin_values) > 1 else 0
            bin_counts[i] = len(bin_values)
    
    deadtime_graphs[deadtime].append({
        'grid_size': grid_size,
        'bin_centers': bin_centers,
        'bin_means': bin_means,
        'bin_errors': bin_errors,
        'bin_counts': bin_counts
    })

for deadtime, graphs in deadtime_graphs.items():
    plt.figure(figsize=(10, 6))
    
    colors = ['blue', 'red', 'green', 'purple']
    markers = ['o', 's', '^', 'D']
    
    graphs.sort(key=lambda x: x['grid_size'], reverse=True)
    
    for i, graph_data in enumerate(graphs):
        grid_size = graph_data['grid_size']
        bin_centers = graph_data['bin_centers']
        bin_means = graph_data['bin_means']
        bin_errors = graph_data['bin_errors']
        
        dsipm_pitch = 1000 / grid_size
        
        plt.plot(bin_centers*100, bin_means, '-', color=colors[i % len(colors)], alpha=0.7)

        plt.errorbar(bin_centers*100, bin_means, yerr=bin_errors, 
                     fmt=markers[i % len(markers)], color=colors[i % len(colors)],
                     label=f'dSiPM pitch {dsipm_pitch:.1f} $\mu$m', capsize=3, markersize=5)
    
    plt.xlabel('Distance from Center [cm]', fontsize=12)
    plt.ylabel('Mean Reception Ratio', fontsize=12)
    plt.title(f'Mean Ratio vs Distance - Deadtime {deadtime}ns', fontsize=14)
    plt.grid(True, alpha=0.3)
    plt.legend(fontsize=10)
    
    plt.xticks(fontsize=10)
    plt.yticks(fontsize=10)
    
    plt.minorticks_on()
    plt.grid(which='minor', alpha=0.15)
    
    output_file = f"{output_dir}/RatioVsRadius_Deadtime{deadtime}ns.png"
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    plt.close()

# Create combined 2x2 plot
fig, axs = plt.subplots(2, 2, figsize=(16, 12), constrained_layout=True)
axs = axs.flatten()

# Sort deadtimes for consistent display
sorted_deadtimes = sorted(deadtime_graphs.keys())

for idx, deadtime in enumerate(sorted_deadtimes):
    ax = axs[idx]
    graphs = deadtime_graphs[deadtime]
    
    colors = ['blue', 'red', 'green', 'purple']
    markers = ['o', 's', '^', 'D']
    
    graphs.sort(key=lambda x: x['grid_size'], reverse=True)
    
    for i, graph_data in enumerate(graphs):
        grid_size = graph_data['grid_size']
        bin_centers = graph_data['bin_centers']
        bin_means = graph_data['bin_means']
        bin_errors = graph_data['bin_errors']
        
        dsipm_pitch = 1000 / grid_size
        
        ax.plot(bin_centers*100, bin_means, '-', color=colors[i % len(colors)], alpha=0.7)

        ax.errorbar(bin_centers*100, bin_means, yerr=bin_errors, 
                    fmt=markers[i % len(markers)], color=colors[i % len(colors)],
                    label=f'dSiPM pitch {dsipm_pitch:.1f} $\mu$m', capsize=3, markersize=5)
    
    ax.set_xlabel('Distance from Center [cm]', fontsize=12)
    ax.set_ylabel('Mean Reception Ratio', fontsize=12)
    ax.set_title(f'Deadtime {deadtime}ns', fontsize=14)
    ax.grid(True, alpha=0.3)
    ax.legend(fontsize=10)
    
    ax.tick_params(axis='both', which='major', labelsize=10)
    
    ax.minorticks_on()
    ax.grid(which='minor', alpha=0.15)

fig.suptitle('Mean Ratio vs Distance for Different Deadtimes', fontsize=16)

combined_output_file = f"{output_dir}/RatioVsRadius_Combined.png"
plt.savefig(combined_output_file, dpi=600, bbox_inches='tight')
plt.close()

print(f"Histograms have been created in the '{output_dir}' directory")
print(f"Combined plot saved as {combined_output_file}")
