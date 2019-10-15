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

CTrackWriterBase::CTrackWriterBase(RFile &aFile) :
	iFile(aFile)
	{
	// No implementation required
	}

CTrackWriterBase::~CTrackWriterBase()
	{
	}

/*CTrackWriterBase* CTrackWriterBase::NewLC()
	{
	CTrackWriterBase* self = new (ELeave) CTrackWriterBase();
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
	}*/

/*CTrackWriterBase* CTrackWriterBase::NewL()
	{
	CTrackWriterBase* self = CTrackWriterBase::NewLC();
	CleanupStack::Pop(); // self;
	return self;
	}*/

/*void CTrackWriterBase::ConstructL()
	{

	}*/


CGPXTrackWriter::CGPXTrackWriter(RFile &aFile, TBool aWriteExtendedData) :
	CTrackWriterBase(aFile),
	iIsWriteExtendedData(aWriteExtendedData)
	{
	// No implementation required
	}

CGPXTrackWriter::~CGPXTrackWriter()
	{
	CloseSegment();
	
	_LIT8(KGPXContentEnd, "\t</trk>\n"
			"</gpx>");
	User::LeaveIfError(iFile.Write(KGPXContentEnd));
	}

CGPXTrackWriter* CGPXTrackWriter::NewLC(RFile &aFile, TBool aWriteExtendedData)
	{
	CGPXTrackWriter* self = new (ELeave) CGPXTrackWriter(aFile, aWriteExtendedData);
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
	}

CGPXTrackWriter* CGPXTrackWriter::NewL(RFile &aFile, TBool aWriteExtendedData)
	{
	CGPXTrackWriter* self = CGPXTrackWriter::NewLC(aFile, aWriteExtendedData);
	CleanupStack::Pop(); // self;
	return self;
	}

void CGPXTrackWriter::ConstructL()
	{
	// Set general format for numbers
	iGeneralRealFormat = TRealFormat();
	iGeneralRealFormat.iType = KRealFormatFixed;
	iGeneralRealFormat.iPoint = '.';
	iGeneralRealFormat.iPlaces = 6;
	iGeneralRealFormat.iTriLen = 0;
	iGeneralRealFormat.iWidth = KDefaultRealWidth;
	
	
	_LIT8(KGPXContentBegining, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
			"<gpx xmlns=\"http://www.topografix.com/GPX/1/1\" version=\"1.1\"\n"
			"creator=\"\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
			"xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd\">\n"
			"\t<trk>\n");
	User::LeaveIfError(iFile.Write(KGPXContentBegining));
	}

void CGPXTrackWriter::AddPoint(const TPositionInfo* aPosInfo)
	{
	TPosition pos;
	aPosInfo->GetPosition(pos);
	
	TBuf/*8*/<30> timeBuff;
	_LIT/*8*/(KTimeFormatISO8601, "%F%Y-%M-%DT%H:%T:%S.%*C3");
	pos.Time().FormatL(timeBuff, KTimeFormatISO8601);
	//TBuf8<30> timeBuff8;
	//timeBuff8.Copy(timeBuff);	
	
	/*_LIT8(KGPXContentTrackPoint, "\t\t\t<trkpt lat=\"%f\" lon=\"%f\">\n"
			"\t\t\t\t<ele>%f</ele>\n"
			"\t\t\t\t<time>%S</time>\n"
			"\t\t\t</trkpt>\n");*/	
	
	OpenSegment();
	
	//TBufC8<10> buff(_L8()("point\n"));
	TBuf8<512> buff; // ToDo: Too much for stack
	//buff.Format(KGPXContentTrackPoint, pos.Latitude(), pos.Longitude(), pos.Altitude(),
	//		&timeBuff8); // ToDo: Using Append will be more effective
	buff.Append(_L("\t\t\t<trkpt lat=\""));
	buff.AppendNum(pos.Latitude(), iGeneralRealFormat);
	buff.Append(_L("\" lon=\""));
	buff.AppendNum(pos.Longitude(), iGeneralRealFormat);
	buff.Append(_L("\">\n\t\t\t\t<ele>"));
	buff.AppendNum(pos.Altitude(), iGeneralRealFormat);
	buff.Append(_L("</ele>\n\t\t\t\t<time>"));
	buff.Append(timeBuff);
	buff.Append(_L("</time>\n"));
	
	// Extended position information
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
				buff.Append(_L("\t\t\t\t<course>"));
				buff.AppendNum(course.Heading(), iGeneralRealFormat);
				buff.Append(_L("</course>\n"));
				}
			
			// Speed
			if (!Math::IsNaN(course.Speed()))
				{
				buff.Append(_L("\t\t\t\t<speed>"));
				buff.AppendNum(course.Speed(), iGeneralRealFormat);
				buff.Append(_L("</speed>\n"));
				}
			}
		
		// Process satellite info
		if (aPosInfo->PositionClassType() & EPositionSatelliteInfoClass)
			{
			const TPositionSatelliteInfoExtended* satelliteInfo =
					static_cast<const TPositionSatelliteInfoExtended*>(aPosInfo);
			
			// Satellites count
			buff.Append(_L("\t\t\t\t<sat>"));
			buff.AppendNum(satelliteInfo->NumSatellitesUsed());
			buff.Append(_L("</sat>\n"));
			
			// HDOP
			buff.Append(_L("\t\t\t\t<hdop>"));
			buff.AppendNum(satelliteInfo->HorizontalDoP(), iGeneralRealFormat);
			buff.Append(_L("</hdop>\n"));
			
			// VDOP
			buff.Append(_L("\t\t\t\t<vdop>"));
			buff.AppendNum(satelliteInfo->VerticalDoP(), iGeneralRealFormat);
			buff.Append(_L("</vdop>\n"));
			
			// PDOP
			buff.Append(_L("\t\t\t\t<pdop>"));
			buff.AppendNum(satelliteInfo->PositionDoP(), iGeneralRealFormat);
			buff.Append(_L("</pdop>\n"));
			}
		}
	
	buff.Append(_L8("\t\t\t</trkpt>\n"));
	
	iFile.Write(buff); // ToDo: Catch possible errors
		// and rename AddPoint to AddPointL
	}

void CGPXTrackWriter::StartNewSegment()
	{
	CloseSegment();
	}

void CGPXTrackWriter::OpenSegment()
	{
	if (!iIsSegmentOpened)
		{
		iFile.Write(_L8("\t\t<trkseg>\n"));
		iIsSegmentOpened = ETrue;
		}
	}

void CGPXTrackWriter::CloseSegment()
	{
	if (iIsSegmentOpened)
		{
		iFile.Write(_L8("\t\t</trkseg>\n"));
		iIsSegmentOpened = EFalse;
		}
	}
