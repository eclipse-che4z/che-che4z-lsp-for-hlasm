#!/usr/bin/env sh

set -e

sudo apt-get update
sudo apt-get install -y qemu-user

mkdir -p bin/linux_x64
cd bin/linux_x64

echo '#!/usr/bin/env sh' > hlasm_language_server
echo 'qemu-aarch64 "$(dirname "$0")/../linux_arm64/hlasm_language_server" "$@"' >> hlasm_language_server

chmod +x hlasm_language_server
