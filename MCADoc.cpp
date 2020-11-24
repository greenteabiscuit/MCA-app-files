
// MCADoc.cpp : implementation of the CMCADoc class
//

#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "MCA.h"
#endif

#include "MCADoc.h"

#include <propkey.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "ftd3xx.h"  // USB 3.0 ftd2xx.h -> ftd3xx.h

FT_HANDLE   ftHandle[4];
FT_STATUS   ftStatus;
DWORD       numDevs;
DWORD		devIndex;
int			devcnt, devlast;
PUCHAR      pbuf; // pbuf[16384];
char        buf[256];
unsigned short ibuf[1986663]; // 3973326/2  1986560はデータサイズのみ
UCHAR pipeID, pipeIDr;

// CMCADoc

IMPLEMENT_DYNCREATE(CMCADoc, CDocument)

BEGIN_MESSAGE_MAP(CMCADoc, CDocument)
	ON_COMMAND(ID_OPERATION_SUBTRACTAFROMB, &CMCADoc::OnOperationSubtractafromb)
	ON_COMMAND(ID_MCA_START, &CMCADoc::OnMcaStart)
	ON_COMMAND(ID_MCA_STOP, &CMCADoc::OnMcaStop)
	ON_COMMAND(ID_MCA_MEMORYCLEAR, &CMCADoc::OnMcaMemoryclear)
	ON_COMMAND(ID_MCA_POINTERCLEAR, &CMCADoc::OnMcaPointerclear)
	ON_COMMAND(ID_MCA_MEMORYREAD, &CMCADoc::OnMcaMemoryread)
	ON_COMMAND(ID_MCA_THRESHOLD, &CMCADoc::OnMcaThreshold)
	ON_COMMAND(ID_MCA_THRESHOLD32777, &CMCADoc::OnMcaThreshold32777)
	ON_COMMAND(ID_OPERATION_ADDATOB, &CMCADoc::OnOperationAddatob)
	ON_COMMAND(ID_OPERATION_SAMPLEDATA, &CMCADoc::OnOperationSampledata)
	ON_COMMAND(ID_FILE_SAVE_AS, &CMCADoc::OnFileSaveAs)
	ON_COMMAND(ID_OPERATION_COPYATOC, &CMCADoc::OnOperationCopyatoc)
	ON_COMMAND(ID_OPERATION_COPYATOD, &CMCADoc::OnOperationCopyatod)
	ON_COMMAND(ID_MCA_WAVEFORMREC, &CMCADoc::OnMcaWaveformrec)
	ON_COMMAND(ID_VIEW_1D, &CMCADoc::OnView1d)
	ON_COMMAND(ID_OPERATION_BIAS, &CMCADoc::OnOperationBias)
	ON_COMMAND(ID_MCA_WAVEMONITOR, &CMCADoc::OnMcaWavemonitor)
	ON_COMMAND(ID_OPERATION_COPYBTOA, &CMCADoc::OnOperationCopybtoa)
	ON_COMMAND(ID_OPERATION_CLEARA, &CMCADoc::OnOperationCleara)
	ON_COMMAND(ID_OPERATION_COPYCTOA, &CMCADoc::OnOperationCopyctoa)
	ON_COMMAND(ID_OPERATION_COPYDTOA, &CMCADoc::OnOperationCopydtoa)
	ON_COMMAND(ID_OPERATION_CLEARB, &CMCADoc::OnOperationClearb)
	ON_COMMAND(ID_OPERATION_CLEARC, &CMCADoc::OnOperationClearc)
	ON_COMMAND(ID_OPERATION_CLEARD, &CMCADoc::OnOperationCleard)
	ON_COMMAND(ID_OPERATION_COPYATOB, &CMCADoc::OnOperationCopyatob)
	ON_COMMAND(ID_ANALYS_FT6000X4, &CMCADoc::OnAnalysFt6000x4)
	ON_COMMAND(ID_ANALYS_FT6000X5, &CMCADoc::OnAnalysFt6000x5)
	ON_COMMAND(ID_ANALYS_FT6000X6, &CMCADoc::OnAnalysFt6000x6)
	ON_COMMAND(ID_VIEW_STATUS_BAR, &CMCADoc::OnViewStatusBar)
