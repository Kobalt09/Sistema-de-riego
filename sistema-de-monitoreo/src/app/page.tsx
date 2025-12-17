"use client";

import { useState, useEffect, useRef } from "react";
import VHSOverlay from "../components/VHSOverlay";

// Pin Configuration
const PIN_HUMEDAD = "V0";
const PIN_MIN_HUM = "V1";
const PIN_INTERVALO = "V2";

export default function Home() {
  // State
  const [token, setToken] = useState("MkGbBwmpKaV2DYEUepVLTzdiXVmdzaYc");
  const [humidity, setHumidity] = useState<string>("--");
  const [minHum, setMinHum] = useState<string>("");
  const [paramInterval, setParamInterval] = useState<string>("");
  const [isConnected, setIsConnected] = useState(false);
  const [statusMsg, setStatusMsg] = useState("Desconectado");
  const [logs, setLogs] = useState<string[]>([]);
  const [isPolling, setIsPolling] = useState(false);

  // References
  const pollingRef = useRef<NodeJS.Timeout | null>(null);
  const logEndRef = useRef<HTMLDivElement>(null);

  // Helpers
  const addLog = (text: string) => {
    const time = new Date().toLocaleTimeString();
    setLogs((prev) => [...prev, `[${time}] ${text}`]);
  };

  useEffect(() => {
    // Auto-scroll logs
    logEndRef.current?.scrollIntoView({ behavior: "smooth" });
  }, [logs]);

  useEffect(() => {
    // Cleanup polling on unmount
    return () => {
      if (pollingRef.current) clearInterval(pollingRef.current);
    };
  }, []);

  const fetchData = async () => {
    if (!token) return;
    const url = `https://blynk.cloud/external/api/get?token=${token}&${PIN_HUMEDAD}&${PIN_MIN_HUM}&${PIN_INTERVALO}`;

    try {
      const response = await fetch(url);
      if (!response.ok) throw new Error("HTTP " + response.status);

      const data = await response.json();

      setIsConnected(true);
      setStatusMsg("Conectado - Recibiendo datos");
      setHumidity(data[PIN_HUMEDAD] || "--");

      // Update placeholders (visual feedback) or values if empty
      // In this React version, we'll keep the input values independent unless we want to force sync
      // For now, let's just log the sync

    } catch (e: any) {
      setIsConnected(false);
      setStatusMsg("Error de Conexión");
      addLog("Error: " + e.message);
    }
  };

  const handleConnect = () => {
    if (!token) {
      addLog("Error: Token vacío");
      return;
    }

    if (pollingRef.current) clearInterval(pollingRef.current);

    addLog("Iniciando conexión a Blynk Cloud...");
    setIsPolling(true);
    fetchData();
    pollingRef.current = setInterval(fetchData, 5000);
  };

  const handleUpdateParams = async () => {
    if (!token) {
      addLog("Error: Falta Token");
      return;
    }

    let params = [];
    if (minHum) params.push(`${PIN_MIN_HUM}=${minHum}`);
    if (paramInterval) params.push(`${PIN_INTERVALO}=${paramInterval}`);

    if (params.length === 0) {
      addLog("Nada que enviar");
      return;
    }

    const url = `https://blynk.cloud/external/api/batch/update?token=${token}&${params.join("&")}`;
    addLog("Enviando comando...");

    try {
      const response = await fetch(url);
      if (response.ok) {
        addLog("Comando enviado OK");
        // Immediate refresh
        setTimeout(fetchData, 1000);
      } else {
        addLog("Error enviando: " + response.status);
      }
    } catch (e: any) {
      addLog("Error red: " + e.message);
    }
  };

  return (
    <div className="crt-monitor flex min-h-screen lg:h-screen flex-col p-4 md:p-8 text-green-500 font-mono selection:bg-green-900 selection:text-green-100 lg:overflow-hidden">
      <VHSOverlay />
      <div className="crt-overlay pointer-events-none"></div>
      <div className="scanlines pointer-events-none"></div>
      <div className="max-w-4xl mx-auto w-full h-full flex flex-col space-y-4 relative z-10">

        {/* Header */}
        <header className="border-b-2 border-green-800 pb-4 mb-4 flex justify-between items-end">
          <div>
            <h1 className="text-xl md:text-3xl font-bold uppercase tracking-widest text-shadow-green animate-text-flicker">
              SISTEMA MONITOREO <span className="animate-pulse">_</span>
            </h1>
            <p className="text-xs md:text-sm opacity-70 mt-1">
              v2.0 // TERMINAL ACCESS // {new Date().toLocaleDateString()}
            </p>
          </div>
          <div className={`text-xs border px-2 py-1 animate-text-flicker ${isConnected ? 'border-green-500 text-green-400' : 'border-red-900 text-red-700'}`}>
            {isConnected ? 'ONLINE' : 'OFFLINE'}
          </div>
        </header>

        <main className="grid grid-cols-1 lg:grid-cols-2 gap-6 flex-1 min-h-0">

          {/* Column 1: Controls & Status */}
          <div className="flex flex-col h-full gap-2">

            {/* Connection Section */}
            <section className="border border-green-800 bg-green-900/5 p-2 rounded-sm flex flex-col justify-center min-h-0">
              <h2 className="text-sm font-bold mb-1 border-b border-green-800/50 pb-1 flex items-center gap-2">
                &gt; CONEXIÓN
              </h2>
              <div className="space-y-2">
                <div className="flex flex-col gap-1">
                  <label htmlFor="token" className="text-[10px] uppercase tracking-wider opacity-80">Auth Token</label>
                  <input
                    id="token"
                    type="password"
                    value={token}
                    onChange={(e) => setToken(e.target.value)}
                    className="bg-black border border-green-700 p-1 text-xs focus:border-green-400 focus:outline-none focus:ring-1 focus:ring-green-500/50 transition-all font-mono"
                    placeholder="Enter Blynk Token..."
                  />
                </div>
                <button
                  onClick={handleConnect}
                  disabled={isPolling}
                  className="w-full border border-green-600 bg-green-900/20 py-1 hover:bg-green-500 hover:text-black transition-colors font-bold uppercase text-xs disabled:opacity-50 disabled:cursor-not-allowed group"
                >
                  {isPolling ? 'CONECTADO' : '[ INICIAR CONEXI\u00D3N ]'}
                </button>
                <div className="text-center text-[10px] opacity-60">
                  STATUS: {statusMsg}
                </div>
              </div>
            </section>

            {/* Live Status Section */}
            <section className="border border-green-800 bg-green-900/5 p-2 rounded-sm flex flex-col justify-center min-h-0">
              <h2 className="text-sm font-bold mb-1 border-b border-green-800/50 pb-1">
                &gt; SENSORES EN TIEMPO REAL
              </h2>
              <div className="flex items-center justify-between">
                <div className="text-xs">HUMEDAD SENSOR A1</div>
                <div className="text-3xl font-bold tracking-tighter animate-pulse">
                  {humidity}<span className="text-sm text-green-700">%</span>
                </div>
              </div>
              <div className="w-full bg-green-900/30 h-1.5 mt-1 rounded-full overflow-hidden">
                <div
                  className="bg-green-500 h-full transition-all duration-1000 ease-out"
                  style={{ width: humidity !== "--" ? `${parseFloat(humidity)}%` : '0%' }}
                ></div>
              </div>
            </section>

            {/* Config Section */}
            <section className="border border-green-800 bg-green-900/5 p-2 rounded-sm flex flex-col justify-center min-h-0">
              <h2 className="text-sm font-bold mb-1 border-b border-green-800/50 pb-1">
                &gt; CONFIGURACIÓN REMOTA
              </h2>
              <div className="grid grid-cols-2 gap-2">
                <div className="space-y-1">
                  <label className="text-[10px] uppercase opacity-80">Min Humedad (%)</label>
                  <input
                    type="number"
                    value={minHum}
                    onChange={(e) => setMinHum(e.target.value)}
                    className="w-full bg-black border border-green-700 p-1 text-xs focus:border-green-400 focus:outline-none"
                    placeholder="Ej: 40"
                  />
                </div>
                <div className="space-y-1">
                  <label className="text-[10px] uppercase opacity-80">Intervalo (s)</label>
                  <input
                    type="number"
                    value={paramInterval}
                    onChange={(e) => setParamInterval(e.target.value)}
                    className="w-full bg-black border border-green-700 p-1 text-xs focus:border-green-400 focus:outline-none"
                    placeholder="Ej: 30"
                  />
                </div>
              </div>
              <button
                onClick={handleUpdateParams}
                className="w-full mt-2 border border-green-600 bg-green-900/20 py-1 hover:bg-green-500 hover:text-black transition-colors font-bold uppercase text-xs"
              >
                [ ACTUALIZAR PARÁMETROS ]
              </button>
            </section>

          </div>

          {/* Column 2: Logs */}
          <div className="flex flex-col h-full min-h-0">
            <section className="border border-green-800 bg-black flex-1 flex flex-col p-1 rounded-sm shadow-[0_0_10px_rgba(0,128,0,0.2)]">
              <h2 className="bg-green-900/20 p-2 text-sm font-bold border-b border-green-800/50 flex justify-between">
                <span>&gt; SYSTEM_LOG</span>
                <span className="animate-pulse">● REC</span>
              </h2>
              <div className="flex-1 overflow-y-auto p-4 font-mono text-xs md:text-sm space-y-1 scrollbar-thin scrollbar-thumb-green-700 scrollbar-track-black">
                {logs.length === 0 && (
                  <div className="opacity-50 italic">
                    &gt; Esperando inicio del sistema...<br />
                    &gt; Listo para conectar.
                  </div>
                )}
                {logs.map((log, i) => (
                  <div key={i} className="break-words">
                    <span className="opacity-50 mr-2">&gt;</span>{log}
                  </div>
                ))}
                <div ref={logEndRef} />
              </div>
            </section>

            <div className="mt-4 text-center">
              <p className="text-[10px] text-green-800">
                SECURE CONNECTION // ENCRYPTED // 128-BIT
              </p>
            </div>
          </div>

        </main>
      </div>
    </div>
  );
}
