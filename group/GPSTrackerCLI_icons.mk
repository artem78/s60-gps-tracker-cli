ifeq (WINS, $(findstring WINS, $(PLATFORM)))
ZDIR=$(EPOCROOT)epoc32\release\$(PLATFORM)\$(CFG)\Z
else
ZDIR=$(EPOCROOT)epoc32\data\z
endif

TARGETDIR=$(ZDIR)\resource\apps
ICONTARGETFILENAME=$(TARGETDIR)\GPSTrackerCLI.mif
ICONDIR=..\gfx

do_nothing:
	@rem do_nothing
	
MAKMAKE : do_nothing
BLD : do_nothing
CLEAN : do_nothing
LIB : do_nothing
CLEANDIR : do_nothing

RESOURCE : $(ICONTARGETFILENAME)

$(ICONTARGETFILENAME) : $(ICONDIR)\GPSTrackerCLI_icon.svg
	mifconv $(ICONTARGETFILENAME) \
		/x /c32 $(ICONDIR)\GPSTrackerCLI_icon.svg

FREEZE : do_nothing
SAVESPACE : do_nothing

RELEASABLES :
	@echo $(ICONTARGETFILENAME)
	
FINAL : do_nothing