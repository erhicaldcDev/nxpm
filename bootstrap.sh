#!/bin/bash
# NXPM Bootstrap Script

set -e

REPO_URL="https://raw.githubusercontent.com/erhicaldcDev/nxpm/main"
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

echo -e "${BLUE}:: Creating temporary directory...${NC}"
rm -rf "$TEMP_DIR"
mkdir -p "$TEMP_DIR"
cd "$TEMP_DIR"

echo -e "${BLUE}:: Downloading cJSON dependency...${NC}"
curl -s -O https://raw.githubusercontent.com/DaveGamble/cJSON/master/cJSON.c
curl -s -O https://raw.githubusercontent.com/DaveGamble/cJSON/master/cJSON.h

echo -e "${BLUE}:: Downloading NXPM sources...${NC}"
curl -s -o nxpm.c "$REPO_URL/nxpm.c"
curl -s -o Makefile "$REPO_URL/Makefile"

if [ ! -f "nxpm.c" ] || [ ! -f "Makefile" ]; then
    echo -e "${RED}Error: Failed to download source files. Check the repository URL.${NC}"
    exit 1
fi

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
