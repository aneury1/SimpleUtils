# Validación del paquete (2026-11-21)

- C++17 / GCC 14.2.0.
- SQLite del sistema 3.46.1 con `ENABLE_PREUPDATE_HOOK`.
- `cmake --build`: correcto.
- `ctest --test-dir build --output-on-failure`: correcto.
- `./build/simple_orm_tests`: 18 casos internos correctos.
- `./build/simple_orm_example`: correcto.
- `cmake --install` y `find_package(simple_orm 2 CONFIG REQUIRED)` desde otro proyecto: correctos.
- Modo alternativo `SIMPLE_ORM_REQUIRE_PREUPDATE=OFF`: compilado y suite correcta.
- Ruta de descarga/compilación SQLite 3.54.1: **NO verificada en este entorno sin acceso de red**. El archivo SQLite 3.54.1 no se incluye en el ZIP; CMake lo descarga mediante `FETCH_3540100` cuando se selecciona.
- No se han verificado compilaciones nativas Windows / macOS / Android.

Ver `docs/TESTING.md` y `docs/ROADMAP.md` para alcance y limitaciones.
