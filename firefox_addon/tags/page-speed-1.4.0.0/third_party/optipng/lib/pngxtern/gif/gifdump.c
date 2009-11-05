/*
 * gifdump.c
 *
 * Copyright (C) 2003-2009 Cosmin Truta.
 * This software is distributed under the same licensing and warranty terms
 * as gifread.c.
 */


#include <stdio.h>
#include "gifread.h"


static int exitCode = 0;


void GIFDump(const char *filename)
{
    FILE *stream;
    struct GIFScreen screen;
    struct GIFImage image;
    struct GIFExtension ext;
    struct GIFGraphicCtlExt graphicExt;

    stream = fopen(filename, "rb");
    if (stream == NULL)
    {
        fprintf(stderr, "Error: Can't open %s\n", filename);
        exitCode = 1;
        return;
    }

    printf("File: %s\n", filename);

    GIFReadScreen(&screen, stream);
    printf("Screen: %u x %u\n", screen.Width, screen.Height);
    if (screen.GlobalColorFlag)
        printf("  Global colors: %u\n", screen.GlobalNumColors);
    if (screen.PixelAspectRatio != 0)
        printf("  Pixel aspect ratio = %u\n", screen.PixelAspectRatio);

    GIFInitImage(&image, &screen, NULL);
    GIFInitExtension(&ext, &screen, NULL, 0);

    for ( ; ; )
    {
        switch (GIFReadNextBlock(&image, &ext, stream))
        {
        case GIF_TERMINATOR:  /* ';' */
            printf("\n");
            fclose(stream);
            return;
        case GIF_IMAGE:       /* ',' */
           printf("Image: %u x %u @ (%u, %u)\n",
               image.Width, image.Height, image.LeftPos, image.TopPos);
           if (image.LocalColorFlag)
               printf("  Local colors: %u\n", image.LocalNumColors);
           printf("  Interlaced: %s\n", image.InterlaceFlag ? "YES" : "NO");
           break;
        case GIF_EXTENSION:   /* '!' */
            if (ext.Label == GIF_GRAPHICCTL)
            {
                GIFGetGraphicCtl(&ext, &graphicExt);
                printf("Graphic Control Extension: 0x%02X\n", ext.Label);
                printf("  Disposal method: %u\n", graphicExt.DisposalMethod);
                printf("  User input flag: %u\n", graphicExt.InputFlag);
                printf("  Delay time     : %u\n", graphicExt.DelayTime);
                if (graphicExt.TransparentFlag)
                    printf("  Transparent    : %u\n", graphicExt.Transparent);
            }
            else
                printf("Extension: 0x%02X\n", ext.Label);
        }
    }
}


int main(int argc, char *argv[])
{
    int i;

    if (argc <= 1)
    {
        fprintf(stderr, "Usage: gifdump <files.gif...>\n");
        return 1;
    }

    for (i = 1; i < argc; ++i)
        GIFDump(argv[i]);

    return exitCode;
}
