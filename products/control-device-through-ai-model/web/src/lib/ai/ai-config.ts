import { createOpenAI } from "@ai-sdk/openai";

/**
 * AI Model Provider Configuration
 *
 * Two self-hosted model providers, both OpenAI-compatible:
 * 1. SGLang server — Qwen 3.8 27B (powerful, best tool calling)
 * 2. Ollama via Cloudflare Tunnel — Qwen 3 4B/1.7B (lightweight, fast)
 *
 * URLs are configured via environment variables for portability.
 */

// Provider 1: SGLang server (Qwen 3.8 27B)
export const sglangProvider = createOpenAI({
  baseURL: process.env.SGLANG_BASE_URL || "http://100.64.0.25:8103/v1",
  apiKey: "not-needed",
  name: "sglang",
});

// Provider 2: Ollama via Cloudflare Tunnel
export const ollamaProvider = createOpenAI({
  baseURL: process.env.OLLAMA_BASE_URL || "https://piano-vpn-shell-zero.trycloudflare.com/v1",
  apiKey: "ollama",
  name: "ollama",
});

// Lookup map for dynamic model selection from frontend
export const MODEL_PROVIDERS = {
  "sglang-qwen38-27b": {
    provider: sglangProvider,
    modelId: "qwen38-27b",
    label: "Qwen 3.8 27B (SGLang)",
    description: "Powerful, best for complex commands",
  },
  "ollama-qwen3-4b": {
    provider: ollamaProvider,
    modelId: "qwen3:4b",
    label: "Qwen 3 4B (Ollama)",
    description: "Balanced speed and quality",
  },
  "ollama-qwen3-1.7b": {
    provider: ollamaProvider,
    modelId: "qwen3:1.7b",
    label: "Qwen 3 1.7B (Ollama)",
    description: "Fastest, for simple commands",
  },
} as const;

export type ModelKey = keyof typeof MODEL_PROVIDERS;
export const DEFAULT_MODEL: ModelKey = "sglang-qwen38-27b";

/** List of model keys for frontend dropdown */
export const MODEL_OPTIONS: { key: ModelKey; label: string; description: string }[] =
  Object.entries(MODEL_PROVIDERS).map(([key, val]) => ({
    key: key as ModelKey,
    label: val.label,
    description: val.description,
  }));
