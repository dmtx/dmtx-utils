/*
libdmtx - Data Matrix Encoding/Decoding Library

Copyright (C) 2008, 2009 Mike Laughton
Copyright (C) 2008 Ryan Raasch
Copyright (C) 2008 Olivier Guilyardi

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

Contact: mike@dragonflylogic.com
*/

#include "dmtxtract.h"

char *programName;

#define NN                      255

/**
 * \brief  Read color of Data Matrix module location
 * \param  dec
 * \param  reg
 * \param  symbolRow
 * \param  symbolCol
 * \param  sizeIdx
 * \return Averaged module color
 */
static int
ReadModuleColor(DmtxDecode *dec, DmtxRegion *reg, int symbolRow, int symbolCol,
      int sizeIdx, int colorPlane)
{
   int i;
   int symbolRows, symbolCols;
   int color, colorTmp;
   double sampleX[] = { 0.5, 0.4, 0.5, 0.6, 0.5 };
   double sampleY[] = { 0.5, 0.5, 0.4, 0.5, 0.6 };
   DmtxVector2 p;

   symbolRows = dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, sizeIdx);
   symbolCols = dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, sizeIdx);

   color = 0;
   for(i = 0; i < 5; i++) {

      p.X = (1.0/symbolCols) * (symbolCol + sampleX[i]);
      p.Y = (1.0/symbolRows) * (symbolRow + sampleY[i]);

      dmtxMatrix3VMultiplyBy(&p, reg->fit2raw);

      //fprintf(stdout, "%dx%d\n", (int)(p.X + 0.5), (int)(p.Y + 0.5));

      dmtxDecodeGetPixelValue(dec, (int)(p.X + 0.5), (int)(p.Y + 0.5),
            colorPlane, &colorTmp);
      color += colorTmp;
   }
   //fprintf(stdout, "\n");
   return color/5;
}

/**
 * \brief  Increment counters used to determine module values
 * \param  img
 * \param  reg
 * \param  tally
 * \param  xOrigin
 * \param  yOrigin
 * \param  mapWidth
 * \param  mapHeight
 * \param  dir
 * \return void
 */
static void
TallyModuleJumps(DmtxDecode *dec, DmtxRegion *reg, int tally[][24], int xOrigin, int yOrigin, int mapWidth, int mapHeight, DmtxDirection dir)
{
   int extent, weight;
   int travelStep;
   int symbolRow, symbolCol;
   int mapRow, mapCol;
   int lineStart, lineStop;
   int travelStart, travelStop;
   int *line, *travel;
   int jumpThreshold;
   int darkOnLight;
   int color;
   int statusPrev, statusModule;
   int tPrev, tModule;

   assert(dir == DmtxDirUp || dir == DmtxDirLeft || dir == DmtxDirDown || dir == DmtxDirRight);

   travelStep = (dir == DmtxDirUp || dir == DmtxDirRight) ? 1 : -1;

   /* Abstract row and column progress using pointers to allow grid
      traversal in all 4 directions using same logic */

   if((dir & DmtxDirHorizontal) != 0x00) {
      line = &symbolRow;
      travel = &symbolCol;
      extent = mapWidth;
      lineStart = yOrigin;
      lineStop = yOrigin + mapHeight;
      travelStart = (travelStep == 1) ? xOrigin - 1 : xOrigin + mapWidth;
      travelStop = (travelStep == 1) ? xOrigin + mapWidth : xOrigin - 1;
   }
   else {
      assert(dir & DmtxDirVertical);
      line = &symbolCol;
      travel = &symbolRow;
      extent = mapHeight;
      lineStart = xOrigin;
      lineStop = xOrigin + mapWidth;
      travelStart = (travelStep == 1) ? yOrigin - 1: yOrigin + mapHeight;
      travelStop = (travelStep == 1) ? yOrigin + mapHeight : yOrigin - 1;
   }


   darkOnLight = (int)(reg->offColor > reg->onColor);
   jumpThreshold = abs((int)(0.4 * (reg->offColor - reg->onColor) + 0.5));

   assert(jumpThreshold >= 0);

   for(*line = lineStart; *line < lineStop; (*line)++) {

      /* Capture tModule for each leading border module as normal but
         decide status based on predictable barcode border pattern */



      *travel = travelStart;
      color = ReadModuleColor(dec, reg, symbolRow, symbolCol, reg->sizeIdx, reg->flowBegin.plane);
      tModule = (darkOnLight) ? reg->offColor - color : color - reg->offColor;

      statusModule = (travelStep == 1 || (*line & 0x01) == 0) ? DmtxModuleOnRGB : DmtxModuleOff;

      weight = extent;

      while((*travel += travelStep) != travelStop) {

         tPrev = tModule;
         statusPrev = statusModule;

         /* For normal data-bearing modules capture color and decide
            module status based on comparison to previous "known" module */

         color = ReadModuleColor(dec, reg, symbolRow, symbolCol, reg->sizeIdx, reg->flowBegin.plane);
         tModule = (darkOnLight) ? reg->offColor - color : color - reg->offColor;

         if(statusPrev == DmtxModuleOnRGB) {
            if(tModule < tPrev - jumpThreshold){
               statusModule = DmtxModuleOff;
            } else {
               statusModule = DmtxModuleOnRGB;
            }
         }
         else if(statusPrev == DmtxModuleOff) {
            if(tModule > tPrev + jumpThreshold) {
               statusModule = DmtxModuleOnRGB;
            } else {
               statusModule = DmtxModuleOff;
            }
         }

         mapRow = symbolRow - yOrigin;
         mapCol = symbolCol - xOrigin;
         assert(mapRow < 24 && mapCol < 24);

         if(statusModule == DmtxModuleOnRGB){
            tally[mapRow][mapCol] += (2 * weight);
         }

         weight--;
      }

      assert(weight == 0);
   }
}

