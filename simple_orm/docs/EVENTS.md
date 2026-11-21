# Live Events y Live Query

## Qué observa el motor

`simple_orm` utiliza `sqlite3_preupdate_hook` cuando se enlaza con una biblioteca SQLite compilada con `ENABLE_PREUPDATE_HOOK`. Emite eventos de cambio de registros y, si conoce la estructura de la tabla, eventos de columna con `oldValue` y `newValue`. Sin preupdate, puede compilarse con `-DSIMPLE_ORM_REQUIRE_PREUPDATE=OFF`, usando `sqlite3_update_hook` para eventos de **fila solamente**.

### Tipos de eventos

`EventKind`: `Insert`, `Update`, `Delete`, `TableCreated`, `TableDropped`, `SchemaChanged`, `Opened`, `Closed`, `Committed`, `RolledBack`.

`Event` contiene `kind`, `database`, `table`, `column` (vacía para evento de fila), `rowId`, `std::optional<Value> oldValue`, `std::optional<Value> newValue`.

**Matiz**: `SchemaChanged` está reservado para una futura capa de migraciones; los DDL genéricos emitidos por `db.execute()` no crean un evento de estructura detallado. `TableCreated`/`TableDropped` se emiten cuando se utilizan las funciones de esquema integradas.

### Ejemplo de observadores

```cpp
auto dbWatch=db.observeDatabase([](const Event& e){ /* cambios de fila/tabla y estado */ });
auto tableWatch=db.observeTable("users",[](const Event& e){ /* cambios de filas */ });
auto columnWatch=db.observeColumn("users","name",[](const Event& e){
    if(e.oldValue) { auto old=*e.oldValue; }
    if(e.newValue) { auto now=*e.newValue; }
});
// Mantener los objetos Subscription vivos mientras interese observar.
columnWatch.unsubscribe(); // desuscripción explícita
```

Se envía un evento de fila por cada INSERT/UPDATE/DELETE, además de un evento por columna cuyo valor haya cambiado. Para INSERT y DELETE, los valores anteriores o nuevos que no existan no estarán presentes (`std::nullopt`), diferenciándose del valor SQL NULL (`Value{nullptr}`).

Los eventos de base/tabla reciben eventos de fila/estructura, no una repetición por columna; sólo el observador de columna recibe eventos de valores individuales.

### Live Query

```cpp
auto live=dao.query()
    .where(field<&User::active>()==true)
    .observe([](const std::vector<User>& result){
        // Actualizar lista/modelo de presentación
    });
```

`observe()` ejecuta la consulta al suscribirse (`emitInitial=true`) y vuelve a ejecutarla después de operaciones sobre la tabla observada. El callback recibe el resultado completo, no un diff incremental. No hace comprobación de equivalencia ni cacheo de resultados. En un evento de modificación que no altere el resultado, el callback también se ejecuta.

### Semántica transaccional

- Los eventos se acumulan durante la transacción y se entregan **después de COMMIT**.
- En `ROLLBACK` se descartan los eventos de cambios en filas/columnas. El observador global puede recibir `RolledBack`.
- Fuera de una transacción explícita, se notifican al terminar cada sentencia confirmada.
- Las notificaciones son **síncronas** y ocurren en el hilo que ejecuta la operación, no en un worker independiente.
- Las excepciones lanzadas por callbacks se silencian intencionalmente para no dar impresión de error después del commit. Los consumidores deben controlar/registrar sus propias excepciones.
- Los eventos provocados por cambios dentro de listeners se encolan para un siguiente lote (con un límite de protección de 100 lotes por ciclo).

### Alcance y restricciones (importante)

- Sólo se observan cambios ejecutados **en la misma conexión** SQLite. No son notificaciones entre procesos, ni red, ni cambios desde otras conexiones, ni un bus persistente.
- En SQLite, `preupdate_hook` cubre las modificaciones de filas que dispara el motor, incluyendo triggers en esa conexión; existen excepciones y casos especiales en la documentación del propio hook.
- Las columnas observadas se obtienen de `PRAGMA table_info`; en estructuras con columnas generadas/ocultas el índice de preupdate podría no corresponder y esas estructuras no están soportadas por Live Events de columna en esta versión.
- No hay soporte para VACUUM, ALTER TABLE genérico y metadata detallada desde SQL arbitrario.
- `Subscription` es RAII y debe destruirse antes de destruir cualquier dato capturado por referencia dentro de su callback.
- Una conexión SQLite no debe compartirse simultáneamente entre hilos, incluso si su motor ha sido configurado en modo FULLMUTEX: `simple_orm` no sincroniza su cola de eventos con operaciones de otras hebras.
- No se debe destruir el `Database` antes de destruir el `Dao`/`Query` que lo utilizan.
