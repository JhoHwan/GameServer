@echo off
cd /d %~dp0

:: 1. Protoc 컴파일러 위치 (솔루션 기준 상대 경로)
SET PROTOC_PATH=..\..\ThirdParty\Protobuf\protoc.exe

:: 2. 파이썬 스크립트 위치 (서버 쪽에 있는거 가져다 씀)
SET SCRIPT_PATH=PacketGenerator.py

:: 3. .proto 파일들이 모여있는 폴더 
SET PROTO_DIR=..\GameServer\Proto

:: 4. 결과물이 생성될 언리얼 프로젝트 내부 경로 (Source/내프로젝트/...)
SET OUTPUT_DIR=C:\UnrealProject\MP2\Source\GameNet\Private\Proto

:: ==========================================================

echo [START] Packet Generation for Unreal Client...

:: 출력 폴더가 없으면 생성
if not exist "%OUTPUT_DIR%" (
    mkdir "%OUTPUT_DIR%"
    echo [INFO] Created directory: %OUTPUT_DIR%
)

:: 1. Protoc 실행 (.pb.cc, .pb.h 생성)
echo [1/2] Generating Protobuf C++ Source...
:: 모든 .proto 파일에 대해 루프
for %%f in (%PROTO_DIR%\*.proto) do (
    "%PROTOC_PATH%" --proto_path="%PROTO_DIR%" --cpp_out="%OUTPUT_DIR%" "%%f"
    if errorlevel 1 goto ERROR
    echo    - Compiled: %%~nxf
)

:: 2. PacketGenerator.py 실행 (핸들러 생성)
echo [2/2] Generating Packet Handlers...
for %%f in (%PROTO_DIR%\Protocol.proto) do (
    python "%SCRIPT_PATH%" --path "%%f" --output ClientPacketHandler --out_path "%OUTPUT_DIR%" --process C
    if errorlevel 1 goto ERROR
    echo    - Generated Handler for: %%~nxf
)

echo.
echo [SUCCESS] All packet generation complete!
pause
exit /b 0

:ERROR
echo.
echo [FAIL] An error has occurred. Please check the path and script.
pause
exit /b 1