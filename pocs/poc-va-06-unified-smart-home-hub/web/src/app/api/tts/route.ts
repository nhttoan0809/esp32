import { NextResponse } from "next/server";
import { synthesizeSpeech } from "@/lib/tts/tts-service";

export const runtime = "nodejs";
export const dynamic = "force-dynamic";

export async function POST(req: Request) {
  try {
    const body = await req.json().catch(() => ({}));
    const { text, voice, rate, pitch } = body;

    if (!text || typeof text !== "string" || !text.trim()) {
      return NextResponse.json(
        { error: "Field 'text' is required and must not be empty" },
        { status: 400 }
      );
    }

    const audioBuffer = await synthesizeSpeech({
      text: text.trim(),
      voice,
      rate,
      pitch,
    });

    return new NextResponse(new Uint8Array(audioBuffer), {
      status: 200,
      headers: {
        "Content-Type": "audio/mpeg",
        "Content-Length": audioBuffer.length.toString(),
        "Cache-Control": "public, max-age=86400, stale-while-revalidate=3600",
      },
    });
  } catch (error) {
    const message = error instanceof Error ? error.message : "TTS synthesis failed";
    console.error("[TTS API Error]:", message);
    return NextResponse.json({ error: message }, { status: 502 });
  }
}

export async function GET(req: Request) {
  try {
    const { searchParams } = new URL(req.url);
    const text = searchParams.get("text");
    const voice = searchParams.get("voice") || undefined;
    const rate = searchParams.get("rate") || undefined;
    const pitch = searchParams.get("pitch") || undefined;

    if (!text || !text.trim()) {
      return NextResponse.json(
        { error: "Query parameter 'text' is required" },
        { status: 400 }
      );
    }

    const audioBuffer = await synthesizeSpeech({
      text: text.trim(),
      voice,
      rate,
      pitch,
    });

    return new NextResponse(new Uint8Array(audioBuffer), {
      status: 200,
      headers: {
        "Content-Type": "audio/mpeg",
        "Content-Length": audioBuffer.length.toString(),
        "Cache-Control": "public, max-age=86400, stale-while-revalidate=3600",
      },
    });
  } catch (error) {
    const message = error instanceof Error ? error.message : "TTS synthesis failed";
    console.error("[TTS API Error]:", message);
    return NextResponse.json({ error: message }, { status: 502 });
  }
}
