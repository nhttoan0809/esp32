import { NextRequest, NextResponse } from "next/server";
import { getSettings } from "@/lib/config";
import { verifyDashboardApiKey } from "@/lib/auth";
import { getGlobalRegistry } from "@/lib/registry";
import type { DeviceStateResponse } from "@/lib/types";

export async function GET(
  req: NextRequest,
  context: { params: Promise<{ deviceId: string }> }
) {
  const { deviceId } = await context.params;
  const settings = getSettings();
  const apiKey = req.headers.get("x-api-key") || undefined;

  if (!verifyDashboardApiKey(apiKey, settings.dashboardApiKey)) {
    return NextResponse.json(
      { detail: "invalid_dashboard_api_key" },
      { status: 401 }
    );
  }

  const registry = getGlobalRegistry(settings.commandTimeoutSeconds);
  const snap = await registry.snapshot(deviceId);

  const response: DeviceStateResponse = {
    device_id: snap.deviceId,
    online: snap.online,
    on: snap.on,
    last_seen: snap.lastSeen,
    pending_on: snap.pendingOn,
    sensors: snap.sensors || {},
  };

  return NextResponse.json(response);
}