/**
 * \brief  Populate array with codeword values based on module colors
 * \param  msg
 * \param  img
 * \param  reg
 * \return DmtxPass | DmtxFail
 */
static DmtxPassFail
PopulateArrayFromMatrix(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg)
{
   //fprintf(stdout, "libdmtx::PopulateArrayFromMatrix()\n");
   int weightFactor;
   int mapWidth, mapHeight;
   int xRegionTotal, yRegionTotal;
   int xRegionCount, yRegionCount;
   int xOrigin, yOrigin;
   int mapCol, mapRow;
   int colTmp, rowTmp, idx;
   int tally[24][24]; /* Large enough to map largest single region */

/* memset(msg->array, 0x00, msg->arraySize); */

   /* Capture number of regions present in barcode */
   xRegionTotal = dmtxGetSymbolAttribute(DmtxSymAttribHorizDataRegions, reg->sizeIdx);
   yRegionTotal = dmtxGetSymbolAttribute(DmtxSymAttribVertDataRegions, reg->sizeIdx);

   /* Capture region dimensions (not including border modules) */
   mapWidth = dmtxGetSymbolAttribute(DmtxSymAttribDataRegionCols, reg->sizeIdx);
   mapHeight = dmtxGetSymbolAttribute(DmtxSymAttribDataRegionRows, reg->sizeIdx);

   weightFactor = 2 * (mapHeight + mapWidth + 2);
   assert(weightFactor > 0);

   /* Tally module changes for each region in each direction */
   for(yRegionCount = 0; yRegionCount < yRegionTotal; yRegionCount++) {

      /* Y location of mapping region origin in symbol coordinates */
      yOrigin = yRegionCount * (mapHeight + 2) + 1;

      for(xRegionCount = 0; xRegionCount < xRegionTotal; xRegionCount++) {

         /* X location of mapping region origin in symbol coordinates */
         xOrigin = xRegionCount * (mapWidth + 2) + 1;
         //fprintf(stdout, "libdmtx::PopulateArrayFromMatrix::xOrigin: %d\n", xOrigin);

         memset(tally, 0x00, 24 * 24 * sizeof(int));
         TallyModuleJumps(dec, reg, tally, xOrigin, yOrigin, mapWidth, mapHeight, DmtxDirUp);
         TallyModuleJumps(dec, reg, tally, xOrigin, yOrigin, mapWidth, mapHeight, DmtxDirLeft);
         TallyModuleJumps(dec, reg, tally, xOrigin, yOrigin, mapWidth, mapHeight, DmtxDirDown);
         TallyModuleJumps(dec, reg, tally, xOrigin, yOrigin, mapWidth, mapHeight, DmtxDirRight);

         /* Decide module status based on final tallies */
         for(mapRow = 0; mapRow < mapHeight; mapRow++) {
         //for(mapRow = mapHeight-1; mapRow >= 0; mapRow--) {
            for(mapCol = 0; mapCol < mapWidth; mapCol++) {
               
               rowTmp = (yRegionCount * mapHeight) + mapRow;
               rowTmp = yRegionTotal * mapHeight - rowTmp - 1;
               colTmp = (xRegionCount * mapWidth) + mapCol;
               idx = (rowTmp * xRegionTotal * mapWidth) + colTmp;
               //fprintf(stdout, "libdmtx::PopulateArrayFromMatrix::idx: %d @ %d,%d\n", idx, mapCol, mapRow);
               //fprintf(stdout, "%c ",tally[mapRow][mapCol]==DmtxModuleOff ? 'X' : ' ');
               if(tally[mapRow][mapCol]/(double)weightFactor >= 0.5){
                  msg->array[idx] = DmtxModuleOnRGB;
                  //fprintf(stdout, "X ");
               } else {
                  msg->array[idx] = DmtxModuleOff;
                  //fprintf(stdout, "  ");
               }

               msg->array[idx] |= DmtxModuleAssigned;
            }
            //fprintf(stdout, "\n");
         }
      }
   }

   return DmtxPass;
}