END_MESSAGE_MAP()


// CMCADoc construction/destruction

CMCADoc::CMCADoc()
{
	// TODO: add one-time construction code here
	int i, j;
	FT_60XCONFIGURATION chipConfig;

	icsr = 0;
	for (i = 0; i < DMAX; i++) {
		a[i] = b[i] = c[i] = d[i] = 0;
		dp[i].x = i; dp[i].y = 0;
	}
	for (i = 0; i < IMAGESIZEX; i++) {
		for (j = 0; j < IMAGESIZEY; j++) {
			image[i][j] = 0; image1[i][j] = 0;
		}
	}
	for (i = 0; i < 100; i++)param[i] = 0;  // Communication between CMCAView and CMCADoc

	param[20] = 0; /* 1-dim 0 2-dim 1*/
	param[19] = 0; // USB DEVICE IDLING
	param[18] = 0; // USB DEVICE ACTIVE =1

	param[2] = 256; /* Initial Scale x-axis : 256 = 8K Channel,  128 = 16K Channel */
	param[3] = 8192 * 16;/* Initial Scale y-axis*/
	param[9] = 0; /* Current Position */
	param[10] = 4; /* transfer length */
	param[11] = 10; /* 10 times */

	for (i = 0; i < 16; i++)roi[i] = 0.0;

	///////////////////////////////////////////////////////////////////////////
		// USB setup
	numDevs = 0;

	ftStatus = FT_ListDevices(&numDevs, NULL, FT_LIST_NUMBER_ONLY);
	devlast = numDevs;
	d[0] = devlast; d[1] = ftStatus;

	for (devcnt = 0; devcnt != devlast; devcnt++) {
		devIndex = (DWORD)devcnt;

		ftStatus = FT_ListDevices((PVOID)devIndex, buf,
			FT_LIST_BY_INDEX | FT_OPEN_BY_SERIAL_NUMBER);
		b[0] = ftStatus;
		if (ftStatus == FT_OK) {

			ftStatus = FT_Create(buf,
				FT_OPEN_BY_SERIAL_NUMBER, &ftHandle[devcnt]); // D3XX用
			param[18] = 1; // active device found
			param[19] = 99; // Just Conneced
			pipeID = 0x2; pipeIDr = 0x82; //pipeID for write pipeIDr for read
			FT_GetChipConfiguration(ftHandle[devcnt], &chipConfig);
			a[0] = chipConfig.FIFOMode;
			a[1] = chipConfig.ChannelConfig;
			a[2] = chipConfig.FIFOClock;
			a[3] = chipConfig.MSIO_Control;
			//chipConfig.FIFOMode = CONFIGURATION_FIFO_MODE_245;
			//chipConfig.ChannelConfig = CONFIGURATION_CHANNEL_CONFIG_1;
		   // FT_SetChipConfiguration(ftHandle[devcnt], &chipConfig);
		}
	}
	pbuf = (PUCHAR)malloc(sizeof(PUCHAR) * 8192);

}

CMCADoc::~CMCADoc()
{

	for (devcnt = 0; devcnt != devlast; devcnt++) {
		ftStatus = FT_Close(ftHandle[devcnt]);  // USB Device CLOSE
	}
	free(pbuf);
}



// CMCADoc serialization

void CMCADoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

#ifdef SHARED_HANDLERS

// Support for thumbnails
void CMCADoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// Modify this code to draw the document's data
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT)GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// Support for Search Handlers
void CMCADoc::InitializeSearchContent()
{
	CString strSearchContent;
	// Set search contents from document's data. 
	// The content parts should be separated by ";"

	// For example:  strSearchContent = _T("point;rectangle;circle;ole object;");
	SetSearchContent(strSearchContent);
}

void CMCADoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl *pChunk = NULL;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != NULL)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// CMCADoc diagnostics

#ifdef _DEBUG
void CMCADoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CMCADoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CMCADoc commands

