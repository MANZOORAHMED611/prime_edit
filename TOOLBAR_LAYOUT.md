# Olive Notepad - Multi-Line Toolbar Layout

## Overview
The toolbars are now organized into **3 rows** to accommodate all 80+ icons efficiently, similar to Notepad++'s multi-toolbar layout while saving vertical space.

## Toolbar Layout (3 Rows)

```
┌──────────────────────────────────────────┬──────────────────────────────────┐
│ File Operations (Row 1 Left)             │ Edit Operations (Row 1 Right)    │
├──────────────────────────────────────────┼──────────────────────────────────┤
│ [New] [Open] [Save] [Save All] |         │ [Undo] [Redo] | [Cut] [Copy]     │
│ [Close] [Close All] [Close Others]       │ [Paste] | [Select All] |         │
│ [Close Right] [Close Left] | [Print]     │ [Dup Line] [Del Line] [Move ↑]   │
│                                           │ [Move ↓] | [Join] [Split] |      │
│                                           │ [Comment] [Block /**/]            │
└──────────────────────────────────────────┴──────────────────────────────────┘

┌──────────────────────────────────────────┬──────────────────────────────────┐
│ Search & Navigation (Row 2 Left)         │ View & Display (Row 2 Right)     │
├──────────────────────────────────────────┼──────────────────────────────────┤
│ [Find] [Replace] | [Next] [Previous] |   │ [Zoom +] [Zoom −] [100%] | [Wrap]│
│ [Go to] | [◄ Tab] [Tab ►] |              │ [Space·] [¶ EOL] [Indent] |      │
│ [★ Mark] [Next ★] [Prev ★] [Clear ★]     │ [Full] [Zen] | [Fold] [Unfold]  │
└──────────────────────────────────────────┴──────────────────────────────────┘

┌──────────────────────────────────────────┬──────────────────────────────────┐
│ Text Formatting (Row 3 Left)             │ Macro Recording (Row 3 Right)    │
├──────────────────────────────────────────┼──────────────────────────────────┤
│ [ABC] [abc] [Abc] [aBc] | [Trim →]       │ [● Rec] [■ Stop] | [▶ Play] |    │
│ [← Trim] [↔ Trim] | [→␣] [␣→] |          │ [Save] [Load]                     │
│ [No ␣] [No Dup] | [A→Z] [Z→A]            │                                   │
└──────────────────────────────────────────┴──────────────────────────────────┘
```

## Row-by-Row Breakdown

### Row 1 Left: File Operations (13 buttons)
**Left Half of Row**

| Button | Function | Shortcut |
|--------|----------|----------|
| New | Create new file | Ctrl+N |
| Open | Open file | Ctrl+O |
| Save | Save current | Ctrl+S |
| Save All | Save all files | Ctrl+Shift+S |
| Close | Close current | Ctrl+W |
| Close All | Close all files | - |
| Close Others | Close all except current | - |
| Close Right | Close tabs to right | - |
| Close Left | Close tabs to left | - |
| Print | Print document | Ctrl+P (disabled) |

### Row 1 Right: Edit Operations (15 buttons)
**Right Half of Row**

| Button | Function | Shortcut |
|--------|----------|----------|
| Undo | Undo last action | Ctrl+Z |
| Redo | Redo last action | Ctrl+Y |
| Cut | Cut selection | Ctrl+X |
| Copy | Copy selection | Ctrl+C |
| Paste | Paste clipboard | Ctrl+V |
| Select All | Select all text | Ctrl+A |
| Dup Line | Duplicate line | Ctrl+D |
| Del Line | Delete line | Ctrl+Shift+L |
| Move ↑ | Move line up | Ctrl+Shift+Up |
| Move ↓ | Move line down | Ctrl+Shift+Down |
| Join | Join lines | - |
| Split | Split lines | - |
| Comment | Toggle comment | Ctrl+/ |
| Block /**/ | Block comment | Ctrl+Shift+/ |

### Row 2 Left: Search & Navigation (12 buttons)
**Left Half of Row**

| Button | Function | Shortcut |
|--------|----------|----------|
| Find | Find text | Ctrl+F |
| Replace | Replace text | Ctrl+H |
| Next | Find next | F3 |
| Previous | Find previous | Shift+F3 |
| Go to | Go to line | Ctrl+G |
| ◄ Tab | Previous tab | Ctrl+PgUp |
| Tab ► | Next tab | Ctrl+PgDown |
| ★ Mark | Toggle bookmark | Ctrl+F2 |
| Next ★ | Next bookmark | F2 |
| Prev ★ | Previous bookmark | Shift+F2 |
| Clear ★ | Clear all bookmarks | - |

### Row 2 Right: View & Display (12 buttons)
**Right Half of Row**

| Button | Function | Shortcut |
|--------|----------|----------|
| Zoom + | Zoom in | Ctrl++ |
| Zoom − | Zoom out | Ctrl+- |
| 100% | Reset zoom | Ctrl+0 |
| Wrap | Word wrap toggle | - |
| Space· | Show whitespace | - |
| ¶ EOL | Show line endings | - |
| Indent | Show indent guides | - |
| Full | Full screen mode | F11 |
| Zen | Distraction free | - |
| [ ] All | Fold all | - |
| [+] All | Unfold all | - |
| [ ] | Toggle fold | - |

### Row 3 Left: Text Formatting (14 buttons)
**Left Half of Row**

