import { NextRequest, NextResponse } from "next/server";
import { getSettings } from "@/lib/config";
import { verifyDashboardApiKey } from "@/lib/auth";
import {
  getGlobalRegistry,
  DeviceOfflineError,
  DeviceDisconnectedError,
  DeviceAckTimeoutError,
  DeviceDidNotApplyError,
} from "@/lib/registry";
import { SetStateRequestSchema, type SetStateResponse } from "@/lib/types";

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

  const parsed = SetStateRequestSchema.safeParse(body);
  if (!parsed.success) {
    return NextResponse.json(
      { detail: "validation_error", errors: parsed.error.issues },
      { status: 422 }
    );
  }

  const registry = getGlobalRegistry(settings.commandTimeoutSeconds);

  try {
    const { commandId, confirmedOn } = await registry.sendStateCommand(
      deviceId,
      parsed.data.on
    );

    const response: SetStateResponse = {
      device_id: deviceId,
      on: confirmedOn,
      synced: true,
      command_id: commandId,
      warning: null,
    };
    return NextResponse.json(response);
  } catch (error) {
    const isErrorNamed = (name: string) =>
      error instanceof Error && (error.name === name || error.constructor.name === name);

    if (error instanceof DeviceOfflineError || isErrorNamed("DeviceOfflineError")) {
      await registry.queueState(deviceId, parsed.data.on);
      const response: SetStateResponse = {
        device_id: deviceId,
        on: parsed.data.on,
        synced: false,
        command_id: null,
        warning: "device_offline_queued",
      };
      return NextResponse.json(response);
    }

    if (error instanceof DeviceDisconnectedError || isErrorNamed("DeviceDisconnectedError")) {
      return NextResponse.json(
        { detail: "device_disconnected" },
        { status: 503 }
      );
    }

    if (error instanceof DeviceAckTimeoutError || isErrorNamed("DeviceAckTimeoutError")) {
      return NextResponse.json(
        { detail: "device_ack_timeout" },
        { status: 504 }
      );
    }

    if (error instanceof DeviceDidNotApplyError || isErrorNamed("DeviceDidNotApplyError")) {
      return NextResponse.json(
        { detail: "device_did_not_apply_state" },
        { status: 502 }
      );
    }

    return NextResponse.json(
      { detail: "internal_server_error" },
      { status: 500 }
    );
  }
}
