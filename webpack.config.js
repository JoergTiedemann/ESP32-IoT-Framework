const HtmlWebPackPlugin = require("html-webpack-plugin");
const MiniCssExtractPlugin = require("mini-css-extract-plugin");
const { CleanWebpackPlugin } = require("clean-webpack-plugin");
const HtmlWebpackInlineSourcePlugin = require("html-webpack-inline-source-plugin");
const CompressionPlugin = require("compression-webpack-plugin");
const EventHooksPlugin = require("event-hooks-webpack-plugin");

const fs = require("fs");
const path = require("path");
const del = require("del");

module.exports = (env, argv) => ({
    
    context: path.resolve(__dirname),

    entry: "./gui/js/index.js",

    output: {
        filename: "bundle.js",
    },

    module: {
        rules: [
            {
                test: /\.(js|jsx)$/,
                exclude: /node_modules/,
                use: {
                    loader: "babel-loader",
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
        alias: {
            react: "preact/compat",
            "react-dom": "preact/compat",
        },
    },

    // Wenn die Compilierug / Erzeugung der HTML Seite durch webpack  fehlschlaegt, dann in der consol npm run dev aufrufen und im Browser dann die URL http://localhost:8080/index.html aufrufen
    // n der Console wird dann angegeben was schief gelaufen ist und wo der Fehler in den JS Scripten liegt
    // die ganzen benoetigten Node.js module werden  in der windows console (cmd) mit npm ci installiert und zwar aus dem Verzeichnis ESP8266 iot Framework
    // die Nachfolgende Zeile kann fuer diesen zweck auskommentiert werden um evtl. noch weitere Infos zu erhalten
    //devtool: 'inline-source-map',

    plugins: [
        new MiniCssExtractPlugin(),
        new HtmlWebPackPlugin({
            template: "./gui/index.html",
            filename: "./index.html",
            inlineSource: ".(js|css)$", // embed all javascript and css inline
        }),
        new CleanWebpackPlugin({
            protectWebpackAssets: (argv.mode === "production"),
            cleanAfterEveryBuildPatterns: ["**/*.js", "**/*.html", "**/*.css", "**/*.js.gz", "**/*.css.gz"],
        }),
        new HtmlWebpackInlineSourcePlugin(),
        new MiniCssExtractPlugin(),
        new CompressionPlugin(),        
        new EventHooksPlugin({
            done: () => {
                if (argv.mode === "production") {
                    const source = "./dist/index.html.gz";
                    const destination = "./src/generated/html.h";

                    const wstream = fs.createWriteStream(destination);
                    wstream.on("error", function (err) {
                        console.log(err);
                    });

                    const data = fs.readFileSync(source);
                    
                    wstream.write("#ifndef HTML_H\n");
                    wstream.write("#define HTML_H\n\n");
                    wstream.write("#include <Arduino.h>\n\n");                

                    wstream.write(`#define html_len ${data.length}\n\n`);

                    wstream.write("const uint8_t html[] PROGMEM = {");

                    for (let i = 0; i < data.length; i++) {
                        if (i % 1000 == 0) {wstream.write("\n");}
                        wstream.write(`0x${(`00${data[i].toString(16)}`).slice(-2)}`);
                        if (i < data.length - 1) {wstream.write(",");}
                    }

                    wstream.write("\n};");

                    wstream.write("\n\n#endif\n");

                    wstream.end();

                    del([source]);
                    del("./dist/");
                }
            },
        }),
    ],
});