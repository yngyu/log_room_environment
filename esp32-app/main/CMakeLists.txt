idf_component_register(
    SRCS
        "src/main.c"

        "src/lib/bme680.c"

        "src/bme680/bme680_task.c"
    INCLUDE_DIRS
        "."
)
