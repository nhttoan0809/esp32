"use client";

import { useRef, useCallback } from "react";

export function useAudio() {
  const audioCtxRef = useRef<AudioContext | null>(null);

  const getAudioContext = useCallback((): AudioContext | null => {
    if (typeof window === "undefined") return null;

    if (!audioCtxRef.current) {
      const AudioCtx =
        window.AudioContext ||
        (window as unknown as { webkitAudioContext: typeof AudioContext }).webkitAudioContext;
      if (AudioCtx) {
        audioCtxRef.current = new AudioCtx();
      }
    }

    if (audioCtxRef.current && audioCtxRef.current.state === "suspended") {
      audioCtxRef.current.resume().catch(() => {});
    }

    return audioCtxRef.current;
  }, []);

  const playTone = useCallback(
    (
      freq: number,
      type: OscillatorType,
      duration: number,
      startTime: number,
      gainLevel = 0.15
    ) => {
      const ctx = getAudioContext();
      if (!ctx) return;

      try {
        const osc = ctx.createOscillator();
        const gain = ctx.createGain();

        osc.type = type;
        osc.frequency.setValueAtTime(freq, startTime);

        gain.gain.setValueAtTime(0, startTime);
        gain.gain.linearRampToValueAtTime(gainLevel, startTime + 0.02);
        gain.gain.exponentialRampToValueAtTime(0.0001, startTime + duration);

        osc.connect(gain);
        gain.connect(ctx.destination);

        osc.start(startTime);
        osc.stop(startTime + duration);
      } catch (err) {
        console.warn("Error playing tone:", err);
      }
    },
    [getAudioContext]
  );

  const playWakeChime = useCallback(() => {
    const ctx = getAudioContext();
    if (!ctx) return;
    const now = ctx.currentTime;
    playTone(523.25, "sine", 0.12, now, 0.15); // C5
    playTone(659.25, "sine", 0.12, now + 0.08, 0.15); // E5
    playTone(783.99, "sine", 0.25, now + 0.16, 0.18); // G5
  }, [getAudioContext, playTone]);

  const playSuccessChime = useCallback(() => {
    const ctx = getAudioContext();
    if (!ctx) return;
    const now = ctx.currentTime;
    playTone(783.99, "sine", 0.15, now, 0.15); // G5
    playTone(1046.5, "sine", 0.3, now + 0.1, 0.18); // C6
  }, [getAudioContext, playTone]);

  const playTimeoutChime = useCallback(() => {
    const ctx = getAudioContext();
    if (!ctx) return;
    const now = ctx.currentTime;
    playTone(440.0, "sine", 0.18, now, 0.12); // A4
    playTone(349.23, "sine", 0.25, now + 0.12, 0.12); // F4
  }, [getAudioContext, playTone]);

  const resumeAudio = useCallback(async () => {
    const ctx = getAudioContext();
    if (ctx && ctx.state === "suspended") {
      try {
        await ctx.resume();
      } catch (err) {
        console.warn("Could not resume AudioContext:", err);
      }
    }
  }, [getAudioContext]);

  return {
    resumeAudio,
    playWakeChime,
    playSuccessChime,
    playTimeoutChime,
  };
}
