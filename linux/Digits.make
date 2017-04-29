TARGET = DigitsVST.so 

CPP = g++
CXXFLAGS = -Wall -O3 -D__cdecl="" -I../sdks/vstsdk2.4 -fvisibility=hidden -DNO_EDITOR -DDIGITS_PRO -I../src/digits -I../src/components -fPIC

CSOURCES  = ../src/components/BufferManager.cpp \
		../src/components/Chorus.cpp ../src/components/Contour.cpp ../src/digits/PhaseDist.cpp ../src/digits/ResoGen.cpp ../src/components/SndBuf.cpp ../src/components/Tables.cpp ../src/components/Voice.cpp ../src/digits/VstCore.cpp ../sdks/vstsdk2.4/public.sdk/source/vst2.x/vstplugmain.cpp
OBJS      = $(CSOURCES:.cpp=.o)
ALLOBJ       = $(OBJS) $(VSTSDKDIR)/public.sdk/source/vst2.x/audioeffect.o $(VSTSDKDIR)/public.sdk/source/vst2.x/audioeffectx.o
SRCDIR    = ../src
VSTSDKDIR = ../sdks/vstsdk2.4

${TARGET} : $(ALLOBJ)
	$(CPP) -fPIC -DPIC -shared ${CXXFLAGS} -o ${TARGET} ${ALLOBJ}  
	strip ${TARGET}

%.o : $(SRCDIR)/%.cpp
	$(CPP) -c $< -o $@ $(CXXFLAGS)

#.c.o : 
#	${CC} ${CC_FLAGS} -c $<

#.cpp.o: 
#	${CC} ${CC_FLAGS} -c $<

clean :
	rm -f ${TARGET}
	rm -f ${ALLOBJ}
	rm -f *~
	rm -f source/*~


