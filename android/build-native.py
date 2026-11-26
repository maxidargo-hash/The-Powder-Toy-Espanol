#!/usr/bin/env python3
"""Cross-compile the touch edition against the libraries of the Android 1.24 APK."""
import argparse
from pathlib import Path
import shlex
import subprocess
from zipfile import ZipFile

ABIS = {
    "arm64-v8a": ("aarch64-linux-android", True),
    "armeabi-v7a": ("armv7a-linux-androideabi", False),
    "x86": ("i686-linux-android", False),
    "x86_64": ("x86_64-linux-android", True),
}
LIBRARIES = ("sdl-1.2", "luajit", "bzip2", "nghttp2", "curl", "fftw3f")
ROOT = Path(__file__).resolve().parent.parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("original_apk", type=Path)
    parser.add_argument("--ndk", type=Path, required=True, help="Android NDK r28 or later (Linux)")
    parser.add_argument("--sdl-port", type=Path, required=True, help="Checkout of jacob1/android-sdl")
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--abi", action="append", choices=ABIS, help="Select an ABI (default: all four)")
    parser.add_argument("--jobs", type=int, default=3)
    args = parser.parse_args()
    toolchain = args.ndk.resolve() / "toolchains/llvm/prebuilt/linux-x86_64/bin"
    work = args.build_dir.resolve()
    work.mkdir(parents=True, exist_ok=True)
    port = args.sdl_port.resolve() / "project"
    headers = work / "headers"
    headers.mkdir(exist_ok=True)
    sdl_headers = headers / "SDL"
    if not sdl_headers.is_symlink():
        sdl_headers.symlink_to(port / "jni/sdl-1.2/include", target_is_directory=True)
    includes = [ROOT / "includes", ROOT / "src", headers, port / "jni/bzip2/include"]
    common_flags = (
        "-O2 -DNDEBUG -DANDROID -DLIN -DTOUCHUI -DLUACONSOLE -DGRAVFFT -DNOMOD "
        "-DLUAJIT -DSDL1_2 -fPIC -ffunction-sections -fdata-sections"
    )
    rules = [
        "rule cxx",
        "  description = CXX $out",
        "  command = $cxx $flags -std=c++17 -MMD -MF $out.d -c $in -o $out",
        "  depfile = $out.d",
        "  deps = gcc",
        "rule cc",
        "  description = CC $out",
        "  command = $cc $flags -MMD -MF $out.d -c $in -o $out",
        "  depfile = $out.d",
        "  deps = gcc",
        "rule link",
        "  description = LINK $out",
        '  command = $cxx -shared -static-libstdc++ -Wl,--gc-sections,--no-undefined,-soname,libapplication.so -Wl,-z,max-page-size=16384 $in $libs -lGLESv1_CM -ldl -llog -lz -lm -o $out && $strip --strip-unneeded $out',
    ]
    targets = []
    sources = sorted(path for path in (ROOT / "src").rglob("*") if path.suffix in (".cpp", ".c"))
    with ZipFile(args.original_apk) as base:
        for abi in args.abi or ABIS:
            triple, wide = ABIS[abi]
            cxx = shlex.quote(str(toolchain / (triple + "21-clang++")))
            cc = shlex.quote(str(toolchain / (triple + "21-clang")))
            abi_includes = includes + [port / "ndk-cross-libs" / abi / "include"]
            quoted_includes = " ".join("-I" + shlex.quote(str(path)) for path in abi_includes)
            flags = f"{common_flags} {quoted_includes}" + (" -D_64BIT" if wide else "")
            strip = shlex.quote(str(toolchain / "llvm-strip"))
            obj_dir = work / abi
            obj_dir.mkdir(exist_ok=True)
            libraries = []
            for name in LIBRARIES:
                library = f"lib{name}.so"
                destination = obj_dir / library
                data = base.read(f"lib/{abi}/{library}")
                if not destination.exists() or destination.read_bytes() != data:
                    destination.write_bytes(data)
                libraries.append(f"{abi}/{library}")
            objects = []
            for path in sources:
                obj = f"{abi}/" + path.relative_to(ROOT / "src").as_posix().replace("/", "_") + ".o"
                source = str(path).replace("$", "$$").replace(" ", "$ ")
                rule = "cc" if path.suffix == ".c" else "cxx"
                rules += [f"build {obj}: {rule} {source}", f"  cxx = {cxx}", f"  cc = {cc}", f"  flags = {flags}"]
                objects.append(obj)
            target = f"{abi}/libapplication.so"
            rules += [
                f"build {target}: link {' '.join(objects)} | {' '.join(libraries)}",
                f"  cxx = {cxx}",
                f"  strip = {strip}",
                f"  libs = {' '.join(libraries)}",
            ]
            targets.append(target)
    rules.append(f"default {' '.join(targets)}")
    (work / "build.ninja").write_text("\n".join(rules) + "\n")
    subprocess.run(["ninja", "-C", str(work), "-j", str(args.jobs)], check=True)


if __name__ == "__main__":
    main()