/**
 * @brief  Main function for the dmtxread Data Matrix scanning utility.
 * @param  argc count of arguments passed from command line
 * @param  argv list of argument passed strings from command line
 * @return Numeric exit code
 */
int
main(int argc, char *argv[])
{
   char *filePath;
   int i;
   int err;
   int fileIndex, imgPageIndex;
   int fileCount;
   int imgScanCount, pageScanCount;
   int width, height;
   unsigned char *pxl;
   UserOptions opt;
   DmtxTime timeout;
   DmtxImage *img;
   DmtxDecode *dec;
   DmtxRegion *reg;
   DmtxMessage *msg;
   MagickBooleanType success;
   MagickWand *wand;

   opt = GetDefaultOptions();

   err = HandleArgs(&opt, &fileIndex, &argc, &argv);
   if(err != DmtxPass)
      ShowUsage(EX_USAGE);

   fileCount = (argc == fileIndex) ? 1 : argc - fileIndex;

   MagickWandGenesis();

   /* Loop once for each image named on command line */
   imgScanCount = 0;
   for(i = 0; i < fileCount; i++) {

      /* Open image from file or stream (might contain multiple pages) */
      filePath = (argc == fileIndex) ? "-" : argv[fileIndex++];

      wand = NewMagickWand();
      if(wand == NULL) {
         FatalError(EX_OSERR, "Magick error");
      }

      /* XXX note this is not the same as MagickSetImageResolution() ...
       * need to research what this is setting. Could be dots per inch, dots
       * per centimeter, or even dots per "image width" */
      if(opt.dpi != DmtxUndefined) {
         success = MagickSetResolution(wand, (double)opt.dpi, (double)opt.dpi);
         if(success == MagickFalse) {
            CleanupMagick(&wand, DmtxTrue);
            FatalError(EX_OSERR, "Unable to set image resolution");
         }
      }

      success = MagickReadImage(wand, filePath);
      if(success == MagickFalse) {
         CleanupMagick(&wand, DmtxTrue);
         FatalError(EX_OSERR, "Unable to open file \"%s\" for reading", filePath);
      }

      width = MagickGetImageWidth(wand);
      height = MagickGetImageHeight(wand);

      /* Loop once for each page within image */
      MagickResetIterator(wand);
      for(imgPageIndex = 0; MagickNextImage(wand) != MagickFalse; imgPageIndex++) {

         /* If requested, only scan specific page */
         if(opt.page != DmtxUndefined && opt.page - 1 != imgPageIndex)
            continue;

         /* Reset timeout for each new page */
         if(opt.timeoutMS != DmtxUndefined)
            timeout = dmtxTimeAdd(dmtxTimeNow(), opt.timeoutMS);

         /* Allocate memory for pixel data */
         pxl = (unsigned char *)malloc(3 * width * height * sizeof(unsigned char));
         if(pxl == NULL) {
            CleanupMagick(&wand, DmtxFalse);
            FatalError(EX_OSERR, "malloc() error");
         }

         /* Copy pixels to known format */
         success = MagickGetImagePixels(wand, 0, 0, width, height, "RGB", CharPixel, pxl);
         if(success == MagickFalse || pxl == NULL) {
            CleanupMagick(&wand, DmtxTrue);
            FatalError(EX_OSERR, "malloc() error");
         }

         /* Initialize libdmtx image */
         img = dmtxImageCreate(pxl, width, height, DmtxPack24bppRGB);
         if(img == NULL) {
            CleanupMagick(&wand, DmtxFalse);
            FatalError(EX_SOFTWARE, "dmtxImageCreate() error");
         }

         dmtxImageSetProp(img, DmtxPropImageFlip, DmtxFlipNone);

         /* Initialize scan */
         dec = dmtxDecodeCreate(img, opt.shrinkMin);
         if(dec == NULL) {
            CleanupMagick(&wand, DmtxFalse);
            FatalError(EX_SOFTWARE, "decode create error");
         }

         err = SetDecodeOptions(dec, img, &opt);
         if(err != DmtxPass) {
            CleanupMagick(&wand, DmtxFalse);
            FatalError(EX_SOFTWARE, "decode option error");
         }

         /* Find and decode every barcode on page */
         pageScanCount = 0;
         for(;;) {
            /* Find next barcode region within image, but do not decode yet */
            if(opt.timeoutMS == DmtxUndefined)
               reg = dmtxRegionFindNext(dec, NULL);
            else
               reg = dmtxRegionFindNext(dec, &timeout);

            /* Finished file or ran out of time before finding another region */
            if(reg == NULL)
               break;

            /* Decode region based on requested barcode mode */
            if(opt.mosaic == DmtxTrue) {
               msg = dmtxMessageCreate(reg->sizeIdx, DmtxFormatMosaic);

               printf("Not Implemented");
               exit(1);
            }
            else {
               msg = dmtxMessageCreate(reg->sizeIdx, DmtxFormatMatrix);

               if (msg != NULL && PopulateArrayFromMatrix(dec, reg, msg) == DmtxPass) {
                  size_t buff_size = (msg->arraySize + 7) / 8;
                  char buffer[buff_size];

                  memset(&buffer, 0x00, buff_size);

                  for (int j = 0; j < msg->arraySize; j++) 
                     buffer[j/8] |= (msg->array[j] == 0x17 ? 1 : 0) << (j % 8);

                  fwrite(&buffer, sizeof(char), buff_size, stdout);
                  fflush(stdout);   
               }
            }

            if (msg != NULL) {
               PrintStats(dec, reg, msg, imgPageIndex, &opt);   

               pageScanCount++;
               imgScanCount++;

               dmtxMessageDestroy(&msg);
            }
            
            dmtxRegionDestroy(&reg);

            if(opt.stopAfter != DmtxUndefined && imgScanCount >= opt.stopAfter)
               break;
         }

         if(opt.diagnose == DmtxTrue)
            WriteDiagnosticImage(dec, "debug.pnm");

         dmtxDecodeDestroy(&dec);
         dmtxImageDestroy(&img);
         free(pxl);
      }

      CleanupMagick(&wand, DmtxFalse);
   }

   MagickWandTerminus();

   exit((imgScanCount > 0) ? EX_OK : 1);
}

