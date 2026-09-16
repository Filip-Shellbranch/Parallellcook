#!/usr/bin/env bash

# Define parameter ranges
T_VALUES=(1 2 4 8 16 32)
N_VALUES=(1 2 4 8 16 32)

# Executable name and output log
PROGRAM="./build/e5.out" # Replace with your compiled executable
OUTPUT_LOG="Exercise_5/results.txt"

# Clear or initialize the results file
echo "T | N | Real_Time_Sec" > "$OUTPUT_LOG"

for T in "${T_VALUES[@]}"; do
    for N in "${N_VALUES[@]}"; do
        echo "Running for T=$T, N=$N..."
        
        # Capture precise execution time (real wall-clock time in seconds)
        # Adjust arguments order/flags (-t, -n, etc.) to match your program's CLI parser
        ELAPSED=$( { TIMEFORMAT='%R'; time "$PROGRAM" "$T" "$N" > /dev/null; } 2>&1 )
        
        # Save combination and runtime to CSV log
        echo "$T | $N | $ELAPSED" >> "$OUTPUT_LOG"
    done
done

echo "Benchmarking complete. Results saved to $OUTPUT_LOG."