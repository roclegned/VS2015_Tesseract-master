// tesseract-c-dll.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "allheaders.h"
#include "baseapi.h"
#include "dict.h"
#include "publictypes.h"

#define EXPORT_API  extern "C" __declspec(dllexport)

tesseract::TessBaseAPI * api = nullptr;

EXPORT_API bool TessLoadEngine()
{
	tesseract::Dict::GlobalDawgCache();
	if (nullptr == api)
		api = new tesseract::TessBaseAPI();
	else {
		api->End();
		api = new tesseract::TessBaseAPI();
	}
	if (nullptr != api)
		return true;
	else
		return false;
}

EXPORT_API void TessUnloadEngine()
{
	if (nullptr == api) 
		return;
	else
		api->End();
}


EXPORT_API bool TessInit(const char * dataPath, const char * language, int OcrEngineMode = 3)
{
	if (nullptr == api){
		return false;
	}
	else {
		api->Init(dataPath, language, tesseract::OcrEngineMode(OcrEngineMode));
		return true;
	}		
}

EXPORT_API void TessSetImage(const unsigned char * imagedata, int width, int height,
	int bytes_per_pixel, int bytes_per_line)
{
	if (nullptr == api) {
		fprintf(stderr, "tesseract object is null.\n");
		exit(1);
	}
	else{
		api->SetImage(imagedata, width, height, bytes_per_pixel, bytes_per_line);
	}
}

EXPORT_API bool TessSetVariable(const char* name, const char* value)
{
	if (nullptr == api) {
		fprintf(stderr, "tesseract object is null.\n");
		return false;
	}
	else {
		return api->SetVariable(name, value);
	}
}

EXPORT_API   char* TesseractRect(const unsigned char* imagedata,
	int bytes_per_pixel, int bytes_per_line,
	int left, int top, int width, int height)
{
	if (nullptr == api) {
		fprintf(stderr, "tesseract object is null.\n");
		return NULL;
	}
	else {
		return api->TesseractRect(imagedata, bytes_per_pixel, bytes_per_line, left, top, width, height);
	}		
}

EXPORT_API void SetRectangle(int left, int top, int width, int height)
{
	if (nullptr == api) {
		fprintf(stderr, "tesseract object is null.\n");
		return;
	}
	else
		api->SetRectangle(left, top, width, height);
}

EXPORT_API char * TessGetUTF8Text()
{
	if (nullptr == api) {
		fprintf(stderr, "tesseract object is null.\n");
		return NULL;
	}		
	else {
		return api->GetUTF8Text();
	}
}

EXPORT_API bool  TessBasicOCR(char * fileName, char ** outText)
{
	if (nullptr == api)
	{
		fprintf(stderr, "tesseract object is null.\n");
		return false;
	}
	Pix * image = pixRead(fileName);
	api->SetImage(image);
	*outText = api->GetUTF8Text();
	pixDestroy(&image);
	return true;
}

EXPORT_API   Boxa * TessGetComponentImagesFromFile(char * fileName, const int level, const bool text_only, Pixa** pixa, int** blockids)
{
	if (nullptr == api || NULL == fileName)
	{
		fprintf(stderr, "tesseract object is null.\n");
		return NULL;
	}
	Pix *image = pixRead(fileName);
	api->SetImage(image);
	return api->GetComponentImages((tesseract::PageIteratorLevel)level, text_only, pixa, blockids);
}


EXPORT_API   Boxa * TessGetComponentImages(const int level, const bool text_only, Pixa** pixa, int** blockids)
{
	if (nullptr == api)
	{
		fprintf(stderr, "tesseract object is null.\n");
		return NULL;
	}
	return api->GetComponentImages((tesseract::PageIteratorLevel)level, text_only, pixa, blockids);
}

EXPORT_API BOX * TessBoxaGetBox(BOXA *boxa, l_int32  index, l_int32  accessflag)
{
	return boxaGetBox(boxa, index, accessflag);
}

EXPORT_API void TessPixaDestroy(PIXA **ppixa)
{
	pixaDestroy(ppixa);
}

EXPORT_API PIX * TessPixaGetPix(PIXA *pixa, l_int32  index, l_int32  accesstype)
{
	return pixaGetPix(pixa, index, accesstype);
}

EXPORT_API BOXA * TessPixaGetBoxa(PIXA *pixa, l_int32 accesstype)
{
	return pixaGetBoxa(pixa, accesstype);
}

EXPORT_API BOX * TessPixaGetBox(PIXA *pixa, l_int32  index, l_int32  accesstype)
{
	return pixaGetBox(pixa, index, accesstype);
}

EXPORT_API void TessBoxDestroy(BOX **pbox)
{
	boxDestroy(pbox);
}

EXPORT_API PIX * TessPixCreate(l_int32  width, l_int32  height, l_int32  depth)
{
	return pixCreate(width, height, depth);
}

EXPORT_API void TessPixDestroy(PIX  **ppix)
{
	return pixDestroy(ppix);
}


//-------------------------直方图相关---------------------------

EXPORT_API NUMA *TessPixGetGrayHistogram(PIX *pixs, l_int32  factor)
{
	return pixGetGrayHistogram(pixs, factor);	
}

EXPORT_API void TessNumaDestroy(NUMA **pna)
{
	numaDestroy(pna);
}






















