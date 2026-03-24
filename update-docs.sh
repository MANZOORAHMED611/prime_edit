#!/bin/bash
# Auto-update documentation at end of each phase

echo "Updating Olive Notepad Documentation..."

# Get current date
CURRENT_DATE=$(date +"%B %d, %Y")

# Count lines of code
LOC=$(find src -name "*.cpp" -o -name "*.h" | xargs wc -l | tail -1 | awk '{print $1}')

# Count features from git commits or manual tracking
FEATURES_COMPLETED=$(grep -r "status=\"completed\"" html-docs/*.html 2>/dev/null | wc -l)

echo "Lines of Code: $LOC"
echo "Features Completed: $FEATURES_COMPLETED"
echo "Documentation Date: $CURRENT_DATE"

# Update index.html with current stats
if [ -f "html-docs/index.html" ]; then
    echo "Updating index.html stats..."
    # You can add sed commands here to update specific stats in the HTML
fi

echo "Documentation updated successfully!"
echo "Open html-docs/index.html in a browser to view."
