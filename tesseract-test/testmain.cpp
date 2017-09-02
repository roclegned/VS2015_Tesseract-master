#include "allheaders.h"
#include "baseapi.h"
#include "dict.h"
#include "renderer.h"

 

tesseract::TessBaseAPI * api = nullptr;

void InitEngine()
{
	tesseract::Dict::GlobalDawgCache();
	if(nullptr == api)
		api = new tesseract::TessBaseAPI();
	else {
		api->End();
		api = new tesseract::TessBaseAPI();
	}
}

void UninitEngine()
{
	if (nullptr == api) return;
	api->End();
}

void FixPageSegMode(tesseract::TessBaseAPI* api,
	tesseract::PageSegMode pagesegmode) {
	if (api->GetPageSegMode() == tesseract::PSM_SINGLE_BLOCK)
		api->SetPageSegMode(pagesegmode);
}

void PreloadRenderers(
	tesseract::TessBaseAPI* api,
	tesseract::PointerVector<tesseract::TessResultRenderer>* renderers,
	tesseract::PageSegMode pagesegmode, const char* outputbase) {
	if (pagesegmode == tesseract::PSM_OSD_ONLY) {
		renderers->push_back(new tesseract::TessOsdRenderer(outputbase));
	}
	else {
		bool b;
		api->GetBoolVariable("tessedit_create_hocr", &b);
		if (b) {
			bool font_info;
			api->GetBoolVariable("hocr_font_info", &font_info);
			renderers->push_back(
				new tesseract::TessHOcrRenderer(outputbase, font_info));
		}

		api->GetBoolVariable("tessedit_create_tsv", &b);
		if (b) {
			bool font_info;
			api->GetBoolVariable("hocr_font_info", &font_info);
			renderers->push_back(
				new tesseract::TessTsvRenderer(outputbase, font_info));
		}

		api->GetBoolVariable("tessedit_create_pdf", &b);
		if (b) {
			bool textonly;
			api->GetBoolVariable("textonly_pdf", &textonly);
			renderers->push_back(new tesseract::TessPDFRenderer(
				outputbase, api->GetDatapath(), textonly));
		}

		api->GetBoolVariable("tessedit_write_unlv", &b);
		if (b) {
			renderers->push_back(new tesseract::TessUnlvRenderer(outputbase));
		}

		api->GetBoolVariable("tessedit_create_boxfile", &b);
		if (b) {
			renderers->push_back(new tesseract::TessBoxTextRenderer(outputbase));
		}

		api->GetBoolVariable("tessedit_create_txt", &b);
		if (b || renderers->empty()) {
			renderers->push_back(new tesseract::TessTextRenderer(outputbase));
		}
	}

	if (!renderers->empty()) {
		// Since the PointerVector auto-deletes, null-out the renderers that are
		// added to the root, and leave the root in the vector.
		for (int r = 1; r < renderers->size(); ++r) {
			(*renderers)[0]->insert((*renderers)[r]);
			(*renderers)[r] = NULL;
		}
	}
}

void PrintBanner() {
	tprintf("Tesseract Open Source OCR Engine v%s with Leptonica\n",
		tesseract::TessBaseAPI::Version());
}

bool  TransferImagesBasic(char * fileName, char ** outText)
{
	if (nullptr == api)
	{
		fprintf(stderr, "tesseract object is null.\n");
		return false;
	}

    //tesseract::PageSegMode pagesegmode = tesseract::PSM_AUTO;
	//FixPageSegMode(api, pagesegmode);
	Pix * image = pixRead(fileName);
	api->SetImage(image);
	*outText = api->GetUTF8Text();
	pixDestroy(&image);
	return true;
}

void TransferImagesIterator(char * fileName)
{
	Pix *image = pixRead(fileName);
	api->Init(NULL, "eng");
	api->SetImage(image);
	api->Recognize(0);
	tesseract::ResultIterator* ri = api->GetIterator();
	tesseract::PageIteratorLevel level = tesseract::RIL_WORD;
	if (ri != 0) {
		do {
			const char* word = ri->GetUTF8Text(level);
			float conf = ri->Confidence(level);
			int x1, y1, x2, y2;
			ri->BoundingBox(level, &x1, &y1, &x2, &y2);
			printf("word: '%s';  \tconf: %.2f; BoundingBox: %d,%d,%d,%d;\n",
				word, conf, x1, y1, x2, y2);
			delete[] word;
		} while (ri->Next(level));
	}
}

void TransferImagesGetComponent(char * fileName)
{
	Pix *image = pixRead(fileName);
	api->Init(NULL, "eng");
	api->SetImage(image);
	Boxa* boxes = api->GetComponentImages(tesseract::RIL_TEXTLINE, true, NULL, NULL);
	printf("Found %d textline image components.\n", boxes->n);
	for (int i = 0; i < boxes->n; i++) {
		BOX* box = boxaGetBox(boxes, i, L_CLONE);
		api->SetRectangle(box->x, box->y, box->w, box->h);
		char* ocrResult = api->GetUTF8Text();
		int conf = api->MeanTextConf();
		fprintf(stdout, "Box[%d]: x=%d, y=%d, w=%d, h=%d, confidence: %d, text: %s",
			i, box->x, box->y, box->w, box->h, conf, ocrResult);
	}
}


int main()
{
	tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();
	Pix *image = pixRead("D:\\WorkSpace\\DEMO\\GitHub\\VS2015_Tesseract-master\\Debug\\00004.song.001.bmp");	
	api->Init(NULL, "eng");
	api->SetImage(image);
	Boxa* boxes = api->GetComponentImages(tesseract::RIL_TEXTLINE, true, NULL, NULL);
	printf("Found %d textline image components.\n", boxes->n);
	for (int i = 0; i < boxes->n; i++) {
		BOX* box = boxaGetBox(boxes, i, L_CLONE);
		api->SetRectangle(box->x, box->y, box->w, box->h);
		char* ocrResult = api->GetUTF8Text();
		int conf = api->MeanTextConf();
		fprintf(stdout, "Box[%d]: x=%d, y=%d, w=%d, h=%d, confidence: %d, text: %s",
			i, box->x, box->y, box->w, box->h, conf, ocrResult);
	}
	getchar();
	return 0;
}

