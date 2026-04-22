import React, { useState } from 'react';

function Login({ onLoginExitosa }) {
  const [loginForm, setLoginForm] = useState({ id: '', user: '', password: '' });
  const [error, setError] = useState(''); 
  const [cargando, setCargando] = useState(false);

  const handleSubmit = async (e) => {
    e.preventDefault();
    setError('');
    setCargando(true);

    try {
      const respuesta = await fetch('http://localhost:8081/api/login', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ 
          user: loginForm.user, 
          password: loginForm.password, 
          id: loginForm.id 
        })
      });
      
      const data = await respuesta.json();
      
      if (!data.success) {
        setError(data.error || "Credenciales incorrectas.");
      } else {
        onLoginExitosa(loginForm.user, loginForm.id);
      }
    } catch (err) {
      setError("Error de red. ¿Está encendido el servidor C++?");
    } finally {
      setCargando(false);
    }
  };

  return (
    <div className="flex-grow flex items-center justify-center bg-[#010409]">
      <form onSubmit={handleSubmit} className="bg-[#161b22] border border-[#30363d] rounded-lg p-8 w-96 shadow-2xl relative overflow-hidden">
        
        <div className="absolute top-0 left-0 w-full h-1 bg-gradient-to-r from-[#2ea043] to-[#238636]"></div>

        <h2 className="text-2xl font-semibold mb-6 text-center text-white">Iniciar Sesión</h2>
        
        {error && (
          <div className="mb-5 bg-[#da3633]/10 border border-[#da3633]/50 text-[#ff7b72] px-3 py-2.5 rounded text-[13px] text-center font-medium">
            {error}
          </div>
        )}

        <div className="space-y-4">
          <div>
            <label className="block text-sm font-medium text-[#c9d1d9] mb-1">ID Partición</label>
            <input 
              type="text" 
              required 
              value={loginForm.id} 
              onChange={e => setLoginForm({...loginForm, id: e.target.value})} 
              className="w-full bg-[#0d1117] border border-[#30363d] rounded px-3 py-2 text-sm text-white outline-none focus:border-[#58a6ff] transition-colors" 
              placeholder="Ej. 441A" 
            />
          </div>
          <div>
            <label className="block text-sm font-medium text-[#c9d1d9] mb-1">Usuario</label>
            <input 
              type="text" 
              required 
              value={loginForm.user} 
              onChange={e => setLoginForm({...loginForm, user: e.target.value})} 
              className="w-full bg-[#0d1117] border border-[#30363d] rounded px-3 py-2 text-sm text-white outline-none focus:border-[#58a6ff] transition-colors" 
              placeholder="root" 
            />
          </div>
          <div>
            <label className="block text-sm font-medium text-[#c9d1d9] mb-1">Contraseña</label>
            <input 
              type="password" 
              required 
              value={loginForm.password} 
              onChange={e => setLoginForm({...loginForm, password: e.target.value})} 
              className="w-full bg-[#0d1117] border border-[#30363d] rounded px-3 py-2 text-sm text-white outline-none focus:border-[#58a6ff] transition-colors" 
              placeholder="••••••••" 
            />
          </div>
        </div>

        <button 
          type="submit" 
          disabled={cargando}
          className="w-full mt-6 bg-[#238636] hover:bg-[#2ea043] disabled:bg-[#238636]/50 disabled:cursor-not-allowed text-white font-semibold py-2 px-4 rounded transition-colors flex justify-center items-center gap-2"
        >
          {cargando ? (
            <>
              <svg className="animate-spin h-4 w-4 text-white" fill="none" viewBox="0 0 24 24"><circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4"></circle><path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"></path></svg>
              Autenticando...
            </>
          ) : (
            'Ingresar'
          )}
        </button>
      </form>
    </div>
  );
}

export default Login;