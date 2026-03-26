# Olive Notepad - HTML Documentation

This directory contains comprehensive technical documentation for Olive Notepad in HTML format.

## Documentation Structure

- **index.html** - Main documentation homepage with project overview and feature summary
- **requirements.html** - Complete requirements specification converted from original markdown
- **architecture.html** - System architecture, component structure, and design patterns
- **phase1.html** - Phase 1 (MVP) implementation details (to be created)
- **phase2.html** - Phase 2 (Power Features) implementation details (to be created)
- **phase3.html** - Phase 3 (Developer Tools) planning (to be created)
- **phase4.html** - Phase 4 (Ecosystem) planning (to be created)
- **api-reference.html** - API documentation for classes and methods (to be created)
- **data-structures.html** - Detailed explanation of piece table and other data structures (to be created)
- **ui-components.html** - UI component documentation (to be created)
- **features.html** - User-facing feature list (to be created)
- **keyboard-shortcuts.html** - Complete keyboard shortcut reference (to be created)
- **building.html** - Build instructions and deployment guide (to be created)

## Viewing Documentation

Simply open `index.html` in any modern web browser:

```bash
cd html-docs
firefox index.html  # Or chrome, chromium, etc.
```

## Documentation Features

- **No JavaScript Required** - Pure HTML + CSS for maximum compatibility
- **Print-Friendly** - Optimized print stylesheets for PDF generation
- **Dark Theme** - Professional dark color scheme matching the editor
- **Responsive Design** - Works on desktop and mobile devices
- **Navigation Sidebar** - Easy navigation between documentation pages

## Auto-Generated Content

This documentation is automatically updated at the end of each development phase with:
- Current implementation status
- Feature completion badges
- Performance metrics
- API changes
- New component documentation

## Generating PDF Documentation

To convert HTML documentation to PDF:

```bash
# Using wkhtmltopdf
wkhtmltopdf --enable-local-file-access index.html notepad-supreme-docs.pdf

# Using browser print (Ctrl+P in Firefox/Chrome)
# Select "Save as PDF" as destination
```

## Contributing to Documentation

When adding new features:
1. Update the relevant HTML files
2. Add new sections with appropriate status badges
3. Update the navigation sidebar if adding new pages
4. Regenerate any auto-generated content
5. Test in multiple browsers for compatibility

---

**Last Updated:** November 28, 2025
**Documentation Version:** 1.0
**Application Version:** 1.0.0
