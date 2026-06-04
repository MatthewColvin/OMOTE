import SCons
import SCons.Environment
from SCons.Script import DefaultEnvironment

import subprocess
from platformio import util

import os
import shutil
import re
import tempfile
env = DefaultEnvironment()

buildEnv : SCons.Environment.Base = env

# Pin ESP-IDF Python deps. Managed components are vendored (see ensureManagedComponents).
if buildEnv["PIOENV"] in ("esp32_Rev1", "esp32_Rev5", "esp32_Rev5_3661", "esp32Debug"):
    buildEnv["ENV"]["PIP_CONSTRAINT"] = os.path.join(buildEnv["PROJECT_DIR"], "esp-idf-constraints.txt")
    buildEnv["ENV"]["IDF_COMPONENT_MANAGER"] = "0"

LINUX_APT_DEPENDENCES = {"libsdl2-dev","libcurl4-openssl-dev","libboost-all-dev"}

OMOTE_ISSUES = "https://github.com/OMOTE-Community/OMOTE-Firmware-object-oriented/issues"
MSYS2_INSTALL = "https://www.msys2.org/wiki/MSYS2-installation/"

# Define color codes
class Colors:
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    MAGENTA = '\033[95m'
    CYAN = '\033[96m'
    ENDC = '\033[0m' # Resets color and style

def PrintEnv():
    # Print all environment variables
    print("\n" + "="*60)
    print("ENVIRONMENT VARIABLES:")
    print("="*60)
    for key in sorted(env.Dictionary().keys()):
        try:
            value = env.subst(f"${key}")
            if value and value != key:  # Only print if substitution worked
                print(f"{key} = {value}")
        except:
            pass
    print("="*60 + "\n")

def runGitCommand(command):
    process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    stdout, stderr = process.communicate()

    if process.returncode != 0:
        print(f"Error executing command: {command}")
        print(stderr)
        return None
    return stdout

def EnsureSubmoduleCheckout():
  runGitCommand("git submodule update --init")

def isAptInstalled(apt):
    try:
        result = subprocess.run(['dpkg', '-s', apt], capture_output=True, text=True)
        return result.returncode == 0
    except:
        return False

def installApt(apt):
    try:
        subprocess.run(['sudo', 'apt-get', 'install', '-y', apt], check=True)
        print(f"Successfully installed {apt}")
    except subprocess.CalledProcessError as e:
        print(f"Failed to install {apt}: {e}")

def verifySimDependencies():
    # Check if apt-get exists on the system
    try:
        subprocess.run(['which', 'apt-get'], check=True, capture_output=True)
        verifyAptDependencies()
    except subprocess.CalledProcessError:
        print("apt-get not found - skipping apt dependencies check")

def verifyAptDependencies():   
    try:
        for dep in LINUX_APT_DEPENDENCES:
          if(not isAptInstalled(dep)):
            print(f"Detected Missing Dependency {dep} Installing...")
            installApt(dep)
    except: 
       print("Failed To Verify Dependencies are Installed!")

def verifyLinuxDependencies():   
    verifySimDependencies()

def remove_asio_sources_on_dep_build():
    """Remove ASIO src directory when building SimulatorHalImpl dependencies"""
    try:
        libdeps_dir = env.subst("$PROJECT_LIBDEPS_DIR")
        platform = env.subst("$PIOENV")
        asio_src = os.path.join(libdeps_dir, platform, "asio", "src")
        
        if os.path.exists(asio_src):
            print(f"[SimulatorHalImpl] Removing ASIO source directory: {asio_src}")
            shutil.rmtree(asio_src)
            # Create a marker file to avoid repeated deletions
            marker = os.path.join(libdeps_dir, platform, "asio", ".header_only_mode")
            open(marker, 'a').close()
    except Exception as e:
        print(f"[SimulatorHalImpl] Warning: Could not remove ASIO sources: {e}")

