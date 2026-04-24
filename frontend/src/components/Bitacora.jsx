import React, { useState, useEffect } from 'react';

function Bitacora({ sesionActiva, irALogin }) {
  const [logs, setLogs] = useState([]);
  const [cargando, setCargando] = useState(false);
  const [error, setError] = useState('');

  const cargarJournaling = async () => {
    setCargando(true);
    setError('');
    try {
      const resp = await fetch('http://3.134.103.249:8081/api/journaling', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ id: sesionActiva.id })
      });
      const data = await resp.json();
      
      if (data.error) {
        setError(data.error);
      } else {
        setLogs(data.data || []);
      }
    } catch (err) {
      setError('Error al conectar con C++ para leer el journaling.');
    } finally {
      setCargando(false);
    }
  };

  useEffect(() => {
    if (sesionActiva) {
      cargarJournaling();
    }
  }, [sesionActiva]);

  if (!sesionActiva) {
    return (
      <div className="flex-grow flex flex-col items-center justify-center bg-[#0d1117] text-[#8b949e]">
        <h2 className="text-xl font-semibold text-[#c9d1d9]">Acceso Denegado</h2>
        <p className="mt-2">Debes iniciar sesión para ver los reportes del sistema.</p>
        <button onClick={irALogin} className="mt-4 bg-[#21262d] border border-[#30363d] hover:bg-[#30363d] text-white px-4 py-2 rounded">Ir al Login</button>
      </div>
    );
  }

  return (
    <div className="flex-grow flex flex-col bg-[#0d1117] p-6 overflow-auto">
      
      <div className="flex justify-between items-center mb-6 border-b border-[#30363d] pb-2">
        <h2 className="text-2xl font-semibold text-white flex items-center gap-2">
          <svg className="w-6 h-6 text-[#8957e5]" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M9 12h6m-6 4h6m2 5H7a2 2 0 01-2-2V5a2 2 0 012-2h5.586a1 1 0 01.707.293l5.414 5.414a1 1 0 01.293.707V19a2 2 0 01-2 2z"></path></svg>
          Reporte: Journaling (Bitácora EXT3)
        </h2>
        <div className="flex items-center gap-4">
          <span className="text-[#8b949e] font-mono text-sm bg-[#161b22] px-3 py-1 rounded border border-[#30363d]">
            Partición: {sesionActiva.id}
          </span>
          <button onClick={cargarJournaling} disabled={cargando} className="bg-[#21262d] border border-[#30363d] text-white px-3 py-1.5 rounded hover:bg-[#30363d] transition-colors flex items-center gap-2 text-sm">
            Actualizar Logs
          </button>
        </div>
      </div>

      {error ? (
        <div className="bg-[#da3633]/10 border border-[#da3633]/50 text-[#ff7b72] px-4 py-3 rounded text-sm text-center font-medium">
          {error}
        </div>
      ) : cargando ? (
        <div className="text-center text-[#8b949e] mt-10 animate-pulse">Leyendo bitácora en C++...</div>
      ) : logs.length === 0 ? (
        <div className="text-center text-[#8b949e] py-10 border border-dashed border-[#30363d] rounded-lg">
          No hay operaciones registradas en el Journaling todavía.
        </div>
      ) : (
        <div className="bg-[#161b22] border border-[#30363d] rounded-lg shadow-lg overflow-y-auto max-h-[65vh]">
          <table className="w-full text-left text-sm text-[#c9d1d9] relative">
            
            <thead className="bg-[#21262d] text-[#8b949e] uppercase text-[11px] tracking-wider border-b border-[#30363d] sticky top-0 z-10 shadow-sm">
              <tr>
                <th className="px-4 py-3">Comando</th>
                <th className="px-4 py-3">Ruta Afectada</th>
                <th className="px-4 py-3">Parámetros / Contenido</th>
                <th className="px-4 py-3">Fecha de Ejecución</th>
              </tr>
            </thead>
            
            <tbody className="divide-y divide-[#30363d]">
              {logs.map((log, index) => (
                <tr key={index} className="hover:bg-[#0d1117] transition-colors">
                  <td className="px-4 py-3 font-mono text-[#58a6ff] font-bold">{log.operacion}</td>
                  <td className="px-4 py-3 font-mono text-[#3fb950]">{log.ruta}</td>
                  <td className="px-4 py-3 font-mono text-[#d29922]">{log.contenido || '-'}</td>
                  <td className="px-4 py-3 text-xs">{log.fecha}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      )}
    </div>
  );
}

export default Bitacora;