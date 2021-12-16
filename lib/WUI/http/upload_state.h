#pragma once

#include "handler.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Uploader;

struct Uploader *uploader_init(const char *boundary, struct HttpHandlers *handlers);
void uploader_feed(struct Uploader *uploader, const char *data, size_t len);
bool uploader_finish(struct Uploader *uploader);
uint16_t uploader_error(const struct Uploader *uploader);

#ifdef __cplusplus
}
#endif
