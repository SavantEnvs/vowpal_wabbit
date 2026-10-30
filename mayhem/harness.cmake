# Injected into VW's own top-level CMake run via -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES (see build.sh) so the
# harnesses are built as ordinary CMake targets against VW's own targets (same defines, include dirs and
# transitive dependencies as the library) without editing any upstream CMakeLists.txt.
function(mayhem_add_harnesses)
  set(harnesses fuzz_text_parser fuzz_json_parser fuzz_model_load fuzz_cache_parser)
  foreach(h ${harnesses})
    foreach(variant "" "-standalone")
      set(tgt ${h}${variant})
      add_executable(${tgt} ${MAYHEM_DIR}/${h}.cc ${MAYHEM_LSAN_OFF_SRC})
      target_link_libraries(${tgt} PRIVATE vw_text_parser vw_json_parser vw_cache_parser vw_core)
      set_target_properties(${tgt} PROPERTIES
        CXX_STANDARD ${VW_CXX_STANDARD} CXX_STANDARD_REQUIRED ON
        RUNTIME_OUTPUT_DIRECTORY ${MAYHEM_OUT_DIR})
      if(variant STREQUAL "")
        target_link_options(${tgt} PRIVATE ${MAYHEM_LIB_FUZZING_ENGINE})
      else()
        target_link_libraries(${tgt} PRIVATE ${MAYHEM_STANDALONE_MAIN_OBJ})
      endif()
    endforeach()
  endforeach()
endfunction()

cmake_language(DEFER CALL mayhem_add_harnesses)
