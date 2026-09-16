import type { Metadata } from "next";
import "./globals.css";

export const metadata: Metadata = {
  title: "Smart Lamp Voice Controller",
  description: "Cloud WebSocket server with Voice Wake-Up dashboard for ESP32 Smart Lamp",
};

export default function RootLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    <html lang="en" className="dark h-full antialiased">
      <body className="min-h-full bg-slate-950 text-slate-100 flex flex-col">
        {children}
      </body>
    </html>
  );
}
