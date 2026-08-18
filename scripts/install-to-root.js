// scripts/install-to-root.js
const { execSync } = require("child_process");
const fs = require("fs");
const path = require("path");

function run(cmd, opts = {}) {
  console.log("> " + cmd);
  execSync(cmd, { stdio: "inherit", ...opts });
}

try {
  // Pfad der Script-Datei; Library-Ordner ist zwei Ebenen höher, passe an wenn nötig
  const scriptDir = path.resolve(__dirname);
  const libDir = path.resolve(scriptDir, ".."); // wenn script in scripts/, libDir = ../
  // Ziel: vier Ebenen höher relativ zur Library
  const target = path.resolve(libDir, "..", "..", "..", "..");
  console.log("Library Verzeichnis:", libDir);
  console.log("Ziel Verzeichnis:", target);

  if (!fs.existsSync(libDir)) {
    throw new Error("Library-Verzeichnis nicht gefunden: " + libDir);
  }
  if (!fs.existsSync(target)) {
    console.log("Zielverzeichnis existiert nicht. Erstelle:", target);
    fs.mkdirSync(target, { recursive: true });
  }

  const srcPkg = path.join(libDir, "package.json");
  const srcLock = path.join(libDir, "package-lock.json");
  if (!fs.existsSync(srcPkg)) {
    throw new Error("package.json in der Library nicht gefunden: " + srcPkg);
  }

  const tgtPkg = path.join(target, "package.json");
  const tgtLock = path.join(target, "package-lock.json");

  // Backup vorhandener Dateien im Ziel
  const now = new Date().toISOString().replace(/[:.]/g, "-");
  if (fs.existsSync(tgtPkg)) {
    const bak = tgtPkg + ".bak." + now;
    console.log("Backup vorhandener package.json ->", bak);
    fs.copyFileSync(tgtPkg, bak);
  }
  if (fs.existsSync(tgtLock)) {
    const bak2 = tgtLock + ".bak." + now;
    console.log("Backup vorhandener package-lock.json ->", bak2);
    fs.copyFileSync(tgtLock, bak2);
  }

  // Kopiere package.json und optional package-lock.json
  console.log("Kopiere package.json nach Ziel...");
  fs.copyFileSync(srcPkg, tgtPkg);
  if (fs.existsSync(srcLock)) {
    console.log("Kopiere package-lock.json nach Ziel...");
    fs.copyFileSync(srcLock, tgtLock);
  } else {
    console.log("Keine package-lock.json in der Library gefunden. Install ohne Lockfile.");
  }

  // npm ausführen: ci wenn lock vorhanden, sonst install
  console.log("Starte npm im Zielverzeichnis...");
  if (fs.existsSync(tgtLock)) {
    run(`npm ci --prefix "${target}" --no-audit --no-fund`);
  } else {
    // legacy-peer-deps als pragmatischer Fallback bei Peer-Konflikten
    run(`npm install --prefix "${target}" --legacy-peer-deps --no-audit --no-fund`);
  }

  console.log("Installation im Ziel abgeschlossen.");
  console.log("Wichtig: Passe ggf. webpack.config.js an, damit resolve.modules den Pfad '../../../../node_modules' enthält.");
} catch (err) {
  console.error("FEHLER:", err && err.message ? err.message : err);
  process.exit(1);
}
