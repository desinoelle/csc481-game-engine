COLOR_GREEN="\e[1;32m"
COLOR_RED="\e[1;31m"
COLOR_NO="\e[0m"


SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR" || exit 1

# CLEAN PREVIOUS BUILD
if [ -d "build" ]; then
    echo -e "${COLOR_GREEN}Deleting preexisting build files...${COLOR_NO}"
    rm -rf build
fi

# CONFIGURE AND BUILD
echo -e "${COLOR_GREEN}Building project...${COLOR_NO}"
mkdir build
cd build || exit 1

cmake ..
cmake --build .