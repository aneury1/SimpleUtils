# Contrato público de simple_orm 2.0.0

## Namespace

Toda la API nueva está bajo `simple_orm`. El include recomendado es `<simple_orm/simple_orm.hpp>`.

## Tipos

```cpp
using Blob = std::vector<std::uint8_t>;
using Value = std::variant<std::nullptr_t,std::int64_t,double,std::string,Blob>;
using Row = std::unordered_map<std::string,Value>;
enum class DataType {Integer,Real,Text,Blob,Any};
```

Correspondencia: enteros, enum y bool → INTEGER; float/double → REAL; `std::string` → TEXT; `Blob` → BLOB; `std::optional<T>` → mismo tipo o NULL. BOOL se almacena como 0/1. Los tipos de datos dinámicos se verifican antes de cada INSERT/UPDATE. Los datos existentes se validan también al mapearlos hacia C++.

### Base de datos

```cpp
struct DatabaseOptions {
    std::string path=":memory:";
    bool foreignKeys=true;
    int busyTimeoutMs=5000;
    JournalMode journalMode=JournalMode::Default;
};
class Database {
public:
    explicit Database(DatabaseOptions = {});
    void open(); void close(); bool isOpen() const noexcept;
    int execute(const std::string& singleSqlStatement);
    std::unique_ptr<Statement> prepare(const std::string& sql);
    Transaction transaction();
    template<typename T> Dao<T> dao(const TableSchema<T>&);
    DynamicDao dao(const DynamicTable&);
    bool hasTable(const std::string&) const;
    void createTableIfNotExists(const std::string&, const std::vector<std::string>&, bool strict=true);
    void dropTableIfExists(const std::string&);
    Subscription observeDatabase(EventCallback);
    Subscription observeTable(const std::string&,EventCallback);
    Subscription observeColumn(const std::string&,const std::string&,EventCallback);
};
```

`execute()` permite **una** sentencia que no devuelva filas. Para un SELECT o PRAGMA que devuelve filas, utiliza `prepare()` y `Statement::next()`. Todas las sentencias vinculadas utilizan `bind(1,Value)`, `bind(2,Value)`... con índices 1-based.

`Database`, `Statement` y `Transaction` no son copiables. Un `Statement` no puede sobrevivir a su `Database`. No se debe cerrar la base de datos mientras existan sentencias abiertas. Una conexión no debe accederse simultáneamente desde varios hilos; usar una conexión por hilo y coordinar los accesos.

### Tablas estáticas

```cpp
struct Contact {
    std::int64_t id=0;
    std::string name;
    std::optional<std::string> phone;
    bool active=true;
};

auto table=makeTable<Contact>("contacts",
    column<&Contact::id>("id").primaryKey().autoIncrement(),
    column<&Contact::name>("name").notNull().unique(),
    column<&Contact::phone>("phone"),
    column<&Contact::active>("active").defaultValue(true));

table.create(db); // STRICT predeterminado
```

Column Builder: `.nullable()`, `.notNull()`, `.primaryKey()`, `.autoIncrement()`, `.unique()`, `.defaultValue(value)`, `.references(table,column)` y `.onDeleteCascade()`.

`TableSchema<T>::create()` crea la tabla si falta; no comprueba ni modifica la estructura existente, ni realiza migraciones. Sólo se admite una clave primaria por tabla en esta versión. El autoincremento requiere `INTEGER PRIMARY KEY`.

### DAO tipado

```cpp
auto dao=db.dao(table);    // equivalente a makeDao(db,table)
Contact c;c.name="Andrea";
std::int64_t rowId=dao.insert(c); // actualiza c.id para autoincremento
std::optional<Contact> one=dao.findById(rowId);
std::vector<Contact> all=dao.findAll();
bool exists=dao.existsById(rowId);
std::int64_t count=dao.count();
c.name="Andrea G";
bool changed=dao.update(c); // actualiza todos los campos que NO son PK
bool removed=dao.removeById(rowId);
int deleted=dao.deleteAll(); // operación destructiva explícita
```

`insert()` retorna el último ROWID insertado. Sólo propaga el ID hacia la entidad cuando su columna de PK tiene `.autoIncrement()` y el valor es cero. No hay control de columnas modificadas ni UPSERT.

### Consultas tipadas

```cpp
auto results=dao.query()
    .where((field<&Contact::active>()==true) &&
           field<&Contact::name>().startsWith("An"))
    .orderBy(asc(field<&Contact::name>()))
    .limit(25).offset(0).all();

auto first=dao.query().where(field<&Contact::name>()=="Andrea").first();
auto total=dao.query().where(field<&Contact::active>()==true).count();
```

Métodos: `.where(Condition)`, `.orderBy(Sort)`, `.limit(int64)`, `.offset(int64)`, `.all()`, `.first()`, `.count()`, `.exists()`, `.observe(callback, emitInitial=true)`.

Column expressions: `==`, `!=`, `<`, `<=`, `>`, `>=`, `eq`, `ne`, `lt`, `le`, `gt`, `ge`, `.isNull()`, `.isNotNull()`; para TEXT: `.startsWith(text)`, `.contains(text)` con escape de comodines LIKE. Composición de condiciones con `&&` y `||`.

### Tabla dinámica y DynamicDao

```cpp
DynamicTable settings("settings");
settings.add(column<std::int64_t>("id").primaryKey().autoIncrement());
settings.add(column<std::string>("key").notNull().unique());
settings.add(column<std::optional<std::string>>("value"));
settings.create(db);
auto dao=db.dao(settings);
auto id=dao.insert({{"key",toValue("host")},{"value",toValue("localhost")}});
auto row=dao.findById(id);
std::vector<Row> rows=dao.findAll();
dao.update(id,{{"value",toValue("127.0.0.1")}});
dao.removeById(id);
```

Tipos de `Row` se validan estrictamente según esquema. La validación se aplica a los campos aportados, el resto se delega a DEFAULT/NOT NULL de SQLite. `DynamicDao` aún no incluye Query Builder para esquemas descubiertos en tiempo de ejecución.

### Transacciones

```cpp
{
   auto tx=db.transaction();
   dao.insert(first);
   dao.insert(second);
   tx.commit();
} // sin commit se hace rollback
```

Sólo transacciones de nivel superior, `BEGIN IMMEDIATE`. No se admiten transacciones anidadas ni savepoints en 2.0.0.

### Errores

`simple_orm::Error` deriva de `std::runtime_error`, e incluye `int code()` con el código SQLite cuando existe. Todos los métodos de datos fallidos lanzan `Error`; los listeners que lanzan excepciones son aislados para que una operación SQL ya confirmada no se presente como fallo.

### Agrupación de esquemas (`Schema`)

```cpp
Schema schema("my_app", 1); // versión de metadatos, NO motor de migraciones
schema.add(contactsTable).add(settingsTable);
schema.createMissing(db);    // transacción única para crear tablas faltantes
```

La función agrupa `CREATE TABLE IF NOT EXISTS` dentro de una transacción. No inspecciona diferencias de columnas ni persiste la versión de esquema, funciones pendientes para un motor de migraciones.