/**
 *
 *
 */
static UserOptions GetDefaultOptions(void)
{
   UserOptions opt;

   memset(&opt, 0x00, sizeof(UserOptions));

   /* Default options */
   opt.codewords = DmtxFalse;
   opt.edgeMin = DmtxUndefined;
   opt.edgeMax = DmtxUndefined;
   opt.scanGap = 2;
   opt.timeoutMS = DmtxUndefined;
   opt.newline = DmtxFalse;
   opt.page = DmtxUndefined;
   opt.squareDevn = DmtxUndefined;
   opt.dpi = DmtxUndefined;
   opt.sizeIdxExpected = DmtxSymbolShapeAuto;
   opt.edgeThresh = 5;
   opt.xMin = NULL;
   opt.xMax = NULL;
   opt.yMin = NULL;
   opt.yMax = NULL;
   opt.correctionsMax = DmtxUndefined;
   opt.diagnose = DmtxFalse;
   opt.mosaic = DmtxFalse;
   opt.stopAfter = DmtxUndefined;
   opt.pageNumbers = DmtxFalse;
   opt.corners = DmtxFalse;
   opt.shrinkMin = 1;
   opt.shrinkMax = 1;
   opt.unicode = DmtxFalse;
   opt.verbose = DmtxFalse;
   opt.gs1 = DmtxUndefined;

   return opt;
}

/**
 * @brief  Set and validate user-requested options from command line arguments.
 * @param  opt runtime options from defaults or command line
 * @param  argcp pointer to argument count
 * @param  argvp pointer to argument list
 * @param  fileIndex pointer to index of first non-option arg (if successful)
 * @return DmtxPass | DmtxFail
 */
