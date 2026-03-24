# Olive Notepad - Toolbar Icon Implementation Guide

## Overview

Olive Notepad uses a 3-row toolbar layout with 71+ toolbar buttons. This document outlines the icon implementation strategy and provides guidance for creating/sourcing proper icons.

## Current Icon Status

### Using Qt Standard Icons
Currently, the application uses Qt's built-in standard icons via `QStyle::standardIcon()`. These provide basic functionality but may not be visually consistent across platforms.

**Advantages:**
- No external dependencies
- Cross-platform compatibility
- Automatic theme adaptation
- Zero setup required

**Disadvantages:**
- Limited icon set
- Platform-dependent appearance
- Not all functions have standard icons
- May look dated or inconsistent

## Recommended Icon Approach

### Option 1: Material Design Icons (Recommended)
Use Material Design Icons - a comprehensive, modern icon set.

**Benefits:**
- 7000+ icons available
- Modern, consistent design
- Free and open source (Apache 2.0)
- SVG format (scalable)
- Widely recognized visual language

**Implementation:**
1. Download Material Design Icons SVG set
2. Add needed icons to `resources/icons/toolbar/`
3. Update `resources.qrc` to include icons
4. Modify `toolbarmanager.cpp` to use new icon paths

**Icon Mapping:**
```cpp
// Example icon paths
file-new.svg         → New file
folder-open.svg      → Open file
content-save.svg     → Save
content-save-all.svg → Save all
close.svg            → Close
undo.svg             → Undo
redo.svg             → Redo
content-cut.svg      → Cut
content-copy.svg     → Copy
content-paste.svg    → Paste
magnify.svg          → Find
find-replace.svg     → Replace
zoom-in.svg          → Zoom in
zoom-out.svg         → Zoom out
```

### Option 2: Font Awesome Icons
Use Font Awesome icon font or SVG sprites.

**Benefits:**
- Large icon library (2000+ free icons)
- Professional design
- Icon font or SVG format
- Widely used in web/desktop apps

**Implementation:**
Similar to Material Design Icons approach.

### Option 3: Custom Icon Pack
Create or commission a custom icon set matching Olive Notepad's brand.

**Benefits:**
- Unique visual identity
- Perfect brand alignment
- Optimized for specific use case

**Disadvantages:**
- Time-consuming to create
- Requires design skills or budget
- Maintenance overhead

## Icon Specifications

### Technical Requirements
- **Format:** SVG (preferred) or PNG
- **Size:** 20x20 pixels (current toolbar icon size)
- **Color:** Monochrome or duotone for consistency
- **Style:** Flat design, simple shapes
- **File naming:** lowercase-with-hyphens.svg

### Design Guidelines
- **Clarity:** Icons should be recognizable at small sizes
- **Consistency:** Use consistent stroke width and style
- **Simplicity:** Avoid excessive detail
- **Contrast:** Ensure good contrast with toolbar background
- **Accessibility:** Consider colorblind users

## Implementation Steps

### Phase 1: Core Icons (High Priority)
Focus on most-used toolbar actions:
1. File operations (New, Open, Save, Close)
2. Edit operations (Undo, Redo, Cut, Copy, Paste)
3. Search (Find, Replace)
4. View (Zoom In/Out, Word Wrap)

### Phase 2: Extended Icons (Medium Priority)
5. Line operations (Duplicate, Delete, Move)
6. Navigation (Go to Line, Bookmarks, Tabs)
7. Code operations (Comment, Format)

### Phase 3: Advanced Icons (Lower Priority)
8. Text formatting (Case conversion, Trim)
9. Macro operations (Record, Play, Save)
10. Advanced view options (Fold, Distraction-free)

## Code Integration

### Current Implementation
```cpp
// toolbarmanager.cpp - Current approach
QAction *newAction = createAction("new", tr("New"), tr("New File (Ctrl+N)"),
                                 "Ctrl+N", SLOT(newFile()));
newAction->setIcon(m_mainWindow->style()->standardIcon(QStyle::SP_FileIcon));
```

### Proposed Implementation with Custom Icons
```cpp
// toolbarmanager.cpp - With custom icons
QAction *newAction = createAction("new", tr("New"), tr("New File (Ctrl+N)"),
                                 "Ctrl+N", SLOT(newFile()),
                                 ":/icons/toolbar/file-new.svg");
```