def install_msys2_packages(msys2_install_path, packages : list[str]):
    msys2_shell_path = os.path.join(msys2_install_path, "msys2_shell.cmd")
    
    # Command to execute within the MSYS2 shell
    package_list = " ".join(packages) 
    command = [
        msys2_shell_path,
        "-defterm",
        "-no-start",
        "-mingw64", 
        "-c",
        f"pacman -S --noconfirm {package_list}"
    ]

    try:
        userinput = input(f"Would you like to install dependencies with command? (y/n)\n {command}:\n")
        if(userinput.lower() != "y"):
            print("Not Installing!")
            return
        
        print(f"Attempting to install MSYS2 package(s): {package_list}...")
        result = subprocess.run(command, check=True, capture_output=True, text=True)
        print(f"Successfully installed {package_list}.")
        print("STDOUT:\n", result.stdout)
        if result.stderr:
            print("STDERR:\n", result.stderr)
    except subprocess.CalledProcessError as e:
        print(f"Error installing {package_list}: {e}")
        print("STDOUT:\n", e.stdout)
        print("STDERR:\n", e.stderr)
    except FileNotFoundError:
        print(f"Error: msys2_shell.cmd not found at {msys2_shell_path}.")
        print("Please ensure MSYS2 is installed and the path is correct.")

def msys2InstallDependencies(msys2_install_path):
    deps = ["mingw-w64-x86_64-gcc", "mingw-w64-x86_64-SDL2",
            "mingw-w64-x86_64-python", "mingw-w64-x86_64-curl",
            "python3-pip"]
    install_msys2_packages(msys2_install_path,deps)

def getMsys64InstallPath() -> str:
    drivesToCheck = ['C', 'D', 'E', 'F']
    for drive in drivesToCheck:
        possibleMsys2InstallPath = f"{drive}:/msys64"
        if os.path.exists(possibleMsys2InstallPath):
            return possibleMsys2InstallPath
    return None

def isOnPath(aDir) -> bool:
    if aDir is None:
        return False
    path = os.environ.get("PATH")
    return re.search(f'{aDir}', path) != None

def verifyWindowsDependencies():
    depsAreSatified = True
    msys2InstallDir = getMsys64InstallPath()
    msys2BinDir = f"{msys2InstallDir}/mingw64/bin"
    
    if (msys2InstallDir is None):
        print(f"{Colors.RED} Need to install install msys2 {Colors.ENDC}")
        print(f"{MSYS2_INSTALL} \n")
        depsAreSatified = False
    elif(not isOnPath(msys2BinDir)):
        print(f"{Colors.RED}msys2 must be on path!{Colors.ENDC}")
        print("Use Admin Powershell to add msys2 to path then restart enviroment(close vscode & reopen):")
        print(f"{Colors.BLUE}[Environment]::SetEnvironmentVariable(\"Path\", $env:Path + \";{msys2BinDir}\", \"Machine\") {Colors.ENDC} \n")
        depsAreSatified = False
    else:
        #TODO: need to conditionally install deps if they are missing
        msys2InstallDependencies(msys2InstallDir)

    if (not depsAreSatified):
        print(f"{Colors.RED}If you think there is a mistake please file an issue or message in discord{Colors.ENDC}")
        print(f"{OMOTE_ISSUES} \n")

def PrintInfo():
    print('')
    print("PIO Build env:", buildEnv["PIOENV"])
    print("Detected Platform:", buildEnv["PLATFORM"])
    print('')

