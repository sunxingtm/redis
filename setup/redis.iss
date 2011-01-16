; TODO modify the installed redis.conf file to log to a file (instead of stdout).
;      NB we have to use absolute paths in the log file (redis does not play
;         well when we mix the "dir" directive and a relative log file path; it
;         will write the log file under current directory, but after its running,
;         it will write it inside the "dir" setted subdirectory).
; TODO modify the installed redis.conf file to save the database and logs under the "data" subdirectory.
; TODO get AppVersion from the version info resource embedded in the redis-service.exe file.
;      see the ISPPExample1.iss file bundled with Inno Setup.
; TODO do not override destiny redis.conf file. instead create a redis.conf.dist
;      with our version, and only override the original if it wasn't modified.
; TODO show a blurb after the install to alert the user to create a dedicated
;      windows account to run redis, change the "data" directory permissions
;      and modify the service startup type to automatic (maybe this should be
;      done automatically?).
; TODO display a redis logo on the left of the setup dialog boxes.
; TODO strip the binaries?

[Setup]
AppId={{B882ADC5-9DA9-4729-899A-F6728C146D40}
AppName=Redis
AppVersion=2.1.8
;AppVerName=Redis 2.1.8
AppPublisher=rgl
AppPublisherURL=https://github.com/rgl/redis
AppSupportURL=https://github.com/rgl/redis
AppUpdatesURL=https://github.com/rgl/redis
DefaultDirName={pf}\Redis
DefaultGroupName=Redis
LicenseFile=..\COPYING
InfoBeforeFile=..\README
OutputDir=.
OutputBaseFilename=redis-setup
SetupIconFile=redis.ico
Compression=lzma
SolidCompression=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "..\src\redis-benchmark.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\src\redis-check-aof.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\src\redis-check-dump.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\src\redis-cli.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\src\redis-server.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\src\redis-service.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\redis.conf"; DestDir: "{app}"
Source: "..\README"; DestDir: "{app}"; DestName: "README.txt"
Source: "..\COPYING"; DestDir: "{app}"; DestName: "COPYING.txt"

[Code]
#include "service.pas"

const
  SERVICE_NAME = 'redis';
  SERVICE_DISPLAY_NAME = 'Redis Server';
  SERVICE_DESCRIPTION = 'Persistent key-value database';

function InitializeSetup(): boolean;
begin
  if IsServiceRunning(SERVICE_NAME) then
  begin
    MsgBox('Please stop the ' + SERVICE_NAME + ' service before running this install', mbError, MB_OK);
    Result := false;
  end
  else
    Result := true
end;

function InitializeUninstall(): boolean;
begin
  if IsServiceRunning(SERVICE_NAME) then
  begin
    MsgBox('Please stop the ' + SERVICE_NAME + ' service before running this uninstall', mbError, MB_OK);
    Result := false;
  end
  else
    Result := true
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  ServicePath: string;
begin
  case CurStep of
    ssPostInstall:
      begin
        if IsServiceInstalled(SERVICE_NAME) then
          Exit;

        ServicePath := ExpandConstant('{app}\redis-service.exe');

        if not InstallService(ServicePath, SERVICE_NAME, SERVICE_DISPLAY_NAME, SERVICE_DESCRIPTION, SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START) then
        begin
          MsgBox('Failed to install the ' + SERVICE_NAME + ' service.' #13#13 'You need to install it manually.', mbError, MB_OK)
        end
      end
  end
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  case CurUninstallStep of
    usPostUninstall:
      begin
        if not RemoveService(SERVICE_NAME) then
        begin
          MsgBox('Failed to uninstall the ' + SERVICE_NAME + ' service.' #13#13 'You need to uninstall it manually.', mbError, MB_OK);
        end
      end
  end
end;
