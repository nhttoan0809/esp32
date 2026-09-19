import { tool } from "ai";
import { z } from "zod";
import { getGlobalRegistry } from "@/lib/registry";
import { getSettings } from "@/lib/config";

/**
 * AI Agent Tools for Smart Home Device Control
 *
 * These tools give the LLM agent the ability to:
 * 1. Query all registered IoT devices and their states
 * 2. Set a specific device to ON or OFF
 * 3. Toggle a device's current state
 *
 * All tools interact with the in-memory DeviceRegistry which manages
 * WebSocket connections to ESP32 devices.
 */

/** Tool 1: Get all devices and their current states */
export const getDevicesTool = tool({
  description:
    "Get list of all registered IoT devices and their current states (online/offline, on/off, sensors). Always call this first before controlling any device.",
  inputSchema: z.object({}),
  execute: async () => {
    const settings = getSettings();
    const registry = getGlobalRegistry(settings.commandTimeoutSeconds);
    const snapshots = await registry.allSnapshots();
    return Object.values(snapshots).map((s) => ({
      device_id: s.deviceId,
      online: s.online,
      on: s.on,
      last_seen: s.lastSeen,
      sensors: s.sensors || {},
    }));
  },
});

/** Tool 4: Get sensor data (temperature, humidity, light, motion, etc.) */
export const getSensorDataTool = tool({
  description:
    "Query real-time environmental sensors from IoT devices (temperature, humidity, light level, motion). Use when user asks about temperature, humidity, climate, how hot/cold the room is, or sensor readings.",
  inputSchema: z.object({
    device_id: z
      .string()
      .optional()
      .describe("Optional specific device ID, e.g. 'esp32-climate-monitor'"),
  }),
  execute: async ({ device_id }) => {
    const settings = getSettings();
    const registry = getGlobalRegistry(settings.commandTimeoutSeconds);
    const snapshots = await registry.allSnapshots();
    const all = Object.values(snapshots);

    if (device_id) {
      const dev = all.find((d) => d.deviceId === device_id);
      if (!dev) {
        return { error: `Device '${device_id}' not found` };
      }
      return {
        device_id: dev.deviceId,
        online: dev.online,
        last_seen: dev.lastSeen,
        sensors: dev.sensors || {},
      };
    }

    return {
      devices: all.map((d) => ({
        device_id: d.deviceId,
        online: d.online,
        last_seen: d.lastSeen,
        sensors: d.sensors || {},
      })),
    };
  },
});

/** Tool 2: Set device state (on/off) */
export const setDeviceStateTool = tool({
  description:
    "Turn a specific IoT device ON or OFF. Use device_id obtained from getDevices tool. Set on=true to turn ON, on=false to turn OFF.",
  inputSchema: z.object({
    device_id: z
      .string()
      .describe("The unique identifier of the device, e.g. 'esp32-smart-lamp'"),
    on: z.boolean().describe("true to turn ON, false to turn OFF"),
  }),
  execute: async ({ device_id, on }) => {
    const settings = getSettings();
    const registry = getGlobalRegistry(settings.commandTimeoutSeconds);
    try {
      const { commandId, confirmedOn } = await registry.sendStateCommand(
        device_id,
        on
      );
      return {
        success: true,
        device_id,
        on: confirmedOn,
        synced: true,
        command_id: commandId,
      };
    } catch {
      // Device offline → queue the desired state
      await registry.queueState(device_id, on);
      return {
        success: true,
        device_id,
        on,
        synced: false,
        warning: "device_offline_queued",
      };
    }
  },
});

/** Tool 3: Toggle device state (flip current state) */
export const toggleDeviceTool = tool({
  description:
    "Toggle the state of a device: if currently ON → turn OFF, if currently OFF → turn ON. Use when user says 'toggle', 'switch', 'change status', or 'đổi trạng thái'.",
  inputSchema: z.object({
    device_id: z
      .string()
      .describe("The unique identifier of the device to toggle"),
  }),
  execute: async ({ device_id }) => {
    const settings = getSettings();
    const registry = getGlobalRegistry(settings.commandTimeoutSeconds);
    const snapshot = await registry.snapshot(device_id);
    const newState = !snapshot.on;

    try {
      const { commandId, confirmedOn } = await registry.sendStateCommand(
        device_id,
        newState
      );
      return {
        success: true,
        device_id,
        previous_state: snapshot.on,
        on: confirmedOn,
        synced: true,
        command_id: commandId,
      };
    } catch {
      await registry.queueState(device_id, newState);
      return {
        success: true,
        device_id,
        previous_state: snapshot.on,
        on: newState,
        synced: false,
        warning: "device_offline_queued",
      };
    }
  },
});

/** Tool 5: Set LED Dimmer brightness */
export const setLedBrightnessTool = tool({
  description:
    "Adjust the brightness of the dimmer LED lamp (0 to 100%). Use when user says 'đặt độ sáng', 'tăng/giảm độ sáng', 'dim the light', or specifies a percentage like '50%', '80%'.",
  inputSchema: z.object({
    device_id: z
      .string()
      .describe("The unique identifier of the device, e.g. 'esp32-smart-hub'"),
    brightness: z
      .number()
      .min(0)
      .max(100)
      .describe("Brightness level as a percentage from 0 to 100"),
  }),
  execute: async ({ device_id, brightness }) => {
    const settings = getSettings();
    const registry = getGlobalRegistry(settings.commandTimeoutSeconds);
    try {
      const { commandId, confirmedBrightness } =
        await registry.sendBrightnessCommand(device_id, brightness);
      return {
        success: true,
        device_id,
        brightness: confirmedBrightness,
        synced: true,
        command_id: commandId,
      };
    } catch {
      return {
        success: false,
        device_id,
        brightness,
        synced: false,
        warning: "device_offline",
      };
    }
  },
});

/** Tool 6: Set Security Mode */
export const setSecurityModeTool = tool({
  description:
    "Set smart hub security mode: 'guard' (chế độ bảo vệ/an ninh, kích hoạt chuông còi khi có chuyển động) or 'eco' (chế độ tiết kiệm năng lượng / thông thường). Use when user asks to arm security, bật chế độ bảo vệ, or chuyển sang eco mode.",
  inputSchema: z.object({
    device_id: z
      .string()
      .describe("The unique identifier of the device, e.g. 'esp32-smart-hub'"),
    mode: z
      .enum(["guard", "eco"])
      .describe("'guard' for intrusion security alarm, 'eco' for normal / power-saving mode"),
  }),
  execute: async ({ device_id, mode }) => {
    const settings = getSettings();
    const registry = getGlobalRegistry(settings.commandTimeoutSeconds);
    try {
      const { commandId, mode: confirmedMode } =
        await registry.sendSecurityModeCommand(device_id, mode);
      return {
        success: true,
        device_id,
        mode: confirmedMode,
        synced: true,
        command_id: commandId,
      };
    } catch {
      return {
        success: false,
        device_id,
        mode,
        synced: false,
        warning: "device_offline",
      };
    }
  },
});