def removeLittleFSArduinoLib():
    """
    (Deprecated) Leaving this around as a reference on how to modify build files.
    WARNING: to the future devs: This can be kinda hacky and cause weirdness in the build system
    Remove the Littlefs arduino lib out of the framework so it properly builds
    """
    applicableBuildEnvs = ["esp32_Rev1", "esp32_Rev5", "esp32Debug"]
    # No need to remove littlefs if the build environment does not require it
    if (buildEnv["PIOENV"] not in applicableBuildEnvs):
        return

    platform = buildEnv.PioPlatform()
    framework_dir = platform.get_package_dir("framework-arduinoespressif32")

    littleFsArduinoLibDir = os.path.join(framework_dir,"libraries","LittleFS")
    print("Removing Arduino littleFS From Framework To Avoid Conflict...")
    if(os.path.isdir(littleFsArduinoLibDir)):
        shutil.rmtree(littleFsArduinoLibDir)
        print("Removed", littleFsArduinoLibDir)
    else:
        print(littleFsArduinoLibDir,"Already Removed")

MANAGED_COMPONENTS = {
    "joltwallet__littlefs": {
        "url": "https://github.com/joltwallet/esp_littlefs.git",
        "tag": "v1.19.1",
        "subdir": None,
    },
    "espressif__esp_websocket_client": {
        "url": "https://github.com/espressif/esp-protocols.git",
        "tag": "websocket-v1.4.0",
        "subdir": "components/esp_websocket_client",
    },
    "espressif__mdns": {
        "url": "https://github.com/espressif/esp-protocols.git",
        "tag": "mdns-v1.8.2",
        "subdir": "components/mdns",
    },
}

def ensureManagedComponents():
    """Vendor IDF managed components (avoids component-manager git errors on Windows)."""
    applicableBuildEnvs = ["esp32_Rev1", "esp32_Rev5", "esp32_Rev5_3661", "esp32Debug"]
    if buildEnv["PIOENV"] not in applicableBuildEnvs:
        return

    # ESP-IDF auto-scans project components/ (PlatformIO overrides EXTRA_COMPONENT_DIRS).
    components_dir = os.path.join(buildEnv["PROJECT_DIR"], "components")
    os.makedirs(components_dir, exist_ok=True)
    for stale in os.listdir(components_dir):
        if stale.startswith("clone_"):
            stale_path = os.path.join(components_dir, stale)
            if os.name == "nt":
                subprocess.run(["cmd", "/c", "rmdir", "/s", "/q", stale_path], capture_output=True)
            else:
                shutil.rmtree(stale_path, ignore_errors=True)

    legacy_managed_dir = os.path.join(buildEnv["PROJECT_DIR"], "managed_components")

    for dest_name, spec in MANAGED_COMPONENTS.items():
        dest_path = os.path.join(components_dir, dest_name)
        ready_marker = os.path.join(dest_path, "CMakeLists.txt")
        if dest_name == "joltwallet__littlefs":
            ready_marker = os.path.join(dest_path, "src", "littlefs", "lfs.h")
        if os.path.isfile(ready_marker):
            continue

        legacy_path = os.path.join(legacy_managed_dir, dest_name)
        if os.path.isfile(os.path.join(legacy_path, "CMakeLists.txt")):
            shutil.copytree(legacy_path, dest_path)
            print(f"Vendored {dest_name} from managed_components cache")
            continue

        clone_dir = tempfile.mkdtemp(prefix=f"omote_{dest_name}_")

        print(f"Vendoring managed component {dest_name} ({spec['tag']})...")
        clone_args = ["git", "clone", "--branch", spec["tag"]]
        if dest_name == "joltwallet__littlefs":
            clone_args.append("--recurse-submodules")
        else:
            clone_args.extend(["--depth", "1"])
        clone_args.extend([spec["url"], clone_dir])
        result = subprocess.run(clone_args, capture_output=True, text=True)
        if result.returncode != 0:
            print(result.stderr)
            raise RuntimeError(f"Failed to clone {spec['url']} @ {spec['tag']}")

        src_path = (
            os.path.join(clone_dir, spec["subdir"]) if spec["subdir"] else clone_dir
        )
        if os.path.isdir(dest_path):
            if os.name == "nt":
                subprocess.run(
                    ["cmd", "/c", "rmdir", "/s", "/q", dest_path],
                    capture_output=True,
                )
            else:
                shutil.rmtree(dest_path, ignore_errors=True)
        shutil.copytree(src_path, dest_path)
        shutil.rmtree(clone_dir, ignore_errors=True)
        print(f"Vendored {dest_name} -> {dest_path}")

