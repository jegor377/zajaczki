#!/usr/bin/env python3

"""
visualize_timeline.py

Creates a Gantt-like chart from combined.csv, showing each process's states
over time in colored intervals.

Usage:
    python visualize_timeline.py
"""

import pandas as pd
import plotly.express as px

def main():
    # 1. Read combined.csv
    #    - We assume timestamp is the index. If your file puts it in the first column,
    #      you can specify: index_col=0, parse_dates=True
    df = pd.read_csv("combined.csv", index_col=0, parse_dates=True)

    # df should look like:
    #                      0         1         2
    # 2025-01-09 10:00:00  INIT      INIT      INIT
    # 2025-01-09 10:00:05  WAITING   INIT      INIT
    # ...
    
    # 2. Reset index so timestamp becomes a column
    df = df.reset_index().rename(columns={"index": "timestamp"})
    # Now we have columns: [timestamp, 0, 1, 2, ...]

    # 3. Melt the wide dataframe into a long form:
    #    columns: [timestamp, process, state]
    long_df = df.melt(
        id_vars="timestamp",
        var_name="process",
        value_name="state"
    )
    # Example result (long_df):
    #   timestamp                 process    state
    # 0 2025-01-09 10:00:00+00:00 0          INIT
    # 1 2025-01-09 10:00:05+00:00 0          WAITING
    # 2 2025-01-09 10:00:00+00:00 1          INIT
    # ...

    # 4. Sort by process and timestamp (just to be safe)
    long_df.sort_values(by=["process", "timestamp"], inplace=True)

    # 5. For a Gantt-like chart, we need start/end times for each consecutive state interval
    #    We'll group by process and identify the intervals for each state.

    intervals = []
    for process, grp in long_df.groupby("process"):
        # We'll track the start of the current state,
        # and when we see a new state, we close the old interval.
        
        # Convert timestamps to actual Python datetimes
        grp["timestamp"] = pd.to_datetime(grp["timestamp"], utc=True)

        grp = grp.reset_index(drop=True)
        
        if grp.empty:
            continue

        # Initialize
        current_state = grp.loc[0, "state"]
        current_start = grp.loc[0, "timestamp"]
        
        # Iterate over rows to find changes
        for i in range(1, len(grp)):
            row_time = grp.loc[i, "timestamp"]
            row_state = grp.loc[i, "state"]
            
            if row_state != current_state:
                # We found a change, so the interval for current_state ends just before this row_time
                intervals.append({
                    "Process": str(process),
                    "State": current_state,
                    "Start": current_start,
                    "Finish": row_time  # the new row_time is start of next state
                })
                # Start a new interval
                current_state = row_state
                current_start = row_time
        
        # Close the final interval from the last start to (optionally) some end
        # If you want to end at the last timestamp, we can do that:
        last_time = grp.loc[len(grp) - 1, "timestamp"]
        intervals.append({
            "Process": str(process),
            "State": current_state,
            "Start": current_start,
            "Finish": last_time + pd.Timedelta(seconds=1)  # add small delta to show last segment
        })

    # 6. Convert intervals to a DataFrame that Plotly can understand
    intervals_df = pd.DataFrame(intervals)

    # intervals_df columns are now: ["Process", "State", "Start", "Finish"]
    # Example row:
    #   Process = "0"
    #   State   = "RUNNING"
    #   Start   = 2025-01-09 10:00:05+00:00
    #   Finish  = 2025-01-09 10:01:10+00:00

    # 7. Create a timeline chart with Plotly
    fig = px.timeline(
        intervals_df,
        x_start="Start",
        x_end="Finish",
        y="Process",
        color="State",  # color by state
        title="State Timeline per Process"
    )

    # 8. Flip the Y-axis so that process_0 is at top (optional)
    fig.update_yaxes(autorange="reversed")

    # 9. Show the figure (interactive window if running locally; in Jupyter it will inline)
    fig.write_html("timeline.html")

if __name__ == "__main__":
    main()
