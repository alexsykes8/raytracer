import matplotlib.pyplot as plt
import os

def read_data(filename):
    x_values = []
    y_values = []

    if not os.path.exists(filename):
        print(f"Warning: {filename} not found in the current directory.")
        return [], []

    with open(filename, 'r') as f:
        for line in f:
            parts = line.split()
            if len(parts) >= 2:
                try:
                    y_val = float(parts[0])
                    x_val = float(parts[1])

                    y_values.append(y_val)
                    x_values.append(x_val)
                except ValueError:
                    continue

    return x_values, y_values

def main():
    files = [
        {'name': 'bvh_test.txt', 'label': 'BVH Test', 'color': '#1f77b4'},
        {'name': 'no_bvh_test.txt', 'label': 'No BVH Test', 'color': '#d62728'}
    ]

    plt.figure(figsize=(12, 7))

    for file_info in files:
        x, y = read_data(file_info['name'])

        if x and y:
            plt.plot(x, y,
                     label=file_info['label'],
                     color=file_info['color'],
                     linewidth=2,
                     marker='.',
                     markersize=5)

    plt.title('Performance Comparison: BVH vs No BVH', fontsize=14)
    plt.ylabel('Time (ms)', fontsize=12)
    plt.xlabel('Number of Scene Objects', fontsize=12)
    plt.legend(fontsize=11)
    plt.grid(True, linestyle='--', alpha=0.7)

    plt.tight_layout()

    output_filename = 'bvh_comparison_plot.png'
    plt.savefig(output_filename, dpi=300)
    print(f"Plot successfully saved to '{output_filename}'")

if __name__ == "__main__":
    main()