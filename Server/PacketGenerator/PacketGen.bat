@echo off
cd /d %~dp0

:: 1. Protoc 컴파일러 위치 (솔루션 기준 상대 경로)
SET PROTOC_PATH=..\..\ThirdParty\Protobuf\protoc.exe

:: 2. 파이썬 스크립트 위치 (서버 쪽에 있는거 가져다 씀)
SET SCRIPT_PATH=PacketGenerator.py

:: 3. .proto 파일들이 모여있는 폴더 
SET PROTO_DIR=..\GameServer\Proto

:: 4. 언리얼 클라이언트 경로 설정 (Network 폴더로 통합)
SET TARGET_DIR=C:\Project\GameServer\Client\MP2\Source\MP2\Network

:: ==========================================================

echo [START] Packet Generation for Unreal Client...

:: 출력 폴더가 없으면 생성
if not exist "%TARGET_DIR%" (
    mkdir "%TARGET_DIR%"
    echo [INFO] Created directory: %TARGET_DIR%
)

:: 1. Protoc 실행 (Target 폴더에 바로 생성)
echo [1/2] Generating Protobuf C++ Source...
for %%f in (%PROTO_DIR%\*.proto) do (
    "%PROTOC_PATH%" -I="%PROTO_DIR%" --cpp_out="%TARGET_DIR%" "%%f"
    if errorlevel 1 goto ERROR
    echo    - Compiled: %%~nxf
)

:: 2. PacketGenerator.py 실행 (핸들러 생성, Target 폴더에 저장)
echo [2/2] Generating Packet Handlers...
for %%f in (%PROTO_DIR%\Protocol.proto) do (
    python "%SCRIPT_PATH%" --path "%%f" --output ClientPacketHandler --out_path "%TARGET_DIR%" --process C
    if errorlevel 1 goto ERROR
    echo    - Generated Handler for: %%~nxf
)

echo.
echo [SUCCESS] All packet generation complete!
exit /b 0

:ERROR
echo.
echo [FAIL] An error has occurred. Please check the path and script.
exit /b 1
