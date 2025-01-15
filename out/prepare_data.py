#!/usr/bin/env python3

"""
Script: prepare_data.py

Usage:
  1. Place this script in the same folder as your process_*.csv files.
  2. Run `python prepare_data.py`.
  3. A `combined.csv` file is produced, containing:
       - Each row is a unique timestamp (ascending).
       - Each column (besides timestamp) is a process rank (e.g. 0, 1, 2...).
       - Each cell shows the state of that process at that timestamp (forward-filled).
"""

import glob
import os
import re
import pandas as pd

def extract_rank(filename):
    """
    Extracts the integer rank from a filename like "process_0.csv"
    Returns None if it doesn't match the expected pattern.
    """
    # Example pattern: process_123.csv -> rank 123
    match = re.search(r'process_(\d+)\.csv$', filename)
    if match:
        return int(match.group(1))
    return None

def main():
    # 1. Find all relevant CSV files (process_*.csv) in the current directory
    csv_files = glob.glob("process_*.csv")

    if not csv_files:
        print("No 'process_*.csv' files found in the current directory.")
        return

    # 2. Read each file into a DataFrame, adding a 'process_id' column from the filename
    df_list = []
    for f in csv_files:
        rank = extract_rank(f)
        if rank is None:
            print(f"Warning: Could not extract rank from {f}, skipping.")
            continue

        # Read the CSV; we assume it has a header row "timestamp, state"
        # If your format differs, adjust accordingly (e.g. header=None, names=[...])
        df = pd.read_csv(f)

        # Add a column for the process ID
        df['process_id'] = rank

        df_list.append(df)

    # If we still have no DataFrames, stop
    if not df_list:
        print("No valid CSV data to process.")
        return

    # 3. Concatenate all DataFrames
    combined = pd.concat(df_list, ignore_index=True)

    # 4. Convert timestamp strings to actual datetime objects
    #    Setting utc=True will parse 'Z' as UTC
    combined['timestamp'] = pd.to_datetime(combined['timestamp'], utc=True, errors='coerce')

    # 5. Drop rows with invalid timestamps, just in case
    combined.dropna(subset=['timestamp'], inplace=True)

    # 6. Sort by timestamp, then by process_id
    combined.sort_values(by=['timestamp', 'process_id'], inplace=True)

        # Keep only the last occurrence for each (timestamp, process_id)
    combined = combined.drop_duplicates(
        subset=['timestamp', 'process_id'],
        keep='last'
    )

    # 7. Pivot to get each process in its own column
    #    index = timestamp, columns = process_id, values = state
    pivoted = combined.pivot(index='timestamp', columns='process_id', values='state')

    # 8. Forward fill to ensure that at each timestamp row, each process has a known state
    pivoted = pivoted.fillna(method='ffill')

    # 9. Save to a combined CSV
    #    The index (timestamp) will appear as the first column in the CSV
    pivoted.to_csv("combined.csv", index=True)

    print("Data preparation complete. Results saved to 'combined.csv'.")

if __name__ == "__main__":
    main()
