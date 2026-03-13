#!/bin/bash

# 스크립트 위치 기준 경로 설정
SCRIPT_DIR=$(dirname "$0")
cd "$SCRIPT_DIR"

# 1. Protoc 컴파일러 위치 (리눅스용)
PROTOC_PATH="../../ThirdParty/Protobuf/protoc"

# 2. 파이썬 스크립트 위치
SCRIPT_PATH="PacketGenerator.py"

# 3. .proto 파일들이 모여있는 폴더 
PROTO_DIR="../GameServer/Proto"

# 4. 언리얼 클라이언트 경로 설정 (WSL 마운트 경로 기본값)
# 윈도우 C:\UnrealProject\MP2\Source\MP2\Network 경로를 WSL에서 접근합니다.
DEFAULT_TARGET_DIR="/mnt/c/UnrealProject/MP2/Source/MP2/Network"
TARGET_DIR="${UNREAL_NETWORK_PATH:-$DEFAULT_TARGET_DIR}"

# ==========================================================

echo "[START] Packet Generation for Unreal Client..."

# 출력 폴더가 없으면 생성
if [ ! -d "$TARGET_DIR" ]; then
    mkdir -p "$TARGET_DIR"
    echo "[INFO] Created directory: $TARGET_DIR"
fi

# 1. Protoc 실행 (Target 폴더에 바로 생성)
echo "[1/2] Generating Protobuf C++ Source..."
for f in "$PROTO_DIR"/*.proto; do
    "$PROTOC_PATH" -I="$PROTO_DIR" --cpp_out="$TARGET_DIR" "$f"
    if [ $? -ne 0 ]; then
        echo "[FAIL] Failed to compile $f"
        exit 1
    fi
    echo "   - Compiled: $(basename "$f")"
done

# 2. PacketGenerator.py 실행 (핸들러 생성, Target 폴더에 저장)
echo "[2/2] Generating Packet Handlers..."
python3 "$SCRIPT_PATH" --path "$PROTO_DIR/Protocol.proto" --output ClientPacketHandler --out_path "$TARGET_DIR" --process C
if [ $? -ne 0 ]; then
    echo "[FAIL] Failed to generate packet handler"
    exit 1
fi
echo "   - Generated Handler for: Protocol.proto"

echo ""
echo "[SUCCESS] All packet generation complete! Files are in $TARGET_DIR"
exit 0
