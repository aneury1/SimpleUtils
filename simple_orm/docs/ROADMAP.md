# Roadmap / Alcance de versión 2.0.0

## Implementado

- Namespace, proyecto y headers `simple_orm` exclusivamente.
- C++17, CMake, SQLite >=3.37.0 y descarga optativa de SQLite 3.54.1.
- Tablas STRICT, columnas estructuradas, restricciones PK/UNIQUE/NOT NULL/DEFAULT/FK/CASCADE.
- ORM estático `TableSchema<T>` con miembros C++.
- ORM dinámico `DynamicTable` / `DynamicDao`.
- Agrupación `Schema` con creación idempotente y transaccional de tablas faltantes (sin migración automática).
- CRUD genérico con parámetros vinculados y sin concatenación de valores.
- Query Builder de filtros, LIKE, orden, paginación, count, first.
- Observación de BD, tabla y columna, Live Query, suscripciones RAII.
- Transacciones `BEGIN IMMEDIATE`, commit y rollback RAII.
- Pruebas automáticas, documentación y ejemplo.

## Pendiente (no afirmar como implementado)

- Engine de migraciones de esquemas, versiones y `ALTER TABLE` seguro.
- JOIN, GROUP BY, HAVING, agregaciones, DISTINCT y subconsultas en el Query Builder.
- Query Builder para `DynamicDao` (actualmente CRUD completo).
- Relaciones como `hasMany/belongsTo`, carga diferida, modelos anidados.
- UPSERT, insertMany/batch y control de campos modificados.
- Índices compuestos y claves primarias compuestas.
- Observadores distribuidos/multiconexión; SQLite cross-process change notification.
- Column events para columnas generadas/ocultas y metadata DDL detallada.
- Multithreading real, connection pool, background dispatcher.
- Conversión oficial de `chrono`, UUID y decimal exacto.
- Auditoría persistida, reintentos de callback, registro de errores de listeners.
- Shim automático para todas las firmas 1.x.

### Limitación semántica destacada

SQLite `STRICT` admite algunas conversiones de tipos que no pierden información. `DynamicDao` valida exactamente el tipo variante recibido, pero al ejecutar SQL directamente sigue aplicando las reglas nativas del motor. `TableSchema<T>` no valida la estructura de una tabla preexistente: usar migraciones explícitas cuando se diseñe ese módulo.
