# EvlyMC

Prototipo mínimo de motor voxel estilo Minecraft en C++17 con OpenGL 3.3 (GLFW + GLAD + GLM + FastNoiseLite).

## Qué hace

- Ventana OpenGL 3.3 core con carga de punteros GL vía GLAD.
- Cámara 3D en primera persona (WASD, espacio/shift, ratón, ctrl para correr, ESC para salir).
- Chunks de 16 x 256 x 16 bloques con malla generada en CPU y subida a VBO/VAO.
- Face culling: solo se emiten las caras que dan a aire o a un bloque transparente distinto.
- Terreno generado con ruido FBm (OpenSimplex2) de FastNoiseLite: piedra, tierra, césped, arena y agua a nivel del mar.
- Niebla de distancia del color del cielo para ocultar el borde del mundo cargado.

## Dependencias

```bash
sudo apt-get install -y cmake libglfw3-dev libglm-dev libgl1-mesa-dev xorg-dev
```

GLAD (generado para GL 3.3 core) y FastNoiseLite van incluidos en `external/`.

## Compilar y ejecutar

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/EvlyMC
```

## Variables de entorno

| Variable | Por defecto | Descripción |
| --- | --- | --- |
| `EVLYMC_SEED` | `1337` | Semilla del generador de terreno. |
| `EVLYMC_RENDER_DISTANCE` | `6` | Radio en chunks del mundo generado. |
| `EVLYMC_EXIT_AFTER_FRAMES` | `0` | Si es > 0, cierra el juego tras N fotogramas (útil para pruebas automáticas). |

## Estructura

```
EvlyMC/
├── CMakeLists.txt
├── external/            # GLAD generado + FastNoiseLite.h
└── src/
    ├── main.cpp         # bucle principal, shaders GLSL embebidos
    ├── Window.{h,cpp}   # ventana GLFW + carga de GLAD
    ├── Camera.{h,cpp}   # cámara en primera persona (GLM)
    ├── Tile.{h,cpp}     # definición de bloques (sólido/transparente, color)
    ├── Chunk.{h,cpp}    # arreglo 3D de bloques, mallado y face culling
    └── World.{h,cpp}    # cuadrícula de chunks y generación de terreno
```

## Siguientes pasos

- Atlas de texturas en vez de colores planos.
- Carga/descarga de chunks en streaming según la cámara.
- Colisiones y física del jugador, romper y colocar bloques.
- Iluminación (skylight y luces de bloque) y transparencia ordenada del agua.
