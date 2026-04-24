import React, { useState, useRef } from 'react';

function Terminal() {
  const [code, setCode] = useState('');
  const [consola, setConsola] = useState('Esperando comandos...\n');
  const [fileName, setFileName] = useState('untitled.smia');
  
  const lineNumbersRef = useRef(null);
  const textAreaRef = useRef(null);

  const handleScroll = () => {
    if (lineNumbersRef.current && textAreaRef.current) {
      lineNumbersRef.current.scrollTop = textAreaRef.current.scrollTop;
    }
  };

  const handleKeyDown = (e) => {
    if (e.key === 'Tab') {
      e.preventDefault();
      const start = e.target.selectionStart;
      const end = e.target.selectionEnd;
      setCode(code.substring(0, start) + "    " + code.substring(end));
      setTimeout(() => { textAreaRef.current.selectionStart = textAreaRef.current.selectionEnd = start + 4; }, 0);
    }
  };

  const handleFileUpload = (e) => {
    const file = e.target.files[0];
    if (!file) return;
    setFileName(file.name);
    const reader = new FileReader();
    reader.onload = (evento) => setCode(evento.target.result);
    reader.readAsText(file);
    e.target.value = null; 
  };

  const ejecutarScript = async () => {
    if (!code.trim()) {
      setConsola('Error: El editor esta vacio.');
      return;
    }
    setConsola('Ejecutando en C++...\n');
    try {
      const respuesta = await fetch('http://3.134.103.249:8081/exec', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ script: code })
      });
      const data = await respuesta.json();
      setConsola(data.success ? data.console : 'ERROR:\n' + data.error);
    } catch (error) {
      setConsola('Error de red. ¿Servidor C++ encendido?\nDetalle: ' + error.message);
    }
  };

  const lineCount = code.split('\n').length;
  const linesArr = Array.from({ length: Math.max(lineCount, 1) }, (_, i) => i + 1);

  return (
    <>
      <div className="flex justify-between items-center px-4 py-2 border-b border-[#30363d] bg-[#0d1117]">
        <span className="text-[12px] font-mono text-[#8b949e]">{fileName}</span>
        <div className="flex gap-3">
          <label className="cursor-pointer text-xs font-medium text-[#c9d1d9] hover:text-white bg-[#21262d] border border-[#30363d] px-3 py-1 rounded transition-colors flex items-center gap-2">
            Subir .smia
            <input type="file" accept=".smia,.txt,.script" className="hidden" onChange={handleFileUpload} />
          </label>
          <button onClick={ejecutarScript} className="bg-[#238636] hover:bg-[#2ea043] text-white px-4 py-1 rounded text-xs font-semibold transition-colors">
             Ejecutar
          </button>
        </div>
      </div>

      <div className="flex h-[55%] relative bg-[#0d1117]">
        <div ref={lineNumbersRef} className="w-12 py-3 pr-4 text-right font-mono text-[13px] text-[#484f58] border-r border-[#30363d] select-none overflow-hidden" style={{ lineHeight: '1.5rem' }}>
          {linesArr.map(num => <div key={num}>{num}</div>)}
        </div>
        <textarea ref={textAreaRef} value={code} onChange={(e) => setCode(e.target.value)} onScroll={handleScroll} onKeyDown={handleKeyDown} spellCheck="false" placeholder="// Escribe comandos..." className="flex-grow p-3 bg-transparent text-[#c9d1d9] font-mono text-[13px] outline-none resize-none whitespace-pre overflow-auto" style={{ lineHeight: '1.5rem' }} />
      </div>

      <div className="h-[45%] flex flex-col border-t border-[#30363d] bg-[#010409]">
        <div className="flex justify-between items-center px-4 py-1.5 border-b border-[#30363d] bg-[#0d1117]">
          <span className="text-[11px] font-mono text-[#8b949e] uppercase tracking-widest flex items-center gap-2">
            <span className="w-2 h-2 rounded-full bg-[#238636]"></span> Salida de C++
          </span>
          <button onClick={() => setConsola('')} className="text-[11px] font-mono text-[#8b949e] hover:text-[#c9d1d9] uppercase tracking-wider">Limpiar</button>
        </div>
        <pre className="flex-grow p-4 text-[#3fb950] font-mono text-[13px] overflow-auto whitespace-pre-wrap leading-relaxed">{consola}</pre>
      </div>
    </>
  );
}

export default Terminal;