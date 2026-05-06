#!/bin/bash

# Копируем конфигурацию clang-format
#cp ../materials/linters/.clang-format .

clang-format -n $(find . -name "*.cpp" -o -name "*.h")
clang-format -i $(find . -name "*.cpp" -o -name "*.h")

# Удаляем временный файл конфигурации
#rm .clang-format
