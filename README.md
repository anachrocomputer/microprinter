Some code to drive the Citizen CBM 1000 till-receipt printer.
It's a thermal printer that is normally used with a PC-based cash
register. It can print text, graphics and barcodes. The C source
here is very much a work-in-progress and has some sections #ifdefed
out for development. What's left in place is a routine that will
print a PBM file (text format), which we use as a way to give
people a QR code that represents the Bristol Hackspace URL.
