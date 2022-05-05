idf_component_register(
    SRCS
        "src/main.c"

        "src/lib/bme68x.c"

        "src/bme680/bme680_task.c"
    INCLUDE_DIRS
        "."
)
