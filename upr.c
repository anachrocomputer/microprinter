/* upr --- test the microprinter                            2009-02-22 */
/* Copyright (c) 2009 John Honniball                                   */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef unsigned char byte;

int Fd = 0;

const int ESC = 0x1b;
const int GS = 0x1d;

const int partialcut = 'V';

const byte buzzer_on = 0x1e;
const byte chsize = 0x21;
const byte inverse = 0x42;
const byte emphasis = 0x45;
const byte doubleprint = 0x47;
const byte fontsel = 0x4D;
const byte flipchars = 0x7B;
const byte rotatechars = 0x56;

const byte commandBarcode = 0x1D;
const byte commandBarcodePrint = 0x6B;
const byte commandBarcodeWidth = 0x77;
const byte commandBarcodeHeight = 0x68;
const byte commandBarcodeTextPosition = 0x48;

const byte barcodeModeUPCA = 0x00;
const byte barcodeModeUPCE = 0x01;
const byte barcodeModeJAN13EAN = 0x02;
const byte barcodeModeJAN8EAN = 0x03;
const byte barcodeModeCODE39 = 0x04;
const byte barcodeModeITF= 0x05;
const byte barcodeModeCODEABAR= 0x06;
const byte barcodeModeCODE128 = 0x07;

const byte barcodeNarrow = 0x02;
const byte barcodeMedium = 0x03;
const byte barcodeWide = 0x04;

const int barcodePrintNone = 0x00;
const int barcodePrintAbove = 0x01;
const int barcodePrintBelow = 0x02;
const int barcodePrintBoth = 0x03;

const int underlineSingle = '1';
const int underlineNone = '0';
const int underlineDouble = '2';

const int resLow = 1;
const int resHigh = 2;

#define justLeft   (1)
#define justCentre (2)
#define justRight  (3)

void printPBM (const char name[], int resolution, int justify)
{
   FILE *fp;
   char lin[256];
   int xsize, ysize;
   char buf[576 * 3];
   char spaces[50];
   byte cmd[5];
   int i, j;
   int x, y;
   int njustsp;
   int maxx;
   int ch;
   int bit;
   int rowht;
   
   if (!((resolution == resLow) || (resolution == resHigh))) {
      fprintf (stderr, "printPBM: invalid resolution\n");
      return;
   }
   
   for (i = 0; i < 50; i++)
      spaces[i] = ' ';

   if ((fp = fopen (name, "r")) == NULL) {
      perror (name);
      return;
   }

   /* Read PBM header */
   fgets (lin, sizeof (lin), fp);

   if (lin[0] != 'P') {
      fprintf (stderr, "Image file '%s' is not a PBM file\n", name);
      fclose (fp);
      return;
   }
   
   if (lin[1] != '1') {
      fprintf (stderr, "Image file '%s' not ASCII PBM file\n", name);
      fclose (fp);
      return;
   }

   /* Skip PBM comment */
   fgets (lin, sizeof (lin), fp);
   
   /* Read PBM X and Y size */
   
   fscanf (fp, "%d %d", &xsize, &ysize);
   
   switch (justify) {
   case justLeft:
      njustsp = 0;
      break;
   case justCentre:
      if (resolution == resLow)
         njustsp = (576 - (xsize * 3)) / 2;
      else
         njustsp = (576 - xsize) / 2;
         
      njustsp /= 12;
      break;
   case justRight:
      if (resolution == resLow)
         njustsp = 576 - (xsize * 3);
      else
         njustsp = 576 - xsize;
         
      njustsp /= 12;
      break;
   default:
      fprintf (stderr, "printPBM: invalid justification\n");
      fclose (fp);
      return;
      break;
   }
   
   if (resolution == resLow)
      maxx = 192;
   else
      maxx = 576;
      
   if (xsize > maxx) {
      fprintf (stderr, "Image width (%d) exceeds maximum (%d)\n", xsize, maxx);
      fclose (fp);
      return;
   }

   /* Set minimum line spacing */
   cmd[0] = ESC;
   cmd[1] = '3';
   cmd[2] = 24;
      
   write (Fd, cmd, 3);
   
   if (resolution == resLow)
      rowht = 8;
   else
      rowht = 24;
      
   /* Loop through PBM file, generating graphics print commands */
   for (y = 0; y < ysize; y += rowht) {
      for (i = 0; i < rowht; i++) {
         bit = 0x80 >> (i % 8);

         for (x = 0; x < xsize; x++) {
            if ((y + i) < ysize) {
               do {        /* Read image pixels from file */
                  ch = fgetc (fp);
               } while (!(ch == '1' || ch == '0'));
            }
            else {
               ch = '0';   /* Add white space padding below image */
            }
            
            j = (x * 3) + (i / 8);
            
            if (ch == '1')
               buf[j] |= bit;
            else
               buf[j] &= ~bit;
               
            if (resolution == resLow)
               buf[j + 2] = buf[j + 1] = buf[j];
         }
      }
   
      /* Add spacing if justifying image */
      if (njustsp > 0)
         write (Fd, spaces, njustsp);
      
      /* Generate actual image print comand */
      cmd[0] = ESC;
      cmd[1] = '*';
      if (resolution == resLow) {
         cmd[2] = 1;    /* 8-dots, double density */
         cmd[3] = (xsize * 3) % 256;
         cmd[4] = (xsize * 3) / 256;
      }
      else {
         cmd[2] = 33;   /* 24-dots, double density */
         cmd[3] = xsize % 256;
         cmd[4] = xsize / 256;
      }
   
      write (Fd, cmd, 5);
      write (Fd, buf, xsize * 3);
      write (Fd, "\r\n", 2);
   }

   /* Restore standard 1/6-inc line spacing */
   cmd[0] = ESC;
   cmd[1] = '2';
      
   write (Fd, cmd, 2);
   
   fclose (fp);
}


