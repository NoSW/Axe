import os
import sys
import argparse
import subprocess
import shutil

parser = argparse.ArgumentParser(description='Generate project using CMake')
parser.add_argument("-c", "--clang", action="store_true", help="run CMake for the first time and run build with clang")
parser.add_argument("-g", "--gcc", action="store_true", help="run CMake for the first time and run build with gcc")
parser.add_argument("--cmake_only", action="store_true", help="run CMake with specified compiler")
parser.add_argument("-m", "--msvc", "--msvc_cmake_only", action="store_true", help="run CMake with msvc")

def install_package(package):
    result = subprocess.check_call([sys.executable, "-m", "pip", "install", package])

def check_exec(name, link):
    found = shutil.which(name)
    if found:
        print(f"Found {found}")
        return True
    else:
        print(f"Failed to found {name} that's need to be added into PATH, download link {link}")
        return False

PJ_ROOT = os.path.split(os.path.realpath(__file__))[0]

def print_then_run(cmd):
    print(cmd)
    return os.system(cmd) == 0

def git_local_config():
    print_then_run("git config core.ignorecase false")

def run_CMake(compiler):
    print(f"Selected compiler: {compiler}")

    build_flags = ""
    if compiler in ["clang", "gcc"] and sys.platform in ["win32", "linux"]:
        cxx_compiler = "g++" if compiler == "gcc" else "clang++"
        build_flags += f" -G \"Ninja Multi-Config\" -DCMAKE_C_COMPILER={compiler} -DCMAKE_CXX_COMPILER={cxx_compiler} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON "
    elif compiler == "msvc" and sys.platform == "win32":
        build_flags += f" -G \"Visual Studio 17 2022\" "
    else:
        print(f"Unsupported compiler {compiler} on {sys.platform}")
        exit(1)

    cmd = f"cmake -S {PJ_ROOT}  -B Build_{compiler} {build_flags}"
    return print_then_run(cmd)

def run_Build(compiler):
    build_folder = os.path.join(PJ_ROOT, f"Build_{compiler}")
    fromScratch = input("\nBuild from scratch? [y/n]").lower() == 'y'
    if compiler != "msvc" and ((not fromScratch) or run_CMake(compiler)):
        mode = "Release" if input("\nStart build in Debug(default) or Release mode? [d/r]").lower() == 'r' else "Debug"
        print_then_run(f"cmake --build {build_folder} --config {mode} -- -j 32")

if __name__ == "__main__":
    args = parser.parse_args()

    if not (check_exec("git", "https://git-scm.com/downloads") and 
            check_exec("cmake", "https://cmake.org/download/") and 
            check_exec("ninja", " https://github.com/ninja-build/ninja/releases")):
        exit(1)

    git_local_config()

    if sys.platform == "darwin":
        print(f"MacOS is NOT added yet")
        exit(1)

    if args.clang:
        run_CMake("clang") if args.cmake_only else run_Build("clang")
    elif args.gcc:
        run_CMake("gcc") if args.cmake_only else run_Build("gcc")
    elif args.msvc or len(sys.argv) == 1:
        print("\nPlease open Build_msvc/Axe.sln and build the project") if run_CMake("msvc") else print("CMake failed")
