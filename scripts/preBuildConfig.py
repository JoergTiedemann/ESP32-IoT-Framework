# preBuildConfig.py
import json
import binascii
import inspect
import os
from pathlib import Path

def preBuildConfigFun(env=None):
    # if env is not None:
    #     print("preBuildConfigFun: PIOENV =", env.get("PIOENV"))
    #     print("preBuildConfigFun: BOARD =", env.get("BOARD"))

    # Pfad dieser Datei (Library)
    this_file = Path(inspect.getframeinfo(inspect.currentframe()).filename).resolve()
    this_dir = this_file.parent

    # Projekt-Root: primär aus PROJECT_DIR (Umgebung) verwenden, sonst vier Ebenen höher
    env_project_dir = os.environ.get("PROJECT_DIR")
    if env_project_dir:
        project_root = Path(env_project_dir).resolve()
    else:
        project_root = (this_dir / ".." / ".." / ".." / ".." / "..").resolve()

    # print("preBuildConfigFun: this_dir =", str(this_dir))
    # print("preBuildConfigFun: project_root =", str(project_root))

    # Eingabe: configuration.json im Projekt-Root (PROJECT_ROOT/configuration.json)
    src_config = project_root / "configuration.json"
    if not src_config.exists():
        raise FileNotFoundError(f"configuration.json nicht gefunden im Projekt-Root: {src_config}")

    # Ausgabe: project_root/src/generated
    out_dir = project_root / "src" / "generated"
    out_dir.mkdir(parents=True, exist_ok=True)

    h_path = out_dir / "config.h"
    cpp_path = out_dir / "config.cpp"

    print("preBuildConfigFun: Lese Konfiguration von:", str(src_config))
    # print("preBuildConfigFun: Schreibe Dateien nach:", str(out_dir))

    # Lade JSON
    with open(src_config, "r", encoding="utf8") as f:
        data = json.load(f)

    if env is not None and env.get("BOARD") != "esp32dev":
        for item in data:
            if item.get("name") == "FirmwareURL" and isinstance(item.get("value"), str):
                if item["value"].endswith(".bin"):
                    item["value"] = item["value"][:-4] + "_s3.bin"
                    print("preBuildConfigFun: FirmwareURL angepasst:", item["value"])

    # Öffne Dateien zum Schreiben (überschreiben)
    with open(h_path, "w", encoding="utf8") as h, open(cpp_path, "w", encoding="utf8") as cpp:
        # Header
        h.write("#ifndef CONFIG_H\n")
        h.write("#define CONFIG_H\n\n")
        h.write("struct configData\n{\n")

        cpp.write("#include <Arduino.h>\n")
        cpp.write('#include "config.h"\n\n')

        # configVersion als CRC32 über die JSON-String-Repräsentation (wie vorher)
        cpp.write("uint32_t configVersion = " + str(binascii.crc32(json.dumps(data).encode())) + "; //generated identifier to compare config with EEPROM\n\n")

        cpp.write("const configData defaults PROGMEM =\n{\n")

        # Loop durch Variablen (keine Typänderungen gegenüber Original)
        first = True
        for item in data:
            if item['type'] != 'separator' and item['type'] != 'label' and item['type'] != 'header':
                if first == True:
                    first = False
                else:
                    cpp.write(',\n')

                if item['type'] == 'char':
                    cpp.write("\t\"" + item['value'] + "\"")
                    h.write("\tchar " + item['name'] + "[" + str(item['length']) + "];\n")
                elif item['type'] == 'color':
                    cpp.write("\t{" + str(item['value'][0]) + ',' + str(item['value'][1]) + ',' + str(item['value'][2]) + '}')
                    h.write("\tuint8_t " + item['name'] +"[3];\n")
                elif item['type'] == 'bool':
                    cpp.write("\t" + str(item['value']).lower())
                    h.write("\tvolatile " + item['type'] + " " + item['name'] +";\n")
                else:
                    cpp.write("\t" + str(item['value']))
                    h.write("\tvolatile " + item['type'] + " " + item['name'] +";\n")

        # Footer
        h.write("};\n\nextern uint32_t configVersion;\n")
        h.write("extern const configData defaults;\n\n")
        h.write("#endif")

        cpp.write("\n};")

    print("preBuildConfigFun: Dateien erzeugt:", str(h_path), str(cpp_path))
