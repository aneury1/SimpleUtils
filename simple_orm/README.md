# simple_orm 2.0.0

Librería SQLite ORM en **C++17** con modelos tipados, tablas dinámicas, DAO CRUD, Query Builder sin SQL escrito manualmente y **Live Events** de cambios confirmados.

> **Estado**: implementación inicial funcional; no es una migración compatible a nivel ABI con `sqlite3_orm` 1.x. Las migraciones automáticas, los JOIN tipados y la observación entre procesos no se implementan todavía. Consulta [docs/ROADMAP.md](docs/ROADMAP.md).

## Requisitos

- Compilador C++17, CMake >= 3.18 (>=3.24 para descarga de SQLite con FetchContent).
- SQLite >= 3.37.0, compilado con `ENABLE_PREUPDATE_HOOK` para eventos de columna.
- O descargar SQLite **3.54.1** en el paso de configuración con `SIMPLE_ORM_SQLITE_PROVIDER=FETCH_3540100` (requiere Internet en ese paso).

Se eliminó SQLite 3.29.0 del ZIP original. Para builds **sin red**, el modo predeterminado enlaza el SQLite del sistema (>=3.37.0). El motor empleado para la validación local fue SQLite 3.46.1, no 3.54.1; el paquete configura la descarga verificada por SHA3 de 3.54.1 pero **esa ruta no se pudo ejecutar sin red en este entorno**.

## Compilar y ejecutar pruebas (Linux)

```bash
sudo apt install cmake g++ libsqlite3-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/simple_orm_example
```

Para compilar con SQLite 3.54.1 fijado (la primera configuración requiere Internet):

```bash
cmake -S . -B build-354 \
  -DSIMPLE_ORM_SQLITE_PROVIDER=FETCH_3540100
cmake --build build-354 -j
ctest --test-dir build-354 --output-on-failure
```

Para sistemas cuyo SQLite no incluya `ENABLE_PREUPDATE_HOOK`, configura `-DSIMPLE_ORM_REQUIRE_PREUPDATE=OFF`; la observación de filas continuará mediante `sqlite3_update_hook`, pero `observeColumn()` lanzará `Error` y la suite probará el modo alternativo sin eventos de columna. Si necesitas columna Live Events, utiliza el proveedor `FETCH_3540100`.

### Windows (Visual Studio)

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DSIMPLE_ORM_SQLITE_PROVIDER=FETCH_3540100
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

### Utilizar desde CMake instalado

```cmake
find_package(simple_orm 2 CONFIG REQUIRED)
add_executable(app main.cpp)
target_link_libraries(app PRIVATE simple_orm::simple_orm)
```

## Uso rápido

```cpp
#include <simple_orm/simple_orm.hpp>
using namespace simple_orm;

struct User {
    std::int64_t id=0;
    std::string name;
    std::optional<std::string> email;
};

int main() {
    Database db({"users.db"});
    db.open();
    auto users=makeTable<User>("users",
        column<&User::id>("id").primaryKey().autoIncrement(),
        column<&User::name>("name").notNull(),
        column<&User::email>("email"));
    users.create(db); // CREATE TABLE IF NOT EXISTS ... STRICT
    auto dao=db.dao(users);
    auto observer=db.observeColumn("users","name",[](const Event& e){
        // Callback síncrono DESPUÉS de commit; nunca se lanza desde SQLite hook.
    });
    User u;u.name="Elena";dao.insert(u);
    auto byId=dao.findById(u.id);
    auto all=dao.query().where(field<&User::name>()=="Elena").all();
    return byId && all.size()==1 ? 0:1;
}
```

## Documentación

- [Guía de API](docs/API.md): firmas públicas, objetos, consultas y ejemplos.
- [Live Events](docs/EVENTS.md): observación, transacciones, restricciones y ciclo de vida.
- [Migración 1.x → 2.x](docs/MIGRATION.md): cambios de namespace, comportamiento y límites de compatibilidad.
- [Pruebas y alcance](docs/TESTING.md): suite, compilación y casos cubiertos.
- [Roadmap y limitaciones](docs/ROADMAP.md): funcionalidades todavía no implementadas.

## Licencia

Código de este paquete: Apache-2.0 (se conserva LICENSE del proyecto original). SQLite: dominio público según su proyecto; el amalgamado SQLite 3.54.1 se obtiene desde sqlite.org al configurar el proveedor descargado.
