# preBuildHTML.py

import os
import subprocess
import shutil
from pathlib import Path


def preBuildHTMLFun():
    """
    Kopiert package.template.json aus dem Framework-Verzeichnis
    ins Projekt-Root als package.json und ersetzt den Platzhalter
    __WEBPACK_CONFIG__ durch den absoluten Pfad der webpack.config.js.

    Anschließend wird 'npm run build' ausgeführt.
    """

    try:
        # Verzeichnis dieser Datei
        script_file = Path(__file__).resolve()
        lib_dir = script_file.parent

        # Projekt-Root bestimmen
        project_root = (lib_dir / ".." / ".." / ".." / ".." / "..").resolve()

        # Falls PlatformIO PROJECT_DIR gesetzt hat, dieses bevorzugen
        env_project_dir = os.environ.get("PROJECT_DIR")
        if env_project_dir:
            env_root = Path(env_project_dir).resolve()
            if env_root.exists():
                project_root = env_root

        # Framework-Verzeichnis
        framework_dir = script_file.parent.parent

        print(f"preBuildHTMLFun: project_root  = {project_root}")
        print(f"preBuildHTMLFun: lib_dir       = {lib_dir}")
        print(f"preBuildHTMLFun: framework_dir = {framework_dir}")

        # Dateien im Framework-Verzeichnis
        template_pkg = framework_dir / "package.template.json"
        webpack_cfg = framework_dir / "webpack.config.js"

        if not template_pkg.exists():
            raise FileNotFoundError(
                f"Template nicht gefunden: {template_pkg}"
            )

        if not webpack_cfg.exists():
            raise FileNotFoundError(
                f"webpack.config.js nicht gefunden: {webpack_cfg}"
            )

        # Template laden
        content = template_pkg.read_text(encoding="utf-8")

        # Pfad JSON-kompatibel machen
        # Forward-Slashes funktionieren auch unter Windows
        webpack_path = webpack_cfg.resolve().as_posix()

        # Platzhalter ersetzen
        content = content.replace(
            "__WEBPACK_CONFIG__",
            webpack_path
        )

        # package.json im Projekt-Root erzeugen
        target_pkg = project_root / "package.json"

        target_pkg.write_text(
            content,
            encoding="utf-8"
        )

        print(f"preBuildHTMLFun: package.json erzeugt: {target_pkg}")
        print(f"preBuildHTMLFun: webpack.config.js = {webpack_path}")

        # npm suchen
        npm_bin = shutil.which("npm.cmd") or shutil.which("npm")

        if not npm_bin:
            raise RuntimeError(
                "npm wurde nicht im PATH gefunden."
            )

        cmd = [npm_bin, "run", "build"]

        print(
            f"preBuildHTMLFun: Starte {' '.join(cmd)} "
            f"im Verzeichnis {project_root}"
        )

        result = subprocess.run(
            cmd,
            cwd=str(project_root),
            shell=False
        )

        if result.returncode != 0:
            raise RuntimeError(
                f"'npm run build' fehlgeschlagen "
                f"(Exitcode {result.returncode})"
            )

        print("preBuildHTMLFun: Build erfolgreich.")

    except Exception as e:
        print("preBuildHTMLFun: FEHLER:", str(e))
        raise