import React, { useState } from 'react';
import Terminal from './components/Terminal';
import Login from './components/Login';
import Explorador from './components/Explorador';
import Discos from './components/Discos';
import Bitacora from './components/Bitacora';

function App() {
  const [vistaActiva, setVistaActiva] = useState('terminal'); 
  const [sesionActiva, setSesionActiva] = useState(null); 

  const manejarLoginExitoso = (user, id) => {
    setSesionActiva({ user, id });
    setVistaActiva('explorador'); 
  };

const manejarLogout = async () => {
    try {
      await fetch('http://3.134.103.249:8081/api/logout', { 
        method: 'POST' 
      });
    } catch (error) {
      console.error("Error al cerrar sesión en el servidor:", error);
    }
    
    setSesionActiva(null);
    setVistaActiva('terminal');
  };


  const manejarExplorarParticion = (idParticion) => {
    if (sesionActiva && sesionActiva.id === idParticion) {
      setVistaActiva('explorador');
    } else {
      alert(`Para explorar la partición ${idParticion}, primero debes iniciar sesión en ella.`);
      setVistaActiva('login');
    }
  };

  return (
    <div className="flex flex-col h-screen bg-[#0d1117] text-[#c9d1d9] font-sans selection:bg-[#1f6feb]/30">
      
      <header className="flex items-center justify-between px-4 h-14 border-b border-[#30363d] bg-[#161b22] shrink-0">
        <div className="flex items-center gap-6">
          <div className="flex items-center gap-2 text-gray-200 font-bold text-sm tracking-wide">
            <div className="w-3 h-3 rounded-full bg-blue-500 shadow-[0_0_8px_rgba(59,130,246,0.8)]"></div>
            C++Disk 2.0
          </div>
          
          <div className="h-6 w-px bg-[#30363d]"></div>
          
          <nav className="flex gap-2">
            <button onClick={() => setVistaActiva('terminal')} className={`px-3 py-1.5 rounded text-sm font-semibold transition-colors ${vistaActiva === 'terminal' ? 'bg-[#21262d] text-white border border-[#30363d]' : 'text-[#8b949e] hover:text-[#c9d1d9]'}`}>
              Terminal
            </button>
            <button onClick={() => setVistaActiva('discos')} className={`px-3 py-1.5 rounded text-sm font-semibold transition-colors ${vistaActiva === 'discos' ? 'bg-[#21262d] text-white border border-[#30363d]' : 'text-[#8b949e] hover:text-[#c9d1d9]'}`}>
              Discos
            </button>
            <button onClick={() => setVistaActiva('explorador')} className={`px-3 py-1.5 rounded text-sm font-semibold transition-colors ${vistaActiva === 'explorador' ? 'bg-[#21262d] text-white border border-[#30363d]' : 'text-[#8b949e] hover:text-[#c9d1d9]'}`}>
              Explorador
            </button>
            <button onClick={() => setVistaActiva('bitacora')} className={`px-3 py-1.5 rounded text-sm font-semibold transition-colors ${vistaActiva === 'bitacora' ? 'bg-[#21262d] text-white border border-[#30363d]' : 'text-[#8b949e] hover:text-[#c9d1d9]'}`}>
              Journal
            </button>
          </nav>
        </div>

        <div className="flex items-center gap-4">
          {sesionActiva ? (
            <div className="flex items-center gap-4">
              <span className="text-xs font-mono text-[#3fb950]"> {sesionActiva.user}@{sesionActiva.id}</span>
              <button onClick={manejarLogout} className="bg-[#da3633] hover:bg-[#f85149] text-white px-3 py-1.5 rounded text-xs font-semibold transition-colors border border-[#rgba(240,246,252,0.1)]">
                Cerrar Sesión
              </button>
            </div>
          ) : (
            <button onClick={() => setVistaActiva('login')} className="bg-[#238636] hover:bg-[#2ea043] text-white px-4 py-1.5 rounded text-xs font-semibold transition-colors border border-[#rgba(240,246,252,0.1)]">
              Iniciar Sesión
            </button>
          )}
        </div>
      </header>

      <main className="flex flex-col flex-grow overflow-hidden relative">
        {vistaActiva === 'terminal' && <Terminal />}
        {vistaActiva === 'discos' && <Discos onExplorar={manejarExplorarParticion} />}
        {vistaActiva === 'login' && <Login onLoginExitosa={manejarLoginExitoso} />}
        {vistaActiva === 'explorador' && <Explorador sesionActiva={sesionActiva} irALogin={() => setVistaActiva('login')} />}
        {vistaActiva === 'bitacora' && <Bitacora sesionActiva={sesionActiva} irALogin={() => setVistaActiva('login')} />}
      </main>

    </div>
  );
}

export default App;