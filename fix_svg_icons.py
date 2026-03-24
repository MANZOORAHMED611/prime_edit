#!/usr/bin/env python3
"""
Fix SVG icons to be properly formatted for Qt with explicit colors
"""

import os
import re

ICON_DIR = 'resources/icons/toolbar/'
ICON_COLOR = '#333333'  # Dark gray for light theme

def fix_svg(filepath):
    """Fix SVG to have proper format for Qt"""
    with open(filepath, 'r') as f:
        content = f.read()

    # Extract the path data
    match = re.search(r'<path[^>]*d="([^"]+)"', content)
    if not match:
        print(f"  WARNING: No path found in {filepath}")
        return

    path_data = match.group(1)

    # Create properly formatted SVG
    fixed_svg = f'''<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24">
  <path fill="{ICON_COLOR}" d="{path_data}"/>
</svg>
'''

    with open(filepath, 'w') as f:
        f.write(fixed_svg)

def fix_all_icons():
    """Fix all SVG icons in the toolbar directory"""

    if not os.path.exists(ICON_DIR):
        print(f"ERROR: Directory {ICON_DIR} not found!")
        return

    icon_files = [f for f in os.listdir(ICON_DIR) if f.endswith('.svg') and f != 'file-new-test.svg']

    print(f"Fixing {len(icon_files)} SVG icons...")

    for icon_file in sorted(icon_files):
        filepath = os.path.join(ICON_DIR, icon_file)
        print(f"  Fixing {icon_file}")
        fix_svg(filepath)

    print(f"\n{'='*60}")
    print(f"Fixed {len(icon_files)} icons!")
    print(f"Icon color: {ICON_COLOR}")
    print(f"{'='*60}")

if __name__ == '__main__':
    fix_all_icons()
