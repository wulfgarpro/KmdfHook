copy "..\Win7Debug\KmdfHelloWorld.sys" %HOMEPATH%\Desktop
..\Debug\SCLoadDriver.exe

REM devcon.exe install "..\Win7Debug\KmdfHelloWorld Package\KmdfHelloWorld.inf" Root\KmdfHelloWorld    