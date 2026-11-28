# Use the system Python SQLite module to regenerate catalogs from maintained SQL.
find_package(Python3 3 REQUIRED COMPONENTS Interpreter)
function(hiasm_database output)
    add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/${output}"
        COMMAND Python3::Interpreter "${PROJECT_SOURCE_DIR}/tools/build_database.py"
            "${CMAKE_BINARY_DIR}/${output}" ${ARGN}
        DEPENDS tools/build_database.py ${ARGN}
        VERBATIM)
    set_property(GLOBAL APPEND PROPERTY HIASM_DATABASES "${CMAKE_BINARY_DIR}/${output}")
endfunction()

hiasm_database(int/hiasm.db "${PROJECT_SOURCE_DIR}/resources/hiasm.db"
    "${PROJECT_SOURCE_DIR}/packs/cpp/cpp_package.sql")
foreach(pack _base cpp web modules)
    hiasm_database("packs/${pack}/elements.db" "${PROJECT_SOURCE_DIR}/packs/${pack}/elements.sql")
endforeach()
get_property(databases GLOBAL PROPERTY HIASM_DATABASES)
add_custom_target(hiasm_catalogs ALL DEPENDS ${databases})
add_dependencies(hiasm-qt-window hiasm_catalogs)

install(DIRECTORY packs/ DESTINATION packs
    PATTERN "build" EXCLUDE PATTERN "elements.db" EXCLUDE
    PATTERN ".goutputstream*" EXCLUDE
    REGEX "/(new|example|tutorial|plugs)/[^/]*[.](exe|o|obj|dll|so)$" EXCLUDE)
foreach(pack _base cpp web modules)
    install(FILES "${CMAKE_BINARY_DIR}/packs/${pack}/elements.db" DESTINATION "packs/${pack}")
endforeach()
foreach(pack cpp web modules)
    install(FILES "$<TARGET_FILE:rtcg_codegen>" DESTINATION "packs/${pack}/compiler")
endforeach()
install(FILES "${CMAKE_BINARY_DIR}/int/hiasm.db" resources/hiasm.sql DESTINATION int)
install(FILES src/qt/core_widgets.cpp src/qt/core_widgets.h src/qt/core_application.cpp DESTINATION packs/cpp/qt/runtime)
set(ruffle_dist "${PROJECT_SOURCE_DIR}/thirdparty/ruffle/web/packages/selfhosted/dist")
install(DIRECTORY "${ruffle_dist}/" DESTINATION packs/cpp/qt/ruffle OPTIONAL
    FILES_MATCHING PATTERN "*.js" PATTERN "*.wasm" PATTERN "LICENSE*")
install(FILES LICENSE README.md DESTINATION .)
install(FILES src/RTCG/LICENSE DESTINATION licenses/RTCG)
if(UNIX AND NOT APPLE)
    install(FILES resources/linux/applications/hiasm.desktop DESTINATION share/applications)
    install(FILES resources/icons/hiasm.png DESTINATION share/pixmaps)
endif()
