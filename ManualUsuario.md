
-----

# Manual de Usuario: Interfaz Web C++Disk (AWS)

**Proyecto 2 - Manejo e Implementación de Archivos**
**Desarrollador:** Pablo Daniel Solares Samayoa
**Carnet:** 202400044

-----

## 1\. Introducción al Sistema

C++Disk es una plataforma web en la nube diseñada para facilitar la interacción con sistemas de archivos EXT2 y EXT3. Para esta segunda fase, el sistema ha evolucionado de una simple consola a un entorno de administración gráfico completo alojado en **Amazon Web Services (AWS)**.

La interfaz, construida con React y Tailwind CSS, mantiene su diseño profesional de alto contraste (Dark Mode), pero ahora incorpora vistas interactivas para la gestión visual de sesiones y el monitoreo en tiempo real de los discos y particiones del servidor.

-----

## 2\. Acceso al Sistema (Despliegue en AWS)

A diferencia de la versión local, ya no es necesario compilar el código en tu computadora. El sistema está activo 24/7 en la infraestructura de AWS.
Link: http://mia1s2026p2-202400044.s3-website.us-east-2.amazonaws.com/

**Paso 1: Acceder a la Interfaz (Frontend)**

1.  Abre tu navegador web preferido.
2.  Ingresa a la URL pública proporcionada por el bucket de Amazon S3 donde está alojada la aplicación.
3.  La interfaz cargará instantáneamente gracias a su arquitectura estática.

**Paso 2: Estado del Backend (C++)**

  * El motor lógico se ejecuta de forma ininterrumpida en una instancia remota de Amazon EC2. El frontend está configurado para comunicarse automáticamente con la dirección IP pública (puerto 8081) de dicho servidor.

-----

## 3\. Entorno de Trabajo y Nuevas Vistas

La plataforma ha sido rediseñada para soportar múltiples vistas, separando la ejecución de comandos de la administración visual. Puedes alternar entre ellas usando la barra de navegación.

### 3.1 Vista Terminal (Consola IDE)

Es el núcleo del sistema, optimizado para la ejecución de scripts `.smia`.

  * **Editor de Código:** Área enriquecida con numeración de líneas automáticas y tabulación inteligente.
  * **Terminal de Salida:** Consola verde retro que muestra las respuestas, errores y transacciones del servidor en tiempo real.
  ![alt text](image.png)

### 3.2 Vista de Autenticación (Login / Logout Visual)

Por motivos de seguridad, los comandos de texto `login` y `logout` han sido deshabilitados en la terminal. Ahora, la autenticación se realiza de forma 100% visual.

  * **Formulario Seguro:** Permite ingresar el Usuario, Contraseña y el ID de la partición (ej. `441A`) donde se desea iniciar sesión.
  * **Manejo de Estado:** Al iniciar sesión exitosamente, la interfaz guarda al usuario activo y habilita los comandos avanzados (como `mkdir`, `copy`, `move`).

### 3.3 Vista Explorador de Discos y Particiones

Un dashboard interactivo que consulta el estado del servidor en tiempo real.

  * **Tarjetas de Discos:** Muestra visualmente todos los discos físicos creados en el servidor EC2, indicando su ruta, tamaño total y tipo de ajuste.
  ![alt text](image-1.png)
  * **Estado de Particiones:** Detalla las particiones de cada disco, mostrando su tamaño, tipo (Primaria, Extendida, Lógica) y, lo más importante, **su estado actual ("Montada" o "No Montada")** junto con su ID generado por el sistema.

    ![alt text](image-2.png)
-----

## 4\. Uso del Sistema 

### 4.1 Iniciar y Cerrar Sesión

1.  Dirígete a la vista de **Login** desde el menú principal.
2.  Ingresa las credenciales (por defecto, usuario: `root`, contraseña: `123`) y el ID de la partición formateada previamente.
3.  Haz clic en **"Iniciar Sesión"**. El sistema validará los datos con el servidor y te redirigirá a la terminal.
4.  Para terminar el trabajo, utiliza el botón **"Cerrar Sesión"** en la barra de navegación. Esto limpiará la sesión en el frontend y en el backend.

### 4.2 Subir y Ejecutar un Script `.smia`

1.  En la vista de **Terminal**, haz clic en **"Subir Script"**.
2.  Selecciona tu archivo de calificación. El código se vaciará en el editor.
3.  Haz clic en el botón azul **"Ejecutar"**.
4.  El frontend enviará el bloque completo a AWS. La **Terminal de Salida** imprimirá el registro detallado, mostrando la creación de usuarios, movimientos de archivos (`move`/`copy`) y pérdida de datos (`loss`).

-----

## 5\. Visualización de Reportes 

Todos los reportes generados por el comando `rep` (`tree`, `disk`, `sb`, `journaling`) son creados físicamente dentro del servidor EC2 de AWS en la ruta especificada (ej. `/home/pablo/Reportes/`).


-----

## 6\. Resolución de Problemas Comunes 
A continuación, se presentan los errores más frecuentes al utilizar la plataforma en la nube y sus soluciones:

  * **Error: "Permission denied" al intentar crear un disco (`mkdisk`).**

      * *Causa:* El servidor EC2 tiene protecciones de seguridad de Linux y no permite escribir en rutas externas al usuario predeterminado.
      * *Solución:* Conéctate por SSH al EC2 y otórgale permisos totales a la carpeta de destino ejecutando `sudo chmod -R 777 /home/pablo/`.

  * **Error: "NetworkError" o la aplicación no hace nada al hacer clic en Ejecutar.**

      * *Causa:* El frontend no puede comunicarse con el backend. Esto suele ocurrir si la IP pública del EC2 cambió tras un reinicio, o si los Security Groups de AWS no están abiertos.
      * *Solución:* Verifica en la consola de AWS que tu instancia tenga el puerto `8081` abierto para tráfico TCP en sus reglas de entrada. Asegúrate también de que la IP en tu código de React coincida con la IP pública actual de la instancia.

  * **El disco se creó en la terminal, pero no aparece en la Vista de Discos.**

      * *Causa:* Falta refrescar el estado del servidor.
      * *Solución:* La vista de discos se actualiza en tiempo real al consultarla. Simplemente recarga la página o navega entre pestañas para forzar al frontend a pedir los datos más frescos al endpoint `/api/discos`.