static DmtxPassFail HandleArgs(UserOptions *opt, int *fileIndex, int *argcp, char **argvp[])
{
   int i;
   int err;
   int optchr;
   int longIndex;
   char *ptr;

   struct option longOptions[] = {
         {"codewords",        no_argument,       NULL, 'c'},
         {"minimum-edge",     required_argument, NULL, 'e'},
         {"maximum-edge",     required_argument, NULL, 'E'},
         {"gap",              required_argument, NULL, 'g'},
         {"list-formats",     no_argument,       NULL, 'l'},
         {"milliseconds",     required_argument, NULL, 'm'},
         {"newline",          no_argument,       NULL, 'n'},
         {"page",             required_argument, NULL, 'p'},
         {"square-deviation", required_argument, NULL, 'q'},
         {"resolution",       required_argument, NULL, 'r'},
         {"symbol-size",      required_argument, NULL, 's'},
         {"threshold",        required_argument, NULL, 't'},
         {"x-range-min",      required_argument, NULL, 'x'},
         {"x-range-max",      required_argument, NULL, 'X'},
         {"y-range-min",      required_argument, NULL, 'y'},
         {"y-range-max",      required_argument, NULL, 'Y'},
         {"max-corrections",  required_argument, NULL, 'C'},
         {"diagnose",         no_argument,       NULL, 'D'},
         {"mosaic",           no_argument,       NULL, 'M'},
         {"stop-after",       required_argument, NULL, 'N'},
         {"page-numbers",     no_argument,       NULL, 'P'},
         {"corners",          no_argument,       NULL, 'R'},
         {"shrink",           required_argument, NULL, 'S'},
         {"unicode",          no_argument,       NULL, 'U'},
         {"gs1",              required_argument, NULL, 'G'},
         {"verbose",          no_argument,       NULL, 'v'},
         {"version",          no_argument,       NULL, 'V'},
         {"help",             no_argument,       NULL,  0 },
         {0, 0, 0, 0}
   };

   programName = Basename((*argvp)[0]);

   *fileIndex = 0;

   for(;;) {
      optchr = getopt_long(*argcp, *argvp,
            "ce:E:g:lm:np:q:r:s:t:x:X:y:Y:vC:DMN:PRS:G:UV", longOptions, &longIndex);
      if(optchr == -1)
         break;

      switch(optchr) {
         case 0: /* --help */
            ShowUsage(EX_OK);
            break;
         case 'l':
            ListImageFormats();
            exit(EX_OK);
            break;
         case 'c':
            opt->codewords = DmtxTrue;
            break;
         case 'e':
            err = StringToInt(&(opt->edgeMin), optarg, &ptr);
            if(err != DmtxPass || opt->edgeMin <= 0 || *ptr != '\0')
               FatalError(EX_USAGE, _("Invalid edge length specified \"%s\""), optarg);
            break;
         case 'E':
            err = StringToInt(&(opt->edgeMax), optarg, &ptr);
            if(err != DmtxPass || opt->edgeMax <= 0 || *ptr != '\0')
               FatalError(EX_USAGE, _("Invalid edge length specified \"%s\""), optarg);
            break;
         case 'g':
            err = StringToInt(&(opt->scanGap), optarg, &ptr);
            if(err != DmtxPass || opt->scanGap <= 0 || *ptr != '\0')
               FatalError(EX_USAGE, _("Invalid gap specified \"%s\""), optarg);
            break;
         case 'm':
            err = StringToInt(&(opt->timeoutMS), optarg, &ptr);
            if(err != DmtxPass || opt->timeoutMS < 0 || *ptr != '\0')
               FatalError(EX_USAGE, _("Invalid timeout (in milliseconds) specified \"%s\""), optarg);
            break;
         case 'n':
            opt->newline = DmtxTrue;
            break;
         case 'p':
            err = StringToInt(&(opt->page), optarg, &ptr);
            if(err != DmtxPass || opt->page < 1 || *ptr != '\0')
               FatalError(EX_USAGE, _("Invalid page specified \"%s\""), optarg);
            break;
         case 'q':
            err = StringToInt(&(opt->squareDevn), optarg, &ptr);
            if(err != DmtxPass || *ptr != '\0' ||
                  opt->squareDevn < 0 || opt->squareDevn > 90)
               FatalError(EX_USAGE, _("Invalid squareness deviation specified \"%s\""), optarg);
            break;
         case 'r':
            err = StringToInt(&(opt->dpi), optarg, &ptr);
            if(err != DmtxPass || *ptr != '\0' || opt->dpi < 1)
               FatalError(EX_USAGE, _("Invalid resolution specified \"%s\""), optarg);
            break;
         case 's':
            /* Determine correct barcode size and/or shape */
            if(*optarg == 'a') {
               opt->sizeIdxExpected = DmtxSymbolShapeAuto;
            }
            else if(*optarg == 's') {
               opt->sizeIdxExpected = DmtxSymbolSquareAuto;
            }
            else if(*optarg == 'r') {
               opt->sizeIdxExpected = DmtxSymbolRectAuto;
            }
            else {
               for(i = 0; i < DmtxSymbolSquareCount + DmtxSymbolRectCount; i++) {
                  if(strncmp(optarg, symbolSizes[i], 8) == 0) {
                     opt->sizeIdxExpected = i;
                     break;
                  }
               }
               if(i == DmtxSymbolSquareCount + DmtxSymbolRectCount)
                  return DmtxFail;
            }
            break;
         case 't':
            err = StringToInt(&(opt->edgeThresh), optarg, &ptr);
            if(err != DmtxPass || *ptr != '\0' ||
                  opt->edgeThresh < 1 || opt->edgeThresh > 100)
               FatalError(EX_USAGE, _("Invalid edge threshold specified \"%s\""), optarg);
            break;
         case 'x':
            opt->xMin = optarg;
            break;
         case 'X':
            opt->xMax = optarg;
            break;
         case 'y':
            opt->yMin = optarg;
            break;
         case 'Y':
            opt->yMax = optarg;
            break;
         case 'v':
            opt->verbose = DmtxTrue;
            break;
         case 'C':
            err = StringToInt(&(opt->correctionsMax), optarg, &ptr);
            if(err != DmtxPass || opt->correctionsMax < 0 || *ptr != '\0')
               FatalError(EX_USAGE, _("Invalid max corrections specified \"%s\""), optarg);
            break;
         case 'D':
            opt->diagnose = DmtxTrue;
            break;
         case 'M':
            opt->mosaic = DmtxTrue;
            break;
         case 'N':
            err = StringToInt(&(opt->stopAfter), optarg, &ptr);
            if(err != DmtxPass || opt->stopAfter < 1 || *ptr != '\0')
               FatalError(EX_USAGE, _("Invalid count specified \"%s\""), optarg);
            break;
         case 'P':
            opt->pageNumbers = DmtxTrue;
            break;
         case 'R':
            opt->corners = DmtxTrue;
            break;
         case 'S':
            err = StringToInt(&(opt->shrinkMin), optarg, &ptr);
            if(err != DmtxPass || opt->shrinkMin < 1 || *ptr != '\0')
               FatalError(EX_USAGE, _("Invalid shrink factor specified \"%s\""), optarg);

            /* XXX later populate shrinkMax based on specified N-N range */
            opt->shrinkMax = opt->shrinkMin;
            break;
         case 'U':
            opt->unicode = DmtxTrue;
            break;
         case 'G':
            err = StringToInt(&(opt->gs1), optarg, &ptr);
            if(err != DmtxPass || opt->gs1 <= 0 || opt->gs1 > 255 || *ptr != '\0')
               FatalError(EX_USAGE, _("Invalid gs1 character specified \"%s\""), optarg);
            break;
         case 'V':
            fprintf(stderr, "%s version %s\n", programName, DmtxVersion);
            fprintf(stderr, "libdmtx version %s\n", dmtxVersion());
            exit(EX_OK);
            break;
         default:
            return DmtxFail;
            break;
      }
   }
   *fileIndex = optind;

   return DmtxPass;
}

