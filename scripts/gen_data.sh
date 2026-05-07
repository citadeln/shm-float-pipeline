#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_FILE="$SCRIPT_DIR/../data/source_data.bin"

# Создаем папку data, если её нет (на уровне выше папки scripts)
mkdir -p "$SCRIPT_DIR/../data"

echo "Генерация случайных данных в $OUTPUT_FILE..."

python3 -c "
import struct
import random
import os

try:
    with open('$OUTPUT_FILE', 'wb') as f:
        for _ in range(100):
            val = random.uniform(-100.0, 100.0)
            f.write(struct.pack('f', val))
    print('Успешно записано 100 float-чисел.')
except Exception as e:
    print(f'Ошибка при записи: {e}')
"

# hexdump -C output.bin | head
# od -t fF output.bin
