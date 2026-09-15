import { useRef, useState, useEffect, useCallback } from "react";
import { HandLandmarker, FilesetResolver } from "@mediapipe/tasks-vision";
import type { HandTrackingState } from "../gesture/types";
import { recognizeGesture } from "../gesture/recognizeGesture";
import {
  createGestureSmoother,
  type GestureSmoother,
} from "../gesture/gestureSmoother";

const MEDIAPIPE_WASM_CDN =
  "https://cdn.jsdelivr.net/npm/@mediapipe/tasks-vision@latest/wasm";

type UseHandTrackingOptions = {
  videoRef: React.RefObject<HTMLVideoElement | null>;
  isActive: boolean;
};

/**
 * Hook that initializes MediaPipe Hand Landmarker and processes video frames
 * to produce hand landmarks and gesture recognition.
 */
export function useHandTracking({
  videoRef,
  isActive,
}: UseHandTrackingOptions): HandTrackingState {
  const landmarkerRef = useRef<HandLandmarker | null>(null);
  const smootherRef = useRef<GestureSmoother | null>(null);
  const rafRef = useRef<number>(0);
  const lastTimeRef = useRef<number>(0);
  const fpsFrameCount = useRef(0);
  const fpsLastTime = useRef(0);

  const [state, setState] = useState<HandTrackingState>({
    landmarks: null,
    gesture: "UNKNOWN",
    isHandDetected: false,
    fps: 0,
  });

  // Initialize MediaPipe
  useEffect(() => {
    let cancelled = false;

    async function init() {
      try {
        const vision = await FilesetResolver.forVisionTasks(MEDIAPIPE_WASM_CDN);
        if (cancelled) return;

        const landmarker = await HandLandmarker.createFromOptions(vision, {
          baseOptions: {
            modelAssetPath:
              "https://storage.googleapis.com/mediapipe-models/hand_landmarker/hand_landmarker/float16/1/hand_landmarker.task",
            delegate: "GPU",
          },
          runningMode: "VIDEO",
          numHands: 1,
          minHandDetectionConfidence: 0.5,
          minHandPresenceConfidence: 0.5,
          minTrackingConfidence: 0.5,
        });

        if (cancelled) {
          landmarker.close();
          return;
        }

        landmarkerRef.current = landmarker;
        smootherRef.current = createGestureSmoother();
      } catch (err) {
        console.error("Failed to initialize MediaPipe Hand Landmarker:", err);
      }
    }

    init();

    return () => {
      cancelled = true;
      if (landmarkerRef.current) {
        landmarkerRef.current.close();
        landmarkerRef.current = null;
      }
    };
  }, []);

  // Process video frames
  const processFrame = useCallback(() => {
    const video = videoRef.current;
    const landmarker = landmarkerRef.current;
    const smoother = smootherRef.current;

    if (!video || !landmarker || !smoother || video.readyState < 2) {
      rafRef.current = requestAnimationFrame(processFrame);
      return;
    }

    const now = performance.now();

    // Avoid processing same frame twice
    if (now === lastTimeRef.current) {
      rafRef.current = requestAnimationFrame(processFrame);
      return;
    }
    lastTimeRef.current = now;

    // FPS calculation
    fpsFrameCount.current++;
    if (now - fpsLastTime.current >= 1000) {
      const fps = fpsFrameCount.current;
      fpsFrameCount.current = 0;
      fpsLastTime.current = now;

      setState((prev) => ({ ...prev, fps }));
    }

    try {
      const result = landmarker.detectForVideo(video, now);

      if (result.landmarks && result.landmarks.length > 0) {
        const landmarks = result.landmarks[0];
        const { gesture } = recognizeGesture(landmarks);
        const stableGesture = smoother.update(gesture);

        setState((prev) => ({
          ...prev,
          landmarks,
          gesture: stableGesture,
          isHandDetected: true,
        }));
      } else {
        smoother.reset();
        setState((prev) => ({
          ...prev,
          landmarks: null,
          gesture: "UNKNOWN",
          isHandDetected: false,
        }));
      }
    } catch {
      // Silently handle frame processing errors
    }

    rafRef.current = requestAnimationFrame(processFrame);
  }, [videoRef]);

  // Start/stop frame processing based on camera state
  useEffect(() => {
    if (isActive) {
      fpsFrameCount.current = 0;
      fpsLastTime.current = performance.now();
      rafRef.current = requestAnimationFrame(processFrame);
    } else {
      cancelAnimationFrame(rafRef.current);
      smootherRef.current?.reset();
      setState({
        landmarks: null,
        gesture: "UNKNOWN",
        isHandDetected: false,
        fps: 0,
      });
    }

    return () => {
      cancelAnimationFrame(rafRef.current);
    };
  }, [isActive, processFrame]);

  return state;
}
