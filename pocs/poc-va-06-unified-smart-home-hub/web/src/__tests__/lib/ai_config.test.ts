import { describe, it, expect } from "vitest";
import { MODEL_PROVIDERS, DEFAULT_MODEL, MODEL_OPTIONS } from "@/lib/ai/ai-config";

describe("AI Config", () => {
  it("defines expected model providers", () => {
    expect(MODEL_PROVIDERS).toHaveProperty("sglang-qwen38-27b");
    expect(MODEL_PROVIDERS).toHaveProperty("ollama-qwen3-4b");
    expect(MODEL_PROVIDERS).toHaveProperty("ollama-qwen3-1.7b");
  });

  it("has a valid default model", () => {
    expect(DEFAULT_MODEL).toBe("sglang-qwen38-27b");
    expect(MODEL_PROVIDERS[DEFAULT_MODEL]).toBeDefined();
  });

  it("provides MODEL_OPTIONS with labels and descriptions for UI", () => {
    expect(MODEL_OPTIONS).toHaveLength(3);
    for (const opt of MODEL_OPTIONS) {
      expect(opt.key).toBeDefined();
      expect(opt.label).toBeTruthy();
      expect(opt.description).toBeTruthy();
    }
  });
});
