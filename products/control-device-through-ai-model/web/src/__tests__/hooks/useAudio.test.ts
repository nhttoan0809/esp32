import { describe, it, expect } from "vitest";
import { renderHook } from "@testing-library/react";
import { useAudio } from "@/hooks/useAudio";

describe("useAudio hook", () => {
  it("plays wake chime without error", () => {
    const { result } = renderHook(() => useAudio());
    expect(() => result.current.playWakeChime()).not.toThrow();
  });

  it("plays success chime without error", () => {
    const { result } = renderHook(() => useAudio());
    expect(() => result.current.playSuccessChime()).not.toThrow();
  });

  it("plays timeout chime without error", () => {
    const { result } = renderHook(() => useAudio());
    expect(() => result.current.playTimeoutChime()).not.toThrow();
  });
});
