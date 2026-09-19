import { NextRequest, NextResponse } from "next/server";
import { getSettings } from "@/lib/config";
import { verifyDashboardApiKey } from "@/lib/auth";

export async function POST(req: NextRequest) {
  const settings = getSettings();
  const apiKey = req.headers.get("x-api-key") || undefined;

  if (!verifyDashboardApiKey(apiKey, settings.dashboardApiKey)) {
    return NextResponse.json(
      { detail: "invalid_dashboard_api_key" },
      { status: 401 }
    );
  }

  return NextResponse.json({ status: "ok", authenticated: true });
}
