import SCons
import SCons.Environment
from SCons.Script import DefaultEnvironment

import subprocess
from platformio import util

import os
import shutil
import re
env = DefaultEnvironment()

buildEnv : SCons.Environment.Base = env

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
    
PrintInfo()
PrintEnv()
EnsureSubmoduleCheckout()

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
