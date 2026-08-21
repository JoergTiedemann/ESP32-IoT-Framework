# add_lib_versions.py
# PlatformIO extra script to collect used library and package versions
# and write them into a C header at <project_root>/src/generated/platformio_used_libraries.h
#
# Usage: add this file as an extra script in platformio.ini (pre: or post:)
# Example platformio.ini:
# extra_scripts = pre:add_lib_versions.py
#
# The script prefers the build-time dependency graph (env.GetLibBuilders / ProjectAsLibBuilder)
# and falls back to scanning .pio/libdeps/<env> restricted to the current PIOENV.
#
# It writes:
#  - #define PLATFORMIO_USED_LIBRARIES "...'lib':'ver',..."
#  - #define LIB_VERSION_<LIBNAME> "version"
#  - #define PIO_PLATFORM_VERSION "x.y.z"
#  - #define PIO_PACKAGE_<NAME>_PKG_VERSION "raw"
#  - #define PIO_PACKAGE_<NAME>_DECODED_VERSION "decoded" (if available)

import os
import re
import json
from platformio.builder.tools.piolib import ProjectAsLibBuilder, PackageItem, LibBuilderBase
from platformio.package.version import get_original_version
from SCons.Script import ARGUMENTS
Import("env", "projenv")


def make_macro_name(lib_name):
    lib_name = lib_name.upper()
    lib_name = re.sub(r"[^A-Z0-9_]", "_", lib_name)
    lib_name = re.sub(r"_+", "_", lib_name)
    return lib_name


def _collect_from_lb(lb, library_versions, library_owner, seen, require_dependent=False):
    """
    Rekursive Sammlung von lb selbst und seinen depbuilders.
    Wenn require_dependent=True, werden nur libs mit lb.dependent==True aufgenommen.
    'seen' verhindert Endlosschleifen.
    """
    if not lb or id(lb) in seen:
        return
    seen.add(id(lb))

    # Wenn require_dependent gesetzt ist, skip wenn nicht dependent
    if require_dependent and not getattr(lb, "dependent", False):
        # trotzdem rekursiv prüfen, weil transitive deps möglicherweise dependent=True
        for child in getattr(lb, "depbuilders", []) or []:
            _collect_from_lb(child, library_versions, library_owner, seen, require_dependent)
        return

    try:
        pkg = PackageItem(lb.path)
    except Exception:
        pkg = None

    owner = ""
    try:
        if pkg and pkg.metadata and pkg.metadata.spec and pkg.metadata.spec.owner:
            owner = pkg.metadata.spec.owner
    except Exception:
        owner = ""

    name = getattr(lb, "name", None) or os.path.basename(getattr(lb, "path", "unknown"))
    version = None
    try:
        version = pkg.metadata.version if (pkg and pkg.metadata and getattr(pkg.metadata, "version", None)) else getattr(lb, "version", None)
    except Exception:
        version = getattr(lb, "version", None)

    library_versions[str(name)] = str(version) if version is not None else ""
    library_owner[str(name)] = str(owner)

    for child in getattr(lb, "depbuilders", []) or []:
        _collect_from_lb(child, library_versions, library_owner, seen, require_dependent)