void soundBuzzer (void)
{
   byte buf[2];
   
   buf[0] = ESC;
   buf[1] = buzzer_on;
   
   write (Fd, buf, 2);
}


void partialCut (void)
{
   byte buf[3];
   
   buf[0] = GS;
   buf[1] = partialcut;
   buf[2] = 0x01;
   
   write (Fd, buf, 3);
}


void feed (void)
{
   byte buf[2];
   int i;
   
   buf[0] = '\r';
   buf[1] = '\n';

   for (i = 0; i < 8; i++)
      write (Fd, buf, 2);
}


void setUnderline (byte mode)
{
   byte buf[3];
   
   buf[0] = ESC;
   buf[1] = '-';
   buf[2] = mode;
   
   write (Fd, buf, 3);
}


void printBitRow (int ndots, byte dots[])
{
   byte buf[5];
   
   buf[0] = ESC;
   buf[1] = '*';
   buf[2] = 33;
   buf[3] = ndots % 256;
   buf[4] = ndots / 256;
   
   write (Fd, buf, 5);
   write (Fd, dots, ndots * 3);
}


void setBarcodeWidth (byte width)  /* 2, 3 or 4. default = 3 */
{
   byte buf[3];
   
   if (width < 2)
      width = 2;

   if (width > 4)
      width = 4;

   buf[0] = commandBarcode;   /* "[1d]H + [77]H + N" (where N = 2, 3 or 4) */
   buf[1] = commandBarcodeWidth;
   buf[2] = width;
   
   write (Fd, buf, 3);
}


void setBarcodeTextPosition (byte position)
{
   byte buf[3];
   
   buf[0] = commandBarcode;
   buf[1] = 0x48;
   buf[2] = position;
   
   write (Fd, buf, 3);
}


void setBarcodeHeight (byte height) /* in dots. default = 162 */
{
   byte buf[3];
   
   buf[0] = commandBarcode;
   buf[1] = commandBarcodeHeight;
   buf[2] = height;
   
   write (Fd, buf, 3);
}


void printBarcode (const char barcode[], byte barcodeMode)
{
   byte buf[3];
   byte eos[1];
   
   eos[0] = '\0';
   
   buf[0] = commandBarcode;
   buf[1] = commandBarcodePrint;
   buf[2] = barcodeMode;

   write (Fd, buf, 3);
   write (Fd, barcode, strlen (barcode));
   write (Fd, eos, 1);
}


void setCharSize (int hmag, int vmag)
{
   byte buf[3];
   
   buf[0] = GS;
   buf[1] = chsize;
   buf[2] = ((hmag - 1) << 4) | (vmag - 1);

   write (Fd, buf, 3);
}


void setCharSizeNormal (void)
{
   setCharSize (1, 1);
}


void setCharSizeDouble (void)
{            
   setCharSize (2, 2);
}


void setPrinterFont (byte font)
{
   byte buf[3];
  
   buf[0] = ESC;
   buf[1] = fontsel;
   buf[2] = font;
  
   write (Fd, buf, 3);
}


void setPrinterFontA (void)
{
  setPrinterFont (0x00);
}


void setPrinterFontB (void)
{
  setPrinterFont (0x01);
}


void setDoublePrint (byte mode)
{
   byte buf[3];
  
   buf[0] = ESC;
   buf[1] = doubleprint;
   buf[2] = mode;
  
   write (Fd, buf, 3);
}


void setDoublePrintOn (void)
{
  setDoublePrint (0x01);
}


void setDoublePrintOff (void)
{
  setDoublePrint (0x00);
}


void setEmphasis (byte mode)
{
   byte buf[3];
  
   buf[0] = ESC;
   buf[1] = emphasis;
   buf[2] = mode;
  
   write (Fd, buf, 3);
}


void setEmphasisOn (void)
{
  setEmphasis (0x01);
}


void setEmphasisOff (void)
{
  setEmphasis (0x00);
}


void setInverse (byte mode)
{
   byte buf[3];
  
   buf[0] = GS;
   buf[1] = inverse;
   buf[2] = mode;
  
   write (Fd, buf, 3);
}