| Button | Function | Shortcut |
|--------|----------|----------|
| ABC | Convert to UPPERCASE | Ctrl+Shift+U |
| abc | Convert to lowercase | Ctrl+U |
| Abc | Convert to Title Case | - |
| aBc | Invert case | - |
| Trim → | Trim trailing whitespace | - |
| ← Trim | Trim leading whitespace | - |
| ↔ Trim | Trim both sides | - |
| →␣ | TAB to spaces | - |
| ␣→ | Spaces to TAB | - |
| No ␣ | Remove empty lines | - |
| No Dup | Remove duplicates | - |
| A→Z | Sort ascending | - |
| Z→A | Sort descending | - |

### Row 3 Right: Macro Recording (5 buttons)
**Right Half of Row**

| Button | Function | Shortcut |
|--------|----------|----------|
| ● Rec | Start recording | Ctrl+Shift+R |
| ■ Stop | Stop recording | - |
| ▶ Play | Playback macro | Ctrl+Shift+P |
| Save | Save macro | - |
| Load | Load macro | - |

## Key Features of Multi-Line Layout

### 1. Logical Grouping
- **Row 1**: File operations (left) + Edit operations (right) - Most frequently used
- **Row 2**: Search/navigation (left) + View options (right)
- **Row 3**: Formatting (left) + Macros (right)

### 2. Space Efficiency
- Uses only 3 horizontal rows instead of 6 vertical toolbars - **50% reduction!**
- Toolbars break at logical points using `addToolBarBreak()`
- All rows split into left/right sections for optimal space usage

### 3. Visual Organization
- Related functions grouped together
- Separators (|) divide functional groups
- Consistent icon sizing (20x20 pixels)
- Compact button labels for maximum icons per row

### 4. Customization
All toolbars can be:
- Moved by dragging
- Hidden/shown via View menu
- Reorganized by user preference
- Settings persist across sessions (Qt automatic)

## Button Label Design

### Compact Labels
Used short, clear labels to fit more buttons:
- "Dup Line" instead of "Duplicate Line"
- "Del Line" instead of "Delete Line"
- "ABC" / "abc" / "Abc" for case operations
- Unicode symbols: ★ (bookmark), → ← ↔ (trim), ● ■ ▶ (macro)

### Tooltip Details
Every button has detailed tooltips showing:
- Full function name
- Keyboard shortcut (if available)
- Brief description of action

## Implementation Details

### Code Structure
```cpp
void ToolbarManager::createToolbars()
{
    createFileToolbar();      // Row 1 (left)
    createEditToolbar();      // Row 1 (right, same row)
    m_mainWindow->addToolBarBreak(); // Force new row

    createSearchToolbar();    // Row 2 (left)
    createViewToolbar();      // Row 2 (right, same row)
    m_mainWindow->addToolBarBreak(); // Force new row

    createFormatToolbar();    // Row 3 (left)
    createMacroToolbar();     // Row 3 (right, same row)
}
```

### Toolbar Break Behavior
- `addToolBarBreak()` forces the next toolbar to a new row
- Without break, toolbars appear side-by-side
- Used strategically to create 3-row layout with left/right splits

### Icon Sizing
- All toolbars use 20x20 pixel icons
- Smaller than default (24x24) to fit more buttons
- Still clearly visible and clickable

## Total Button Count

| Row | Toolbar | Buttons |
|-----|---------|---------|
| 1L | File Operations | 13 |
| 1R | Edit Operations | 15 |
| 2L | Search & Navigation | 12 |
| 2R | View & Display | 12 |
| 3L | Text Formatting | 14 |
| 3R | Macro Recording | 5 |
| **Total** | **6 Toolbars in 3 Rows** | **71 Buttons** |

*Note: Additional functions available via menus but not shown in toolbars*

## Comparison with Single-Row Layout

### Old Layout (6 separate toolbars stacked vertically)
```
[File Toolbar - Full Width - 7 buttons]
[Edit Toolbar - Full Width - 12 buttons]
[Search Toolbar - Full Width - 9 buttons]
[View Toolbar - Full Width - 12 buttons]
[Format Toolbar - Full Width - 15 buttons]
[Macro Toolbar - Full Width - 5 buttons]

= 6 rows of toolbars (very tall, takes up editor space)
```

### New Layout (3 rows with strategic splitting)
```
[File Toolbar - Half] [Edit Toolbar - Half] (13+15 buttons)
[Search Toolbar - Half] [View Toolbar - Half] (12+12 buttons)
[Format Toolbar - Half] [Macro Toolbar - Half] (14+5 buttons)

= 3 rows of toolbars (very compact, maximum editor space)
```

## Benefits

1. **Maximum Editor Space** - 50% fewer rows (3 vs 6) - **Best space efficiency!**
2. **Better Organization** - Related tools grouped logically with optimal pairing
3. **All Icons Visible** - No need to scroll or hide toolbars
4. **Familiar Layout** - Similar to Notepad++ multi-bar approach
5. **Efficient Use of Width** - All rows split left/right for balanced toolbar usage

## Future Enhancements

1. **Custom Icons** - Replace Qt standard icons with custom icon pack
2. **Toolbar Presets** - Save/load toolbar configurations
3. **Collapsible Rows** - Hide entire rows when not needed
4. **Icon-Only Mode** - Option to show icons without text for even more compact display
5. **Overflow Handling** - Chevron menu for buttons that don't fit on narrow windows
