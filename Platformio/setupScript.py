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
    applicableBuildEnvs = ["esp32_Rev1", "esp32Debug"]
    # No need to replace mbedTLS libs if the build environment does not require it
    if (buildEnv["PIOENV"] not in applicableBuildEnvs):
        return

    platform = buildEnv.PioPlatform()
    framework_dir = platform.get_package_dir("framework-arduinoespressif32")
    framework_idf_arduino_libs_dir = framework_dir + "/tools/esp32-arduino-libs"
    
    targetFolders = ["esp32"]

    project_dir = buildEnv.get("PROJECT_DIR", "")
    project_mbed_tls_dir = project_dir + "/mbedTlsLibs"

    print("Transferring libs from", project_mbed_tls_dir , "to", framework_idf_arduino_libs_dir)
    
    # For both esp32 and esp32s3 move the working libs into the framework for linking
    for targetFolder in targetFolders:
        project_target_mbed_tls_dir = os.path.join(project_mbed_tls_dir, targetFolder)
        framework_idf_arduino_libs_target_dir = os.path.join(framework_idf_arduino_libs_dir, targetFolder, "lib")
        # Check if source directory exists
        if not os.path.exists(project_target_mbed_tls_dir):
            print(f"Source directory {project_target_mbed_tls_dir} does not exist please report issue to github")
            return      
        # Check if destination directory exists
        if not os.path.exists(framework_idf_arduino_libs_target_dir):
            print(f"Framework directory {framework_idf_arduino_libs_target_dir} does not exist please report issue to github")
            return    
        # Copy all .a files
        for file in os.listdir(project_target_mbed_tls_dir):
            if file.endswith('.a'):
                src_file = os.path.join(project_target_mbed_tls_dir, file)
                dst_file = os.path.join(framework_idf_arduino_libs_target_dir, file)
                shutil.copy2(src_file, dst_file)

def removeLittleFSArduinoLib():
    """
    Remove the Littlefs arduino lib out of the framework so it properly builds
    """
    applicableBuildEnvs = ["esp32_Rev1", "esp32_Rev5", "esp32Debug"]
    # No need to remove littlefs if the build environment does not require it
    if (buildEnv["PIOENV"] not in applicableBuildEnvs):
        return

    platform = buildEnv.PioPlatform()
    framework_dir = platform.get_package_dir("framework-arduinoespressif32")

    littleFsArduinoLibDir = os.path.join(framework_dir,"libraries","LittleFS")
    print("Removing Arduino littleFS From Framework To Avoid Conflict")
    if(os.path.isdir(littleFsArduinoLibDir)):
        shutil.rmtree(littleFsArduinoLibDir)
    

PrintInfo()
EnsureSubmoduleCheckout()
verifyDependencies()
remove_espLittleFsLib()
removeLittleFSArduinoLib()
#replace_mbedTlsLibs()