void CMCADoc::OnMcaStart()
{

	DWORD       WriteNum, TransNum;


	devcnt = 0;	// device #0
	TransNum = 0; WriteNum = 1;
	pbuf[0] = 0x05; // AFE START COMMAND
	ftStatus = FT_WritePipe(ftHandle[devcnt], pipeID, pbuf, WriteNum, &TransNum, NULL);
	b[0] = ftStatus;
	param[19] = 1;

	SetModifiedFlag();
	UpdateAllViews(NULL);


}


void CMCADoc::OnMcaStop()
{

	DWORD       WriteNum, TransNum;

	devcnt = 0;	// device #0
	TransNum = 0; WriteNum = 1;
	pbuf[0] = 0x06; // AFE STOP COMMAND
	ftStatus = FT_WritePipe(ftHandle[devcnt], pipeID, pbuf, WriteNum, &TransNum, NULL);
	b[0] = ftStatus;
	param[19] = 1;

	SetModifiedFlag();
	UpdateAllViews(NULL);

}


void CMCADoc::OnMcaMemoryclear()
{
	DWORD       WriteNum, TransNum;


	devcnt = 0;	// device #0
	TransNum = 0; WriteNum = 1;
	pbuf[0] = 0x01; // MEMORY CLEAR COMMAND
	ftStatus = FT_WritePipe(ftHandle[devcnt], pipeID, pbuf, WriteNum, &TransNum, NULL);
	b[0] = ftStatus;
	param[19] = 1;

}


void CMCADoc::OnMcaPointerclear()
{

	DWORD       WriteNum, TransNum;


	devcnt = 0;	// device #0		address counter clear command 0x20;
	TransNum = 0; WriteNum = 1;
	pbuf[0] = 0x02; //
	ftStatus = FT_WritePipe(ftHandle[devcnt], pipeID, pbuf, WriteNum, &TransNum, NULL);
	b[0] = ftStatus;
	param[19] = 2;

	SetModifiedFlag();
	UpdateAllViews(NULL);

}


void CMCADoc::OnMcaMemoryread()
{
	int i, j;
	DWORD       WriteNum, TransNum;
	DWORD       ReadNum;
	PUCHAR  bufc; // [8192];

	devcnt = 0;	// device #0
	bufc = (PUCHAR)malloc(sizeof(PUCHAR) * 8192);

	for (j = 0; j < 1024; j++) bufc[j] = 0;
	TransNum = 0; WriteNum = 1;
	ReadNum = 1024;


	ULONG  ulBytesTransferred = 0;
	ULONG  buffersize, ulActualBytesToTransfer;

	ulBytesTransferred = 200;
	ulActualBytesToTransfer = 4096;

	pbuf[0] = 0x02; // RESET POINTER
	ftStatus = FT_WritePipe(ftHandle[devcnt], pipeID, pbuf, WriteNum, &TransNum, NULL);

	ftStatus = FT_SetStreamPipe(ftHandle, FALSE, FALSE, 0x82, ulActualBytesToTransfer);
	ftStatus = FT_ReadPipe(ftHandle[devcnt], 0x82, bufc, 4096, &ulBytesTransferred, NULL);

	for (j = 0; j < 2048; j++) a[j] = bufc[2 * j] + bufc[2 * j + 1] * 256;

	FT_ClearStreamPipe(ftHandle[devcnt], FALSE, FALSE, 0x82);//stops 0x82 pipe to stop
	free(bufc);
	param[19] = 5;

	SetModifiedFlag();
	UpdateAllViews(NULL);
}


void CMCADoc::OnMcaThreshold()
{

	DWORD       WriteNum, TransNum;

	// Threshold UP

	devcnt = 0;	// device #0
	TransNum = 0; WriteNum = 1;
	pbuf[0] = 0x8;
	ftStatus = FT_WritePipe(ftHandle[devcnt], pipeID, pbuf, WriteNum, &TransNum, NULL);
	param[19] = 16;
	SetModifiedFlag();
	UpdateAllViews(NULL);

}


void CMCADoc::OnMcaThreshold32777()
{
	DWORD       WriteNum, TransNum;

	// Threshold RESET

	devcnt = 0;	// device #0
	TransNum = 0; WriteNum = 1;
	pbuf[0] = 0x5;
	ftStatus = FT_WritePipe(ftHandle[devcnt], pipeID, pbuf, WriteNum, &TransNum, NULL);
	param[19] = 6;

	SetModifiedFlag();
	UpdateAllViews(NULL);

}



