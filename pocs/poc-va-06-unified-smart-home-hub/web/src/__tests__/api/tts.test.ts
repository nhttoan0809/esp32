import { describe, it, expect, vi, beforeEach } from "vitest";
import { POST, GET } from "@/app/api/tts/route";
import * as ttsService from "@/lib/tts/tts-service";

describe("TTS API Route (/api/tts)", () => {
  beforeEach(() => {
    vi.clearAllMocks();
  });

  describe("POST /api/tts", () => {
    it("returns 400 if text is missing or empty", async () => {
      const req = new Request("http://localhost:8000/api/tts", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ text: "   " }),
      });

      const res = await POST(req);
      expect(res.status).toBe(400);
      const json = await res.json();
      expect(json.error).toContain("text");
    });

    it("returns 400 if body is invalid JSON", async () => {
      const req = new Request("http://localhost:8000/api/tts", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: "invalid-json",
      });

      const res = await POST(req);
      expect(res.status).toBe(400);
    });

    it("synthesizes audio and returns 200 audio/mpeg", async () => {
      const mockAudio = Buffer.from("mock-mp3-audio-data");
      const synthSpy = vi
        .spyOn(ttsService, "synthesizeSpeech")
        .mockResolvedValue(mockAudio);

      const req = new Request("http://localhost:8000/api/tts", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          text: "Đã bật đèn",
          voice: "vi-VN-HoaiMyNeural",
          rate: "+5%",
          pitch: "+0Hz",
        }),
      });

      const res = await POST(req);
      expect(res.status).toBe(200);
      expect(res.headers.get("Content-Type")).toBe("audio/mpeg");
      expect(res.headers.get("Content-Length")).toBe(mockAudio.length.toString());

      expect(synthSpy).toHaveBeenCalledWith({
        text: "Đã bật đèn",
        voice: "vi-VN-HoaiMyNeural",
        rate: "+5%",
        pitch: "+0Hz",
      });

      synthSpy.mockRestore();
    });

    it("returns 502 if upstream synthesis fails", async () => {
      const synthSpy = vi
        .spyOn(ttsService, "synthesizeSpeech")
        .mockRejectedValue(new Error("Microsoft TTS Connection Refused"));

      const req = new Request("http://localhost:8000/api/tts", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ text: "Hello" }),
      });

      const res = await POST(req);
      expect(res.status).toBe(502);
      const json = await res.json();
      expect(json.error).toBe("Microsoft TTS Connection Refused");

      synthSpy.mockRestore();
    });
  });

  describe("GET /api/tts", () => {
    it("returns 400 if text query param is missing", async () => {
      const req = new Request("http://localhost:8000/api/tts");
      const res = await GET(req);
      expect(res.status).toBe(400);
    });

    it("synthesizes audio via query params", async () => {
      const mockAudio = Buffer.from("mock-mp3-bytes");
      const synthSpy = vi
        .spyOn(ttsService, "synthesizeSpeech")
        .mockResolvedValue(mockAudio);

      const req = new Request(
        "http://localhost:8000/api/tts?text=Xin%20ch%C3%A0o&voice=vi-VN-HoaiMyNeural"
      );
      const res = await GET(req);
      expect(res.status).toBe(200);
      expect(res.headers.get("Content-Type")).toBe("audio/mpeg");

      expect(synthSpy).toHaveBeenCalledWith({
        text: "Xin chào",
        voice: "vi-VN-HoaiMyNeural",
        rate: undefined,
        pitch: undefined,
      });

      synthSpy.mockRestore();
    });
  });

  describe("tts-service logic", () => {
    it("detectDefaultVoice correctly identifies Vietnamese vs English", () => {
      expect(ttsService.detectDefaultVoice("Đèn phòng khách đã bật")).toBe(
        "vi-VN-HoaiMyNeural"
      );
      expect(ttsService.detectDefaultVoice("Xin chao", "vi-VN")).toBe(
        "vi-VN-HoaiMyNeural"
      );
      expect(ttsService.detectDefaultVoice("Living room light turned on")).toBe(
        "en-US-AriaNeural"
      );
    });

    it("supported voices list contains Hoai My and Nam Minh", () => {
      const ids = ttsService.SUPPORTED_TTS_VOICES.map((v) => v.id);
      expect(ids).toContain("vi-VN-HoaiMyNeural");
      expect(ids).toContain("vi-VN-NamMinhNeural");
      expect(ids).toContain("en-US-AriaNeural");
      expect(ids).toContain("en-US-GuyNeural");
    });
  });
});