void setInverseOn (void)
{
  setInverse (0x01);
}


void setInverseOff (void)
{
  setInverse (0x00);
}


void setInvertChars (byte mode)
{
   byte buf[3];
  
   buf[0] = ESC;
   buf[1] = flipchars;
   buf[2] = mode;
  
   write (Fd, buf, 3);
}


void setInvertCharsOn (void)
{
  setInvertChars (0x01);
}


void setInvertCharsOff (void)
{
  setInvertChars (0x00);
}


void setRotateChars (byte mode)
{
   byte buf[3];
  
   buf[0] = ESC;
   buf[1] = rotatechars;
   buf[2] = mode;
  
   write (Fd, buf, 3);
}


void setRotateCharsOn (void)
{
  setRotateChars (0x01);
}


void setRotateCharsOff (void)
{
  setRotateChars (0x00);
}


void testBitImage (void)
{
   byte image[40 * 8 * 3];
   int i;

   for (i = 0; i < 40 * 8 * 3; i++) {
      if ((i / 3) & 1)
         image[i] = i % 256;
      else
         image[i] = i % 256;
   }
   
   printBitRow (40 * 8, image);
   
   write (Fd, "\r\n", 2);
}


void print (const char *str)
{
   int n = strlen (str);
   
   if (write (Fd, str, n) != n)
      perror ("write");
}


static int openPrinterPort (const char *port)
{
   int fd;
   struct termios tbuf;

   fd = open (port, O_RDWR | O_NOCTTY);
   
   if (fd < 0) {
      perror (port);
      exit (1);
   }
   
   if (tcgetattr (fd, &tbuf) < 0) {
      perror ("tcgetattr");
      exit (1);
   }
   
   cfsetospeed (&tbuf, B38400);
   cfsetispeed (&tbuf, B38400);
   cfmakeraw (&tbuf);
   
   if (tcsetattr (fd, TCSAFLUSH, &tbuf) < 0) {
      perror ("tcsetattr");
      exit (1);
   }
   
   return (fd);
}


int main (const int argc, const char *argv[])
{
// Fd = openPrinterPort ("/dev/ttyACM0");
   Fd = openPrinterPort ("/dev/ttyUSB1");
// Fd = openPrinterPort ("/dev/ttyS2");

#ifdef NOTDEF
   print ("Hello from the Citizen CBM1000 microprinter\r\n");
   
   soundBuzzer ();
   
   setPrinterFontB ();
   
   print ("Hello, Hackspace (font B, condensed)\r\n");
   
   setPrinterFontA ();

   setUnderline (underlineSingle);
   
   print ("Hello, world (single underline)\r\n");
   
   setUnderline (underlineDouble);

   print ("Hello, world (double underline)\r\n");
   
   setUnderline (underlineNone);

   setDoublePrintOn ();

   print ("Hello, world (double print)\r\n");

   setDoublePrintOff ();

   setEmphasisOn ();

   print ("Hello, world (emphasis)\r\n");

   setEmphasisOff ();

   setInvertCharsOn ();

   print ("Hello, world (inverted)\r\n");

   setInvertCharsOff ();

   setInverseOn ();

   print ("Hello, world (inverse black/white)\r\n");

   setInverseOff ();

   setRotateCharsOn ();

   print ("Hello, world (rotated)\r\n");

   setRotateCharsOff ();
   
   setCharSizeDouble ();
   
   print ("Hello, world\r\n(double size)\r\n");
   
   setCharSizeNormal ();
   
   setCharSize (2, 1);
   
   print ("Hello, world\r\n(double width)\r\n");
   
   setCharSizeNormal ();
   
   setCharSize (1, 2);
   
   print ("Hello, world (double height)\r\n");

   setCharSizeNormal ();

   setCharSize (3, 3);
   
   print ("Hello, world\r\n(triple size)\r\n");

   setCharSizeNormal ();

#endif

#if 0
   setBarcodeWidth (barcodeMedium);
   setBarcodeHeight (160);
   setBarcodeTextPosition (barcodePrintBelow);

   /* Heinz Curried Beans, 200g */
   printBarcode ("5000157024923", barcodeModeJAN13EAN);
   
   /* Phil's college badge number */
// printBarcode ("9913", barcodeModeCODE39);
   
   testBitImage ();
   
   print ("\n");
   print ("000000000111111111122222222223333333333444444444\n");
   print ("123456789012345678901234567890123456789012345678\n");

#endif
// print ("BRISTOL HACKSPACE\r\n");
// print ("BV Studios 2014 Open Studios\r\n");
   print ("FUN WITH PEN PLOTTERS\r\n");
   print ("Bristol Mini Maker Faire 2015\r\n");

   printPBM ("qr_sample.pbm", resLow, justCentre);

// print ("End of Line\r\n");
   print ("http://bristol.hackspace.org.uk\r\n");

   feed ();
   partialCut ();
   
   close (Fd);
   
   return (0);
}
