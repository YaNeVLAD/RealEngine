@echo off
setlocal enabledelayedexpansion

:: --- Конфигурация ---
set "OUTPUT_FILE=codebase_master.txt"

:: Список папок для игнорирования (через пробел)
set "IGNORE_DIRS=release .git .idea .vscode node_modules vendor bin obj .terraform"

:: Список файлов для игнорирования
set "IGNORE_FILES=go.sum package-lock.json %OUTPUT_FILE% %~nx0"

echo Starting to collect code into %OUTPUT_FILE%...

:: Очищаем или создаем выходной файл
type nul > "%OUTPUT_FILE%"

:: Рекурсивный обход всех файлов в текущей папке
for /r %%F in (*) do (
    set "filePath=%%F"
    set "fileName=%%~nxF"
    set "skip="

    :: 1. Проверка игнорируемых папок
    for %%D in (%IGNORE_DIRS%) do (
        echo %%F | findstr /i "\\%%D\\" >nul
        if !errorlevel! equ 0 set skip=1
    )

    :: 2. Проверка игнорируемых файлов
    for %%I in (%IGNORE_FILES%) do (
        if /i "!fileName!"=="%%I" set skip=1
    )

    :: 3. Если файл не в списке игнорирования
    if not defined skip (
        :: Проверка на текстовый файл (Batch не имеет mime-type, 
        :: используем findstr для поиска "нулевых" байтов как признак бинарника)
        findstr /v /p /l "" "%%F" >nul 2>&1
            echo Processing: %%~fF
            (
                echo ----------------------------
                :: Вывод относительного пути
                set "relPath=%%F"
                set "relPath=!relPath:%cd%\=!"
                echo !relPath!
                echo ----------------------------
                echo ```
                type "%%F"
                echo.
                echo ```
                echo.
                echo.
            ) >> "%OUTPUT_FILE%"
    )
)

echo Done. All code collected in %OUTPUT_FILE%
pause