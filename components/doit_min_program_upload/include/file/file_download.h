#ifndef __FILE_DOWNLOAD_H__
#define __FILE_DOWNLOAD_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "file_common.h"

    // void doit_file_decode(lv_obj_t *psd_obj_);  // Deprecated: wrong signature
    void doit_file_decode(void);
    // doit_file_result_t doit_file_download(const char *url);
    doit_file_result_t doit_file_download(const char *url, const char *dir_name);

#ifdef __cplusplus
}
#endif

#endif // __FILE_DOWNLOAD__