// scripts/install-to-root.js
const { execSync } = require("child_process");
const fs = require("fs");
const path = require("path");

function run(cmd, opts = {}) {
  console.log("> " + cmd);
  execSync(cmd, { stdio: "inherit", ...opts });
}

function backupIfExists(filePath) {
  if (!fs.existsSync(filePath)) return;
  const now = new Date().toISOString().replace(/[:.]/g, "-");
  const bak = `${filePath}.bak.${now}`;
  fs.copyFileSync(filePath, bak);
  console.log("Backup erstellt:", bak);
}

try {
  // Pfad der Script-Datei; Library-Ordner ist eine Ebene höher (scripts/)
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

  // Quell- und Zielpfade für package.json / package-lock.json
  const srcPkg = path.join(libDir, "package.json");
  const srcLock = path.join(libDir, "package-lock.json");
  if (!fs.existsSync(srcPkg)) {
    throw new Error("package.json in der Library nicht gefunden: " + srcPkg);
  }

  const tgtPkg = path.join(target, "package.json");
  const tgtLock = path.join(target, "package-lock.json");

  // Backup vorhandener Dateien im Ziel
  if (fs.existsSync(tgtPkg)) {
    backupIfExists(tgtPkg);
  }
  if (fs.existsSync(tgtLock)) {
    backupIfExists(tgtLock);
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

  // --- NEU: babel.config.js / babel.config.json kopieren ---
  const srcBabelJs = path.join(libDir, "babel.config.js");
  const srcBabelJson = path.join(libDir, "babel.config.json");
  const tgtBabelJs = path.join(target, "babel.config.js");
  const tgtBabelJson = path.join(target, "babel.config.json");

  if (fs.existsSync(srcBabelJs)) {
    console.log("Gefundene babel.config.js in Library. Sichere ggf. vorhandene Datei und kopiere...");
    if (fs.existsSync(tgtBabelJs)) backupIfExists(tgtBabelJs);
    fs.copyFileSync(srcBabelJs, tgtBabelJs);
    console.log("Kopiert: babel.config.js ->", tgtBabelJs);
  } else if (fs.existsSync(srcBabelJson)) {
    console.log("Gefundene babel.config.json in Library. Sichere ggf. vorhandene Datei und kopiere...");
    if (fs.existsSync(tgtBabelJson)) backupIfExists(tgtBabelJson);
    fs.copyFileSync(srcBabelJson, tgtBabelJson);
    console.log("Kopiert: babel.config.json ->", tgtBabelJson);
  } else {
    // Fallback: prüfe auch auf .babelrc (falls der Nutzer das verwendet)
    const srcBabelRc = path.join(libDir, ".babelrc");
    const tgtBabelRc = path.join(target, ".babelrc");
    if (fs.existsSync(srcBabelRc)) {
      console.log("Gefundene .babelrc in Library. Sichere ggf. vorhandene Datei und kopiere...");
      if (fs.existsSync(tgtBabelRc)) backupIfExists(tgtBabelRc);
      fs.copyFileSync(srcBabelRc, tgtBabelRc);
      console.log("Kopiert: .babelrc ->", tgtBabelRc);
    } else {
      console.log("Keine babel.config.js / babel.config.json / .babelrc in der Library gefunden. Nichts kopiert.");
    }
  }
  // --- ENDE NEU ---

  // npm ausfÃ¼hren: ci wenn lock vorhanden, sonst install
  console.log("Starte npm im Zielverzeichnis...");
  if (fs.existsSync(tgtLock)) {
    run(`npm ci --prefix "${target}" --no-audit --no-fund`);
  } else {
    // legacy-peer-deps als pragmatischer Fallback bei Peer-Konflikten
    run(`npm install --prefix "${target}" --legacy-peer-deps --no-audit --no-fund`);
  }

  console.log("Installation im Ziel abgeschlossen.");
  console.log("Wichtig: Passe ggf. webpack.config.js an, damit resolve.modules den Pfad '../../../../node_modules' enthÃ¤lt.");
} catch (err) {
  console.error("FEHLER:", err && err.message ? err.message : err);
  process.exit(1);
}
