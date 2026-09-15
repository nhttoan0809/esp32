import { useRef, useEffect } from "react";
import type { HandLandmark } from "../gesture/types";
import { HAND_CONNECTIONS } from "../gesture/landmarks";

type HandCanvasProps = {
  landmarks: HandLandmark[] | null;
  videoRef: React.RefObject<HTMLVideoElement | null>;
};

const POINT_COLOR = "#06b6d4";
const CONNECTION_COLOR = "rgba(6, 182, 212, 0.5)";
const POINT_RADIUS = 5;
const CONNECTION_WIDTH = 2;

/**
 * Draws hand landmarks and connections on a canvas overlaying the video.
 * Canvas is mirrored to match the mirrored video feed.
 */
export function HandCanvas({ landmarks, videoRef }: HandCanvasProps) {
  const canvasRef = useRef<HTMLCanvasElement | null>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    const video = videoRef.current;
    if (!canvas || !video) return;

    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    // Sync canvas dimensions with displayed video
    const rect = canvas.getBoundingClientRect();
    canvas.width = rect.width;
    canvas.height = rect.height;

    // Clear previous frame
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    if (!landmarks || landmarks.length === 0) return;

    const w = canvas.width;
    const h = canvas.height;

    // Draw connections
    ctx.strokeStyle = CONNECTION_COLOR;
    ctx.lineWidth = CONNECTION_WIDTH;
    for (const [startIdx, endIdx] of HAND_CONNECTIONS) {
      const start = landmarks[startIdx];
      const end = landmarks[endIdx];
      if (!start || !end) continue;

      // Mirror X to match scaleX(-1) on video
      ctx.beginPath();
      ctx.moveTo((1 - start.x) * w, start.y * h);
      ctx.lineTo((1 - end.x) * w, end.y * h);
      ctx.stroke();
    }

    // Draw landmark points
    ctx.fillStyle = POINT_COLOR;
    for (const landmark of landmarks) {
      const x = (1 - landmark.x) * w;
      const y = landmark.y * h;

      ctx.beginPath();
      ctx.arc(x, y, POINT_RADIUS, 0, Math.PI * 2);
      ctx.fill();
    }

    // Highlight fingertips with larger circles
    const fingertips = [4, 8, 12, 16, 20];
    ctx.fillStyle = "#22c55e";
    for (const idx of fingertips) {
      const tip = landmarks[idx];
      if (!tip) continue;
      ctx.beginPath();
      ctx.arc((1 - tip.x) * w, tip.y * h, POINT_RADIUS + 2, 0, Math.PI * 2);
      ctx.fill();
    }
  }, [landmarks, videoRef]);

  return (
    <canvas
      ref={canvasRef}
      className="absolute top-0 left-0 w-full h-full pointer-events-none"
    />
  );
}
