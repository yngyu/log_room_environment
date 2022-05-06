idf_component_register(
    SRCS
        "src/main.c"

        "src/bme680/bme680_task.c"
        "src/mg812/mg812_task.c"

        "src/lib/bme68x.c"
    INCLUDE_DIRS
        "."
)
