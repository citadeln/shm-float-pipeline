#!/bin/bash

set -e

cd build
./producer ../data/source_data.bin & 
producer_pid=$! # Сохраняем ID процесса Producer, чтобы потом его "убить"
echo "Producer запущен с PID: $producer_pid"

# Даем Producer секунду на запуск и инициализацию семафоров
sleep 1

# Запускаем Consumer (он дождется Producer и считает данные)
./consumer ../data/source_data.bin ../data/output.bin

# Ждем завершения фонового процесса Producer (на всякий случай)
wait $producer_pid 2>/dev/null || true

echo "--- START TESTS ---"
./test_utils
echo "--- END TESTS ---"

#cd build
#strace -f ./consumer ... 2>&1 | head -n 50
#journalctl -ke | tail -n 50
#tail -n 50 /var/log/syslog