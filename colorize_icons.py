#!/usr/bin/env python3
import os
import re

# Olive theme color palette
COLORS = {
    'primary': '#6b8e23',      # Olive green
    'secondary': '#9b6b3f',    # Brown
    'accent': '#556b2f',       # Dark olive
    'blue': '#4a90e2',         # Blue
    'red': '#d9534f',          # Red
    'orange': '#f0ad4e',       # Orange
    'purple': '#8e44ad',       # Purple
    'teal': '#5bc0de',         # Teal
}

# Icon color mappings - assign colors based on function
ICON_COLORS = {
    # File operations - Blue
    'file-new.svg': COLORS['blue'],
    'folder-open.svg': COLORS['orange'],
    'content-save.svg': COLORS['primary'],
    'content-save-all.svg': COLORS['primary'],
    'close.svg': COLORS['red'],
    'close-all.svg': COLORS['red'],
    'close-box.svg': COLORS['red'],
    'printer.svg': COLORS['purple'],
    
    # Edit operations - Green/Olive
    'undo.svg': COLORS['primary'],
    'redo.svg': COLORS['accent'],
    'content-cut.svg': COLORS['red'],
    'content-copy.svg': COLORS['blue'],
    'content-paste.svg': COLORS['primary'],
    'select-all.svg': COLORS['teal'],
    'content-duplicate.svg': COLORS['blue'],
    'delete-line.svg': COLORS['red'],
    'delete-empty.svg': COLORS['red'],
    
    # Navigation - Teal
    'arrow-up.svg': COLORS['teal'],
    'arrow-down.svg': COLORS['teal'],
    'chevron-left.svg': COLORS['teal'],
    'chevron-right.svg': COLORS['teal'],
    'chevron-up-down.svg': COLORS['teal'],
    
    # Search - Orange
    'magnify.svg': COLORS['orange'],
    'magnify-plus.svg': COLORS['orange'],
    'magnify-minus.svg': COLORS['orange'],
    'find-replace.svg': COLORS['orange'],
    
    # Bookmarks - Purple
    'bookmark-outline.svg': COLORS['purple'],
    'bookmark-plus.svg': COLORS['purple'],
    'bookmark-minus.svg': COLORS['purple'],
    'bookmark-remove.svg': COLORS['red'],
    
    # Format - Brown/Secondary
    'format-indent-increase.svg': COLORS['secondary'],
    'format-indent-decrease.svg': COLORS['secondary'],
    'format-letter-case.svg': COLORS['secondary'],
    'format-letter-case-upper.svg': COLORS['secondary'],
    'format-letter-case-lower.svg': COLORS['secondary'],
    'keyboard-tab.svg': COLORS['secondary'],
    'keyboard-space.svg': COLORS['secondary'],
    
    # View - Blue
    'fullscreen.svg': COLORS['blue'],
    'fullscreen-exit.svg': COLORS['blue'],
    'unfold-more.svg': COLORS['blue'],
    'unfold-less.svg': COLORS['blue'],
    'wrap.svg': COLORS['blue'],
    
    # Macro - Accent
    'record-circle.svg': COLORS['red'],
    'stop-circle.svg': COLORS['red'],
    'play-circle.svg': COLORS['primary'],
    
    # Comments
    'comment-outline.svg': COLORS['accent'],
    'comment-multiple.svg': COLORS['accent'],
}

def colorize_icon(filepath, color):
    """Replace the fill color in an SVG icon"""
    with open(filepath, 'r') as f:
        content = f.read()
    
    # Replace fill="#333333" or fill="currentColor" with new color
    content = re.sub(r'fill="[^"]*"', f'fill="{color}"', content)
    
    with open(filepath, 'w') as f:
        f.write(content)

def main():
    icons_dir = '/home/abusulaiman/dev/repository/notepad_supreme/resources/icons/toolbar'
    
    colorized_count = 0
    for icon_name, color in ICON_COLORS.items():
        filepath = os.path.join(icons_dir, icon_name)
        if os.path.exists(filepath):
            colorize_icon(filepath, color)
            colorized_count += 1
            print(f"Colorized {icon_name} -> {color}")
        else:
            print(f"Warning: {icon_name} not found")
    
    # Default color for remaining icons
    default_color = COLORS['primary']
    for filename in os.listdir(icons_dir):
        if filename.endswith('.svg') and filename not in ICON_COLORS:
            filepath = os.path.join(icons_dir, filename)
            colorize_icon(filepath, default_color)
            colorized_count += 1
            print(f"Colorized {filename} -> {default_color} (default)")
    
    print(f"\nTotal icons colorized: {colorized_count}")

if __name__ == '__main__':
    main()
