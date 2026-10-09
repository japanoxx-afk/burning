$ErrorActionPreference='Stop'
& $env:BG_PYTHON -m PyInstaller --noconfirm --clean --onefile --windowed --name BurningGroundLauncher --add-data 'assets;assets' launcher.py
if ($LASTEXITCODE) { throw 'launcher build failed' }
