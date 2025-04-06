import SCons
import SCons.Environment
from SCons.Script import DefaultEnvironment

import subprocess
from platformio import util

import os
import shutil
env = DefaultEnvironment()

buildEnv : SCons.Environment.Base = env

LINUX_APT_DEPENDENCES = {"libsdl2-dev","libcurl4-openssl-dev","libboost-all-dev"}

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

def verifyDependencies():   
  verifySimDependencies()

def PrintInfo():
    print('')
    print("PIO Build env:", buildEnv["PIOENV"])
    print("Detected Platform:", buildEnv["PLATFORM"])
    print('')


def remove_espLittleFsLib():
    """
    Remove espLittleFsLib from the build environment if it exists.
    This is a workaround to avoid conflicts with the LittleFS library.
    """
    lib = "-lesp_littlefs"
    if lib in buildEnv.get('LIBS', []):
        buildEnv['LIBS'].remove(lib)
        print(f"Removed {lib} from LIBS to avoid conflicts.")
        print('')


# This should be removed with #41 
def replace_mbedTlsLibs():
    """
    Replace mbedTLS libraries with the correct ones for the build environment.
    This is a workaround to help get definition for mbedTLS that are necessary for idf websockets.
    """
    platform = buildEnv.PioPlatform()
    framework_dir = platform.get_package_dir("framework-arduinoespressif32")
    framework_idf_arduino_libs_dir = framework_dir + "/tools/esp32-arduino-libs/esp32/lib/"
    
    project_dir = buildEnv.get("PROJECT_DIR", "")
    project_mbed_tls_dir = project_dir + "/mbedTlsLibs"

    print("Transferring libs from", project_mbed_tls_dir , "to", framework_idf_arduino_libs_dir)
    # Check if source directory exists
    if not os.path.exists(project_mbed_tls_dir):
        print(f"Source directory {project_mbed_tls_dir} does not exist please report issue to github")
        return      
    # Check if destination directory exists
    if not os.path.exists(framework_idf_arduino_libs_dir):
        print(f"Framework directory {framework_idf_arduino_libs_dir} does not exist please report issue to github")
        return    
    # Copy all .a files
    for file in os.listdir(project_mbed_tls_dir):
        if file.endswith('.a'):
            src_file = os.path.join(project_mbed_tls_dir, file)
            dst_file = os.path.join(framework_idf_arduino_libs_dir, file)
            shutil.copy2(src_file, dst_file)

PrintInfo()
EnsureSubmoduleCheckout()
verifyDependencies()
remove_espLittleFsLib()
replace_mbedTlsLibs()