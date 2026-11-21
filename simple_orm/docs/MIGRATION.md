# Migración de sqlite3_orm 1.x a simple_orm 2.0

La migración deliberadamente rompe compatibilidad ABI y algunos nombres de API. **No** reescribe bases de datos existentes; SQLite sigue usando los mismos archivos. La nueva API no importa las clases originales directamente. Puede coexistir con el código 1.x mediante adaptación por etapas en la aplicación.

| API antigua | API nueva |
|---|---|
| `#include <sqlite3_orm.hpp>` | `#include <simple_orm/simple_orm.hpp>` |
| `namespace sqlite3_orm` | `namespace simple_orm` |
| `Database db("file.db")` | `Database db({"file.db"})` |
| `db.Open()` | `db.open()` |
| `db.Close()` | `db.close()` |
| `db.ExecuteSQL(sql)` | `db.execute(sql)` (una sentencia; no consulta) |
| `SelectTableAll(...)` | `db.dao(schema).findAll()` |
| `InsertIntoTable(...)` | `db.dao(schema).insert(entity)` |
| `UpdateTable(...)` | `db.dao(schema).update(entity)` |
| `DeleteRow(...)` | `db.dao(schema).removeById(pk)` |
| `BeginTran()/Commit()/RollBack()` | `auto tx=db.transaction(); tx.commit()/rollback()` |
| `TableResult` / strings | `std::vector<T>`, `std::optional<T>`, `Value`, `Row` |

### Estrategia recomendada

1. Crear copia de seguridad de cada base de datos 1.x.
2. Compilar el proyecto anfitrión con C++17.
3. Reemplazar includes y namespace.
4. Definir clases `struct` para sus tablas clave; usar `DynamicTable` para las demás.
5. Sustituir concatenación de SQL por DAO y expresiones.
6. Si existen tablas antiguas sin STRICT, **no** eliminarlas ni recrearlas; `.create()` con `IF NOT EXISTS` no cambia su esquema.
7. Añadir observers y comprobar el momento de entrega respecto al COMMIT.
8. Ejecutar las pruebas con una copia de datos reales.

### Cambios de comportamiento que debes revisar

- `execute()` lanza `Error` ante fallo y no acepta múltiples sentencias separadas por `;`.
- Las consultas devuelven objetos tipados y distinguen SQL NULL de cadena vacía.
- `DeleteRow` sin filtro del diseño anterior se reemplaza por `deleteAll()` explícito.
- `autoIncrement()` sólo propaga el ID autogenerado si se omite o vale cero.
- El mapping estático requiere que las propiedades del modelo sean accesibles por puntero a miembro.
- Se necesita SQLite >=3.37.0, y 3.54.1 está disponible como descarga de amalgamado fijado.
- La versión 2.0 no incluye un shim automático para nombres y clases del proyecto 1.x.