### Updated createAction Method
```cpp
QAction* ToolbarManager::createAction(const QString &name, const QString &text,
                                     const QString &tooltip, const QString &shortcut,
                                     const char *slot, const QString &iconPath)
{
    QAction *action = new QAction(text, m_mainWindow);
    action->setToolTip(tooltip);

    if (!shortcut.isEmpty()) {
        action->setShortcut(QKeySequence(shortcut));
    }

    if (!iconPath.isEmpty()) {
        QIcon icon(iconPath);
        if (!icon.isNull()) {
            action->setIcon(icon);
        } else {
            // Fallback to Qt standard icon or default
            qWarning() << "Icon not found:" << iconPath;
            action->setIcon(m_mainWindow->style()->standardIcon(QStyle::SP_FileIcon));
        }
    }

    if (slot) {
        connect(action, SIGNAL(triggered()), m_mainWindow, slot);
    }

    m_actions[name] = action;
    return action;
}
```

## Icon Resources

### Free Icon Sources
1. **Material Design Icons** - https://materialdesignicons.com/
2. **Font Awesome** - https://fontawesome.com/
3. **Feather Icons** - https://feathericons.com/
4. **Heroicons** - https://heroicons.com/
5. **Bootstrap Icons** - https://icons.getbootstrap.com/
6. **Tabler Icons** - https://tabler-icons.io/

### Qt Icon Themes
Consider using Qt's icon theme system for automatic platform integration:
```cpp
QIcon::fromTheme("document-new", fallbackIcon);
```

## Toolbar Icon Complete List

### File Operations (13 icons)
- file-new
- folder-open
- content-save
- content-save-all
- close
- close-all
- close-others
- close-right
- close-left
- printer (print - placeholder)

### Edit Operations (15 icons)
- undo
- redo
- content-cut
- content-copy
- content-paste
- select-all
- content-duplicate (duplicate line)
- delete-line
- arrow-up (move line up)
- arrow-down (move line down)
- vector-combine (join lines)
- vector-split (split lines)
- comment-outline (toggle comment)
- comment-multiple (block comment)

### Search & Navigation (12 icons)
- magnify (find)
- find-replace
- arrow-right-bold (find next)
- arrow-left-bold (find previous)
- format-list-numbered (go to line)
- chevron-left (previous tab)
- chevron-right (next tab)
- bookmark-outline (toggle bookmark)
- bookmark-plus (next bookmark)
- bookmark-minus (previous bookmark)
- bookmark-remove (clear bookmarks)

### View & Display (12 icons)
- magnify-plus (zoom in)
- magnify-minus (zoom out)
- numeric-100-box (reset zoom)
- wrap (word wrap)
- format-pilcrow (show whitespace)
- format-line-weight (show EOL)
- format-indent-increase (indent guide)
- fullscreen
- fullscreen-exit (distraction free)
- chevron-double-up (fold all)
- chevron-double-down (unfold all)
- unfold-more-horizontal (toggle fold)

### Text Formatting (14 icons)
- format-letter-case-upper (UPPERCASE)
- format-letter-case-lower (lowercase)
- format-letter-case (Title Case)
- format-letter-case-alt (invert case)
- format-align-right (trim trailing)
- format-align-left (trim leading)
- format-horizontal-align-center (trim both)
- keyboard-tab (tab to space)
- keyboard-space (space to tab)
- delete-empty (remove empty lines)
- content-duplicate-minus (remove duplicates)
- sort-alphabetical-ascending (A→Z)
- sort-alphabetical-descending (Z→A)

### Macro Operations (5 icons)
- record-circle (start recording)
- stop-circle (stop recording)
- play-circle (playback)
- content-save-settings (save macro)
- folder-open-outline (load macro)

## Next Steps

1. **Choose Icon Set** - Select Material Design Icons or Font Awesome
2. **Download Icons** - Get SVG files for all required icons
3. **Organize Files** - Place in `resources/icons/toolbar/`
4. **Update QRC** - Add all icons to `resources/resources.qrc`
5. **Update Code** - Modify `toolbarmanager.cpp` to use new icons
6. **Test** - Build and verify all icons display correctly
7. **Document** - Update README with icon attribution

## License Considerations

Ensure chosen icon set allows:
- Commercial use (if applicable)
- Modification
- Redistribution
- Attribution requirements met

**Recommended License:** Apache 2.0, MIT, or CC0 (public domain)
