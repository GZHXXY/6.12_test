@echo off
..\..\..\UITools\style_table -stylefile project.bin -prj 0x4
copy .\project.bin ..\..\..\..\ui_upgrade\upgrade.sty
copy .\result.bin  ..\..\..\..\ui_upgrade\upgrade.res
copy .\result.str  ..\..\..\..\ui_upgrade\upgrade.str
..\..\..\UITools\redefined -infile ename.h -prefix UPGRADE -outfile rename.h
copy .\version.txt  ..\..\..\..\ui_upgrade\upgrade.json
if exist "..\..\..\..\..\..\..\apps\watch\include\ui\" (
    copy .\rename.h     ..\..\..\..\..\..\..\apps\watch\include\ui\style_upgrade_new.h
)

pause
exit

