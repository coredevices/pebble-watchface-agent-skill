#!/usr/bin/env python3
"""
Create animated GIF previews for each platform by capturing multiple frames.

Usage:
    python3 create_preview_gif.py [project_dir] [--frames N] [--delay MS]

Options:
    project_dir     Project directory (default: current directory)
    --frames N      Number of frames to capture (default: 10)
    --delay MS      Delay between frames in milliseconds (default: 500)

Creates:
    - preview_emery.gif
    - preview_basalt.gif (if emulator running)
    - preview_chalk.gif (if emulator running)

Requires: Pillow, pebble SDK installed
"""

import sys
import os
import subprocess
import time
import argparse
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("Error: Pillow is required. Install with: pip3 install Pillow")
    sys.exit(1)


def capture_frames(emulator: str, project_dir: Path, num_frames: int, frame_delay_ms: int) -> list:
    """Capture multiple frames from an emulator"""
    frames = []
    temp_dir = project_dir / "temp_frames"
    temp_dir.mkdir(exist_ok=True)

    print(f"Capturing {num_frames} frames from {emulator}...")

    for i in range(num_frames):
        frame_path = temp_dir / f"frame_{emulator}_{i:03d}.png"

        # Capture screenshot
        result = subprocess.run(
            ["pebble", "screenshot", "--no-open", "--emulator", emulator, str(frame_path)],
            capture_output=True,
            text=True
        )

        if result.returncode != 0:
            print(f"Warning: Failed to capture frame {i} for {emulator}")
            continue

        if frame_path.exists():
            frames.append(Image.open(frame_path).copy())

        # Wait between frames
        time.sleep(frame_delay_ms / 1000.0)

        # Progress indicator
        print(f"  Frame {i+1}/{num_frames}", end="\r")

    print(f"  Captured {len(frames)} frames for {emulator}")

    # Cleanup temp files
    for f in temp_dir.glob(f"frame_{emulator}_*.png"):
        f.unlink()

    try:
        temp_dir.rmdir()
    except OSError:
        pass  # Directory not empty, other platform frames may exist

    return frames


def create_gif(frames: list, output_path: Path, frame_duration_ms: int = 200):
    """Create animated GIF from frames"""
    if not frames:
        print(f"No frames to create GIF: {output_path}")
        return False

    # Save as animated GIF
    frames[0].save(
        output_path,
        save_all=True,
        append_images=frames[1:],
        duration=frame_duration_ms,
        loop=0  # Loop forever
    )
    print(f"Created: {output_path}")
    return True


def create_preview_gifs(project_dir: str = ".", num_frames: int = 10, frame_delay_ms: int = 500):
    """Create animated GIF previews for all platforms"""

    project_path = Path(project_dir)
    platforms = ["emery", "basalt", "aplite", "chalk"]

    print(f"Creating preview GIFs with {num_frames} frames each...")
    print(f"Frame capture delay: {frame_delay_ms}ms")
    print()

    for platform in platforms:
        print(f"\n--- {platform.upper()} ---")

        # Check if emulator is running by trying to capture
        test_result = subprocess.run(
            ["pebble", "screenshot", "--no-open", "--emulator", platform, "/dev/null"],
            capture_output=True,
            text=True
        )

        if test_result.returncode != 0:
            print(f"Skipping {platform} - emulator not running")
            print(f"Start with: pebble install --emulator {platform}")
            continue

        # Capture frames
        frames = capture_frames(platform, project_path, num_frames, frame_delay_ms)

        if frames:
            # Create GIF
            gif_path = project_path / f"preview_{platform}.gif"
            create_gif(frames, gif_path, frame_duration_ms=200)

    print("\nDone!")


def main():
    parser = argparse.ArgumentParser(description="Create animated GIF previews for Pebble watchfaces")
    parser.add_argument("project_dir", nargs="?", default=".", help="Project directory")
    parser.add_argument("--frames", type=int, default=10, help="Number of frames to capture")
    parser.add_argument("--delay", type=int, default=500, help="Delay between frames (ms)")

    args = parser.parse_args()
    create_preview_gifs(args.project_dir, args.frames, args.delay)


if __name__ == "__main__":
    main()
