#!/usr/bin/env python3
"""
Analyze a logic-analyzer capture of a push-button signal.

The signal is idle HIGH (1) and goes LOW (0) while the button is pressed.
Mechanical contact bounce causes extra, very short 0/1 transitions right
around the "real" press/release edges.
"""

import numpy as np
from pathlib import Path

# --- Parameters --------------------------------------------------------- #
FREQUENCY_HZ = 4_000_000                                   # logic analyzer sample rate
INPUT_FILE = Path(__file__).with_name("logic_analyzer_data.csv")      # capture file: column "logic", 0/1 per sample
BOUNCE_THRESHOLD_MS = 30                                    # edges closer together than this = bounce
# -------------------------------------------------------------------------- #

BOUNCE_THRESHOLD_S = BOUNCE_THRESHOLD_MS / 1000.0


def read_samples(path):
    """Read the 0/1 logic-analyzer samples into a numpy array (skips the header line)."""
    with open(path, "rb") as f:
        header = f.readline()
        assert header.strip() == b"logic", f"unexpected header: {header!r}"
        raw = np.fromfile(f, dtype=np.uint8)
    # every remaining line is exactly one digit ('0'/'1') followed by '\n'
    digits = raw[0::2] - ord("0")
    return digits.astype(np.int8)


def compute_timestamps(n_samples, frequency_hz):
    """Timestamp (seconds) of every sample, given a constant sample rate."""
    return np.arange(n_samples, dtype=np.float64) / frequency_hz


def find_edges(signal, timestamps):
    """Every 0<->1 transition as (sample_index, timestamp, 'falling'/'rising')."""
    diffs = np.diff(signal)
    idx = np.nonzero(diffs)[0] + 1
    directions = np.where(diffs[idx - 1] > 0, "rising", "falling")
    return idx, timestamps[idx], directions


def debounce_edges(edge_times, edge_dirs, threshold_s):
    """
    Ignore-window debounce filter: an edge occurring less than `threshold_s`
    after the last *accepted* edge is a glitch, not a real transition.

    Consecutive glitches that hang off the same accepted edge (the same
    press-down or release moment) are grouped into a "glitch sequence".

    Returns:
      accepted  - the accepted (time, direction) edges
      glitches  - flat list of every individual glitch pulse
      sequences - glitch pulses grouped by the accepted edge they cluster
                  around, with that edge's time/direction as the anchor
    """
    accepted = []
    glitches = []
    sequences = []
    last_accepted_t = None
    last_accepted_dir = None
    prev_t = None
    current_seq = None
    for t, d in zip(edge_times, edge_dirs):
        if last_accepted_t is None or (t - last_accepted_t) >= threshold_s:
            accepted.append((t, d))
            last_accepted_t = t
            last_accepted_dir = d
            current_seq = None
        else:
            # the pulse from the previous raw edge up to this rejected edge
            # was at LOW if this edge is a rising edge (0->1), HIGH otherwise
            level = "LOW" if d == "rising" else "HIGH"
            pulse = {"start": prev_t, "end": t, "duration": t - prev_t, "level": level}
            glitches.append(pulse)
            if current_seq is None:
                current_seq = {
                    "anchor_time": last_accepted_t,
                    "anchor_dir": last_accepted_dir,
                    "start": last_accepted_t,
                    "end": t,
                    "pulses": [pulse],
                }
                sequences.append(current_seq)
            else:
                current_seq["end"] = t
                current_seq["pulses"].append(pulse)
        prev_t = t

    for seq in sequences:
        seq["duration"] = seq["end"] - seq["start"]
    return accepted, glitches, sequences


def build_presses(accepted_edges, signal, timestamps):
    """
    Pair up debounced falling -> rising edges into press events
    (start time, end time, duration). Handles the button already being
    pressed at t=0 or still pressed at the end of the capture.
    """
    presses = []
    pending_start = timestamps[0] if signal[0] == 0 else None

    for t, direction in accepted_edges:
        if direction == "falling":
            pending_start = t
        elif direction == "rising" and pending_start is not None:
            presses.append({"start": pending_start, "end": t, "duration": t - pending_start})
            pending_start = None

    if pending_start is not None:
        presses.append({
            "start": pending_start,
            "end": timestamps[-1],
            "duration": timestamps[-1] - pending_start,
            "incomplete": True,
        })
    return presses


def link_sequences_to_presses(sequences, presses):
    """
    Tag each glitch sequence with the press it belongs to: a sequence
    anchored on a falling edge happened as that press *started*, one
    anchored on a rising edge happened as that press *ended* (released).
    """
    for seq in sequences:
        for n, p in enumerate(presses, 1):
            if seq["anchor_dir"] == "falling" and p["start"] == seq["anchor_time"]:
                seq["press_number"], seq["location"] = n, "start"
                break
            if seq["anchor_dir"] == "rising" and p["end"] == seq["anchor_time"]:
                seq["press_number"], seq["location"] = n, "end"
                break


def build_bounce_sequences(sequences):
    """
    Of all the glitches, only some represent a genuine contact bounce:
    - at the *start* of a press (falling edge), the button is settling
      LOW; a HIGH glitch there means it briefly let go again - a bounce.
    - at the *end* of a press (rising edge), the button is settling
      HIGH; a LOW glitch there means it briefly touched again - a bounce.
    LOW glitches at a press start (and HIGH glitches at a press end) are
    just the button still making contact, not a bounce.

    Returns a list shaped like `sequences`, but containing only the
    pulses that qualify as bounces (and only the sequences that have at
    least one), with start/end/duration recomputed over those pulses.
    """
    bounce_sequences = []
    for seq in sequences:
        target_level = "HIGH" if seq["anchor_dir"] == "falling" else "LOW"
        pulses = [p for p in seq["pulses"] if p["level"] == target_level]
        if not pulses:
            continue
        start = pulses[0]["start"]
        end = pulses[-1]["end"]
        bounce_sequences.append({
            "anchor_time": seq["anchor_time"],
            "anchor_dir": seq["anchor_dir"],
            "press_number": seq.get("press_number"),
            "location": seq.get("location"),
            "start": start,
            "end": end,
            "duration": end - start,
            "pulses": pulses,
        })
    return bounce_sequences


