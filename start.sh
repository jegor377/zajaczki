#!/bin/bash

# Compile
mpic++ main.cpp -o main

# Run the C (MPI) program for 10 seconds, then kill if it’s still running
timeout 10s mpirun -np 9 --oversubscribe ./party

cd out

# 2. After the C program finishes or is killed by 'timeout', run the data-preparation Python script
python3 prepare_data.py

# 3. Then run the visualization Python script
python3 visualize_timeline.py

cd ..