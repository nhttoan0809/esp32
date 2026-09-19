import {
  streamText,
  convertToModelMessages,
  createUIMessageStreamResponse,
  toUIMessageStream,
  stepCountIs,
  type UIMessage,
} from "ai";
import {
  MODEL_PROVIDERS,
  DEFAULT_MODEL,
  type ModelKey,
} from "@/lib/ai/ai-config";
import { SYSTEM_PROMPT } from "@/lib/ai/smart-home-agent";
import {
  getDevicesTool,
  setDeviceStateTool,
  toggleDeviceTool,
  getSensorDataTool,
} from "@/lib/ai/device-tools";

export async function POST(req: Request) {
  const { messages, modelKey }: { messages: UIMessage[]; modelKey?: string } =
    await req.json();

  // Dynamic model selection from frontend dropdown
  const selectedKey =
    modelKey && modelKey in MODEL_PROVIDERS
      ? (modelKey as ModelKey)
      : DEFAULT_MODEL;
  const { provider, modelId } = MODEL_PROVIDERS[selectedKey];

  const result = streamText({
    model: provider.chat(modelId),
    system: SYSTEM_PROMPT,
    messages: await convertToModelMessages(messages),
    tools: {
      getDevices: getDevicesTool,
      setDeviceState: setDeviceStateTool,
      toggleDevice: toggleDeviceTool,
      getSensorData: getSensorDataTool,
    },
    stopWhen: stepCountIs(5),
    maxOutputTokens: 1024,
  });

  return createUIMessageStreamResponse({
    stream: toUIMessageStream({ stream: result.stream }),
  });
}
