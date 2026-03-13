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

# 4. 출력 경로 설정
# (A) 언리얼 클라이언트 경로 (WSL 마운트 경로 기본값)
DEFAULT_TARGET_DIR="/mnt/c/UnrealProject/MP2/Source/MP2/Network"
TARGET_DIR="${UNREAL_NETWORK_PATH:-$DEFAULT_TARGET_DIR}"

# (B) 더미 클라이언트 경로 (리눅스 로컬)
DUMMY_TARGET_DIR="../DummyClient/Packet"

# ==========================================================

echo "[START] Packet Generation..."

# 출력 폴더 생성
for dir in "$TARGET_DIR" "$DUMMY_TARGET_DIR"; do
    if [ ! -d "$dir" ]; then
        mkdir -p "$dir"
        echo "[INFO] Created directory: $dir"
    fi
done

# 1. Protoc 실행
echo "[1/2] Generating Protobuf C++ Source..."
for f in "$PROTO_DIR"/*.proto; do
    "$PROTOC_PATH" -I="$PROTO_DIR" --cpp_out="$TARGET_DIR" "$f"
    # 더미 클라이언트용 소스도 생성 (필요 시)
    "$PROTOC_PATH" -I="$PROTO_DIR" --cpp_out="$DUMMY_TARGET_DIR" "$f"
    
    if [ $? -ne 0 ]; then
        echo "[FAIL] Failed to compile $f"
        exit 1
    fi
    echo "   - Compiled: $(basename "$f")"
done

# 2. PacketGenerator.py 실행 (핸들러 생성)
echo "[2/2] Generating Packet Handlers..."
# (A) 언리얼용
python3 "$SCRIPT_PATH" --path "$PROTO_DIR/Protocol.proto" --output ClientPacketHandler --out_path "$TARGET_DIR" --process C
# (B) 더미 클라이언트용
python3 "$SCRIPT_PATH" --path "$PROTO_DIR/Protocol.proto" --output ClientPacketHandler --out_path "$DUMMY_TARGET_DIR" --process C

if [ $? -ne 0 ]; then
    echo "[FAIL] Failed to generate packet handler"
    exit 1
fi
echo "   - Generated Handler for: Protocol.proto"

echo ""
echo "[SUCCESS] All packet generation complete! Files are in $TARGET_DIR"
exit 0
