// webpack.config.js
const HtmlWebPackPlugin = require("html-webpack-plugin");
const MiniCssExtractPlugin = require("mini-css-extract-plugin");
const { CleanWebpackPlugin } = require("clean-webpack-plugin");
const HtmlWebpackInlineSourcePlugin = require("html-webpack-inline-source-plugin");
const CompressionPlugin = require("compression-webpack-plugin");
const EventHooksPlugin = require("event-hooks-webpack-plugin");

const fs = require("fs");
const path = require("path");
const del = require("del");
const zlib = require("zlib");

// Projekt-Root (das aktuelle Arbeitsverzeichnis beim Starten des Builds)
const projectRoot = path.resolve(process.cwd());
// Pfad relativ zur Config-Datei (falls du lokal baust)
const configRelativeNodeModules = path.resolve(__dirname, "../../../../node_modules");

// Debugausgaben
console.log("WEBPACK CONFIG LOADED");
console.log("BUILD CWD (projectRoot):", projectRoot);
console.log("CONFIG DIR (__dirname):", __dirname);
console.log("project node_modules:", path.join(projectRoot, "node_modules"));
console.log("configRelativeNodeModules:", configRelativeNodeModules);

module.exports = (env, argv) => ({
  context: path.resolve(__dirname),

  entry: "./gui/js/index.js",

  output: {
    path: path.resolve(__dirname, "dist"),
    filename: "bundle.js",
    publicPath: ""
  },

  // Webpack filesystem cache in das zentrale node_modules/.cache legen (priorisiert projectRoot)
  cache: {
    type: "filesystem",
    cacheDirectory: path.resolve(projectRoot, "node_modules/.cache/webpack")
  },

  module: {
    rules: [
      {
        test: /\.(js|jsx)$/,
        exclude: /node_modules/,
        use: {
          loader: "babel-loader",
          options: {
            cacheDirectory: path.resolve(projectRoot, "node_modules/.cache/babel-loader")
          }
        },
      },
      {
        test: /\.html$/,
        use: [
          {
            loader: "html-loader",
            options: { minimize: true },
          },
        ],
      },
      {
        test: /\.css$/,
        use: [MiniCssExtractPlugin.loader, "css-loader"],
      },
      {
        test: /\.(jpg|png|gif|svg)$/,
        use: [
          {
            loader: "url-loader",
            options: {
              limit: 10000,
              name: "[name].[ext]",
              outputPath: "img/",
              publicPath: "img/",
            },
          },
          {
            loader: "image-webpack-loader",
            options: {
              pngquant: {
                quality: "20-40",
              },
            },
          },
        ],
      },
    ],
  },

  optimization: {
    minimize: true,
  },

  resolve: {
    extensions: ['.js', '.jsx', '.json'],
    modules: [
      path.join(projectRoot, "node_modules"),
      configRelativeNodeModules,
      "node_modules"
    ],
    alias: {
      react: "preact/compat",
      "react-dom": "preact/compat",
    },
  },

  resolveLoader: {
    modules: [
      path.join(projectRoot, "node_modules"),
      configRelativeNodeModules,
      "node_modules"
    ]
  },

  plugins: [
    new MiniCssExtractPlugin(),
    new HtmlWebPackPlugin({
      template: path.resolve(__dirname, "gui", "index.html"),
      filename: "index.html",
      inlineSource: ".(js|css)$",
    }),
    new CleanWebpackPlugin({
      protectWebpackAssets: (argv.mode === "production"),
      cleanAfterEveryBuildPatterns: ["**/*.js", "**/*.html", "**/*.css", "**/*.js.gz", "**/*.css.gz"],
    }),
    new HtmlWebpackInlineSourcePlugin(),
    new CompressionPlugin(),

    // done-Hook: erzeugt gz aus dist/index.html und schreibt html.h in projectRoot/src/generated
    new EventHooksPlugin({
      done: () => {
        if (argv.mode === "production") {
          try {
            const distDir = path.resolve(__dirname, "dist"); // dist der Config (Library)
            const htmlPath = path.join(distDir, "index.html");
            const gzPath = path.join(distDir, "index.html.gz");

            // NEU: Ziel unterhalb des Projekt-Root
            const destination = path.resolve(projectRoot, "src", "generated", "html.h");

            console.log("done-hook: distDir =", distDir);
            console.log("done-hook: htmlPath =", htmlPath);
            console.log("done-hook: destination (projectRoot/src/generated/html.h) =", destination);

            if (!fs.existsSync(htmlPath)) {
              console.warn("Warnung: dist/index.html nicht gefunden — überspringe Erzeugung von html.h");
              return;
            }

            // gzip aus index.html erzeugen (unabhängig von CompressionPlugin)
            const htmlBuffer = fs.readFileSync(htmlPath);
            const gzBuffer = zlib.gzipSync(htmlBuffer);
            fs.writeFileSync(gzPath, gzBuffer);

            // Header-Datei schreiben (in projectRoot/src/generated)
            const data = gzBuffer;
            const outDir = path.dirname(destination);
            if (!fs.existsSync(outDir)) fs.mkdirSync(outDir, { recursive: true });

            const wstream = fs.createWriteStream(destination);
            wstream.write("#ifndef HTML_H\n");
            wstream.write("#define HTML_H\n\n");
            wstream.write("#include <Arduino.h>\n\n");
            wstream.write(`#define html_len ${data.length}\n\n`);
            wstream.write("const uint8_t html[] PROGMEM = {");
            for (let i = 0; i < data.length; i++) {
              if (i % 1000 === 0) wstream.write("\n");
              wstream.write(`0x${(`00${data[i].toString(16)}`).slice(-2)}`);
              if (i < data.length - 1) wstream.write(",");
            }
            wstream.write("\n};\n\n#endif\n");
            wstream.end();

            // Aufräumen: temporäre gz und dist entfernen (optional)
            try { fs.unlinkSync(gzPath); } catch (e) { /* ignore */ }
            try { del.sync([distDir]); } catch (e) { console.warn("Warnung beim Löschen von dist:", e.message); }

            console.log("Header-Datei erzeugt:", destination);
          } catch (err) {
            console.error("Fehler im done-Hook:", err && err.message ? err.message : err);
          }
        }
      },
    }),
  ],
});
