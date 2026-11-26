#!/usr/bin/env python3
"""Package the translated native engine in the original mobile Android shell."""
import argparse
import hashlib
import os
from pathlib import Path
import re
import subprocess
import tempfile
import xml.etree.ElementTree as ET
from zipfile import ZipFile

APP_ID = "io.github.maxidargo_hash.powdertoy.es"
ORIGINAL_ID = "uk.co.powdertoy.tpt"
ANDROID = "{http://schemas.android.com/apk/res/android}"
ORIGINAL_SHA256 = "431789879324569dab4d2480c458d9af4bcdfb13c3846e8d526b1e1739e2ec47"
ABIS = ("arm64-v8a", "armeabi-v7a", "x86", "x86_64")
LABEL = "The Powder Toy Español"


def run(*command):
    subprocess.run(command, check=True)


def localize_android_resources(directory):
    default = directory / "values/strings.xml"
    strings = ET.parse(default)
    upstream_es = ET.parse(directory / "values-es/strings.xml")
    catalog = {entry.get("name"): entry.text for entry in upstream_es.iterfind("string")}
    catalog.update({
        "app_name": LABEL,
        "display_size_desktop": "Escritorio, sin emulación",
        "notification_app_is_running": "%s está en ejecución",
        "notification_stop": "Detener",
        "restarting_please_wait": "Reiniciando…",
        "storage_access": "Permiso para escribir en la tarjeta SD",
        "tv_borders": "Bordes de TV",
        "video_orientation_autodetect": "Detectar automáticamente la orientación de la pantalla",
    })
    for entry in strings.iterfind("string"):
        if entry.get("name") in catalog:
            entry.text = catalog[entry.get("name")]
    strings.write(default, encoding="utf-8", xml_declaration=True)
    for translation in directory.glob("values-*/strings.xml"):
        tree = ET.parse(translation)
        for entry in tree.iterfind("string"):
            if entry.get("name") == "app_name":
                entry.text = LABEL
                tree.write(translation, encoding="utf-8", xml_declaration=True)
                break


def configure_manifest(directory):
    manifest = directory / "AndroidManifest.xml"
    ET.register_namespace("android", ANDROID[1:-1])
    tree = ET.parse(manifest)
    root = tree.getroot()
    assert root.get("package") == ORIGINAL_ID
    # Keep the original classes, whose JNI entry points use the Java package.
    root.set("package", APP_ID)
    for declaration in ("permission", "uses-permission"):
        for entry in root.iterfind(declaration):
            name = entry.get(ANDROID + "name", "")
            if name.startswith(ORIGINAL_ID + "."):
                entry.set(ANDROID + "name", APP_ID + name[len(ORIGINAL_ID):])
    tree.write(manifest, encoding="utf-8", xml_declaration=True)
    config = directory / "apktool.yml"
    data = config.read_text()
    data = re.sub(r"(?m)^  minSdkVersion: 16$", "  minSdkVersion: 21", data)
    data = re.sub(r"(?m)^  versionCode: 132$", "  versionCode: 403", data)
    data = re.sub(r"(?m)^  versionName: 1\.24$", "  versionName: 1.24-es", data)
    config.write_text(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("original_apk", type=Path)
    parser.add_argument("--native-dir", type=Path, required=True)
    parser.add_argument("--apktool", type=Path, required=True)
    parser.add_argument("--build-tools", type=Path, required=True)
    parser.add_argument("--keystore", type=Path, required=True)
    parser.add_argument("--key-alias", default="androidkey")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if hashlib.sha256(args.original_apk.read_bytes()).hexdigest() != ORIGINAL_SHA256:
        parser.error("Use the original Android 1.24 APK (versionCode 132) specified in the README.")
    if "ANDROID_KEYSTORE_PASS" not in os.environ:
        parser.error("Set ANDROID_KEYSTORE_PASS for the signing keystore.")
    with tempfile.TemporaryDirectory(prefix="powder-android-") as name:
        work = Path(name)
        decoded = work / "original"
        apktool = ("java", "-jar", str(args.apktool.resolve()))
        run(*apktool, "d", "-s", "--frame-path", str(work / "framework"),
            "-o", str(decoded), str(args.original_apk))
        configure_manifest(decoded)
        localize_android_resources(decoded / "res")
        for abi in ABIS:
            source = args.native_dir / abi / "libapplication.so"
            (decoded / "lib" / abi / source.name).write_bytes(source.read_bytes())
        unsigned = work / "unsigned.apk"
        aligned = work / "aligned.apk"
        run(*apktool, "b", "--frame-path", str(work / "framework"),
            "-a", str(args.build_tools / "aapt2"), "-o", str(unsigned), str(decoded))
        with ZipFile(args.original_apk) as original, ZipFile(unsigned) as assembled:
            assert original.read("classes.dex") == assembled.read("classes.dex")
            for path in original.namelist():
                if path.startswith("lib/") and not path.endswith("/libapplication.so"):
                    assert original.read(path) == assembled.read(path), path
        run(str(args.build_tools / "zipalign"), "-P", "16", "-f", "4", str(unsigned), str(aligned))
        args.output.parent.mkdir(parents=True, exist_ok=True)
        run(str(args.build_tools / "apksigner"), "sign", "--ks", str(args.keystore),
            "--ks-key-alias", args.key_alias, "--ks-pass", "env:ANDROID_KEYSTORE_PASS",
            "--out", str(args.output), str(aligned))
        run(str(args.build_tools / "apksigner"), "verify", "--verbose", str(args.output))


if __name__ == "__main__":
    main()
