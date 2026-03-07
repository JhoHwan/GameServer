@echo off
cd /d %~dp0

:: 1. Protoc 컴파일러 위치 (솔루션 기준 상대 경로)
SET PROTOC_PATH=..\..\ThirdParty\Protobuf\protoc.exe

:: 2. 파이썬 스크립트 위치 (서버 쪽에 있는거 가져다 씀)
SET SCRIPT_PATH=PacketGenerator.py

:: 3. .proto 파일들이 모여있는 폴더 
SET PROTO_DIR=..\GameServer\Proto

:: 4. 언리얼 Public / Private 경로 설정
SET PUBLIC_DIR=C:\UnrealProject\MP2\Source\GameNet\Public
SET PRIVATE_DIR=C:\UnrealProject\MP2\Source\GameNet\Private

:: ==========================================================

echo [START] Packet Generation for Unreal Client...

:: 출력 폴더가 없으면 생성
if not exist "%PUBLIC_DIR%" (
    mkdir "%PUBLIC_DIR%"
    echo [INFO] Created directory: %PUBLIC_DIR%
)
if not exist "%PRIVATE_DIR%" (
    mkdir "%PRIVATE_DIR%"
    echo [INFO] Created directory: %PRIVATE_DIR%
)

:: 1. Protoc 실행 (dllexport_decl 적용 및 Private 폴더에 우선 생성)
echo [1/3] Generating Protobuf C++ Source (with GAMENET_API)...
for %%f in (%PROTO_DIR%\*.proto) do (
    "%PROTOC_PATH%" -I="%PROTO_DIR%" --cpp_out=dllexport_decl=GAMENET_API:"%PRIVATE_DIR%" "%%f"
    if errorlevel 1 goto ERROR
    echo    - Compiled: %%~nxf
)

:: 2. 헤더 파일(.h)을 Public 폴더로 이동
echo [2/3] Moving Header files to Public Directory...
move /Y "%PRIVATE_DIR%\*.h" "%PUBLIC_DIR%\" >nul
if errorlevel 1 goto ERROR

:: 3. PacketGenerator.py 실행 (핸들러 생성, Public 폴더에 저장)
echo [3/3] Generating Packet Handlers...
for %%f in (%PROTO_DIR%\Protocol.proto) do (
    python "%SCRIPT_PATH%" --path "%%f" --output ClientPacketHandler --out_path "%PUBLIC_DIR%" --process C
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