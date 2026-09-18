import { Communicate } from "edge-tts-universal";

export const SUPPORTED_TTS_VOICES = [
  {
    id: "vi-VN-HoaiMyNeural",
    name: "Hoài My (Nữ)",
    lang: "vi-VN",
    gender: "Female",
    isDefault: true,
  },
  {
    id: "vi-VN-NamMinhNeural",
    name: "Nam Minh (Nam)",
    lang: "vi-VN",
    gender: "Male",
    isDefault: false,
  },
  {
    id: "en-US-AriaNeural",
    name: "Aria (Female)",
    lang: "en-US",
    gender: "Female",
    isDefault: true,
  },
  {
    id: "en-US-GuyNeural",
    name: "Guy (Male)",
    lang: "en-US",
    gender: "Male",
    isDefault: false,
  },
] as const;

export type SupportedVoiceId = (typeof SUPPORTED_TTS_VOICES)[number]["id"];

export const DEFAULT_VI_VOICE: SupportedVoiceId = "vi-VN-HoaiMyNeural";
export const DEFAULT_EN_VOICE: SupportedVoiceId = "en-US-AriaNeural";

export interface SynthesizeOptions {
  text: string;
  voice?: string;
  rate?: string;
  pitch?: string;
}

// Simple In-memory LRU-like cache for frequent responses (e.g. "Đã bật đèn")
const MAX_CACHE_ITEMS = 100;
const audioCache = new Map<string, Buffer>();

function getCacheKey(text: string, voice: string, rate: string, pitch: string): string {
  return `${voice}|${rate}|${pitch}|${text.trim().toLowerCase()}`;
}

export function clearTTSCache(): void {
  audioCache.clear();
}

export function detectDefaultVoice(text: string, langHint?: string): SupportedVoiceId {
  const hasVietnamese =
    /[àáảãạăắằẳẵặâấầẩẫậèéẻẽẹêếềểễệìíỉĩịòóỏõọôốồổỗộơớờởỡợùúủũụưứừửữựỳýỷỹỵđ]/i.test(
      text
    );
  if (hasVietnamese || langHint === "vi-VN") {
    return DEFAULT_VI_VOICE;
  }
  return DEFAULT_EN_VOICE;
}

/**
 * Synthesizes text to MP3 audio using Microsoft Edge Neural TTS.
 * Includes in-memory caching for repeated voice sentences.
 */
export async function synthesizeSpeech({
  text,
  voice,
  rate = "+0%",
  pitch = "+0Hz",
}: SynthesizeOptions): Promise<Buffer> {
  const trimmed = text.trim();
  if (!trimmed) {
    throw new Error("Text cannot be empty");
  }

  const selectedVoice = voice || detectDefaultVoice(trimmed);
  const cacheKey = getCacheKey(trimmed, selectedVoice, rate, pitch);

  const cached = audioCache.get(cacheKey);
  if (cached) {
    return cached;
  }

  const communicate = new Communicate(trimmed, {
    voice: selectedVoice,
    rate,
    pitch,
  });

  const chunks: Buffer[] = [];
  for await (const chunk of communicate.stream()) {
    if (chunk.type === "audio" && chunk.data) {
      chunks.push(chunk.data);
    }
  }

  if (chunks.length === 0) {
    throw new Error("No audio data received from Microsoft Edge TTS service");
  }

  const resultBuffer = Buffer.concat(chunks);

  // Evict oldest item if cache limit exceeded
  if (audioCache.size >= MAX_CACHE_ITEMS) {
    const oldestKey = audioCache.keys().next().value;
    if (oldestKey) audioCache.delete(oldestKey);
  }
  audioCache.set(cacheKey, resultBuffer);

  return resultBuffer;
}
