#!/usr/bin/env bash
# Зависимости для сборки iot_device_core на Raspberry Pi 4
# (Raspberry Pi OS Bookworm / Debian-based)

set -euo pipefail

echo "=== iot_device_core — установка зависимостей (Raspberry Pi 4) ==="

echo "Обновление индекса пакетов..."
sudo apt-get update

echo "Установка компилятора, CMake, pkg-config, libgpiod, jsoncpp..."
sudo apt-get install -y \
  build-essential \
  cmake \
  pkg-config \
  git \
  libgpiod-dev \
  libjsoncpp-dev

echo "Добавление пользователя $USER в группу gpio (нужен перелогин или reboot)..."
sudo usermod -aG gpio "$USER"

echo "Готово. Выполните перелогин или reboot, затем cmake -S . -B build && cmake --build build -j"
