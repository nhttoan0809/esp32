import { describe, it, expect } from "vitest";
import {
  cleanSpeechText,
  detectWakeWord,
  detectCommand,
} from "@/lib/voice_matcher";

describe("Voice Matcher (English Only)", () => {
  it("cleans speech text punctuation and spaces", () => {
    expect(cleanSpeechText("  Turn on the lamp, please!  ")).toBe("turn on the lamp please");
    expect(cleanSpeechText("Wake up... Turn on!")).toBe("wake up turn on");
  });

  it("detects wake words in English", () => {
    expect(detectWakeWord("wake up").detected).toBe(true);
    expect(detectWakeWord("hey lamp, are you there?").detected).toBe(true);
    expect(detectWakeWord("smart lamp").detected).toBe(true);
    expect(detectWakeWord("hello lamp").detected).toBe(true);
    expect(detectWakeWord("something unrelated").detected).toBe(false);
  });

  it("extracts remainder when wake word and command are spoken together", () => {
    const res = detectWakeWord("wake up turn on the light");
    expect(res.detected).toBe(true);
    expect(res.remainder).toBe("turn on the light");

    const res2 = detectWakeWord("hey lamp switch off");
    expect(res2.detected).toBe(true);
    expect(res2.remainder).toBe("switch off");
  });

  it("detects Turn ON commands in English", () => {
    expect(detectCommand("turn on")).toBe("on");
    expect(detectCommand("turn on the light")).toBe("on");
    expect(detectCommand("turn on the lamp")).toBe("on");
    expect(detectCommand("light on")).toBe("on");
    expect(detectCommand("lamp on")).toBe("on");
    expect(detectCommand("power on")).toBe("on");
    expect(detectCommand("switch on")).toBe("on");
  });

  it("detects Turn OFF commands in English", () => {
    expect(detectCommand("turn off")).toBe("off");
    expect(detectCommand("turn off the light")).toBe("off");
    expect(detectCommand("turn off the lamp")).toBe("off");
    expect(detectCommand("light off")).toBe("off");
    expect(detectCommand("lamp off")).toBe("off");
    expect(detectCommand("power off")).toBe("off");
    expect(detectCommand("switch off")).toBe("off");
  });

  it("detects Toggle commands in English", () => {
    expect(detectCommand("toggle")).toBe("toggle");
    expect(detectCommand("toggle lamp")).toBe("toggle");
    expect(detectCommand("toggle light")).toBe("toggle");
    expect(detectCommand("change status")).toBe("toggle");
    expect(detectCommand("change state")).toBe("toggle");
    expect(detectCommand("switch state")).toBe("toggle");
  });

  it("handles negative intent by returning null", () => {
    expect(detectCommand("don't turn on")).toBeNull();
    expect(detectCommand("do not turn off")).toBeNull();
    expect(detectCommand("never switch on")).toBeNull();
  });

  it("handles allowSingleWords option properly", () => {
    // In SLEEPING mode (allowSingleWords: false), single words should NOT trigger
    expect(detectCommand("on", { allowSingleWords: false })).toBeNull();
    expect(detectCommand("off", { allowSingleWords: false })).toBeNull();

    // In AWAKE mode (allowSingleWords: true), single words ARE recognized
    expect(detectCommand("on", { allowSingleWords: true })).toBe("on");
    expect(detectCommand("off", { allowSingleWords: true })).toBe("off");
    expect(detectCommand("toggle", { allowSingleWords: true })).toBe("toggle");
  });
});