/**
 * @brief  Display program usage and exit with received status.
 * @param  status error code returned to OS
 * @return void
 */
static void
ShowUsage(int status)
{
   if(status != 0) {
      fprintf(stderr, _("Usage: %s [OPTION]... [FILE]...\n"), programName);
      fprintf(stderr, _("Try `%s --help' for more information.\n"), programName);
   }
   else {
      fprintf(stderr, _("Usage: %s [OPTION]... [FILE]...\n"), programName);
      fprintf(stderr, _("\
Scan image FILE for Data Matrix barcodes and print decoded results to\n\
standard output.  Note that %s may find multiple barcodes in one image.\n\
\n\
Example: Scan top third of IMAGE001.png and stop after first barcode is found:\n\
\n\
   %s -n -Y33%% -N1 IMAGE001.png\n\
\n\
OPTIONS:\n"), programName, programName);
      fprintf(stderr, _("\
  -c, --codewords             print codewords extracted from barcode pattern\n\
  -e, --minimum-edge=N        pixel length of smallest expected edge in image\n\
  -E, --maximum-edge=N        pixel length of largest expected edge in image\n\
  -g, --gap=N                 use scan grid with gap of N pixels between lines\n\
  -l, --list-formats          list supported image formats\n"));
      fprintf(stderr, _("\
  -m, --milliseconds=N        stop scan after N milliseconds (per image)\n\
  -n, --newline               print newline character at the end of decoded data\n\
  -p, --page=N                only scan Nth page of images\n\
  -q, --square-deviation=N    allow non-squareness of corners in degrees (0-90)\n\
  -r, --resolution=N          resolution for vector images (PDF, SVG, etc...)\n"));
      fprintf(stderr, _("\
  -s, --symbol-size=[asr|RxC] only consider barcodes of specific size or shape\n\
        a = All sizes         [default]\n\
        s = Only squares\n\
        r = Only rectangles\n\
      RxC = Exactly this many rows and columns (10x10, 8x18, etc...)\n"));
      fprintf(stderr, _("\
  -t, --threshold=N           ignore weak edges below threshold N (1-100)\n\
  -x, --x-range-min=N[%%]      do not scan pixels to the left of N (or N%%)\n\
  -X, --x-range-max=N[%%]      do not scan pixels to the right of N (or N%%)\n\
  -y, --y-range-min=N[%%]      do not scan pixels below N (or N%%)\n\
  -Y, --y-range-max=N[%%]      do not scan pixels above N (or N%%)\n"));
      fprintf(stderr, _("\
  -C, --corrections-max=N     correct at most N errors (0 = correction disabled)\n\
  -D, --diagnose              make copy of image with additional diagnostic data\n\
  -M, --mosaic                interpret detected regions as Data Mosaic barcodes\n\
  -N, --stop-after=N          stop scanning after Nth barcode is returned\n\
  -P, --page-numbers          prefix decoded message with fax/tiff page number\n"));
      fprintf(stderr, _("\
  -R, --corners               prefix decoded message with corner locations\n\
  -S, --shrink=N              internally shrink image by a factor of N\n\
  -U, --unicode               print Extended ASCII in Unicode (UTF-8)\n\
  -G, --gs1=N                 enable GS1 mode and define character to represent FNC1\n\
  -v, --verbose               use verbose messages\n\
  -V, --version               print program version information\n\
      --help                  display this help and exit\n"));
      fprintf(stderr, _("\nReport bugs to <mike@dragonflylogic.com>.\n"));
   }

   exit(status);
}

