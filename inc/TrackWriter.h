/*
 ============================================================================
 Name		: TrackWriter.h
 Author	  : artem78
 Version	 : 1.0
 Copyright   : 
 Description : Classes for writing track to the file
 ============================================================================
 */

#ifndef TRACKWRITER_H
#define TRACKWRITER_H

// INCLUDES
#include <e32std.h>
#include <e32base.h>
#include <lbs.h>
#include <f32file.h>

// CLASS DECLARATION

/**
 *  CTrackWriterBase
 * 
 */
class CTrackWriterBase : public CBase
	{
public:
	// Constructors and destructor

	CTrackWriterBase(RFile &aFile);
	
	/**
	 * Destructor.
	 */
	~CTrackWriterBase();

	/**
	 * Two-phased constructor.
	 */
	//static CTrackWriterBase* NewL();

	/**
	 * Two-phased constructor.
	 */
	//static CTrackWriterBase* NewLC();
	
	virtual void AddPoint(const TPositionInfo* aPosInfo) = 0;

//private:

	/**
	 * Constructor for performing 1st stage construction
	 */
	//CTrackWriterBase(TPath aFilePath);

	/**
	 * EPOC default constructor for performing 2nd stage construction
	 */
	//void ConstructL();

protected:
	RFile iFile;

	};


class CGPXTrackWriter : public CTrackWriterBase
	{
public:
	// Constructors and destructor
	
	/**
	 * Destructor.
	 */
	~CGPXTrackWriter();

	/**
	 * Two-phased constructor.
	 */
	static CGPXTrackWriter* NewL(RFile &aFile, TBool aWriteExtendedData = EFalse,
			const TDesC &aCreator = KNullDesC);

	/**
	 * Two-phased constructor.
	 */
	static CGPXTrackWriter* NewLC(RFile &aFile, TBool aWriteExtendedData = EFalse,
			const TDesC &aCreator = KNullDesC);
	
	void AddPoint(const TPositionInfo* aPosInfo);
	void StartNewSegment();

private:

	/**
	 * Constructor for performing 1st stage construction
	 */
	CGPXTrackWriter(RFile &aFile, TBool aWriteExtendedData);

	/**
	 * EPOC default constructor for performing 2nd stage construction
	 */
	void ConstructL(const TDesC &aCreator);
	
	TRealFormat iGeneralRealFormat;
	TBool iIsSegmentOpened;
	TBool iIsWriteExtendedData;
	TBuf8<100> iCreator;
	
	void OpenSegment();
	void CloseSegment();

	};

#endif // TRACKWRITER_H
