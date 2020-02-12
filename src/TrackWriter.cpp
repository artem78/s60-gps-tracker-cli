/*
 ============================================================================
 Name		: TrackWriter.cpp
 Author	  : artem78
 Version	 : 1.0
 Copyright   : 
 Description : Classes for writing track to the file
 ============================================================================
 */

#include "TrackWriter.h"
#include "LBSSatelliteExtended.h"

// CTrackWriterBase

CTrackWriterBase::CTrackWriterBase(RFile &aFile) :
	iFile(aFile)
	{
	// No implementation required
	}


// CGPXTrackWriter

CGPXTrackWriter::CGPXTrackWriter(RFile &aFile, TBool anIsWriteExtendedData) :
	CTrackWriterBase(aFile),
	iIsWriteExtendedData(anIsWriteExtendedData)
	{
	// No implementation required
	}

CGPXTrackWriter::~CGPXTrackWriter()
	{
	// FixMe: Leave may occured in destructor
	
	CloseSegmentL();

	iXml->CloseTagL(); // </trk>
	iXml->CloseTagL(); // </gpx>
	
	delete iXml;
	}

CGPXTrackWriter* CGPXTrackWriter::NewLC(RFile &aFile, TBool anIsWriteExtendedData,
		const TDesC &aCreator)
	{
	CGPXTrackWriter* self = new (ELeave) CGPXTrackWriter(aFile, anIsWriteExtendedData);
	CleanupStack::PushL(self);
	self->ConstructL(aCreator);
	return self;
	}

CGPXTrackWriter* CGPXTrackWriter::NewL(RFile &aFile, TBool anIsWriteExtendedData,
		const TDesC &aCreator)
	{
	CGPXTrackWriter* self = CGPXTrackWriter::NewLC(aFile, anIsWriteExtendedData, aCreator);
	CleanupStack::Pop(); // self;
	return self;
	}

void CGPXTrackWriter::ConstructL(const TDesC &aCreator)
	{
	if (iCreator.MaxLength() >= aCreator.Length())
		iCreator.Copy(aCreator);
	else
		User::Leave(/*KErrArgument*/ /*KErrBadDescriptor*/ KErrOverflow);
	
	// Set general format for numbers
	iGeneralRealFormat = TRealFormat();
	iGeneralRealFormat.iType = KRealFormatFixed;
	iGeneralRealFormat.iPoint = '.';
	iGeneralRealFormat.iPlaces = 6;
	iGeneralRealFormat.iTriLen = 0;
	iGeneralRealFormat.iWidth = KDefaultRealWidth;
	
	// Create XML object
	iXml = CSimpleXMLWriter::NewL(iFile, TXMLVersion(1, 0), ETrue);
	
	iXml->OpenTagL(_L("gpx"));
	iXml->AddAttributeL(_L("xmlns"), _L("http://www.topografix.com/GPX/1/1"));
	iXml->AddAttributeL(_L("version"), _L("1.1"));
	iXml->AddAttributeL(_L("creator"), iCreator);
	iXml->AddAttributeL(_L("xmlns:xsi"),
			_L("http://www.w3.org/2001/XMLSchema-instance"));
	iXml->AddAttributeL(_L("xsi:schemaLocation"),
			_L("http://www.topografix.com/GPX/1/1 "
			"http://www.topografix.com/GPX/1/1/gpx.xsd"));
	iXml->OpenTagL(_L("trk"));
	}

void CGPXTrackWriter::AddPointL(const TPositionInfo* aPosInfo)
	{
	TPosition pos;
	aPosInfo->GetPosition(pos);
	
	// Get point`s date and time
	TBuf<30> timeBuff;
	_LIT(KTimeFormatISO8601, "%F%Y-%M-%DT%H:%T:%S.%*C3");
	pos.Time().FormatL(timeBuff, KTimeFormatISO8601);
	
	OpenSegmentL();
	
	// Write base position information
	iXml->OpenTagL(_L("trkpt"));
	iXml->AddAttributeL(_L("lat"), pos.Latitude(), iGeneralRealFormat);
	iXml->AddAttributeL(_L("lon"), pos.Longitude(), iGeneralRealFormat);
	
	iXml->OpenTagL(_L("ele"));
	iXml->AddTextL(pos.Altitude(), iGeneralRealFormat);
	iXml->CloseTagL(); // </ele>
	
	iXml->OpenTagL(_L("time"));
	iXml->AddTextL(timeBuff);
	iXml->CloseTagL(); // </time>
	
	
	// Write extended position information
	if (iIsWriteExtendedData)
		{
		// Process course info
		if (aPosInfo->PositionClassType() & EPositionCourseInfoClass)
			{
			const TPositionCourseInfo* courseInfo =
					static_cast<const TPositionCourseInfo*>(aPosInfo);
			TCourse course;
			courseInfo->GetCourse(course);
			
			// Course
			if (!Math::IsNaN(course.Heading()))
				{
				iXml->OpenTagL(_L("course"));
				iXml->AddTextL(course.Heading(), iGeneralRealFormat);
				iXml->CloseTagL(); // </course>
				}
			
			// Speed
			if (!Math::IsNaN(course.Speed()))
				{
				iXml->OpenTagL(_L("speed"));
				iXml->AddTextL(course.Speed(), iGeneralRealFormat);
				iXml->CloseTagL(); // </speed>
				}
			}
		
		// Process satellite info
		if (aPosInfo->PositionClassType() & EPositionSatelliteInfoClass)
			{
			const TPositionSatelliteInfoExtended* satelliteInfo =
					static_cast<const TPositionSatelliteInfoExtended*>(aPosInfo);
			
			// Satellites count
			iXml->OpenTagL(_L("sat"));
			iXml->AddTextL(satelliteInfo->NumSatellitesUsed());
			iXml->CloseTagL(); // </sat>
			
			// HDOP
			if (!Math::IsNaN(satelliteInfo->HorizontalDoP()))
				{
				iXml->OpenTagL(_L("hdop"));
				iXml->AddTextL(satelliteInfo->HorizontalDoP(), iGeneralRealFormat);
				iXml->CloseTagL(); // </hdop>
				}
			
			// VDOP
			if (!Math::IsNaN(satelliteInfo->VerticalDoP()))
				{
				iXml->OpenTagL(_L("vdop"));
				iXml->AddTextL(satelliteInfo->VerticalDoP(), iGeneralRealFormat);
				iXml->CloseTagL(); // </vdop>
				}
			
			// PDOP
			if (!Math::IsNaN(satelliteInfo->PositionDoP()))
				{
				iXml->OpenTagL(_L("pdop"));
				iXml->AddTextL(satelliteInfo->PositionDoP(), iGeneralRealFormat);
				iXml->CloseTagL(); // </pdop>
				}
			}
		}
	
	iXml->CloseTagL(); // </trkpt>
	}

void CGPXTrackWriter::StartNewSegmentL()
	{
	CloseSegmentL();
	}

void CGPXTrackWriter::OpenSegmentL()
	{
	if (!iIsSegmentOpened)
		{
		iXml->OpenTagL(_L("trkseg"));
		iIsSegmentOpened = ETrue;
		}
	}

void CGPXTrackWriter::CloseSegmentL()
	{
	if (iIsSegmentOpened)
		{
		iXml->CloseTagL(); // </trkseg>
		iIsSegmentOpened = EFalse;
		}
	}
