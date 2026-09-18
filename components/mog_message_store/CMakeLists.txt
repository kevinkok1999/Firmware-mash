idf_component_register(
    SRCS
        "mog_store_snapshot.c"
        "mog_store_journal.c"
        "mog_store_state.c"
        "mog_receiver_store.c"
    INCLUDE_DIRS "include"
    REQUIRES mog_core
)
