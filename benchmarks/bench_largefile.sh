#!/bin/bash
# Olive Notepad Large File Benchmark Suite
# Usage: ./bench_largefile.sh [path-to-olive-notepad-binary]

BINARY="${1:-./build/Olive Notepad}"
TEMP_DIR=$(mktemp -d)

echo "=== Olive Notepad Large File Benchmark ==="
echo "Binary: $BINARY"
echo "Temp dir: $TEMP_DIR"
echo ""

# Generate test files
echo "Generating test files..."

echo "  100MB..."
python3 -c "
for i in range(2_000_000):
    print(f'Line {i}: The quick brown fox jumps over the lazy dog. Lorem ipsum dolor sit amet.')
" > "$TEMP_DIR/test_100mb.txt"

echo "  500MB..."
python3 -c "
for i in range(10_000_000):
    print(f'Line {i}: The quick brown fox jumps over the lazy dog. Lorem ipsum dolor sit amet.')
" > "$TEMP_DIR/test_500mb.txt"

echo "  1GB..."
python3 -c "
for i in range(20_000_000):
    print(f'Line {i}: The quick brown fox jumps over the lazy dog. Lorem ipsum dolor sit amet.')
" > "$TEMP_DIR/test_1gb.txt"

echo ""
echo "File sizes:"
ls -lh "$TEMP_DIR"/*.txt
echo ""

# Benchmark: File open time
echo "=== Open Time ==="
for file in "$TEMP_DIR"/test_*.txt; do
    name=$(basename "$file")
    size=$(du -h "$file" | cut -f1)

    start=$(date +%s%N)
    timeout 30 "$BINARY" --benchmark-open "$file" 2>/dev/null
    end=$(date +%s%N)

    ms=$(( (end - start) / 1000000 ))
    echo "  $name ($size): ${ms}ms"
done

# Benchmark: Memory usage
echo ""
echo "=== Memory Usage ==="
for file in "$TEMP_DIR"/test_*.txt; do
    name=$(basename "$file")
    "$BINARY" "$file" &
    PID=$!
    sleep 3
    RSS=$(ps -o rss= -p $PID 2>/dev/null | tr -d ' ')
    if [ -n "$RSS" ]; then
        RSS_MB=$((RSS / 1024))
        echo "  $name: ${RSS_MB}MB RSS"
    fi
    kill $PID 2>/dev/null
    wait $PID 2>/dev/null
done

# Cleanup
rm -rf "$TEMP_DIR"
echo ""
echo "=== Benchmark Complete ==="
