if(NOT MeshMC_DISABLE_JAVA_DOWNLOADER)
    target_sources(MeshMC_logic PRIVATE
        JavaRuntime.h
        JavaDownloadTask.h
        JavaDownloadTask.cpp
    )
endif()