def _scan_project_libdeps(env, library_versions, library_owner):
    """
    Fallback: Scannt .pio/libdeps/<PIOENV> (oder PROJECT_LIBDEPS_DIR/<PIOENV>) und ergänzt library_versions.
    Nur Bibliotheken aus dem aktuellen PIOENV-Ordner werden berücksichtigt.
    """
    try:
        pioenv = env.get("PIOENV") or ""
        proj_libdeps_root = env.get("PROJECT_LIBDEPS_DIR")  # kann None sein
        candidate = None

        # Prüfe PROJECT_LIBDEPS_DIR direkt
        if proj_libdeps_root:
            if pioenv:
                path_with_env = os.path.join(proj_libdeps_root, pioenv)
                if os.path.isdir(path_with_env):
                    candidate = path_with_env
            if not candidate and os.path.isdir(proj_libdeps_root):
                # manchmal ist PROJECT_LIBDEPS_DIR bereits inklusive envname
                candidate = proj_libdeps_root

        # Fallback auf <PROJECT_DIR>/.pio/libdeps/<PIOENV>
        if not candidate:
            fallback = os.path.join(env.get("PROJECT_DIR") or os.getcwd(), ".pio", "libdeps")
            if pioenv:
                fallback = os.path.join(fallback, pioenv)
            if os.path.isdir(fallback):
                candidate = fallback

        if not candidate:
            return

        # Scan nur des gewählten Verzeichnisses
        for entry in os.listdir(candidate):
            libpath = os.path.join(candidate, entry)
            if not os.path.isdir(libpath):
                continue
            name = entry
            version = ""
            owner = ""

            # Versuche library.json
            libjson = os.path.join(libpath, "library.json")
            if os.path.isfile(libjson):
                try:
                    with open(libjson, "r", encoding="utf-8") as fh:
                        data = json.load(fh)
                        name = data.get("name", name)
                        version = data.get("version", version)
                except Exception:
                    pass

            # package.json
            if not version:
                pkgjson = os.path.join(libpath, "package.json")
                if os.path.isfile(pkgjson):
                    try:
                        with open(pkgjson, "r", encoding="utf-8") as fh:
                            data = json.load(fh)
                            version = data.get("version", version)
                    except Exception:
                        pass

            # library.properties (Arduino)
            if not version:
                props = os.path.join(libpath, "library.properties")
                if os.path.isfile(props):
                    try:
                        with open(props, "r", encoding="utf-8") as fh:
                            for line in fh:
                                if line.strip().startswith("version="):
                                    version = line.strip().split("=", 1)[1].strip()
                                    break
                    except Exception:
                        pass

            # piopkg metadata
            if not version:
                for fname in (".piopkgmanager.json", ".piopkgmeta.json", ".piopkg.json"):
                    fpath = os.path.join(libpath, fname)
                    if os.path.isfile(fpath):
                        try:
                            with open(fpath, "r", encoding="utf-8") as fh:
                                data = json.load(fh)
                                version = data.get("version", version)
                                break
                        except Exception:
                            pass

            # fallback: parse version from folder name patterns like Name@1.2.3 or Name-1.2.3
            if not version:
                m = re.search(r'@([\d\.]+)', entry)
                if not m:
                    m = re.search(r'-(\d+\.\d+[\d\.]*)$', entry)
                if m:
                    version = m.group(1)

            library_versions[str(name)] = str(version) if version is not None else ""
            library_owner[str(name)] = str(owner)
    except Exception as e:
        print("add_lib_info: error scanning PROJECT_LIBDEPS_DIR:", e)


def _write_header(project_dir, library_versions, library_owner, env):
    """
    Erzeugt die Headerdatei project_dir/src/generated/platformio_used_libraries.h
    """
    # Build macro string like original: 'owner/lib':'version','lib':'version'
    macro_value = ""
    for lib, version in library_versions.items():
        strowner = library_owner.get(lib, "")
        if strowner:
            macro_value += "'" + strowner + "/" + lib + "':'" + version + "',"
        else:
            macro_value += "'" + lib + "':'" + version + "',"
    if macro_value:
        macro_value = macro_value[:-1]  # letztes Komma entfernen

    escaped_macro = macro_value.replace('"', '\\"')

    header_lines = []
    header_lines.append("/* Auto-generated by scripts/add_lib_info.py */")
    header_lines.append("#pragma once")
    header_lines.append("")
    header_lines.append('// PLATFORMIO_USED_LIBRARIES: map of used libraries and versions')
    header_lines.append('#define PLATFORMIO_USED_LIBRARIES "' + escaped_macro + '"')
    header_lines.append("")

    for lib, version in library_versions.items():
        header_lines.append('#define LIB_VERSION_%s "%s"' % (make_macro_name(lib), str(version)))

    # Platform- und Paketinfos
    try:
        platform = env.PioPlatform()
        used_packages = platform.dump_used_packages()
        pkg_metadata = PackageItem(platform.get_dir()).metadata
        platform_version = str(pkg_metadata.version if pkg_metadata else platform.version)
        header_lines.append('#define PIO_PLATFORM_VERSION "%s"' % platform_version)
        for package in used_packages:
            pio_package_version = package.get("version", "")
            name_converter = lambda name: name.upper().replace(" ", "_").replace("-", "_")
            pkg_macro = name_converter(package.get("name", "UNKNOWN"))
            header_lines.append('#define PIO_PACKAGE_%s_PKG_VERSION "%s"' % (pkg_macro, pio_package_version))
            pio_decoded_version = get_original_version(pio_package_version)
            if pio_decoded_version is not None:
                header_lines.append('#define PIO_PACKAGE_%s_DECODED_VERSION "%s"' % (pkg_macro, pio_decoded_version))
    except Exception:
        # non-fatal
        pass

    target_dir = os.path.join(project_dir, "src", "generated")
    os.makedirs(target_dir, exist_ok=True)
    header_path = os.path.join(target_dir, "platformio_used_libraries.h")
    with open(header_path, "w", encoding="utf-8") as fh:
        fh.write("\n".join(header_lines) + "\n")
    print("add_lib_info: wrote header to", header_path)
    return header_path


