# preBuildDash.py
import json
import inspect
import os
from pathlib import Path

def preBuildDashFun():
    # Pfad dieser Datei (Library)
    this_file = Path(inspect.getframeinfo(inspect.currentframe()).filename).resolve()
    this_dir = this_file.parent

    # Projekt-Root: primär aus PROJECT_DIR (Umgebung) verwenden, sonst vier Ebenen höher
    env_project_dir = os.environ.get("PROJECT_DIR")
    if env_project_dir:
        project_root = Path(env_project_dir).resolve()
    else:
        project_root = (this_dir / ".." / ".." / ".." / ".." / "..").resolve()

    # print("preBuildDashFun: this_dir =", str(this_dir))
    # print("preBuildDashFun: project_root =", str(project_root))

    # Eingabe: dashboard.json im Projekt-Root (PROJECT_ROOT/dashboard.json)
    src_dashboard = project_root / "dashboard.json"
    if not src_dashboard.exists():
        raise FileNotFoundError(f"dashboard.json nicht gefunden im Projekt-Root: {src_dashboard}")

    # Ausgabe: project_root/src/generated/dash.h
    out_dir = project_root / "src" / "generated"
    out_dir.mkdir(parents=True, exist_ok=True)
    h_path = out_dir / "dash.h"

    print("preBuildDashFun: Lese Dashboard von:", str(src_dashboard))
    # print("preBuildDashFun: Schreibe dash.h nach:", str(h_path))
    historicdataCount = "0"

    # Lade JSON
    with open(src_dashboard, "r", encoding="utf8") as f:
        data = json.load(f)

    # Schreibe Header (gleiche Logik wie vorher, keine Typänderungen)
    with open(h_path, "w", encoding="utf8") as h:
        h.write("#ifndef DASH_H\n")
        h.write("#define DASH_H\n\n")
        h.write("struct dashboardData\n{\n")

        for item in data:
            if item['type'] != 'separator' and item['type'] != 'label' and item['type'] != 'header':
                if item['type'] == 'char':
                    h.write("\tchar " + item['name'] + "[" + str(item['length']) + "];\n")
                elif item['type'] == 'bool':
                    h.write("\t" + item['type'] + " " + item['name'] +";\n")
                elif item['type'] == 'historic':
                    print("*********************************************************************************************************")
                    print("*********************************************************************************************************")
                    print("Attention: historicdata used! Only 1 historicdata array is allowed !, it has to be send after dashdata! ")
                    print("*********************************************************************************************************")
                    print("*********************************************************************************************************")
                    historicdataCount = "1"
                elif item['type'] == 'historic1' or item['type'] == 'historic2':
                    print("*********************************************************************************************************")
                    print("*********************************************************************************************************")
                    print("Attention: historicdata used! Exactly 2 historicdata arrays must be send after dashdata! ")
                    print("*********************************************************************************************************")
                    print("*********************************************************************************************************")
                    if historicdataCount != '2' and item['type'] == 'historic1':
                      historicdataCount = "1"
                    if item['type'] == 'historic2':
                      historicdataCount = "2"
                else:
                    h.write("\t" + item['type'] + " " + item['name'] +";\n")

        h.write("};\n\n")
        h.write("const int ciHistoricDatasetCount =" +historicdataCount +";\n\n#endif\n")
    print("preBuildDashFun: Datei erzeugt:", str(h_path))
