catalogBuilder = $$hiasmRoot/tools/build_database.py
catalogPython = $$getenv(HIASM_PYTHON)
isEmpty(catalogPython): catalogPython = python

defineTest(hiasmCatalog) {
    catalogId = $$1
    catalogOutput = $$hiasmOutput/$$2
    catalogInputs = $$3
    catalogCommand = $$shell_quote($$shell_path($$catalogPython)) $$shell_quote($$shell_path($$catalogBuilder))
    catalogCommand += $$shell_quote($$shell_path($$catalogOutput))
    for(catalogInput, catalogInputs): catalogCommand += $$shell_quote($$shell_path($$catalogInput))

    $${catalogId}.target = $$catalogOutput
    $${catalogId}.depends = $$catalogBuilder $$catalogInputs
    $${catalogId}.commands = $$catalogCommand
    QMAKE_EXTRA_TARGETS += $$catalogId
    PRE_TARGETDEPS += $$catalogOutput
    export($${catalogId}.target)
    export($${catalogId}.depends)
    export($${catalogId}.commands)
    export(QMAKE_EXTRA_TARGETS)
    export(PRE_TARGETDEPS)
    return(true)
}

hiasmCatalog(hiasmMainCatalog, int/hiasm.db, $$hiasmRoot/resources/hiasm.db $$hiasmRoot/packs/cpp/cpp_package.sql)
for(pack, $$list(_base cpp web modules)) {
    hiasmCatalog(hiasmCatalog_$$pack, packs/$$pack/elements.db, $$hiasmRoot/packs/$$pack/elements.sql)
}
