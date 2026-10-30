target_sources(MeshMC_logic PRIVATE
    # network stuffs
    ByteArraySink.h
    ChecksumValidator.h
    Download.cpp
    Download.h
    FileSink.cpp
    FileSink.h
    HttpMetaCache.cpp
    HttpMetaCache.h
    MetaCacheSink.cpp
    MetaCacheSink.h
    NetAction.h
    NetJob.cpp
    NetJob.h
    # The one API question that does not fit in a URL, and so cannot be a
    # Net::Download. See JsonPost.h.
    JsonPost.cpp
    JsonPost.h
    PasteUpload.cpp
    PasteUpload.h
    Sink.h
    Validator.h
)

add_unit_test(FileSink
    SOURCES FileSink_test.cpp
    LIBS MeshMC_logic
    )