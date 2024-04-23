@cl /LD dllmain.cpp dllmain_conhost.cpp miniaudio.cpp audio.cpp ntdll.lib /I. /std:c++20 /EHsc /link /out:getInput64.dll
@pause
@del *.obj *.lib *.exp