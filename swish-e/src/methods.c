/*
** methods.c
*/

#include "swish.h"

#ifdef ALLOW_FILESYSTEM_INDEXING_DATA_SOURCE
extern struct _indexing_data_source_def FileSystemIndexingDataSource;
#endif

#ifdef ALLOW_HTTP_INDEXING_DATA_SOURCE
extern struct _indexing_data_source_def HTTPIndexingDataSource;
#endif

struct _indexing_data_source_def *data_sources[] = {

#ifdef ALLOW_FILESYSTEM_INDEXING_DATA_SOURCE
    &FileSystemIndexingDataSource,
#endif

#ifdef ALLOW_HTTP_INDEXING_DATA_SOURCE
    &HTTPIndexingDataSource,
#endif

    0
};
