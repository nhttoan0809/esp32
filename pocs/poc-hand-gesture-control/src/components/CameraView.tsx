import React from "react";

type CameraViewProps = {
  videoRef: React.RefObject<HTMLVideoElement | null>;
  isActive: boolean;
};

/**
 * Renders the webcam video feed.
 * The video is mirrored (scaleX -1) so it feels natural like a mirror.
 */
export function CameraView({ videoRef, isActive }: CameraViewProps) {
  return (
    <video
      ref={videoRef}
      autoPlay
      playsInline
      muted
      className={`w-full h-full object-cover ${isActive ? "opacity-100" : "opacity-0"}`}
      style={{ transform: "scaleX(-1)" }}
    />
  );
}
