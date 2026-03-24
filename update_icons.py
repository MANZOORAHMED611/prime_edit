#!/usr/bin/env python3
"""
Update toolbarmanager.cpp to use proper Material Design Icons
"""

import re

# Icon mapping: action name -> icon file name
ICON_MAP = {
    'new': 'file-new.svg',
    'open': 'folder-open.svg',
    'save': 'content-save.svg',
    'saveAll': 'content-save-all.svg',
    'close': 'close.svg',
    'closeAll': 'close-all.svg',
    'closeOthers': 'close-box.svg',
    'closeRight': 'chevron-right.svg',
    'closeLeft': 'chevron-left.svg',
    'print': 'printer.svg',
    'undo': 'undo.svg',
    'redo': 'redo.svg',
    'cut': 'content-cut.svg',
    'copy': 'content-copy.svg',
    'paste': 'content-paste.svg',
    'selectAll': 'select-all.svg',
    'duplicateLine': 'content-duplicate.svg',
    'deleteLine': 'delete-line.svg',
    'moveLineUp': 'arrow-up.svg',
    'moveLineDown': 'arrow-down.svg',
    'joinLines': 'vector-combine.svg',
    'splitLines': 'call-split.svg',
    'toggleComment': 'comment-outline.svg',
    'blockComment': 'comment-multiple.svg',
    'find': 'magnify.svg',
    'replace': 'find-replace.svg',
    'findNext': 'chevron-right.svg',
    'findPrevious': 'chevron-left.svg',
    'goToLine': 'format-list-numbered.svg',
    'previousTab': 'tab-left.svg',
    'nextTab': 'tab-right.svg',
    'toggleBookmark': 'bookmark-outline.svg',
    'nextBookmark': 'bookmark-plus.svg',
    'prevBookmark': 'bookmark-minus.svg',
    'clearBookmarks': 'bookmark-remove.svg',
    'zoomIn': 'magnify-plus.svg',
    'zoomOut': 'magnify-minus.svg',
    'resetZoom': 'numeric-100.svg',
    'wordWrap': 'wrap.svg',
    'showWhitespace': 'format-pilcrow.svg',
    'showEOL': 'format-line-weight.svg',
    'showIndent': 'format-indent-increase.svg',
    'fullScreen': 'fullscreen.svg',
    'distractionFree': 'fullscreen-exit.svg',
    'foldAll': 'unfold-less.svg',
    'unfoldAll': 'unfold-more.svg',
    'toggleFold': 'chevron-up-down.svg',
    'toUpperCase': 'format-letter-case-upper.svg',
    'toLowerCase': 'format-letter-case-lower.svg',
    'toTitleCase': 'format-letter-case.svg',
    'toSentenceCase': 'format-text-variant.svg',
    'toInvertCase': 'format-text-variant.svg',
    'toRandomCase': 'format-text-variant.svg',
    'trimTrailing': 'format-align-right.svg',
    'trimLeading': 'format-align-left.svg',
    'trimBoth': 'format-align-center.svg',
    'tabToSpace': 'keyboard-tab.svg',
    'spaceToTab': 'keyboard-space.svg',
    'removeEmptyLines': 'delete-empty.svg',
    'removeDuplicates': 'content-duplicate-minus.svg',
    'sortAscending': 'sort-ascending.svg',
    'sortDescending': 'sort-descending.svg',
    'startRecording': 'record-circle.svg',
    'stopRecording': 'stop-circle.svg',
    'playbackMacro': 'play-circle.svg',
    'saveMacro': 'content-save-settings.svg',
    'loadMacro': 'folder-open-outline.svg',
}

def update_icons():
    """Update toolbarmanager.cpp with proper icon paths"""

    with open('src/ui/toolbarmanager.cpp', 'r') as f:
        content = f.read()

    # Pattern to match setIcon calls with Qt standard icons
    pattern = r'(\w+Action)->setIcon\(m_mainWindow->style\(\)->standardIcon\(QStyle::\w+\)\);'

    def replace_icon(match):
        action_var = match.group(1)
        # Extract action name from variable (e.g., 'newAction' -> 'new')
        action_name = action_var.replace('Action', '')
        action_name = action_name[0].lower() + action_name[1:]  # camelCase

        if action_name in ICON_MAP:
            icon_file = ICON_MAP[action_name]
            return f'{action_var}->setIcon(QIcon(":/icons/toolbar/{icon_file}"));'
        else:
            # Keep original if no mapping found
            return match.group(0)

    # Replace all icon assignments
    updated_content = re.sub(pattern, replace_icon, content)

    # Write back
    with open('src/ui/toolbarmanager.cpp', 'w') as f:
        f.write(updated_content)

    print("Icon paths updated successfully!")
    print(f"Updated {len(ICON_MAP)} icon mappings")

if __name__ == '__main__':
    update_icons()
