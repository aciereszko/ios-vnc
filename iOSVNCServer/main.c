//
//  main.c
//  iOSVNCServer
//
//  Created by Michal Pietras on 28/06/2016.
//  Copyright © 2016 TestingBot. All rights reserved.
//

#include <stdio.h>
#include <stdint.h>

#include <rfb/rfb.h>
#include <rfb/keysym.h>
#include <png.h>

#include "iosscreenshot.h"

int extract_png(png_bytep png, png_size_t png_size,
                 png_uint_32 *width, png_uint_32 *height,
                 png_bytep *raw, png_size_t *raw_size);

int main(int argc,char** argv) {
    rfbScreenInfoPtr rfbScreen;
    iosss_handle_t handle;
    void *imgData = NULL;
    size_t imgSize = 0;
    uint32_t width, height;
    void *rawData = NULL;
    size_t rawSize = 0;

    if (!(handle = iosss_create())) {
        fputs("ERROR: Cannot create an iosss_handle_t.\n", stderr);
        return -1;
    }
    if (iosss_take(handle, &imgData, &imgSize)) {
        fputs("ERROR: Cannot take a screenshot.\n", stderr);
        return -1;
    }
    if (extract_png(imgData, imgSize, &width, &height, (png_bytep *)&rawData, &rawSize)) {
        fputs("ERROR: Cannot decode PNG.\n", stderr);
        return -1;
    }
    free(imgData);

    if (!(rfbScreen = rfbGetScreen(&argc, argv, width, height, 8, 3, 4))) {
        fputs("ERROR: Cannot create a rfbScreenInfoPtr.\n", stderr);
        return -1;
    }
    rfbScreen->desktopName = "TestingBot";
    rfbScreen->alwaysShared = TRUE;
    rfbScreen->port = 5901;

    if (!(rfbScreen->frameBuffer = (char*)malloc(rawSize))) {
        fputs("ERROR: Cannot allocate memory.\n", stderr);
        return -1;
    }
    memcpy(rfbScreen->frameBuffer, rawData, rawSize);
    free(rawData);

    rfbInitServer(rfbScreen);
    rfbRunEventLoop(rfbScreen, -1, TRUE);
    while (rfbIsActive(rfbScreen)) {
        if (iosss_take(handle, &imgData, &imgSize)) {
            fputs("ERROR: Cannot take a screenshot.\n", stderr);
            return -1;
        }
        if (extract_png(imgData, imgSize, &width, &height, (png_bytep *)&rawData, &rawSize)) {
            fputs("ERROR: Cannot decode PNG.\n", stderr);
            return -1;
        }
        free(imgData);
        if (width != rfbScreen->width ||
            height != rfbScreen->height) {
            fputs("ERROR: Unexpected change of the screen size.\n", stderr);
            return -1;
        }
        memcpy(rfbScreen->frameBuffer, rawData, rawSize);
        free(rawData);
        rfbMarkRectAsModified(rfbScreen, 0, 0, width, height);
    }

    return 0;
}

int extract_png(png_bytep png, png_size_t png_size,
                 png_uint_32 *width, png_uint_32 *height,
                 png_bytep *raw, png_size_t *raw_size) {
    png_image image;
    memset(&image, 0, sizeof(image));
    image.version = PNG_IMAGE_VERSION;
    if (png_image_begin_read_from_memory(&image, png, png_size)) {
        image.format = PNG_FORMAT_RGBA;
        *raw_size = PNG_IMAGE_SIZE(image);
        *raw = malloc(*raw_size);
        if (*raw != NULL &&
            png_image_finish_read(&image, NULL, *raw, 0, NULL)) {
            *width = image.width;
            *height = image.height;
        } else {
            *width = 0;
            *height = 0;
            *raw_size = -1;
            free(*raw);
            *raw = NULL;
            return -1;
        }
        png_image_free(&image);
    } else {
        *width = 0;
        *height = 0;
        *raw_size = -1;
        *raw = NULL;
        return -1;
    }
    return 0;
}
