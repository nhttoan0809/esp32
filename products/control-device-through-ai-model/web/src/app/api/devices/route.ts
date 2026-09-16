import { NextRequest, NextResponse } from "next/server";
import { getSettings } from "@/lib/config";
import { verifyDashboardApiKey } from "@/lib/auth";
import { getGlobalRegistry } from "@/lib/registry";
import type { DeviceStateResponse } from "@/lib/types";

export async function GET(req: NextRequest) {
  const settings = getSettings();
  const apiKey = req.headers.get("x-api-key") || undefined;

  if (!verifyDashboardApiKey(apiKey, settings.dashboardApiKey)) {
    return NextResponse.json(
      { detail: "invalid_dashboard_api_key" },
      { status: 401 }
    );
  }

  const registry = getGlobalRegistry(settings.commandTimeoutSeconds);
  const configuredIds = Object.keys(settings.deviceTokens);
  const registeredIds = await registry.allDeviceIds();
  const allIds = Array.from(new Set([...configuredIds, ...registeredIds])).sort();

  const results: DeviceStateResponse[] = [];
  for (const id of allIds) {
    const snap = await registry.snapshot(id);
    results.push({
      device_id: snap.deviceId,
      online: snap.online,
      on: snap.on,
      last_seen: snap.lastSeen,
      pending_on: snap.pendingOn,
    });
  }

  return NextResponse.json(results);
}
