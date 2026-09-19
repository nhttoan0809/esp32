import { NextRequest, NextResponse } from "next/server";
import { getSettings } from "@/lib/config";
import { verifyDashboardApiKey } from "@/lib/auth";
import { getGlobalRegistry, DeviceOfflineError } from "@/lib/registry";
import { SetSecurityModeRequestSchema } from "@/lib/types";

export async function PUT(
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

  let body: unknown;
  try {
    body = await req.json();
  } catch {
    return NextResponse.json({ detail: "invalid_json" }, { status: 400 });
  }

  const parsed = SetSecurityModeRequestSchema.safeParse(body);
  if (!parsed.success) {
    return NextResponse.json(
      { detail: "validation_error", errors: parsed.error.issues },
      { status: 422 }
    );
  }

  const registry = getGlobalRegistry(settings.commandTimeoutSeconds);

  try {
    const { commandId, mode: confirmedMode } =
      await registry.sendSecurityModeCommand(deviceId, parsed.data.mode);

    return NextResponse.json({
      device_id: deviceId,
      mode: confirmedMode,
      synced: true,
      command_id: commandId,
    });
  } catch (error) {
    if (error instanceof DeviceOfflineError) {
      return NextResponse.json(
        { detail: "device_offline" },
        { status: 503 }
      );
    }
    return NextResponse.json(
      { detail: "internal_server_error" },
      { status: 500 }
    );
  }
}
