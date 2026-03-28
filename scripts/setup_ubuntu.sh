#!/usr/bin/env bash

set -e

echo "Updating package index..."
sudo apt update

echo "Installing required packages..."
sudo apt install -y \
  build-essential \
  cmake \
  git \
  libgpiod-dev \
  libjsoncpp-dev

echo "Adding current user to 'gpio' group (you must re-login)..."
sudo usermod -aG gpio "$USER"

echo "Done. Reboot or re-login to apply group changes."

