import React, { useState, useEffect } from 'react';

function Discos({ onExplorar }) {
  const [discos, setDiscos] = useState([]);
  const [cargando, setCargando] = useState(false);
  const [error, setError] = useState('');

  const cargarDiscos = async () => {
    setCargando(true);
    setError('');
    try {
      const resp = await fetch('http://localhost:8081/api/discos');
      const data = await resp.json();
      
      if (data.error) {
        setError(data.error);
      } else {
        setDiscos(data.data || []);
      }
    } catch (err) {
      setError('Error al conectar con C++ para leer los discos.');
    } finally {
      setCargando(false);
    }
  };

  useEffect(() => {
    cargarDiscos();
  }, []);

  const formatBytes = (bytes) => {
    if (bytes >= 1048576) return (bytes / 1048576).toFixed(2) + ' MB';
    if (bytes >= 1024) return (bytes / 1024).toFixed(2) + ' KB';
    return bytes + ' B';
  };

  return (
    <div className="flex-grow flex flex-col bg-[#0d1117] p-6 overflow-auto">
      
      <div className="flex justify-between items-center mb-6 border-b border-[#30363d] pb-2">
        <h2 className="text-2xl font-semibold text-white flex items-center gap-2">
          <svg className="w-6 h-6 text-[#8b949e]" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M5 8h14M5 8a2 2 0 110-4h14a2 2 0 110 4M5 8v10a2 2 0 002 2h10a2 2 0 002-2V8m-9 4h4"></path></svg>
          Selección de Disco Creado
        </h2>
        <button onClick={cargarDiscos} disabled={cargando} className="bg-[#21262d] border border-[#30363d] text-white px-3 py-1.5 rounded hover:bg-[#30363d] transition-colors flex items-center gap-2 text-sm">
          <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15"></path></svg>
          Actualizar Vista
        </button>
      </div>

      {error && (
        <div className="mb-6 bg-[#da3633]/10 border border-[#da3633]/50 text-[#ff7b72] px-4 py-3 rounded text-sm text-center font-medium">
          {error}
        </div>
      )}

      {cargando ? (
        <div className="text-center text-[#8b949e] mt-10 animate-pulse">Obteniendo discos del sistema...</div>
      ) : discos.length === 0 ? (
        <div className="flex flex-col items-center justify-center py-10 text-[#8b949e] border border-dashed border-[#30363d] rounded-lg">
          <svg className="w-12 h-12 mb-4 opacity-50" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="1.5" d="M20 13V6a2 2 0 00-2-2H6a2 2 0 00-2 2v7m16 0v5a2 2 0 01-2 2H6a2 2 0 01-2-2v-5m16 0h-2.586a1 1 0 00-.707.293l-2.414 2.414a1 1 0 01-.707.293h-3.172a1 1 0 01-.707-.293l-2.414-2.414A1 1 0 006.586 13H4"></path></svg>
          <p>No se encontraron discos activos.</p>
          <p className="text-xs mt-1">Asegúrate de ejecutar tus comandos en la terminal primero.</p>
        </div>
      ) : (
        <div className="grid grid-cols-1 md:grid-cols-2 xl:grid-cols-3 gap-6">
          {discos.map((disco, dIdx) => (
            <div key={dIdx} className="bg-[#161b22] border border-[#30363d] rounded-lg p-5 shadow-lg flex flex-col h-full">
              
              <div className="flex items-center gap-3 mb-4 border-b border-[#30363d] pb-4">
                <div className="p-3 bg-[#0d1117] rounded-full border border-[#30363d]">
                  <svg className="w-6 h-6 text-[#58a6ff]" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="1.5" d="M9 17v-2m3 2v-4m3 4v-6m2 10H7a2 2 0 01-2-2V5a2 2 0 012-2h5.586a1 1 0 01.707.293l5.414 5.414a1 1 0 01.293.707V19a2 2 0 01-2 2z"></path></svg>
                </div>
                <div>
                  <h3 className="text-white font-bold text-lg">{disco.name}</h3>
                  <div className="flex gap-3 text-xs text-[#8b949e] font-mono mt-1">
                    <span className="bg-[#21262d] px-2 py-0.5 rounded border border-[#30363d]">Capacidad: {formatBytes(disco.size)}</span>
                    <span className="bg-[#21262d] px-2 py-0.5 rounded border border-[#30363d]">Fit: {disco.fit}</span>
                  </div>
                </div>
              </div>

              <div className="flex-grow">
                <h4 className="text-[11px] uppercase tracking-wider text-[#8b949e] mb-3 font-semibold">Particiones Registradas:</h4>
                
                {disco.partitions && disco.partitions.length > 0 ? (
                  <div className="flex flex-col gap-2">
                    {disco.partitions.map((p, pIdx) => (
                      <div key={pIdx} className="bg-[#0d1117] p-3 rounded border border-[#30363d] flex flex-col">
                        
                        <div className="flex justify-between items-center mb-2">
                          <div className="flex items-center gap-2">
                            <span className={`text-[10px] font-bold px-1.5 py-0.5 rounded text-white ${p.type === 'P' ? 'bg-[#238636]' : p.type === 'E' ? 'bg-[#d29922]' : 'bg-[#8957e5]'}`}>
                              {p.type === 'P' ? 'Primaria' : p.type === 'E' ? 'Extendida' : 'Lógica'}
                            </span>
                            <span className="text-sm text-[#c9d1d9] font-medium">{p.name}</span>
                          </div>
                          <span className={`text-[10px] font-bold px-2 py-0.5 rounded border ${p.estado === 'Montada' ? 'bg-[#238636]/20 text-[#3fb950] border-[#3fb950]/30' : 'bg-[#da3633]/10 text-[#ff7b72] border-[#ff7b72]/30'}`}>
                            {p.estado} {p.id && `(${p.id})`}
                          </span>
                        </div>
                        
                        <div className="flex justify-between text-xs text-[#8b949e] font-mono border-t border-[#30363d] pt-1.5 mt-1.5">
                          <span>Tamaño: {formatBytes(p.size)}</span>
                          <span>Fit: {p.fit}</span>
                        </div>

                        {p.estado === 'Montada' && p.id && (
                          <button 
                            onClick={() => onExplorar(p.id)}
                            className="mt-3 w-full bg-[#21262d] border border-[#30363d] text-white px-3 py-1.5 rounded hover:bg-[#30363d] hover:text-[#58a6ff] transition-colors text-xs font-semibold flex justify-center items-center gap-2"
                          >
                            <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M15 12a3 3 0 11-6 0 3 3 0 016 0z"></path><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M2.458 12C3.732 7.943 7.523 5 12 5c4.478 0 8.268 2.943 9.542 7-1.274 4.057-5.064 7-9.542 7-4.477 0-8.268-2.943-9.542-7z"></path></svg>
                            Navegar en Explorador
                          </button>
                        )}

                      </div>
                    ))}
                  </div>
                ) : (
                  <div className="text-xs text-[#8b949e] italic text-center p-4 bg-[#0d1117] rounded border border-[#30363d]">
                    No contiene particiones MBR activas.
                  </div>
                )}
              </div>
              
            </div>
          ))}
        </div>
      )}
    </div>
  );
}

export default Discos;