import os
import sys
import argparse
import subprocess
import shutil

parser = argparse.ArgumentParser(description='Generate project using CMake')
parser.add_argument("-c", "--clang", action="store_true", help="run CMake and run build with clang")
parser.add_argument("-g", "--gcc", action="store_true", help="run CMake and run build with gcc")
parser.add_argument("-m", "--msvc", action="store_true", help="run CMake with VS2022 (default)")

def install_package(package):
    result = subprocess.check_call([sys.executable, "-m", "pip", "install", package])

PJ_ROOT = os.path.split(os.path.realpath(__file__))[0]

def print_then_run(cmd):
    print(cmd)
    return os.system(cmd) == 0

def git_local_config():
    if shutil.which("git"):
        print_then_run("git config core.ignorecase false")

BUILD_DIR_PREFIX = f"{PJ_ROOT}/Build_{sys.platform}"

def clear_cmake_cache_files(build_dir):
    cache_file = f"{build_dir}/CMakeCache.txt"
    if input(f"Do you want to delete {cache_file} first? If you change the value of the variable in CMakeLists.txt, recommend delete it?[y/n]").lower() == 'y' and os.path.exists(cache_file):
        os.remove(cache_file)

def run_with_clang_or_gcc(compiler):
    clear_cmake_cache_files(f"{BUILD_DIR_PREFIX}_{compiler}")
    cxx_compiler = "g++" if compiler == "gcc" else "clang++"
    generator = " -G \"Ninja Multi-Config\" " if shutil.which("ninja") else " -G \"  Unix Makefiles\" "
    if print_then_run(f"cmake -S {PJ_ROOT} -B {BUILD_DIR_PREFIX}_{compiler} {generator} -DCMAKE_C_COMPILER={compiler} -DCMAKE_CXX_COMPILER={cxx_compiler} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON "):
        mode = "Release" if input("\nStart build in Debug(default) or Release mode? [d/r]").lower() == 'r' else "Debug"
        print_then_run(f"cmake --build {BUILD_DIR_PREFIX}_{compiler} --config {mode} -- -j {os.cpu_count() if os.cpu_count() else 1}")

def run_with_msvc():
    clear_cmake_cache_files(f"{BUILD_DIR_PREFIX}_msvc")
    print_then_run(f"cmake -S {PJ_ROOT}  -B {BUILD_DIR_PREFIX}_msvc  -G \"Visual Studio 17 2022\" ")
    print(f"\nPlease open generated *.sln in above folder and build it with VS2022")

def run_default():
    clear_cmake_cache_files(f"{BUILD_DIR_PREFIX}")
    if print_then_run(f"cmake -S {PJ_ROOT}  -B {BUILD_DIR_PREFIX}"):
        print_then_run(f"cmake --build {BUILD_DIR_PREFIX}")

if __name__ == "__main__":
    args = parser.parse_args()

    if not shutil.which("cmake"):
        print("Failed to found cmake, please install cmake first")
        exit(1)

    git_local_config()

    if args.clang:
        run_with_clang_or_gcc("clang")
    elif args.gcc:
        run_with_clang_or_gcc("gcc")
    elif sys.platform == "win32":
        run_with_msvc()
    else:
        run_default()