BOOL CMCADoc::OnSaveDocument(LPCTSTR lpszPathName)
{
	CStdioFile afile;
	CString Ctmp;
	//	LPTSTR lpszPathNamep;
	FILE *fp;
	int i;
	fp = fopen("data.txt", "wt");
	//for (i = 0; i < 8192; i++) { fprintf(fp, "%lf\n", a[i]); }
	fclose(fp);

	//	lpszPathNamep=(LPTSTR)malloc(sizeof(LPTSTR) * 256);
	//	lstrcpyn(lpszPathNamep,lpszPathName,lstrlen(lpszPathName)-3);
	//	lstrcat(lpszPathNamep,"bin");

	//	afile.Open(lpszPathNamep, CFile::modeCreate | CFile::modeWrite | CFile::typeText);

	//	int i = 0, j=0,k=0;
	//	for (i = 0; i < IMAGESIZEX; i++) { for (j = 0; j < IMAGESIZEY; j++) { ibuf[k] = image[i][j]; k++; } }

		//afile.Write(ibuf,3973120 );
		//	afile.Write((const void *)image[0][0], 3973120);

			//afile.Flush();

		//afile.Close();
		//free(lpszPathNamep);
	UpdateAllViews(NULL);
	return TRUE;
	//return CDocument::OnSaveDocument(lpszPathName);
}

BOOL CMCADoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	CStdioFile afile;
	CFileException fe;
	int i, j, k, l;
	unsigned char bf[10];

	CString Ctmp;
	// typeText -> typeBinary
	if (!afile.Open(lpszPathName, CFile::modeRead | CFile::typeBinary))
		return false;

	//ASCII DATA
	//for(i=0;i<DMAX;i++){afile.ReadString(Ctmp); 
	//a[i]=atof(CT2A(Ctmp));}
	//
	// Read TIFF Data format

	afile.Read(ibuf, 16);

	for (i = 0; i < 8; i++) { a[i] = ibuf[i]; }

	afile.Read(ibuf, 3973120);
	afile.Close();
	l = 0;
	// block data? 
	for (i = 0; i < IMAGESIZEX; i++) {

		for (j = 0; j < IMAGESIZEY; j++) {
			image[i][j] = ibuf[l];
			a[j] = ibuf[l];
			l++;

			//l += 0;
		}
	}
	//(int) ibuf[i + j * 512] + (int) 256 * ibuf[i +1+ j * 512]; } }

	UpdateAllViews(NULL);

	return TRUE;
	//return CDocument::OnOpenDocument(lpszPathName );

}



BOOL CMCADoc::OnNewDocument()
{
	// TODO: ここに特定なコードを追加するか、もしくは基本クラスを呼び出してください。

	return CDocument::OnNewDocument();
}




void CMCADoc::OnMcaWaveformrec()
{
	// TODO: ここにコマンド ハンドラー コードを追加します。		
	DWORD       WriteNum, TransNum;

	// USB input

	devcnt = 0;	// device #0
	TransNum = 0; WriteNum = 1;
	pbuf[0] = 0x04;
	ftStatus = FT_WritePipe(ftHandle[devcnt], pipeID, pbuf, WriteNum, &TransNum, NULL);
	param[19] = 3;
	SetModifiedFlag();
	UpdateAllViews(NULL);

}


void CMCADoc::OnView1d()
{
	param[20] = 1 - param[20]; // 1D/ 2D toggle
}


