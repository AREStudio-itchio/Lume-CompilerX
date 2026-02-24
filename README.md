# LumeCompilerX
**Lume CompilerX es un wrapper programado en C++, es una recreación de Lume Compiler pero mejor licenciado bajo la BSL 1.0 (Boost)**.

# Cómo funciona
Lanzamos una instancia de Microsoft Edge desde Program Files para poder ejecutarlo en modo app para no tener bordes y guardamos diferentes archivos de configuración en la carpeta TEMP para no tener que heredar datos de Microsoft Edge normal. Luego de terminar la ejecución, la carpeta TEMP es eliminada automáticamente.

# Uso
Sencillo, compila Lume Compiler con este comando (en G++):
```cmd
g++ main.cpp -o LumeApp.exe -static -lws2_32 -mwindows
```
y crea una carpeta local y pon tu código HTML (JavaScript y Cascading StyleSheets) dentro y ejecuta la app.
Si quieres eliminar el aviso del traductor de Edge, elimina la definición del idioma en el documento para que Edge no sepa en qué idioma está y no lo traduzca.

# Licencia
Como siempre, **el software está distribuido bajo la BSL 1.0 (Boost)**, siempre licencio estos proyectos que distribuyes (voluntáriamente o involuntáriamente) en BSL 1.0 (Boost) o zlib para que no afecte la licencia, si no, licencio bajo MIT o alguna otra.

# Personalización
**El título de la ventana es el de la página.**
**El ícono de la ventana es el mismo que el del favicon del documento.**
