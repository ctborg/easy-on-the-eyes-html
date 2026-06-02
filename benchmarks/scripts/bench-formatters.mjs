#!/usr/bin/env node

import { existsSync, readFileSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { spawnSync } from "node:child_process";

const __dirname = dirname(fileURLToPath(import.meta.url));
const repoRoot = resolve(__dirname, "../..");
const fixturesDir = join(repoRoot, "benchmarks/fixtures");

const rounds = Number.parseInt(process.env.BENCH_ROUNDS || "20", 10);
const warmups = Number.parseInt(process.env.BENCH_WARMUPS || "3", 10);

const fixtures = [
  { name: "HTML", lang: "html", parser: "html", beautifyType: "html", file: join(fixturesDir, "sample.html") },
  { name: "CSS", lang: "css", parser: "css", beautifyType: "css", file: join(fixturesDir, "sample.css") },
  { name: "JS", lang: "js", parser: "babel", beautifyType: "js", file: join(fixturesDir, "sample.js") },
  { name: "JSON", lang: "json", parser: "json", beautifyType: "js", file: join(fixturesDir, "sample.json") },
];

function localBin(name) {
  const suffix = process.platform === "win32" ? ".cmd" : "";
  const candidate = join(repoRoot, "node_modules/.bin", `${name}${suffix}`);
  return existsSync(candidate) ? candidate : null;
}

function npxCommand(pkg, args) {
  const npx = process.platform === "win32" ? "npx.cmd" : "npx";
  return { cmd: npx, args: ["--yes", pkg, ...args], viaNpx: true };
}

function formatterCommands(fixture) {
  const eote = join(repoRoot, "build/easy-on-the-eyes");
  const prettier = localBin("prettier");
  const beautify = localBin("js-beautify");
  return [
    {
      name: "easy-on-the-eyes",
      cmd: eote,
      args: ["--lang", fixture.lang],
      stdinFile: fixture.file,
      available: existsSync(eote),
      note: existsSync(eote) ? "" : "missing build/easy-on-the-eyes",
    },
    {
      name: prettier ? "prettier" : "prettier (npx)",
      ...(prettier
        ? { cmd: prettier, args: ["--parser", fixture.parser, fixture.file], viaNpx: false }
        : npxCommand("prettier", ["--parser", fixture.parser, fixture.file])),
      available: true,
      note: prettier ? "" : "npx includes package startup/download overhead",
    },
    {
      name: beautify ? "js-beautify" : "js-beautify (npx)",
      ...(beautify
        ? { cmd: beautify, args: ["--type", fixture.beautifyType, fixture.file], viaNpx: false }
        : npxCommand("js-beautify", ["--type", fixture.beautifyType, fixture.file])),
      available: true,
      note: beautify ? "" : "npx includes package startup/download overhead",
    },
  ];
}

function runOnce(command) {
  const input = command.stdinFile ? { input: readFileSync(command.stdinFile) } : {};
  const start = process.hrtime.bigint();
  const result = spawnSync(command.cmd, command.args, {
    ...input,
    cwd: repoRoot,
    encoding: "utf8",
    maxBuffer: 64 * 1024 * 1024,
  });
  const end = process.hrtime.bigint();
  if (result.error) {
    return { ok: false, error: result.error.message };
  }
  if (result.status !== 0) {
    return { ok: false, error: (result.stderr || result.stdout || `exit ${result.status}`).trim() };
  }
  return { ok: true, ms: Number(end - start) / 1_000_000 };
}

function stats(samples) {
  const sorted = [...samples].sort((a, b) => a - b);
  const sum = samples.reduce((a, b) => a + b, 0);
  return {
    mean: sum / samples.length,
    min: sorted[0],
    p50: sorted[Math.floor(sorted.length * 0.5)],
    p95: sorted[Math.min(sorted.length - 1, Math.floor(sorted.length * 0.95))],
  };
}

function fmt(n) {
  return `${n.toFixed(2)}ms`;
}

console.log(`Benchmark rounds: ${rounds}, warmups: ${warmups}`);
console.log("Measures full CLI process startup and formatting time.");
console.log("For fairer JS formatter numbers, run: npm i -D prettier js-beautify\n");

for (const fixture of fixtures) {
  console.log(`## ${fixture.name}`);
  const rows = [];
  for (const command of formatterCommands(fixture)) {
    if (!command.available) {
      rows.push({ formatter: command.name, result: `skip (${command.note})` });
      continue;
    }
    for (let i = 0; i < warmups; i++) {
      const warm = runOnce(command);
      if (!warm.ok) {
        rows.push({ formatter: command.name, result: `error (${warm.error.slice(0, 120)})` });
        continue;
      }
    }
    const samples = [];
    let error = null;
    for (let i = 0; i < rounds; i++) {
      const sample = runOnce(command);
      if (!sample.ok) {
        error = sample.error;
        break;
      }
      samples.push(sample.ms);
    }
    if (error) {
      rows.push({ formatter: command.name, result: `error (${error.slice(0, 120)})` });
      continue;
    }
    const s = stats(samples);
    rows.push({
      formatter: command.name,
      result: `mean ${fmt(s.mean)} | p50 ${fmt(s.p50)} | p95 ${fmt(s.p95)} | min ${fmt(s.min)}${command.note ? ` | ${command.note}` : ""}`,
    });
  }
  const width = Math.max(...rows.map((r) => r.formatter.length));
  for (const row of rows) {
    console.log(`${row.formatter.padEnd(width)}  ${row.result}`);
  }
  console.log("");
}
