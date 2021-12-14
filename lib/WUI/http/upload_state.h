#pragma once

#include "handler.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Uploader;

struct Uploader *uploader_init(const char *boundary, HttpHandlers *handlers);
// TODO: Error handling?
void upleader_feed(struct Uploader *uploader, const char *data, size_t len);
void uploader_finish(struct Uploader *uploader);

#ifdef __cplusplus
}
#endif
