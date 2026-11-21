# Pruebas automatizadas

Sin GoogleTest ni dependencias adicionales: `tests/test_main.cpp` contiene las pruebas C++17 y se integra con CTest.

## Compilar

```bash
cmake -S . -B build -DSIMPLE_ORM_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Casos de prueba incluidos

1. Creación de tabla STRICT y CRUD sobre `Dao<Person>`.
2. Query Builder: AND, filtros numéricos, LIKE, ordenación, paginación, NULL.
3. Prevención de inyección SQL en datos parametrizados.
4. CRUD sobre tablas dinámicas y valores `Row` tipados.
5. Validación estricta de valores dinámicos y nombres de columna desconocidos.
6. Eventos observados a nivel BD, tabla y columna, con old/new values.
7. COMMIT/ROLLBACK: no emitir cambios de transacciones revertidas.
8. Prepared Statements y captura de cambios de SQL directo (misma conexión).
9. Escapes seguros de nombres SQL y columna con palabra reservada.
10. Claves foráneas `ON DELETE CASCADE`.
11. Eventos de creación/eliminación y desuscripción.
12. Manejo de restricciones UNIQUE dentro de una transacción.
13. NULL frente a TEXT y eliminación explícita de todos los registros.
14. Esquema inválido y rechazo de multi-SQL.
15. Live Query: nueva consulta tras commit y ningún evento tras rollback.
16. BLOB vacío.
17. Callback de observer que ejecuta una escritura adicional.
18. Agrupación `Schema` con creación idempotente de tablas faltantes.

### Resultado de validación local

- GCC 14.2.0 / CMake, Ubuntu-like Linux.
- SQLite 3.46.1 del sistema con `ENABLE_PREUPDATE_HOOK`.
- 18 pruebas internas pasan; CTest ejecuta un binario agregado.
- Ejemplo `examples/contacts.cpp` ejecutado correctamente.

El modo sin preupdate también se prueba simulando `-DSIMPLE_ORM_SQLITE_LINK_PREUPDATE=OFF -DSIMPLE_ORM_REQUIRE_PREUPDATE=OFF`.

La ruta de CMake que descarga SQLite 3.54.1 no pudo ser probada en esta sesión por falta de acceso de red desde el entorno de compilación. No se han verificado aún compilaciones nativas de Windows ni Android.
