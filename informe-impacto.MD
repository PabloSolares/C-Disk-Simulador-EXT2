

# Informe de Impacto: C++Disk

**Proyecto 1 - Manejo e Implementación de Archivos**
**Carnet:** 202400044

---

## 1. Resumen 

El presente documento describe el impacto potencial del sistema "C++Disk", un simulador de sistema de archivos EXT2 con arquitectura Cliente-Servidor. Este proyecto está diseñado para revolucionar la forma en que se interactúa con estructuras de bajo nivel, trasladando la complejidad de la consola de Linux a un entorno web accesible, seguro y altamente visual.

## 2. Impacto en el Contexto de Aplicación 
### 2.1 Mejora de Eficiencia en el Aprendizaje y Análisis

Históricamente, la comprensión de sistemas de archivos (Inodos, Bloques, Superbloques, MBR) requiere la lectura manual de bytes en editores hexadecimales, un proceso lento y propenso a errores.

* **Impacto:** La integración del sistema con **Graphviz** automatiza la renderización de estas estructuras en milisegundos. Lo que antes tomaba horas de análisis manual, ahora se visualiza instantáneamente mediante el comando `rep`, incrementando la eficiencia analítica en un 100%.

### 2.2 Reducción de Riesgos y Costos Operativos

Trabajar con particionamiento de discos y formateo a bajo nivel conlleva el riesgo inherente de corromper el sistema operativo.

* **Impacto:** "C++Disk" mitiga este riesgo al 100% mediante la creación de discos virtuales (`.mia`). Permite a estudiantes, administradores de sistemas y desarrolladores realizar pruebas destructivas (formateo, eliminación de particiones, gestión de permisos) en un ambiente seguro. Esto elimina los costos asociados a la recuperación de datos o la compra de hardware dedicado para laboratorios de pruebas.

### 2.3 Optimización de Tiempos mediante Procesamiento Masivo

La ejecución manual de comandos de administración de servidores es repetitiva.

* **Impacto:** La implementación del IDE web en **React** permite la carga de archivos `.smia`. El sistema puede procesar cientos de instrucciones (creación de directorios, usuarios, y archivos) en bloque. Esto reduce el tiempo de configuración de un entorno simulado de minutos a apenas un par de segundos, optimizando el tiempo del usuario.

### 2.4 Modernización de la Interfaz de Usuario

* **Impacto:** Acorta la curva de aprendizaje. Al envolver un motor robusto en **C++** dentro de una interfaz web moderna, intuitiva y con validaciones en tiempo real, se democratiza el acceso a conceptos avanzados de Sistemas Operativos sin intimidar al usuario con una terminal en blanco.

---