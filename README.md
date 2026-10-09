# Burning Ground 16:9 런처

BG v2.00 / StarCraft 1.16.1용 Windows 런처입니다. 내부 해상도 1280×720, 1536×864 및 원본 640×480을 선택할 수 있습니다.

## 사용

1. releases/BurningGroundLauncher-0.1.0.exe를 실행합니다.
2. BG_v2.00.exe가 있는 게임 폴더와 해상도를 선택합니다.
3. 게임 실행을 누릅니다. W-MODE 플러그인 질문에는 **아니요**를 선택합니다.

원본 게임을 수정하지 않고 %LOCALAPPDATA%/BurningGround/runtime에 실행 복사본을 만듭니다. 최초 복사는 시간이 필요합니다. 저장 게임과 프로필은 별도 실행 폴더에 저장됩니다. 게임 데이터는 배포하지 않습니다.

기존 HUD는 640픽셀 폭을 유지합니다. 1280×720 싱글 경기에서 지도 표시와 확장 영역 유닛 조작을 확인했습니다. 1536×864 전체 화면 메뉴와 패치 적용도 확인했습니다. 멀티플레이, 장시간 경기, 모든 캠페인 및 저장 불러오기는 미검증입니다. 실험적 버전입니다.

## 업데이트

런처 업데이트 버튼은 main/version.json을 확인하고 새 EXE의 크기와 SHA-256을 검증한 뒤 런처를 교체하고 재실행합니다. 이전 EXE는 .previous로 보관합니다. 게임 데이터는 업데이트하지 않습니다.

배포자는 releases/에 새 EXE를 올리고 version.json의 version, url, size, sha256을 갱신합니다. manifest는 별도로 서명하지 않으며 GitHub 저장소 권한과 HTTPS를 신뢰합니다.

## 빌드

Python과 requirements-build.txt를 설치합니다. BG_PYTHON을 python.exe 경로로 지정하고 build_launcher.ps1을 실행합니다. 네이티브 빌드는 BG_CXX를 i686 MinGW g++.exe 경로로 지정하고 build_native.ps1을 실행합니다.

테스트: python -m unittest discover -s tests

## 참고 및 라이선스

- https://github.com/inwenis/decompile-sc : MIT. 화면/메뉴/HUD/세션/Storm 코드. vendor/sc-display/LICENSE 및 PROVENANCE.md 참조.
- https://github.com/FunkyFr3sh/cnc-ddraw : MIT. assets/cnc-ddraw-LICENSE.txt 참조.

StarCraft 및 BG 자산은 포함하지 않습니다.
