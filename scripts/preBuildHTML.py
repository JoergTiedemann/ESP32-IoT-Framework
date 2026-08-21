# preBuildHTML.py
import os
import subprocess
import sys
import shutil
from pathlib import Path

def preBuildHTMLFun():
    """
    Führt 'npm run build:esp' im Projekt-Root aus.
    Projekt-Root wird als vier Ebenen über dem Ordner dieser Datei angenommen.
    Liefert aussagekräftige Logs und wirft bei Fehlern eine Exception.
    """
    try:
        # Ordner dieser Datei (Library/config/scripts/...)
        script_file = Path(__file__).resolve()
        lib_dir = script_file.parent

        # Projekt-Root: vier Ebenen höher (wie in deinen anderen Skripten)
        project_root = (lib_dir / ".." / ".." / ".." / ".." / "..").resolve()

        # print("preBuildHTMLFun: script dir:", str(script_file.parent))
        # print("preBuildHTMLFun: project root (vier Ebenen höher):", str(project_root))

        # Fallback: falls PROJECT_DIR in env gesetzt ist (SCons env), nutze das
        env_project_dir = os.environ.get("PROJECT_DIR")
        if env_project_dir:
            env_root = Path(env_project_dir).resolve()
            print("preBuildHTMLFun: PROJECT_DIR env gefunden:", str(env_root))
            # optional: wenn env_root ein plausibler Projekt-Root ist, verwende ihn
            if (env_root / "package.json").exists():
                project_root = env_root
                print("preBuildHTMLFun: Verwende PROJECT_DIR als project_root:", str(project_root))

        # Prüfe, ob package.json im project_root existiert
        pkg = project_root / "package.json"
        if not pkg.exists():
            print("preBuildHTMLFun: WARNUNG: package.json im project root nicht gefunden:", str(pkg))
            # trotzdem versuchen, npm im project_root auszuführen (falls gewünscht)
        
        # Bestimme npm-Binary (Windows: npm.cmd wird bevorzugt)
        npm_bin = shutil.which("npm") or shutil.which("npm.cmd")
        if not npm_bin:
            raise RuntimeError("npm nicht gefunden im PATH. Bitte npm installieren oder PATH anpassen.")

        # Kommando: npm run build:esp
        cmd = [npm_bin, "run", "build"]

        print("preBuildHTMLFun: Starte:", " ".join(cmd), "im Verzeichnis:", str(project_root))

        # Führe das Kommando im project_root aus; gebe stdout/stderr direkt weiter
        result = subprocess.run(cmd, cwd=str(project_root), shell=False)

        if result.returncode != 0:
            raise RuntimeError(f"'npm run build' schlug fehl mit Exitcode {result.returncode}")

        print("preBuildHTMLFun: 'npm run build' erfolgreich abgeschlossen.")
    except Exception as e:
        # Log und re-raise, damit der Build-Prozess die Fehlersituation sieht
        print("preBuildHTMLFun: FEHLER:", str(e))
        raise
