# Burning Ground 16:9 런처

BG v2.00 / StarCraft 1.16.1용 Windows 런처입니다. 내부 해상도 1024×576, 1280×720, 1536×864 및 원본 640×480을 선택할 수 있습니다. v0.2.0부터 확장 화면의 하단 HUD를 중앙에 배치하고 HUD 양옆을 화면 끝까지 지형으로 채웁니다. HUD 자체의 정보 패널은 읽기 쉽게 기존 배경을 유지합니다.

## 사용

1. releases/BurningGroundLauncher-0.3.1.exe를 실행합니다.
2. BG_v2.00.exe가 있는 게임 폴더와 해상도를 선택합니다.
3. 게임 실행을 누릅니다. W-MODE 플러그인 질문에는 **아니요**를 선택합니다.

원본 게임을 수정하지 않고 %LOCALAPPDATA%/BurningGround/runtime에 실행 복사본을 만듭니다. 최초 복사는 시간이 필요합니다. 저장 게임과 프로필은 별도 실행 폴더에 저장됩니다. 게임 데이터는 배포하지 않습니다.

기존 HUD는 640픽셀 폭을 유지합니다. v0.2.0은 1024×576 싱글 경기에서 중앙 HUD, 선택 정보, 하단 지형 표시를 확인했습니다. 1280×720과 1536×864는 동일한 생성기로 새 패치 테이블을 검증했습니다. 멀티플레이, 장시간 경기, 모든 캠페인 및 저장 불러오기는 미검증입니다. 실험적 버전입니다.

## 업데이트

v0.3.1: 경과 시간 글자를 기존의 절반 크기로 줄이고 미니맵 오른쪽 위의 빈 지형 영역으로 옮겼습니다.

v0.3.0: 자원/인구수 표시를 오른쪽 상단에 배치하고 HUD 오른쪽 위에 경기 경과 시간을 분:초로 표시합니다. 시간은 게임 내부 경과 시간(0x0058D6F8)을 읽으므로 게임 속도와 일시정지를 따릅니다. [BWAPI의 BWGame 구조 정의](https://github.com/bwapi/bwapi/blob/main/bwapi/BWAPI/Source/BW/BWGame.h)의 elapsedTime 오프셋을 참고했습니다. 시계는 게임 상태나 클릭 영역을 바꾸지 않는 화면 오버레이입니다. 원본 640×480 모드는 기존 화면을 유지합니다.

런처 업데이트 버튼은 main/version.json을 확인하고 새 EXE의 크기와 SHA-256을 검증한 뒤 런처를 교체하고 재실행합니다. 이전 EXE는 .previous로 보관합니다. 게임 데이터는 업데이트하지 않습니다.

배포자는 releases/에 새 EXE를 올리고 version.json의 version, url, size, sha256을 갱신합니다. manifest는 별도로 서명하지 않으며 GitHub 저장소 권한과 HTTPS를 신뢰합니다.

## 빌드

Python과 requirements-build.txt를 설치합니다. BG_PYTHON을 python.exe 경로로 지정하고 build_launcher.ps1을 실행합니다. 네이티브 빌드는 BG_CXX를 i686 MinGW g++.exe 경로로 지정하고 build_native.ps1을 실행합니다.

테스트: python -m unittest discover -s tests

## 참고 및 라이선스

- https://github.com/inwenis/decompile-sc : MIT. 화면/메뉴/HUD/세션/Storm 코드. vendor/sc-display/LICENSE 및 PROVENANCE.md 참조.
- https://github.com/FunkyFr3sh/cnc-ddraw : MIT. assets/cnc-ddraw-LICENSE.txt 참조.

StarCraft 및 BG 자산은 포함하지 않습니다.
