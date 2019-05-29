set PACK=WinHier-1.5
if not exist %PACK% mkdir %PACK%
copy READMEJP.txt %PACK%
copy README.txt %PACK%
copy Constants.txt %PACK%
copy WinHier32.exe %PACK%
copy WinHier64.exe %PACK%
copy MsgGetter32.exe %PACK%
copy MsgGetter64.exe %PACK%
copy MsgGet64.dll %PACK%
copy MsgGet32.dll %PACK%
copy LICENSE.txt %PACK%
if exist v2xker32.dll copy v2xker32.dll %PACK%
