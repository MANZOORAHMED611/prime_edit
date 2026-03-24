#!/usr/bin/env python3
"""
Download Material Design Icons for Olive Notepad toolbar
"""

import urllib.request
import os

# Icon mapping: local_name -> MDI icon name
ICONS = {
    # File Operations
    'file-new.svg': 'file-document-plus',
    'folder-open.svg': 'folder-open',
    'content-save.svg': 'content-save',
    'content-save-all.svg': 'content-save-all',
    'close.svg': 'close',
    'close-all.svg': 'close-box-multiple',
    'close-box.svg': 'close-box',
    'printer.svg': 'printer',

    # Edit Operations
    'undo.svg': 'undo',
    'redo.svg': 'redo',
    'content-cut.svg': 'content-cut',
    'content-copy.svg': 'content-copy',
    'content-paste.svg': 'content-paste',
    'select-all.svg': 'select-all',
    'content-duplicate.svg': 'content-duplicate',
    'delete-line.svg': 'table-row-remove',
    'arrow-up.svg': 'arrow-up-bold',
    'arrow-down.svg': 'arrow-down-bold',
    'vector-combine.svg': 'vector-combine',
    'call-split.svg': 'call-split',
    'comment-outline.svg': 'comment-outline',
    'comment-multiple.svg': 'comment-multiple-outline',

    # Search & Navigation
    'magnify.svg': 'magnify',
    'find-replace.svg': 'find-replace',
    'chevron-right.svg': 'chevron-right',
    'chevron-left.svg': 'chevron-left',
    'format-list-numbered.svg': 'format-list-numbered',
    'tab-left.svg': 'chevron-left-circle',
    'tab-right.svg': 'chevron-right-circle',
    'bookmark-outline.svg': 'bookmark-outline',
    'bookmark-plus.svg': 'bookmark-plus-outline',
    'bookmark-minus.svg': 'bookmark-minus-outline',
    'bookmark-remove.svg': 'bookmark-remove',

    # View & Display
    'magnify-plus.svg': 'magnify-plus-outline',
    'magnify-minus.svg': 'magnify-minus-outline',
    'numeric-100.svg': 'numeric-100-box-outline',
    'wrap.svg': 'wrap',
    'format-pilcrow.svg': 'format-pilcrow',
    'format-line-weight.svg': 'format-line-weight',
    'format-indent-increase.svg': 'format-indent-increase',
    'fullscreen.svg': 'fullscreen',
    'fullscreen-exit.svg': 'fullscreen-exit',
    'unfold-less.svg': 'unfold-less-horizontal',
    'unfold-more.svg': 'unfold-more-horizontal',
    'chevron-up-down.svg': 'unfold-more-horizontal',

    # Text Formatting
    'format-letter-case-upper.svg': 'format-letter-case-upper',
    'format-letter-case-lower.svg': 'format-letter-case-lower',
    'format-letter-case.svg': 'format-letter-case',
    'format-text-variant.svg': 'format-text-variant',
    'format-align-right.svg': 'format-align-right',
    'format-align-left.svg': 'format-align-left',
    'format-align-center.svg': 'format-align-center',
    'keyboard-tab.svg': 'keyboard-tab',
    'keyboard-space.svg': 'keyboard-space',
    'delete-empty.svg': 'delete-empty-outline',
    'content-duplicate-minus.svg': 'minus-box-multiple-outline',
    'sort-ascending.svg': 'sort-alphabetical-ascending',
    'sort-descending.svg': 'sort-alphabetical-descending',

    # Macro Operations
    'record-circle.svg': 'record-circle',
    'stop-circle.svg': 'stop-circle',
    'play-circle.svg': 'play-circle-outline',
    'content-save-settings.svg': 'content-save-settings-outline',
    'folder-open-outline.svg': 'folder-open-outline',
}

BASE_URL = 'https://raw.githubusercontent.com/Templarian/MaterialDesign-SVG/master/svg/'
OUTPUT_DIR = 'resources/icons/toolbar/'

def download_icons():
    """Download all icons from Material Design Icons repository"""

    if not os.path.exists(OUTPUT_DIR):
        os.makedirs(OUTPUT_DIR)

    success_count = 0
    fail_count = 0

    for local_name, mdi_name in ICONS.items():
        url = f"{BASE_URL}{mdi_name}.svg"
        output_path = os.path.join(OUTPUT_DIR, local_name)

        try:
            print(f"Downloading {mdi_name}.svg -> {local_name}")
            urllib.request.urlretrieve(url, output_path)
            success_count += 1
        except Exception as e:
            print(f"  ERROR: Failed to download {mdi_name}: {e}")
            fail_count += 1

    print(f"\n{'='*60}")
    print(f"Download complete!")
    print(f"Success: {success_count}/{len(ICONS)}")
    print(f"Failed: {fail_count}/{len(ICONS)}")
    print(f"{'='*60}")

if __name__ == '__main__':
    download_icons()
