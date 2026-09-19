import { describe, it, expect, beforeEach } from "vitest";
import { getGlobalRegistry } from "@/lib/registry";
import {
  getDevicesTool,
  setDeviceStateTool,
  toggleDeviceTool,
  getSensorDataTool,
  setLedBrightnessTool,
  setSecurityModeTool,
} from "@/lib/ai/device-tools";


interface DeviceSummary {
  device_id: string;
  online: boolean;
  on: boolean | null;
  last_seen: string | null;
}

interface ToolActionResult {
  success: boolean;
  device_id: string;
  on?: boolean;
  synced: boolean;
  warning?: string;
}

type ExecuteFn<TArgs, TResult> = (args: TArgs, options?: unknown) => Promise<TResult>;

describe("Device Tools for AI Agent", () => {
  const registry = getGlobalRegistry(5);

  const executeGetDevices = getDevicesTool.execute as unknown as ExecuteFn<
    Record<string, never>,
    DeviceSummary[]
  >;
  const executeSetState = setDeviceStateTool.execute as unknown as ExecuteFn<
    { device_id: string; on: boolean },
    ToolActionResult
  >;
  const executeToggle = toggleDeviceTool.execute as unknown as ExecuteFn<
    { device_id: string },
    ToolActionResult
  >;
  const executeGetSensorData = getSensorDataTool.execute as unknown as ExecuteFn<
    { device_id?: string },
    { devices?: Array<{ device_id: string; sensors: Record<string, unknown> }>; error?: string }
  >;

  beforeEach(async () => {
    // Register test devices with initial state
    await registry.queueState("esp32-test-lamp", false);
  });

  it("getDevicesTool returns array of snapshots", async () => {
    const result = await executeGetDevices({});
    expect(Array.isArray(result)).toBe(true);
    const found = result.find((d) => d.device_id === "esp32-test-lamp");
    expect(found).toBeDefined();
    expect(found?.online).toBe(false);
  });

  it("setDeviceStateTool queues state when device is offline", async () => {
    const result = await executeSetState({
      device_id: "esp32-test-lamp",
      on: true,
    });
    expect(result.success).toBe(true);
    expect(result.device_id).toBe("esp32-test-lamp");
    expect(result.on).toBe(true);
    expect(result.synced).toBe(false);
    expect(result.warning).toBe("device_offline_queued");
  });

  it("toggleDeviceTool toggles state when device is offline", async () => {
    const result = await executeToggle({
      device_id: "esp32-test-lamp",
    });
    expect(result.success).toBe(true);
    expect(result.device_id).toBe("esp32-test-lamp");
    expect(result.synced).toBe(false);
  });

  it("getSensorDataTool queries devices and returns sensor structure", async () => {
    const result = await executeGetSensorData({});
    expect(result.devices).toBeDefined();
    expect(Array.isArray(result.devices)).toBe(true);
  });

  it("setLedBrightnessTool handles offline device gracefully", async () => {
    const executeBrightness = setLedBrightnessTool.execute as unknown as ExecuteFn<
      { device_id: string; brightness: number },
      { success: boolean; device_id: string; brightness: number; warning?: string }
    >;
    const result = await executeBrightness({
      device_id: "esp32-test-lamp",
      brightness: 75,
    });
    expect(result.success).toBe(false);
    expect(result.warning).toBe("device_offline");
  });

  it("setSecurityModeTool handles offline device gracefully", async () => {
    const executeSecurity = setSecurityModeTool.execute as unknown as ExecuteFn<
      { device_id: string; mode: "guard" | "eco" },
      { success: boolean; device_id: string; mode: string; warning?: string }
    >;
    const result = await executeSecurity({
      device_id: "esp32-test-lamp",
      mode: "guard",
    });
    expect(result.success).toBe(false);
    expect(result.warning).toBe("device_offline");
  });
});

