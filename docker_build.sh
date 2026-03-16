#!/bin/bash
# =============================================================================
# Docker build and run script for llvm-tutor (Ubuntu with apt LLVM)
# 
# Supports both development mode (volume mount) and static build mode.
# =============================================================================

set -e

IMAGE_NAME="llvm-tutor:llvm-21"
DOCKERFILE="Dockerfile_ubuntu_apt"
CONTAINER_NAME="llvm-tutor-dev"
TUTOR_DIR="/llvm-tutor"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_help() {
    echo "Usage: $0 [command]"
    echo ""
    echo "Commands:"
    echo "  build     Build the Docker image (base image with dependencies)"
    echo "  dev       Start dev container with local folder mounted (edit locally, build in docker)"
    echo "  run       Run the container interactively (no mount - uses image content)"
    echo "  test      Run the test suite inside a dev container"
    echo "  build-in-docker    Build the project inside dev container"
    echo "  clean     Remove the Docker image"
    echo "  help      Show this help message"
    echo ""
    echo "Examples:"
    echo "  # Start development environment (mount current folder)"
    echo "  $0 dev"
    echo ""
    echo "  # Inside container - build the project"
    echo "  mkdir -p build && cd build && cmake -DLT_LLVM_INSTALL_DIR=/usr/lib/llvm-21 .. && make -j\$(nproc)"
    echo ""
    echo "  # Run tests inside container"
    echo "  cd build && lit test/"
}

cmd_build() {
    echo -e "${YELLOW}Building Docker image: $IMAGE_NAME...${NC}"
    docker build -f "$DOCKERFILE" -t="$IMAGE_NAME" .
    echo -e "${GREEN}Build completed successfully!${NC}"
    echo ""
    echo -e "${BLUE}Next steps:${NC}"
    echo "  $0 dev    - Start dev container with your local files mounted"
}

cmd_dev() {
    echo -e "${YELLOW}Starting development container with local folder mounted...${NC}"
    echo -e "${BLUE}Current directory will be mounted at $TUTOR_DIR${NC}"
    docker run --rm -it \
        -v "$(pwd):$TUTOR_DIR" \
        --hostname=llvm-tutor \
        "$IMAGE_NAME" /bin/bash
}

cmd_run() {
    echo -e "${YELLOW}Running container interactively (no mount)...${NC}"
    docker run --rm -it --hostname=llvm-tutor "$IMAGE_NAME" /bin/bash
}

cmd_test() {
    echo -e "${YELLOW}Running tests inside dev container...${NC}"
    docker run --rm \
        -v "$(pwd):$TUTOR_DIR" \
        "$IMAGE_NAME" /bin/bash -c "cd $TUTOR_DIR/build && lit test/"
}

cmd_build_in_docker() {
    echo -e "${YELLOW}Building llvm-tutor inside dev container...${NC}"
    docker run --rm \
        -v "$(pwd):$TUTOR_DIR" \
        "$IMAGE_NAME" /bin/bash -c "
            mkdir -p $TUTOR_DIR/build && 
            cd $TUTOR_DIR/build && 
            cmake -DLT_LLVM_INSTALL_DIR=/usr/lib/llvm-21 .. && 
            make -j\$(nproc)
        "
}

cmd_clean() {
    echo -e "${YELLOW}Removing Docker image: $IMAGE_NAME...${NC}"
    docker rmi "$IMAGE_NAME" || echo -e "${RED}Image not found or could not be removed.${NC}"
}

# Main
COMMAND="${1:-help}"

case "$COMMAND" in
    build)
        cmd_build
        ;;
    dev)
        cmd_dev
        ;;
    run)
        cmd_run
        ;;
    test)
        cmd_test
        ;;
    build-in-docker)
        cmd_build_in_docker
        ;;
    clean)
        cmd_clean
        ;;
    help|*)
        print_help
        ;;
esac