def main():
    """
    Hauptfunktion: sammelt verwendete Bibliotheken und schreibt die Header.
    Wird beim Import durch PlatformIO automatisch ausgeführt.
    """
    try:
        # print("add_lib_versions: script entry")
        project_dir = env.get("PROJECT_DIR") or os.getcwd()
        # print("add_lib_versions: PROJECT_DIR =", project_dir)
        # print("add_lib_versions: PIOVERBOSE =", ARGUMENTS.get("PIOVERBOSE", 0))

        library_versions = {}
        library_owner = {}

        # 1) Versuche ProjectAsLibBuilder wie im Original (wenn möglich)
        try:
            project = ProjectAsLibBuilder(env, project_dir)
            try:
                ldf_mode = LibBuilderBase.lib_ldf_mode.fget(project)
            except Exception:
                ldf_mode = ""
            try:
                project.search_deps_recursive()
            except Exception:
                pass
            # Wenn chain-mode, korrigiere wie original
            try:
                lib_builders = env.GetLibBuilders() or []
                if ldf_mode and ldf_mode.startswith("chain") and getattr(project, "depbuilders", None):
                    # _correct_found_libs erwartet lib_builders
                    _correct_found_libs(lib_builders)
            except Exception:
                pass
            # Sammle aus project.depbuilders (falls vorhanden)
            try:
                seen = set()
                for lb in getattr(project, "depbuilders", []) or []:
                    _collect_from_lb(lb, library_versions, library_owner, seen, require_dependent=False)
            except Exception:
                pass
        except Exception:
            # ProjectAsLibBuilder nicht verfügbar oder fehlgeschlagen -> weiter mit env.GetLibBuilders()
            pass

        # 2) Primäre Quelle: env.GetLibBuilders() (nur libs, die tatsächlich dependent sind)
        try:
            lbs = env.GetLibBuilders() or []
            # print("add_lib_versions: env.GetLibBuilders() count:", len(lbs))
            # Sammle Namen, die als dependent markiert sind (verwendete libs)
            used_lib_names = set()
            seen_names = set()
            def collect_used_names(lb):
                if not lb or id(lb) in seen_names:
                    return
                seen_names.add(id(lb))
                if getattr(lb, "dependent", False):
                    name = getattr(lb, "name", None) or os.path.basename(getattr(lb, "path", "unknown"))
                    used_lib_names.add(str(name))
                for child in getattr(lb, "depbuilders", []) or []:
                    collect_used_names(child)
            for lb in lbs:
                collect_used_names(lb)

            # Wenn used_lib_names leer ist, akzeptiere alle env.GetLibBuilders() als verwendet
            require_dependent = bool(used_lib_names)
            seen = set()
            for lb in lbs:
                _collect_from_lb(lb, library_versions, library_owner, seen, require_dependent=require_dependent)
        except Exception as e:
            print("add_lib_info: env.GetLibBuilders() failed:", e)

        # 3) Fallback: falls nur sehr wenige oder keine libs gefunden wurden, scanne .pio/libdeps/<PIOENV>
        if not library_versions or len(library_versions) <= 1:
            _scan_project_libdeps(env, library_versions, library_owner)

        print("add_lib_info: collected libraries:", library_versions)

        # 4) Schreibe Header in project/src/generated
        header_path = _write_header(project_dir, library_versions, library_owner, env)

        # 5) Füge generated-Ordner dem Include-Pfad hinzu, damit '#include "generated/..."' funktioniert
        generated_dir = os.path.dirname(header_path)
        if os.path.isdir(generated_dir):
            env.Append(CPPPATH=[generated_dir])
            # print("add_lib_info: appended CPPPATH:", generated_dir)

        return header_path

    except Exception as e:
        print("add_lib_info: fatal error:", e)
        raise


# Wenn PlatformIO dieses Modul importiert, führe main() aus.
try:
    main()
except Exception:
    # Fehler werden bereits geloggt; Import darf nicht komplett abbrechen
    pass
