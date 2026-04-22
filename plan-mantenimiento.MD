
# Plan de Mantenimiento y Escalabilidad

**Ciclo de Vida del Sistema a Largo Plazo**

---

## 1. Mantenimiento Preventivo y Correctivo (Corto Plazo)

Para garantizar que el sistema mantenga su estabilidad tras la entrega inicial, se establecen las siguientes acciones:

* **Auditoría de Memoria:** Ejecución periódica de herramientas  sobre el motor en C++ para detectar y mitigar posibles fugas de memoria durante la simulación de discos de gran tamaño.
* **Monitoreo del Servidor REST:** Revisión de los *logs* generados por el framework **Crow** para identificar cuellos de botella en las peticiones HTTP y asegurar que el middleware de CORS funcione sin interrupciones en diferentes navegadores.
* **Validación de Basura en RAM:** Mantenimiento de las funciones de limpieza (`memset` y forzado a `-1`) en la creación de inodos y bloques para asegurar que los reportes sigan generándose limpios ante cualquier actualización del sistema operativo anfitrión.

## 2. Mantenimiento Perfectivo: Actualización de Tecnologías (Mediano Plazo)

El sistema depende de librerías modernas que requieren actualización constante por seguridad y rendimiento:

* **Frontend:** Actualización periódica de las dependencias de `Node.js`,  `React` y `Tailwind CSS` mediante auditorías de `npm` para parchear vulnerabilidades web.
* **Backend:** Actualización de la versión del compilador a estándares más recientes (ej. migrar de C++17 a C++20) para aprovechar mejoras en el manejo de concurrencia e hilos.

## 3. Futuros Desarrollos y Mejoras (Largo Plazo)

La arquitectura modular del sistema permite una escalabilidad horizontal y vertical. Se proponen las siguientes mejoras para futuras versiones :

1. **Implementación de Sistema EXT3 (Journaling):** * Agregar una bitácora de transacciones (Journal) que registre las operaciones antes de escribirlas en los inodos. Esto permitirá recuperación ante fallos o caídas del servidor simulado.
2. **Nuevos Comandos de Gestión (Comandos POSIX):**
* Desarrollo de comandos adicionales como `cp` (copiar), `mv` (mover/renombrar), `find` (búsqueda por nombre o extensión) y `chmod` (cambio dinámico de permisos de 3 dígitos).


3. **Gestión Concurrente de Usuarios:**
* Adaptar el backend para soportar múltiples sesiones de usuario simultáneas, permitiendo que varios clientes interactúen con el mismo disco virtual al mismo tiempo sin colisiones de lectura/escritura.


4. **Visor de Reportes Integrado en React:**
* Ampliar el frontend para que lea las imágenes generadas por Graphviz (`.jpg`, `.png`) en el servidor Ubuntu y las devuelva en base64 para mostrarlas directamente en un panel lateral de la interfaz web, eliminando la necesidad de buscar los archivos en el explorador del sistema operativo.


