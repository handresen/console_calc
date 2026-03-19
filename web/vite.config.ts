import { createReadStream, existsSync } from "node:fs";
import { cp, mkdir } from "node:fs/promises";
import path from "node:path";

import { defineConfig, type Plugin } from "vite";

const wasmArtifactDir = path.resolve(__dirname, "../build/emscripten-host");
const wasmOutputFiles = ["console_calc.mjs", "console_calc.wasm"];

function contentTypeFor(filePath: string): string {
  if (filePath.endsWith(".mjs")) {
    return "text/javascript";
  }
  if (filePath.endsWith(".wasm")) {
    return "application/wasm";
  }
  return "application/octet-stream";
}

function consoleCalcWasmArtifacts(): Plugin {
  return {
    name: "console-calc-wasm-artifacts",
    configureServer(server) {
      server.middlewares.use((req, res, next) => {
        const rawPath = (req.url ?? "/").split("?")[0];
        const wasmPrefix = "/wasm/";
        const wasmIndex = rawPath.indexOf(wasmPrefix);
        if (wasmIndex === -1) {
          next();
          return;
        }

        const relativePath = rawPath.slice(wasmIndex + wasmPrefix.length);
        if (relativePath.length === 0) {
          next();
          return;
        }

        const filePath = path.resolve(wasmArtifactDir, relativePath);
        if (!filePath.startsWith(wasmArtifactDir) || !existsSync(filePath)) {
          next();
          return;
        }

        res.setHeader("Content-Type", contentTypeFor(filePath));
        createReadStream(filePath).pipe(res);
      });
    },
    async writeBundle() {
      const distWasmDir = path.resolve(__dirname, "dist/wasm");
      await mkdir(distWasmDir, { recursive: true });
      for (const fileName of wasmOutputFiles) {
        const source = path.join(wasmArtifactDir, fileName);
        if (existsSync(source)) {
          await cp(source, path.join(distWasmDir, fileName));
        }
      }
    },
  };
}

export default defineConfig({
  base: "/cc/",
  plugins: [consoleCalcWasmArtifacts()],
  server: {
    allowedHosts: ["hvrd.com"],
    host: true,
    port: 3003,
    fs: {
      allow: [path.resolve(__dirname, "..")],
    },
  },
});
