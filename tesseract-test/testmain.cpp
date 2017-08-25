#include "allheaders.h"
#include "baseapi.h"

//#pragma commenct(lib, "giflib.lib")
//#pragma commenct(lib, "libjpeg.lib")
//#pragma commenct(lib, "liblept.lib")
//#pragma commenct(lib, "libpng.lib")
//#pragma commenct(lib, "libtiff.lib")
//#pragma commenct(lib, "libwebp.lib")
//#pragma commenct(lib, "openjpeg.lib")
//#pragma commenct(lib, "zlib.lib")
//#pragma commenct(lib, "libtesseract.lib")

int main()
{
	char * outText;
	tesseract::TessBaseAPI * api = new tesseract::TessBaseAPI();
	// Initialize tesseract-ocr with English, without specifying tessdata path
	if (api->Init(NULL, "eng")) {
		fprintf(stderr, "Could not initialize tesseract.\n");
		exit(1);
	}

	// Open input image with leptonica library
	Pix *image = pixRead("Monitor.BMP");
	api->SetImage(image);
	// Get OCR result
	outText = api->GetUTF8Text();
	printf("OCR output:\n%s", outText);

	// Destroy used object and release memory
	api->End();
	delete[] outText;
	pixDestroy(&image);
	getchar();
	return 0;
}

