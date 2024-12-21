@echo off

IF EXIST bin (
	rmdir /s /q bin
	echo Deleted bin..
)

IF EXIST "Theia.sln" (
	del /q "Theia.sln"
	echo Deleted Solution File..
)

IF EXIST .vs (
	rmdir /s /q .vs
	echo Deleted VS Temp Files..
)

FOR %%f IN ("src\Theia.vcxproj", "src\Theia.vcxproj.filters", "src\Theia.vcxproj.user") DO (
	IF EXIST %%f (
		del /q %%f
		echo Deleted %%f
	)
)

echo Cleanup Finished!

PAUSE