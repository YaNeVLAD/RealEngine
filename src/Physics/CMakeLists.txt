re_add_module(RePhysics)

target_link_libraries(RePhysics
        PUBLIC
        ReCore

        Jolt
)

target_compile_definitions(RePhysics PRIVATE RE_PHYSICS_EXPORTS)