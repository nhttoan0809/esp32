import { describe, it, expect, beforeEach } from "vitest";
import { getGlobalRegistry } from "@/lib/registry";
import { getDevicesTool, setDeviceStateTool, toggleDeviceTool } from "@/lib/ai/device-tools";

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
});
