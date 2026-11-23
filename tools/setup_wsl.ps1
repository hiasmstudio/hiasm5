[CmdletBinding()]
param([string]$Distribution = 'Debian', [switch]$InstallDependencies, [switch]$Gui, [switch]$VerifyGui)
$ErrorActionPreference = 'Stop'
if($VerifyGui) { $Gui = $true }
if(-not (Get-Command wsl.exe -ErrorAction SilentlyContinue)) {
    throw 'Install WSL and a Debian/Ubuntu distribution, then rerun this script.'
}
if($InstallDependencies) {
    $packages = @('g++','pkg-config','util-linux','procps')
    if($Gui) { $packages += 'libgtk-4-dev' }
    & wsl.exe --distribution $Distribution --user root --exec apt-get update
    if($LASTEXITCODE) { throw 'WSL apt index update failed' }
    & wsl.exe --distribution $Distribution --user root --exec apt-get install -y --no-install-recommends @packages
    if($LASTEXITCODE) { throw 'WSL development dependency installation failed' }
}
& wsl.exe --distribution $Distribution --exec g++ --version
if($LASTEXITCODE) { throw 'The distribution needs g++; use -InstallDependencies on Debian/Ubuntu' }
& wsl.exe --distribution $Distribution --exec setsid --version
if($LASTEXITCODE) { throw 'setsid is required for managed process lifecycle' }
& wsl.exe --distribution $Distribution --exec /bin/kill --version
if($LASTEXITCODE) { throw '/bin/kill is required for managed stop' }
if($Gui) {
    & wsl.exe --distribution $Distribution --exec pkg-config --modversion gtk4
    if($LASTEXITCODE) { throw 'GTK4 development files are missing; use Debian 12+/Ubuntu 22.04+' }
}
if($VerifyGui) {
    # Arguments and paths stay argv entries, never interpolated shell source.
    $probe = Join-Path ([IO.Path]::GetTempPath()) ('hiasm-wsl-gui-' + [guid]::NewGuid().ToString('N'))
    $null = New-Item -ItemType Directory -Path $probe
    try {
        $source = @'
#include <gtk/gtk.h>
#include <iostream>
int main() {
  gtk_init();
  auto window=gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(window), "HiAsm WSL GTK4 probe");
  gtk_window_present(GTK_WINDOW(window));
  const auto deadline=g_get_monotonic_time()+500000;
  while(g_get_monotonic_time()<deadline) {
    while(g_main_context_iteration(nullptr,false)) {}
    g_usleep(1000);
  }
  gtk_window_destroy(GTK_WINDOW(window));
  std::cout << "OK WSL GTK4 window lifecycle" << std::endl;
}
'@
        [IO.File]::WriteAllText((Join-Path $probe 'probe.cpp'), $source, (New-Object Text.UTF8Encoding($false)))
        $flags = & wsl.exe --distribution $Distribution --exec pkg-config --cflags --libs gtk4
        if($LASTEXITCODE) { throw 'GTK4 pkg-config failed' }
        [IO.File]::WriteAllText((Join-Path $probe 'gtk.flags'), ($flags -join [Environment]::NewLine), (New-Object Text.UTF8Encoding($false)))
        & wsl.exe --distribution $Distribution --cd $probe --exec g++ -std=c++17 probe.cpp -o probe '@gtk.flags'
        if($LASTEXITCODE) { throw 'WSL GTK4 probe compilation failed' }
        & wsl.exe --distribution $Distribution --cd $probe --exec timeout 20 ./probe
        if($LASTEXITCODE) { throw 'WSL GTK4 window failed; check DISPLAY/Wayland/WSLg and target runtime' }
    } finally {
        foreach($name in @('probe.cpp','gtk.flags','probe')) {
            $path = Join-Path $probe $name
            if(Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path -Force }
        }
        Remove-Item -LiteralPath $probe -Force
    }
}
Write-Output "WSL toolchain ready: $Distribution"
Write-Output "Set HIASM_WSL_DISTRO=$Distribution before launching HiAsm; select TargetPlatform=Linux."
Write-Output 'GTK4 windows additionally require a working display, for example WSLg.'
