"""Burning Ground launcher: isolated native widescreen runtime and GitHub updates."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import threading
import time
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import urllib.request

VERSION = '0.2.0'
REPO = 'japanoxx-afk/burning'
MANIFEST_URL = f'https://raw.githubusercontent.com/{REPO}/main/version.json'
DEFAULT_GAME = r'C:\Users\seo\Downloads\Starcraft 1.16.1 FOR MOD'
HOME = Path(os.environ.get('LOCALAPPDATA', str(Path.home()))) / 'BurningGround'
RESOURCE = Path(getattr(sys, '_MEIPASS', Path(__file__).parent)) / 'assets'
MODES = ('1024x576', '1280x720', '1536x864', '640x480 (원본)')
SOURCE_HASHES = {
    'StarCraft.exe':'3e2211ce7a105e5a7b67ebbd33f163a45bf4c45b4d0d759f6d1d966448e0c38f',
    'BG_v2.00.exe':'a2775df3703e24860e580af60723955069355961fc3b0ca0928c15a552c74df1',
    'storm.dll':'706ff2164ca472f27c44235ed55586644e5c86e68cd69b62d76f5a78778bff25',
}

def sha(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(1024*1024), b''): h.update(chunk)
    return h.hexdigest()

def version_key(value):
    parts = value.split('.')
    if len(parts) != 3 or not all(p.isdigit() for p in parts):
        raise ValueError('버전 형식이 올바르지 않습니다.')
    return tuple(map(int, parts))

def load_config():
    try: return json.loads((HOME/'config.json').read_text(encoding='utf-8'))
    except (OSError, ValueError): return {}

def prepare_runtime(source, mode, fullscreen, status):
    if mode not in MODES: raise ValueError('지원하지 않는 해상도입니다.')
    source = Path(source).resolve()
    for name in ('BG_v2.00.exe', 'StarCraft.exe', 'storm.dll', 'fmod.dll'):
        if not (source/name).is_file(): raise ValueError(f'게임 폴더에 {name} 파일이 없습니다.')
    status('BG 모드 및 게임 버전 검증 중…')
    for name,expected in SOURCE_HASHES.items():
        if sha(source/name)!=expected: raise ValueError(f'지원하는 BG v2.00 / StarCraft 1.16.1 파일과 다릅니다: {name}')
    runtime = HOME/'runtime'
    runtime.mkdir(parents=True, exist_ok=True)
    # Refresh changed installation files without importing player saves or settings.
    for file in source.iterdir():
        if file.is_file() and (file.suffix.lower() in ('.mpq','.snp','.dll') or file.name in ('BG_v2.00.exe','StarCraft.exe')):
            dest = runtime/file.name
            if not dest.exists() or dest.stat().st_size != file.stat().st_size or dest.stat().st_mtime_ns != file.stat().st_mtime_ns:
                status(f'실행 폴더 준비: {file.name}')
                shutil.copy2(file,dest)
    if (source/'maps').exists(): shutil.copytree(source/'maps',runtime/'maps',dirs_exist_ok=True)
    for name in ('ddraw.dll','bg_display.dll','bg_bridge.dll','bg_start.exe'):
        shutil.copy2(RESOURCE/name,runtime/name)
    width,height = (1920,1080)
    (runtime/'ddraw.ini').write_text(f'''[ddraw]
width={width}
height={height}
fullscreen={'true' if fullscreen else 'false'}
windowed=true
maintas=true
boxing=false
renderer=auto
maxfps=60
vsync=true
adjmouse=true
devmode=false
resolutions=2
nonexclusive=true
''',encoding='ascii')
    return runtime

def launch_game(source,mode,fullscreen,status):
    import ctypes
    kernel=ctypes.WinDLL('kernel32',use_last_error=True)
    kernel.CreateMutexW.restype=ctypes.c_void_p
    kernel.CreateMutexW.argtypes=[ctypes.c_void_p,ctypes.c_bool,ctypes.c_wchar_p]
    kernel.CloseHandle.argtypes=[ctypes.c_void_p]
    handle=kernel.CreateMutexW(None,False,'Local\\BurningGroundRuntime')
    error=ctypes.get_last_error()
    if not handle: raise RuntimeError('게임 실행 잠금을 만들 수 없습니다.')
    if error==183:
        kernel.CloseHandle(handle)
        raise RuntimeError('이미 BG 게임이 실행 중입니다.')
    try: return _launch_game(source,mode,fullscreen,status)
    finally: kernel.CloseHandle(handle)

def _launch_game(source,mode,fullscreen,status):
    runtime=prepare_runtime(source,mode,fullscreen,status)
    env=os.environ.copy()
    env.update(BG_GAME_DIR=str(runtime), BG_DISPLAY_DLL=str(runtime/'bg_display.dll'),
               SCPLUGIN_LOG=str(HOME/'display.log'), SCPLUGIN_WIDESCREEN='1',
               SCPLUGIN_WS_STAGE='3', SCPLUGIN_WS_GEOMETRY=mode,
               SCPLUGIN_MENU_CENTRE='1', SCPLUGIN_STORM_PRESENT='2')
    if mode.startswith('640'):
        # The bridge selects the isolated install; the display DLL runs only in native modes.
        env['BG_DISPLAY_DLL']='original'
    with open(HOME/'launch.log','a',encoding='utf-8') as log:
        status('BG 모드 실행 중…')
        proc=subprocess.Popen([str(runtime/'bg_start.exe'),str(runtime/'BG_v2.00.exe'),str(runtime/'bg_bridge.dll')],cwd=runtime,env=env,stdout=log,stderr=log,creationflags=0x08000000)
        code=proc.wait()
    if code: raise RuntimeError(f'게임 실행이 종료되었습니다 (코드 {code}). 로그 폴더를 확인하세요.')
    status('게임이 종료되었습니다.')

def fetch_manifest():
    req=urllib.request.Request(MANIFEST_URL,headers={'User-Agent':f'BurningGround/{VERSION}','Cache-Control':'no-cache'})
    with urllib.request.urlopen(req,timeout=20) as response:
        manifest=json.loads(response.read(65536))
    version_key(manifest['version'])
    url=manifest['url']
    if not url.startswith(f'https://raw.githubusercontent.com/{REPO}/main/releases/'):
        raise ValueError('업데이트 주소가 저장소와 일치하지 않습니다.')
    if len(manifest['sha256'])!=64 or any(c not in '0123456789abcdef' for c in manifest['sha256']):
        raise ValueError('업데이트 파일 검증 정보가 잘못되었습니다.')
    if not 1024 < manifest['size'] < 100*1024*1024: raise ValueError('업데이트 파일 크기가 잘못되었습니다.')
    return manifest

def download_update(manifest,status):
    folder=HOME/'updates';folder.mkdir(parents=True,exist_ok=True)
    target=folder/f"BurningGroundLauncher-{manifest['version']}.exe"
    part=target.with_suffix('.part')
    req=urllib.request.Request(manifest['url'],headers={'User-Agent':f'BurningGround/{VERSION}'})
    try:
        with urllib.request.urlopen(req,timeout=30) as response,open(part,'wb') as out:
            total=0
            while True:
                chunk=response.read(256*1024)
                if not chunk: break
                total+=len(chunk)
                if total>manifest['size']: raise ValueError('다운로드 크기가 검증 정보와 다릅니다.')
                out.write(chunk);status(f'런처 업데이트 다운로드 {total*100//manifest["size"]}%')
        if part.stat().st_size!=manifest['size'] or sha(part)!=manifest['sha256']:
            raise ValueError('다운로드 파일의 SHA-256 검증에 실패했습니다.')
        with open(part,'rb') as f:
            if f.read(2)!=b'MZ': raise ValueError('실행 파일 형식이 아닙니다.')
        part.replace(target)
        return target
    finally:
        part.unlink(missing_ok=True)

def apply_update(staged):
    if not getattr(sys,'frozen',False): raise ValueError('빌드된 EXE에서 업데이트를 사용하세요.')
    script=HOME/'updates'/'apply.ps1'
    script.write_text('''param([int]$LauncherPid,[string]$Staged,[string]$Target)
$ErrorActionPreference='Stop'
try {
  Wait-Process -Id $LauncherPid -ErrorAction SilentlyContinue
  $candidate=$Target+'.new'
  Copy-Item -LiteralPath $Staged -Destination $candidate -Force
  if (Test-Path -LiteralPath $Target) { Copy-Item -LiteralPath $Target -Destination ($Target+'.previous') -Force }
  for ($attempt=0; $attempt -lt 30; $attempt++) {
    try { Move-Item -LiteralPath $candidate -Destination $Target -Force; break }
    catch { if ($attempt -eq 29) { throw }; Start-Sleep -Milliseconds 500 }
  }
  Start-Process -FilePath $Target
} catch { $_ | Out-File -LiteralPath ($Staged+'.error.txt') }
''',encoding='utf-8-sig')
    subprocess.Popen(['powershell.exe','-NoProfile','-ExecutionPolicy','Bypass','-File',str(script),'-LauncherPid',str(os.getpid()),'-Staged',str(staged),'-Target',sys.executable],creationflags=0x08000000)

class App(tk.Tk):
    def __init__(self):
        super().__init__()
        HOME.mkdir(parents=True,exist_ok=True)
        self.title(f'Burning Ground 런처 v{VERSION}')
        self.geometry('680x470');self.resizable(False,False)
        self.configure(bg='#131a22')
        style=ttk.Style(self);style.theme_use('clam')
        style.configure('TFrame',background='#131a22')
        style.configure('TLabel',background='#131a22',foreground='#e6edf3',font=('맑은 고딕',10))
        style.configure('TButton',font=('맑은 고딕',11),padding=10)
        style.configure('TCheckbutton',background='#131a22',foreground='#e6edf3')
        cfg=load_config()
        self.path=tk.StringVar(value=cfg.get('game_dir',DEFAULT_GAME))
        self.mode=tk.StringVar(value=cfg.get('mode','1024x576'))
        self.full=tk.BooleanVar(value=cfg.get('fullscreen',True))
        self.status=tk.StringVar(value='게임 폴더를 확인하고 실행하세요.')
        frame=ttk.Frame(self,padding=26);frame.pack(fill='both',expand=True)
        ttk.Label(frame,text='BURNING GROUND',font=('맑은 고딕',24,'bold'),foreground='#ffab54').pack(anchor='w')
        ttk.Label(frame,text='Fall of the Gods  ·  BG v2.00  ·  StarCraft 1.16.1').pack(anchor='w',pady=(2,24))
        ttk.Label(frame,text='게임 설치 폴더').pack(anchor='w')
        row=ttk.Frame(frame);row.pack(fill='x',pady=(6,18))
        ttk.Entry(row,textvariable=self.path).pack(side='left',fill='x',expand=True)
        ttk.Button(row,text='폴더 선택',command=self.choose).pack(side='left',padx=(8,0))
        row=ttk.Frame(frame);row.pack(fill='x')
        ttk.Label(row,text='게임 내부 해상도').pack(side='left')
        ttk.Combobox(row,textvariable=self.mode,values=MODES,state='readonly',width=23).pack(side='left',padx=16)
        ttk.Checkbutton(row,text='전체 화면',variable=self.full).pack(side='left')
        ttk.Label(frame,text='16:9 화면 · 하단 HUD 중앙 배치 · HUD 주변 지형 표시\nW-MODE 플러그인 질문은 ‘아니요’를 선택하세요. 멀티플레이는 미검증입니다.',wraplength=620).pack(anchor='w',pady=(16,20))
        row=ttk.Frame(frame);row.pack(fill='x')
        self.start=ttk.Button(row,text='게임 실행',command=self.run);self.start.pack(side='left',fill='x',expand=True)
        self.update=ttk.Button(row,text='런처 업데이트',command=self.update_launcher);self.update.pack(side='left',padx=8)
        ttk.Button(row,text='로그 폴더',command=lambda:os.startfile(HOME)).pack(side='left')
        ttk.Label(frame,textvariable=self.status,wraplength=620,foreground='#9bb4ca').pack(anchor='w',pady=(22,0))
        self.busy=False
    def choose(self):
        folder=filedialog.askdirectory(initialdir=self.path.get())
        if folder:self.path.set(folder)
    def say(self,text):self.after(0,lambda:self.status.set(text))
    def save(self):
        (HOME/'config.json').write_text(json.dumps({'game_dir':self.path.get(),'mode':self.mode.get(),'fullscreen':self.full.get()},ensure_ascii=False,indent=2),encoding='utf-8')
    def work(self,fn):
        if self.busy:return
        self.busy=True;self.start.configure(state='disabled');self.update.configure(state='disabled')
        def task():
            try:fn()
            except Exception as exc:
                msg=str(exc);self.say(msg);self.after(0,lambda:messagebox.showerror('Burning Ground',msg))
            finally:self.after(0,self.done)
        threading.Thread(target=task,daemon=True).start()
    def done(self):
        self.busy=False;self.start.configure(state='normal');self.update.configure(state='normal')
    def run(self):
        self.save();source,mode,full=self.path.get(),self.mode.get(),self.full.get()
        self.work(lambda:launch_game(source,mode,full,self.say))
    def update_launcher(self):
        self.save()
        def update():
            self.say('GitHub 최신 버전 확인 중…')
            manifest=fetch_manifest()
            if version_key(manifest['version'])<=version_key(VERSION):
                self.say(f'최신 버전입니다 (v{VERSION}).');return
            target=download_update(manifest,self.say)
            apply_update(target)
            self.after(0,self.destroy)
        self.work(update)

if __name__=='__main__':
    if '--launch-test' in sys.argv:
        HOME.mkdir(parents=True,exist_ok=True)
        launch_game(DEFAULT_GAME,'1280x720',False,print)
    else:App().mainloop()
