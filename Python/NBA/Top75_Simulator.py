import subprocess
import ast
from collections import Counter

final = []

for i in range(1000):
    print(f"Run #{i + 1}")
    output = subprocess.check_output(["python", "Top 75 Predictor.py"])
    value = output.decode().strip()

    # Convert the returned string representation of list into an actual list
    try:
        parsed_list = ast.literal_eval(value)
        if isinstance(parsed_list, list):
            final.append(parsed_list)
    except Exception as e:
        print("Error parsing output:", e)

# Flatten list of lists
flat_list = [player for sublist in final for player in sublist]

# Count occurrences
counts = Counter(flat_list)

# Sort by count descending (optional)
sorted_counts = dict(sorted(counts.items(), key=lambda x: x[1], reverse=True))

print(sorted_counts)