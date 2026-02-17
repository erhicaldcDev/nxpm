#!/bin/bash
# NXPM Bootstrap Script

set -e
REPO_BASE="https://raw.githubusercontent.com/erhicaldcDev/nxpm/main"
SRC_URL="${REPO_BASE}/src"

TEMP_DIR="/tmp/nxpm_bootstrap"

GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${BLUE}:: Starting NXPM installation...${NC}"

if [ "$EUID" -ne 0 ]; then
  echo -e "${RED}Error: Please run as root (sudo).${NC}"
  exit 1
fi

echo -e "${BLUE}:: Creating temporary directory structure...${NC}"
rm -rf "$TEMP_DIR"
mkdir -p "$TEMP_DIR/src"
cd "$TEMP_DIR"

echo -e "${BLUE}:: Downloading cJSON dependency...${NC}"
curl -s -o src/cJSON.c https://raw.githubusercontent.com/DaveGamble/cJSON/master/cJSON.c
curl -s -o src/cJSON.h https://raw.githubusercontent.com/DaveGamble/cJSON/master/cJSON.h

echo -e "${BLUE}:: Downloading NXPM sources...${NC}"

FILES="builder.c builder.h config.h db.c db.h fs.c fs.h git.c git.h main.c network.c network.h ui.c ui.h"

for file in $FILES; do
    echo "   Fetching $file..."
    curl -s -f -o "src/$file" "$SRC_URL/$file" || {
        echo -e "${RED}Error downloading src/$file${NC}"
        exit 1
    }
done

echo "   Fetching Makefile..."
curl -s -f -o Makefile "$REPO_BASE/Makefile" || {
     echo -e "${RED}Error downloading Makefile${NC}"
     exit 1
}

echo -e "${BLUE}:: Compiling...${NC}"
if ! command -v make &> /dev/null; then
    echo -e "${RED}Error: 'make' is not installed.${NC}"
    exit 1
fi

make

echo -e "${BLUE}:: Installing to /usr/local/bin...${NC}"
make install

echo -e "${BLUE}:: Cleaning up...${NC}"
cd /
rm -rf "$TEMP_DIR"

echo -e "${GREEN}:: Success! NXPM has been installed.${NC}"
echo -e "Type 'nxpm' to verify installation."