/**
 *
 *
 */
static DmtxPassFail
SetDecodeOptions(DmtxDecode *dec, DmtxImage *img, UserOptions *opt)
{
   int err;

#define RETURN_IF_FAILED(e) if(e != DmtxPass) { return DmtxFail; }

   err = dmtxDecodeSetProp(dec, DmtxPropScanGap, opt->scanGap);
   RETURN_IF_FAILED(err)

   if(opt->gs1 != DmtxUndefined) {
      err = dmtxDecodeSetProp(dec, DmtxPropFnc1, opt->gs1);
      RETURN_IF_FAILED(err)
   }

   if(opt->edgeMin != DmtxUndefined) {
      err = dmtxDecodeSetProp(dec, DmtxPropEdgeMin, opt->edgeMin);
      RETURN_IF_FAILED(err)
   }

   if(opt->edgeMax != DmtxUndefined) {
      err = dmtxDecodeSetProp(dec, DmtxPropEdgeMax, opt->edgeMax);
      RETURN_IF_FAILED(err)
   }

   if(opt->squareDevn != DmtxUndefined) {
      err = dmtxDecodeSetProp(dec, DmtxPropSquareDevn, opt->squareDevn);
      RETURN_IF_FAILED(err)
   }

   err = dmtxDecodeSetProp(dec, DmtxPropSymbolSize, opt->sizeIdxExpected);
   RETURN_IF_FAILED(err)

   err = dmtxDecodeSetProp(dec, DmtxPropEdgeThresh, opt->edgeThresh);
   RETURN_IF_FAILED(err)

   if(opt->xMin) {
      err = dmtxDecodeSetProp(dec, DmtxPropXmin, ScaleNumberString(opt->xMin, img->width));
      RETURN_IF_FAILED(err)
   }

   if(opt->xMax) {
      err = dmtxDecodeSetProp(dec, DmtxPropXmax, ScaleNumberString(opt->xMax, img->width));
      RETURN_IF_FAILED(err)
   }

   if(opt->yMin) {
      err = dmtxDecodeSetProp(dec, DmtxPropYmin, ScaleNumberString(opt->yMin, img->height));
      RETURN_IF_FAILED(err)
   }

   if(opt->yMax) {
      err = dmtxDecodeSetProp(dec, DmtxPropYmax, ScaleNumberString(opt->yMax, img->height));
      RETURN_IF_FAILED(err)
   }

#undef RETURN_IF_FAILED

   return DmtxPass;
}

/**
 * @brief  Print decoded message to standard output
 * @param  opt runtime options from defaults or command line
 * @param  dec pointer to DmtxDecode struct
 * @return DmtxPass | DmtxFail
 */
static DmtxPassFail
PrintStats(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg,
      int imgPageIndex, UserOptions *opt)
{
   int height;
   int dataWordLength;
   int rotateInt;
   double rotate;
   DmtxVector2 p00, p10, p11, p01;

   height = dmtxDecodeGetProp(dec, DmtxPropHeight);

   p00.X = p00.Y = p10.Y = p01.X = 0.0;
   p10.X = p01.Y = p11.X = p11.Y = 1.0;
   dmtxMatrix3VMultiplyBy(&p00, reg->fit2raw);
   dmtxMatrix3VMultiplyBy(&p10, reg->fit2raw);
   dmtxMatrix3VMultiplyBy(&p11, reg->fit2raw);
   dmtxMatrix3VMultiplyBy(&p01, reg->fit2raw);

   dataWordLength = dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords, reg->sizeIdx);
   if(opt->verbose == DmtxTrue) {

/*    rotate = (2 * M_PI) + (atan2(reg->fit2raw[0][1], reg->fit2raw[1][1]) -
            atan2(reg->fit2raw[1][0], reg->fit2raw[0][0])) / 2.0; */
      rotate = (2 * M_PI) + atan2(p10.Y - p00.Y, p10.X - p00.X);

      rotateInt = (int)(rotate * 180/M_PI + 0.5);
      if(rotateInt >= 360)
         rotateInt -= 360;

      fprintf(stderr, "--------------------------------------------------\n");
      fprintf(stderr, "       Matrix Size: %d x %d\n",
            dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, reg->sizeIdx),
            dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, reg->sizeIdx));
      fprintf(stderr, "    Data Codewords: %d (capacity %d)\n",
            dataWordLength - msg->padCount, dataWordLength);
      fprintf(stderr, "   Error Codewords: %d\n",
            dmtxGetSymbolAttribute(DmtxSymAttribSymbolErrorWords, reg->sizeIdx));
      fprintf(stderr, "      Data Regions: %d x %d\n",
            dmtxGetSymbolAttribute(DmtxSymAttribHorizDataRegions, reg->sizeIdx),
            dmtxGetSymbolAttribute(DmtxSymAttribVertDataRegions, reg->sizeIdx));
      fprintf(stderr, "Interleaved Blocks: %d\n",
            dmtxGetSymbolAttribute(DmtxSymAttribInterleavedBlocks, reg->sizeIdx));
      fprintf(stderr, "    Rotation Angle: %d\n", rotateInt);
      fprintf(stderr, "          Corner 0: (%0.1f, %0.1f)\n", p00.X, height - 1 - p00.Y);
      fprintf(stderr, "          Corner 1: (%0.1f, %0.1f)\n", p10.X, height - 1 - p10.Y);
      fprintf(stderr, "          Corner 2: (%0.1f, %0.1f)\n", p11.X, height - 1 - p11.Y);
      fprintf(stderr, "          Corner 3: (%0.1f, %0.1f)\n", p01.X, height - 1 - p01.Y);
      fprintf(stderr, "--------------------------------------------------\n");
   }

   if(opt->pageNumbers == DmtxTrue)
      fprintf(stderr, "%d:", imgPageIndex + 1);

   if(opt->corners == DmtxTrue) {
      fprintf(stderr, "%d,%d:", (int)(p00.X + 0.5), height - 1 - (int)(p00.Y + 0.5));
      fprintf(stderr, "%d,%d:", (int)(p10.X + 0.5), height - 1 - (int)(p10.Y + 0.5));
      fprintf(stderr, "%d,%d:", (int)(p11.X + 0.5), height - 1 - (int)(p11.Y + 0.5));
      fprintf(stderr, "%d,%d:", (int)(p01.X + 0.5), height - 1 - (int)(p01.Y + 0.5));
   }

   return DmtxPass;
}


