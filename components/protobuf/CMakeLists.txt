set(SOURCES cast_channel.pb.c)

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS "."
                       REQUIRES
                           nanopb
                           )
