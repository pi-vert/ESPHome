#!/usr/bin/env bash
# Installer le binaire Caddy des dépôts Ubuntu dans le compte utilisateur.
set -euo pipefail

temporary_dir=$(mktemp -d)
trap 'rm -rf "$temporary_dir"' EXIT

(
  cd "$temporary_dir"
  apt-get download caddy
)
destination="$HOME/.local/share/esphome-caddy"
mkdir -p "$destination"
dpkg-deb --extract "$temporary_dir"/*.deb "$destination"
"$destination/usr/bin/caddy" version
