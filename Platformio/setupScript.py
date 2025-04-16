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
    print("Removing Arduino littleFS From Framework To Avoid Conflict...")
    if(os.path.isdir(littleFsArduinoLibDir)):
        shutil.rmtree(littleFsArduinoLibDir)
        print("Removed", littleFsArduinoLibDir)
    else:
        print(littleFsArduinoLibDir,"Already Removed")
    

PrintInfo()
EnsureSubmoduleCheckout()
verifyDependencies()

removeLittleFSArduinoLib()