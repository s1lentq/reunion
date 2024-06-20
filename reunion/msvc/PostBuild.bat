@echo OFF
::
:: Post-build auto-deploy script
:: Create and fill PublishPath.ini file with path to deployment folder
:: I.e. PublishPath.ini should contain one line with a folder path
:: Call it so:
:: IF EXIST "$(ProjectDir)PostBuild.bat" (CALL "$(ProjectDir)PostBuild.bat" "$(TargetDir)" "$(TargetName)" "$(TargetExt)" "$(ProjectDir)")
::

chcp 65001

SET targetDir=%~1
SET targetName=%~2
SET targetExt=%~3
SET projectDir=%~4

IF NOT EXIST "%projectDir%\PublishPath.ini" (
	ECHO No deployment path specified. Create PublishPath.ini near PostBuild.bat with paths on separate lines for auto deployment.
	exit /B 0
)

FOR /f "eol=; tokens=1-2 delims= usebackq" %%a IN ("%projectDir%\PublishPath.ini") DO (
	call :processParam %%a
)

goto:End

:processParam []

	set includeDll=1
	set includePdb=1

	IF [%2]==[Include] (
		set includeDll=0
		set includePdb=0

		IF [%3]==[dll] (
			set includeDll=1
		) ELSE IF [%3]==[pdb] (
			set includePdb=1
		)

		IF [%4]==[dll] (
			set includeDll=1
		) ELSE IF [%4]==[pdb] (
			set includePdb=1
		)
	)

	:: Remove quotes
	set v=%1
	set fullpath=%v:"=%

	IF NOT EXIST "%fullpath%" (
		@ECHO Invalid path specified: %fullpath%
	) ELSE (
		@ECHO Deploying to: %fullpath%
		IF NOT "%fullpath%" == "" (
			IF EXIST "%fullpath%" (
				IF [%includeDll%]==[1] (
					@copy /Y "%targetDir%%targetName%%targetExt%" "%fullpath%"
				)

				IF NOT ERRORLEVEL 1 (
					IF [%includePdb%]==[1] (
						IF EXIST "%targetDir%%targetName%.pdb" (
							@copy /Y "%targetDir%%targetName%.pdb" "%fullpath%"
						)
					)
				) ELSE (
					@ECHO PostBuild.bat ^(27^) : warning : Can't copy '%targetName%%targetExt%' to deploy path '%fullpath%'
				)
			)
		)
	)

:End
exit /B 0
