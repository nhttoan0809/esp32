# Hand Gesture Control PoC

Điều khiển thiết bị IoT ảo bằng cử chỉ tay thông qua MediaPipe Hand Landmarker.

## Cử chỉ được hỗ trợ

| Cử chỉ | Hành động | Mô tả |
|---------|-----------|-------|
| 🖐 OPEN_PALM | TURN_ON | Bật LED |
| ✊ FIST | TURN_OFF | Tắt LED |
| 👍 THUMBS_UP | INCREMENT | Tăng counter |
| ☝️ POINT | SELECT | Chọn/bỏ chọn thiết bị |

## Kiến trúc

```
Webcam → MediaPipe (21 landmarks) → Gesture Recognition → Temporal Smoothing → Action Mapping → Action Controller → Virtual IoT Device
```

## Tech Stack

- React 19 + TypeScript
- Vite 8
- TailwindCSS v4
- MediaPipe Tasks Vision (CDN)

## Chạy

```bash
cd pocs/poc-hand-gesture-control
pnpm install
pnpm dev
```

Mở trình duyệt tại `http://localhost:5173`, nhấn **Bắt đầu Camera**, cấp quyền camera và đưa tay trước webcam.

## Cấu trúc thư mục

```
src/
├── components/      # UI components
│   ├── CameraView   # Webcam video feed
│   ├── HandCanvas   # Landmark overlay
│   ├── GestureStatus # Diagnostics panel
│   └── DevicePanel  # Virtual IoT device
├── hooks/           # React hooks
│   ├── useCamera    # Webcam lifecycle
│   └── useHandTracking  # MediaPipe + frame processing
├── gesture/         # Gesture recognition domain
│   ├── types        # Shared types
│   ├── landmarks    # Landmark constants + geometry
│   ├── gestures     # Finger-state detection
│   ├── recognizeGesture  # 4-gesture classifier
│   ├── actionMapper      # Gesture → Action mapping
│   ├── gestureSmoother   # Temporal smoothing
│   └── actionController  # Action debounce
├── App.tsx          # Main application
└── main.tsx         # Entry point
```

## Mở rộng ESP32

Kiến trúc hỗ trợ thay thế virtual device bằng `DeviceTransport` interface để gửi action qua WebSocket/MQTT đến ESP32:

```typescript
type DeviceTransport = {
  send: (action: DeviceAction) => Promise<void> | void;
  disconnect?: () => void;
};
```