void CMCADoc::OnMcaWavemonitor()
{
	// FT600 1024 bytes read

	int i, j;
	DWORD       WriteNum, TransNum;
	DWORD       ReadNum;
	PUCHAR  bufc; // [8192];
	ULONG  ulBytesTransferred = 0;
	ULONG  buffersize, ulActualBytesToTransfer;

	devcnt = 0;	// device #0
	bufc = (PUCHAR)malloc(sizeof(PUCHAR) * 8192);

	TransNum = 0; WriteNum = 1;
	ReadNum = 1024;

	for (int i = 0; i < 5; i++) {
		pbuf[0] = 0x05; // ADC START
		ftStatus = FT_WritePipe(ftHandle[devcnt], pipeID, pbuf, WriteNum, &TransNum, NULL);
	}
	pbuf[0] = 0x05; // ADC START
	ftStatus = FT_WritePipe(ftHandle[devcnt], pipeID, pbuf, WriteNum, &TransNum, NULL);

	pbuf[0] = 0x06; // ADC STOP
	ftStatus = FT_WritePipe(ftHandle[devcnt], pipeID, pbuf, WriteNum, &TransNum, NULL);

	pbuf[0] = 0x02; // RESET POINTER
	ftStatus = FT_WritePipe(ftHandle[devcnt], pipeID, pbuf, WriteNum, &TransNum, NULL);

	ulBytesTransferred = 200;
	ulActualBytesToTransfer = 4096;

	ftStatus = FT_SetStreamPipe(ftHandle, FALSE, FALSE, 0x82, ulActualBytesToTransfer);
	ftStatus = FT_ReadPipe(ftHandle[devcnt], 0x82, bufc, 4096, &ulBytesTransferred, NULL);
	
	for (j = 0; j < 2048; j++) a[j] = bufc[2 * j] + bufc[2 * j + 1] * 256;
	//for (j = 0; j < 2048; j++) c[j] = bufc[2 * j];
	//for (j = 0; j < 2048; j++) d[j] = bufc[2 * j + 1] * 256;
	//for (j = 0; j < 8192; j++) a[j] = bufc[2 * j];
	//for (j = 0; j < 2048; j++) a[j] = 500;

	b[0] = ftStatus; b[1] = ulBytesTransferred;
	FT_ClearStreamPipe(ftHandle[devcnt], FALSE, FALSE, 0x82);

	param[19] = 5;

	SetModifiedFlag();
	UpdateAllViews(NULL);
	free(bufc);

}



void CMCADoc::OnOperationCopybtoa()
{
	// TODO: ここにコマンド ハンドラー コードを追加します。
	int i;
	for (i = 0; i < DMAX; i++) { a[i] = b[i]; }
	UpdateAllViews(NULL);
}


void CMCADoc::OnOperationCleara()
{
	// TODO: ここにコマンド ハンドラー コードを追加します。
	int i;
	for (i = 0; i < DMAX; i++) { a[i] = 0; }
	UpdateAllViews(NULL);
}


void CMCADoc::OnOperationCopyctoa()
{
	// TODO: ここにコマンド ハンドラー コードを追加します。
	int i;
	for (i = 0; i < DMAX; i++) { a[i] = c[i]; }
	UpdateAllViews(NULL);
}


void CMCADoc::OnOperationCopydtoa()
{
	// TODO: ここにコマンド ハンドラー コードを追加します。
	int i;
	for (i = 0; i < DMAX; i++) { a[i] = d[i]; }
	UpdateAllViews(NULL);
}


void CMCADoc::OnOperationClearb()
{
	// 
	int i;
	for (i = 0; i < DMAX; i++) { b[i] = 0; }
	UpdateAllViews(NULL);
}


void CMCADoc::OnOperationClearc()
{
	// TODO: ここにコマンド ハンドラー コードを追加します。
	int i;
	for (i = 0; i < DMAX; i++) { c[i] = 0; }
	UpdateAllViews(NULL);
}


void CMCADoc::OnOperationCleard()
{
	// TODO: ここにコマンド ハンドラー コードを追加します。
	int i;
	for (i = 0; i < DMAX; i++) { d[i] = 0; }
	UpdateAllViews(NULL);
}


void CMCADoc::OnOperationCopyatob()
{
	// TODO: Add your command handler code here

	int i, j;
	for (i = 0; i < IMAGESIZEX; i++) {
		for (j = 0; j < IMAGESIZEY; j++) { image1[i][j] = image[i][j]; }
	}

	UpdateAllViews(NULL);
}

