idf_component_register(SRCS "mog_receive_flow.c"
                       INCLUDE_DIRS "include"
                       REQUIRES mog_core mog_dedup mog_message_store)