/**
 *
 *
 */
static void
CleanupMagick(MagickWand **wand, int magickError)
{
   char *excMessage;
   ExceptionType excSeverity;

   if(magickError == DmtxTrue) {
      excMessage = MagickGetException(*wand, &excSeverity);
/*    fprintf(stderr, "%s %s %lu %s\n", GetMagickModule(), excMessage); */
      MagickRelinquishMemory(excMessage);
   }

   if(*wand != NULL) {
      DestroyMagickWand(*wand);
      *wand = NULL;
   }
}

/**
 * @brief  List supported input image formats on stdout
 * @return void
 */
static void
ListImageFormats(void)
{
   int i, idx;
   int row, rowCount;
   int col, colCount;
   unsigned long totalCount;
   char **list;

   list = MagickQueryFormats("*", &totalCount);

   if(list == NULL)
      return;

   fprintf(stderr, "\n");

   colCount = 7;
   rowCount = totalCount / colCount;
   if(totalCount % colCount)
      rowCount++;

   for(i = 0; i < colCount * rowCount; i++) {
      col = i % colCount;
      row = i / colCount;
      idx = col * rowCount + row;
      fprintf(stderr, "%10s", (idx < totalCount) ? list[col * rowCount + row] : " ");
      fprintf(stderr, "%s", (col + 1 < colCount) ? " " : "\n");
   }
   fprintf(stderr, "\n");

   MagickRelinquishMemory(list);
}

/**
 *
 *
 */
static void
WriteDiagnosticImage(DmtxDecode *dec, char *imagePath)
{
   int totalBytes, headerBytes;
   int bytesWritten;
   unsigned char *pnm;
   FILE *fp;

   fp = fopen(imagePath, "wb");
   if(fp == NULL) {
      perror(programName);
      FatalError(EX_CANTCREAT, _("Unable to create image \"%s\""), imagePath);
   }

   pnm = dmtxDecodeCreateDiagnostic(dec, &totalBytes, &headerBytes, 0);
   if(pnm == NULL)
      FatalError(EX_OSERR, _("Unable to create diagnostic image"));

   bytesWritten = fwrite(pnm, sizeof(unsigned char), totalBytes, fp);
   if(bytesWritten != totalBytes)
      FatalError(EX_IOERR, _("Unable to write diagnostic image"));

   free(pnm);
   fclose(fp);
}

/**
 *
 *
 */
static int
ScaleNumberString(char *s, int extent)
{
   int err;
   int numValue;
   int scaledValue;
   char *terminate;

   assert(s != NULL);

   err = StringToInt(&numValue, s, &terminate);
   if(err != DmtxPass)
      FatalError(EX_USAGE, _("Integer value required"));

   scaledValue = (*terminate == '%') ? (int)(0.01 * numValue * extent + 0.5) : numValue;

   if(scaledValue < 0)
      scaledValue = 0;

   if(scaledValue >= extent)
      scaledValue = extent - 1;

   return scaledValue;
}