void CMCADoc::OnOperationCopyatoc()
{


	int i, j;
	for (i = 0; i < IMAGESIZEX; i++) {
		for (j = 0; j < IMAGESIZEY; j++) { image2[i][j] = image[i][j]; }
	}

	UpdateAllViews(NULL);
}


void CMCADoc::OnOperationCopyatod()
{
	int i;
	for (i = 0; i < DMAX; i++) { d[i] = a[i]; }
	UpdateAllViews(NULL);
}

void CMCADoc::OnOperationAddatob()
{
	int i, j;
	for (i = 0; i < IMAGESIZEX; i++) {
		for (j = 0; j < IMAGESIZEY; j++) { image1[i][j] += image[i][j]; }
	}

	UpdateAllViews(NULL);
}


void CMCADoc::OnOperationSampledata()
{
	int i, j;
	for (i = 0; i < IMAGESIZEX; i++) {
		for (j = 0; j < IMAGESIZEY; j++) {
			image[i][j] = image[i][j] / image2[i][j] * 16384;
			//if (image[i][j] < 0)image[i][j] = 0;
		}
	}

	SetModifiedFlag();
	UpdateAllViews(NULL);
}


void CMCADoc::OnOperationBias()
{
	// 現在のデータを画像データのREFデータとする。(image1[][])
	int i, j;
	for (i = 0; i < IMAGESIZEX; i++) {
		for (j = 0; j < IMAGESIZEY; j++) {
			image1[i][j] = image[i][j];
		}
	}
	UpdateAllViews(NULL);
}



void CMCADoc::OnOperationSubtractafromb()
{
	int i, j;
	for (i = 0; i < IMAGESIZEX; i++) {
		for (j = 0; j < IMAGESIZEY; j++) {
			image[i][j] -= image1[i][j];
			//if (image[i][j] < 0)image[i][j] = 0;
		}
	}

	SetModifiedFlag();
	UpdateAllViews(NULL);

}



void CMCADoc::OnAnalysFt6000x4()
{
	// TODO: ここにコマンド ハンドラー コードを追加します。
	//STATE 1

	DWORD       WriteNum, TransNum;

	devcnt = 0;	// device #0
	TransNum = 0; WriteNum = 1;
	pbuf[0] = 0x06;
	int j;
	// write 256 from 0 to 2048
	for (j = 0; j < 2048; j++) a[j] = 256;
	ftStatus = FT_WritePipe(ftHandle[devcnt], pipeID, pbuf, WriteNum, &TransNum, NULL);
	param[19] = 3;
	SetModifiedFlag();
	UpdateAllViews(NULL);

}


void CMCADoc::OnAnalysFt6000x5()
{
	// TODO: ここにコマンド ハンドラー コードを追加します。
	// STATE 2
	DWORD       WriteNum, TransNum;

	devcnt = 0;	// device #0
	TransNum = 0; WriteNum = 1;
	pbuf[0] = 0x03;
	ftStatus = FT_WritePipe(ftHandle[devcnt], pipeID, pbuf, WriteNum, &TransNum, NULL);
	param[19] = 3;
	SetModifiedFlag();
	UpdateAllViews(NULL);
}


void CMCADoc::OnAnalysFt6000x6()
{
	// TODO: ここにコマンド ハンドラー コードを追加します。
	// STATE 3
	DWORD       WriteNum, TransNum;

	devcnt = 0;	// device #0
	TransNum = 0; WriteNum = 1;
	pbuf[0] = 0x04;
	ftStatus = FT_WritePipe(ftHandle[devcnt], pipeID, pbuf, WriteNum, &TransNum, NULL);
	param[19] = 3;
	SetModifiedFlag();
	UpdateAllViews(NULL);
}


void CMCADoc::OnViewStatusBar()
{
	// TODO: ここにコマンド ハンドラー コードを追加します。
	DWORD       WriteNum, TransNum;

	devcnt = 0;	// device #0
	TransNum = 0; WriteNum = 1;
	pbuf[0] = 0x08;
	ftStatus = FT_WritePipe(ftHandle[devcnt], pipeID, pbuf, WriteNum, &TransNum, NULL);
	param[19] = 3;
	SetModifiedFlag();
	UpdateAllViews(NULL);
}