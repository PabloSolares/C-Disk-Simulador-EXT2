import React, { useState, useEffect } from 'react';

function Explorador({ sesionActiva, irALogin }) {
  const [rutaActual, setRutaActual] = useState('/');
  const [elementos, setElementos] = useState([]);
  const [contenidoArchivo, setContenidoArchivo] = useState(null);
  const [cargando, setCargando] = useState(false);

  useEffect(() => {
    if (sesionActiva) {
      cargarDirectorio(rutaActual);
    }
  }, [rutaActual, sesionActiva]);

  const cargarDirectorio = async (path) => {
    setCargando(true);
    try {
      const resp = await fetch('http://localhost:8081/api/explorar', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ id: sesionActiva.id, path: path })
      });
      const data = await resp.json();
      
      if (data.tipo === 'carpeta') {
        setElementos(data.contenido || []);
        setContenidoArchivo(null); 
      } else if (data.tipo === 'archivo') {
        setContenidoArchivo(data.texto || 'El archivo está vacío.');
      } else if (data.error) {
        console.error("Error del servidor C++:", data.error);
        if (path !== '/') {
          setRutaActual('/'); 
          setContenidoArchivo(null);
        }
      }
    } catch (err) {
      console.error("Error de red al cargar el directorio:", err);
    } finally {
      setCargando(false);
    }
  };

  const handleDobleClick = (elemento) => {
    if (elemento.nombre === '.' || elemento.nombre === '..') return;
    
    const nuevaRuta = rutaActual === '/' ? `/${elemento.nombre}` : `${rutaActual}/${elemento.nombre}`;
    setRutaActual(nuevaRuta);
  };

  const irAtras = () => {
    if (rutaActual === '/') {
      setContenidoArchivo(null);
      return;
    }
    
    const partes = rutaActual.split('/').filter(p => p !== '');
    partes.pop(); 
    
    const rutaPadre = partes.length === 0 ? '/' : '/' + partes.join('/');
    
    setContenidoArchivo(null); 
    setRutaActual(rutaPadre); 
  };

 
  if (!sesionActiva) {
    return (
      <div className="flex-grow flex flex-col items-center justify-center bg-[#0d1117] text-[#8b949e]">
        <svg className="w-16 h-16 mb-4 opacity-50" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="1.5" d="M12 15v2m-6 4h12a2 2 0 002-2v-6a2 2 0 00-2-2H6a2 2 0 00-2 2v6a2 2 0 002 2zm10-10V7a4 4 0 00-8 0v4h8z"></path></svg>
        <h2 className="text-xl font-semibold text-[#c9d1d9]">Acceso Denegado</h2>
        <p className="mt-2">Debes iniciar sesión para explorar el sistema de archivos.</p>
        <button onClick={irALogin} className="mt-4 bg-[#21262d] border border-[#30363d] hover:bg-[#30363d] text-white px-4 py-2 rounded transition-colors">
          Ir al Login
        </button>
      </div>
    );
  }

  return (
    <div className="flex-grow flex flex-col bg-[#0d1117] p-6 overflow-auto">
      <h2 className="text-2xl font-semibold mb-6 text-white border-b border-[#30363d] pb-2">Visualizador del Sistema de Archivos</h2>
      
      <div className="flex items-center gap-4 mb-6">
        <button 
          onClick={irAtras} 
          disabled={rutaActual === '/'} 
          className="bg-[#21262d] border border-[#30363d] text-white px-3 py-1.5 rounded hover:bg-[#30363d] disabled:opacity-50 disabled:cursor-not-allowed transition-colors"
        >
          ← Atrás
        </button>
        
        <div className="flex items-center gap-2 bg-[#161b22] border border-[#30363d] rounded px-4 py-2 font-mono text-sm w-full shadow-inner">
          <span className="text-[#3fb950] font-bold">{sesionActiva.user}@{sesionActiva.id}:</span>
          <span className="text-[#58a6ff]">{rutaActual}</span>
        </div>
      </div>

      {cargando ? (
        <div className="text-center text-[#8b949e] mt-10 animate-pulse">Cargando datos del disco...</div>
      ) : contenidoArchivo !== null ? (
        
        <div className="bg-[#010409] border border-[#30363d] rounded-lg p-5 flex-grow overflow-auto shadow-lg">
          <h3 className="text-[#8b949e] mb-4 font-semibold flex items-center gap-2 border-b border-[#30363d] pb-2">
            <svg className="w-5 h-5 text-[#58a6ff]" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M9 12h6m-6 4h6m2 5H7a2 2 0 01-2-2V5a2 2 0 012-2h5.586a1 1 0 01.707.293l5.414 5.414a1 1 0 01.293.707V19a2 2 0 01-2 2z"></path></svg>
            Contenido del archivo:
          </h3>
          <pre className="text-[#c9d1d9] font-mono text-sm whitespace-pre-wrap leading-relaxed">{contenidoArchivo}</pre>
        </div>

      ) : (

        <div className="grid grid-cols-2 sm:grid-cols-4 md:grid-cols-6 lg:grid-cols-8 gap-6">
          {elementos.map((item, index) => (
            <div 
              key={index} 
              onDoubleClick={() => handleDobleClick(item)}
              className="flex flex-col items-center gap-2 p-3 rounded-lg hover:bg-[#161b22] border border-transparent hover:border-[#30363d] cursor-pointer transition-colors group relative"
            >
              
              <div className="absolute -top-2 -right-2 flex flex-col gap-1 opacity-0 group-hover:opacity-100 transition-opacity z-10">
                <span className="bg-[#238636] text-white text-[9px] font-mono px-1.5 py-0.5 rounded shadow border border-[#2ea043]">
                  {item.permisos || '664'}
                </span>
                <span className="bg-[#1f6feb] text-white text-[9px] font-mono px-1.5 py-0.5 rounded shadow border border-[#388bfd]">
                  UID: {item.propietario || '1'}
                </span>
              </div>

              {item.tipo === 'carpeta' ? (
                <svg className="w-12 h-12 text-[#e3b341] group-hover:scale-105 transition-transform" fill="currentColor" viewBox="0 0 20 20"><path d="M2 6a2 2 0 012-2h5l2 2h5a2 2 0 012 2v6a2 2 0 01-2 2H4a2 2 0 01-2-2V6z"></path></svg>
              ) : (
                <svg className="w-12 h-12 text-[#8b949e] group-hover:scale-105 transition-transform" fill="currentColor" viewBox="0 0 20 20"><path fillRule="evenodd" d="M4 4a2 2 0 012-2h4.586A2 2 0 0112 2.586L15.414 6A2 2 0 0116 7.414V16a2 2 0 01-2 2H6a2 2 0 01-2-2V4zm2 6a1 1 0 011-1h6a1 1 0 110 2H7a1 1 0 01-1-1zm1 3a1 1 0 100 2h6a1 1 0 100-2H7z" clipRule="evenodd"></path></svg>
              )}
              
              <span className="text-xs font-medium text-center text-[#c9d1d9] group-hover:text-white break-all line-clamp-2">
                {item.nombre}
              </span>
            </div>
          ))}
          
          {elementos.length === 0 && (
            <div className="col-span-full flex flex-col items-center justify-center py-10 text-[#8b949e]">
              <svg className="w-10 h-10 mb-2 opacity-50" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="1.5" d="M5 19a2 2 0 01-2-2V7a2 2 0 012-2h4l2 2h4a2 2 0 012 2v1M5 19h14a2 2 0 002-2v-5a2 2 0 00-2-2H9a2 2 0 00-2 2v5a2 2 0 01-2 2z"></path></svg>
              <span>Esta carpeta está vacía.</span>
            </div>
          )}
        </div>
      )}
    </div>
  );
}

export default Explorador;