def divider(title):
    line = "=" * 70
    print(f"\n{line}\n{title}\n{line}\n")


def main():
    signal = read_samples(INPUT_FILE)
    timestamps = compute_timestamps(len(signal), FREQUENCY_HZ)

    print(f"Loaded {len(signal):,} samples @ {FREQUENCY_HZ / 1e6:g} MHz "
          f"({timestamps[-1] * 1000:.3f} ms total)")

    edge_idx, edge_t, edge_dir = find_edges(signal, timestamps)

    divider(f"RAW EDGES ({len(edge_idx)})")
    for i, t, d in zip(edge_idx, edge_t, edge_dir):
        print(f"  sample {i:>10d}  t = {t * 1000:10.4f} ms  {d}")

    accepted, glitches, sequences = debounce_edges(edge_t, edge_dir, BOUNCE_THRESHOLD_S)
    presses = build_presses(accepted, signal, timestamps)
    link_sequences_to_presses(sequences, presses)
    bounce_sequences = build_bounce_sequences(sequences)
    bounce_count = sum(len(seq["pulses"]) for seq in bounce_sequences)

    divider(f"GLITCHES ({len(glitches)} glitches, {len(sequences)} sequences)")
    for n, seq in enumerate(sequences, 1):
        where = (f"Press #{seq['press_number']} {seq['location']}"
                 if "press_number" in seq else "unmatched press")
        print(f"Glitch sequence #{n} ({where}): "
              f"start = {seq['start'] * 1000:10.4f} ms  "
              f"end = {seq['end'] * 1000:10.4f} ms  "
              f"duration = {seq['duration'] * 1000:8.4f} ms  "
              f"({len(seq['pulses'])} glitches)")
        for m, g in enumerate(seq["pulses"], 1):
            print(f"    glitch {m}: start = {g['start'] * 1000:10.4f} ms  "
                  f"end = {g['end'] * 1000:10.4f} ms  "
                  f"duration = {g['duration'] * 1000:8.4f} ms  ({g['level']} glitch)")

    divider(f"BOUNCES ONLY ({bounce_count})")
    for n, seq in enumerate(bounce_sequences, 1):
        where = (f"Press #{seq['press_number']} {seq['location']}"
                 if seq["press_number"] is not None else "unmatched press")
        print(f"Bounce sequence #{n} ({where}): "
              f"start = {seq['start'] * 1000:10.4f} ms  "
              f"end = {seq['end'] * 1000:10.4f} ms  "
              f"duration = {seq['duration'] * 1000:8.4f} ms  "
              f"({len(seq['pulses'])} bounces)")
        for m, b in enumerate(seq["pulses"], 1):
            print(f"    bounce {m}: start = {b['start'] * 1000:10.4f} ms  "
                  f"end = {b['end'] * 1000:10.4f} ms  "
                  f"duration = {b['duration'] * 1000:8.4f} ms  ({b['level']} glitch)")

    divider(f"PRESSES ({len(presses)})")
    for n, p in enumerate(presses, 1):
        flag = "  (incomplete - still pressed at end of capture)" if p.get("incomplete") else ""
        print(f"Press #{n:2d}: start = {p['start'] * 1000:10.4f} ms  "
              f"end = {p['end'] * 1000:10.4f} ms  "
              f"duration = {p['duration'] * 1000:8.4f} ms{flag}")

    divider("SUMMARY")
    sample_period_ms = 1000 / FREQUENCY_HZ
    print(f"Sample rate:             {FREQUENCY_HZ / 1e6:g} MHz "
          f"(timestep = {sample_period_ms:.6f} ms/sample)")
    print(f"Capture length:          {timestamps[-1] * 1000:.3f} ms")
    print(f"Bounce threshold:        {BOUNCE_THRESHOLD_MS} ms")
    falling_count = int(np.count_nonzero(edge_dir == "falling"))
    print(f"Total raw edges:         {len(edge_idx)}")
    print(f"  raw falling edges:     {falling_count}")
    print(f"Total glitches:          {len(glitches)}")
    print(f"Total glitch sequences:  {len(sequences)}")
    print(f"Total bounces:           {bounce_count}")
    print(f"Total presses:           {len(presses)}")

    if presses:
        shortest_press = min(presses, key=lambda p: p["duration"])
        longest_press = max(presses, key=lambda p: p["duration"])
        print(f"Shortest press:          {shortest_press['duration'] * 1000:.4f} ms  "
              f"(start = {shortest_press['start'] * 1000:.4f} ms)")
        print(f"Longest press:           {longest_press['duration'] * 1000:.4f} ms  "
              f"(start = {longest_press['start'] * 1000:.4f} ms)")

    if glitches:
        shortest_glitch = min(glitches, key=lambda g: g["duration"])
        longest_glitch = max(glitches, key=lambda g: g["duration"])
        print(f"Shortest glitch:         {shortest_glitch['duration'] * 1000:.4f} ms  "
              f"(start = {shortest_glitch['start'] * 1000:.4f} ms, {shortest_glitch['level']})")
        print(f"Longest glitch:          {longest_glitch['duration'] * 1000:.4f} ms  "
              f"(start = {longest_glitch['start'] * 1000:.4f} ms, {longest_glitch['level']})")


if __name__ == "__main__":
    main()
