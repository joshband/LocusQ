import { resolve } from "node:path";
import { defineConfig } from "vite";

export default defineConfig({
  publicDir: false,
  build: {
    emptyOutDir: false,
    minify: "esbuild",
    outDir: resolve(__dirname, "generated"),
    lib: {
      entry: resolve(__dirname, "src/index.ts"),
      formats: ["iife"],
      fileName: () => "index.js",
      name: "LocusQWebUi"
    },
    rollupOptions: {
      output: {
        inlineDynamicImports: true
      }
    }
  }
});