def configureExtraComponentDirs():
    """PlatformIO only adds src/ + Arduino to EXTRA_COMPONENT_DIRS; include components/."""
    applicableBuildEnvs = ["esp32_Rev1", "esp32_Rev5", "esp32_Rev5_3661", "esp32Debug"]
    if buildEnv["PIOENV"] not in applicableBuildEnvs:
        return

    platform = buildEnv.PioPlatform()
    framework_dir = platform.get_package_dir("framework-arduinoespressif32")
    if "@" in os.path.basename(framework_dir):
        renamed = os.path.join(
            os.path.dirname(framework_dir),
            os.path.basename(framework_dir).replace("@", "-"),
        )
        if os.path.isdir(renamed):
            framework_dir = renamed

    # components/ is auto-scanned by ESP-IDF; only extend PIO defaults (src + Arduino).
    project_dir = buildEnv["PROJECT_DIR"]
    extra = ";".join([os.path.join(project_dir, "src"), framework_dir])
    buildEnv.Append(BOARDcmake_extra_args=f'-DEXTRA_COMPONENT_DIRS:PATH={extra}')

def patchLvglLittleFsDriver():
    """Patch LVGL Arduino LittleFS driver for Arduino core 3.x API."""
    applicableBuildEnvs = ["esp32_Rev1", "esp32_Rev5", "esp32_Rev5_3661", "esp32Debug"]
    if buildEnv["PIOENV"] not in applicableBuildEnvs:
        return

    libdeps_dir = buildEnv.subst("$PROJECT_LIBDEPS_DIR")
    platform = buildEnv.subst("$PIOENV")
    lv_fs_path = os.path.join(
        libdeps_dir, platform, "lvgl", "src", "libs", "fsdrv", "lv_fs_arduino_esp_littlefs.cpp"
    )

    if not os.path.isfile(lv_fs_path):
        return

    with open(lv_fs_path, "r", encoding="utf-8") as f:
        content = f.read()

    original = content

    if '#include "FS.h"' not in content:
        content = content.replace(
            '#include "LittleFS.h"',
            '#include "FS.h"\n#ifndef CONFIG_LITTLEFS_PAGE_SIZE\n#define CONFIG_LITTLEFS_PAGE_SIZE 256\n#endif\n#include "LittleFS.h"',
        )

    content = content.replace(
        "    int rc = lf->file.seek(pos, mode);\n\n    return rc < 0 ? LV_FS_RES_UNKNOWN : LV_FS_RES_OK;",
        "    if(!lf->file.seek(pos, mode)) {\n        return LV_FS_RES_UNKNOWN;\n    }\n\n    return LV_FS_RES_OK;",
    )

    if content != original:
        with open(lv_fs_path, "w", encoding="utf-8") as f:
            f.write(content)
        print(f"Patched LVGL LittleFS driver: {lv_fs_path}")

PrintInfo()
# PrintEnv()
EnsureSubmoduleCheckout()
ensureManagedComponents()
configureExtraComponentDirs()
patchLvglLittleFsDriver()
buildEnv.AddPreAction("buildprog", lambda source, target, env: patchLvglLittleFsDriver())

# Remove the ASIO src folder when building SimulatorHalImpl to avoide trying to build 
# the asio source files. 
if("sim" in buildEnv["PIOENV"]):
    remove_asio_sources_on_dep_build()

if(buildEnv["PLATFORM"] != "win32"):
    verifyLinuxDependencies()
else:
    # TODO: Add back when we can check for dependencies to ensure script does not hang
    # verifyWindowsDependencies()
    pass
