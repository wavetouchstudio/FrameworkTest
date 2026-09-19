@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

echo === Checking for local changes ===
git diff --quiet --exit-code
set LOCAL_UNSTAGED=%errorlevel%
git diff --cached --quiet --exit-code
set LOCAL_STAGED=%errorlevel%

set STASHED=0
if not "%LOCAL_UNSTAGED%%LOCAL_STAGED%"=="00" (
    echo Stashing local changes...
    git stash push -u -m "git-sync auto-stash"
    if errorlevel 1 (
        echo Stash failed. Aborting.
        pause
        exit /b 1
    )
    set STASHED=1
)

echo.
echo === Pulling ===
git pull
if errorlevel 1 (
    echo Pull failed. Resolve manually, then run this script again.
    if "!STASHED!"=="1" echo Your changes are stashed - run 'git stash pop' once fixed.
    pause
    exit /b 1
)

if "%STASHED%"=="1" (
    echo.
    echo === Restoring stashed changes ===
    git stash pop
    if errorlevel 1 (
        echo STASH POP CONFLICT - resolve manually before doing anything else.
        pause
        exit /b 1
    )
)

echo.
echo === Current status ===
git status --short
echo.

set /p COMMIT_MSG="Commit message (leave blank to skip commit/push): "
if "%COMMIT_MSG%"=="" (
    echo No message entered, skipping commit/push.
    goto :end
)

git add -A
git commit -m "%COMMIT_MSG%"
if errorlevel 1 (
    echo Nothing to commit or commit failed.
    goto :end
)

echo.
echo === Pushing ===
git push
if errorlevel 1 (
    echo Push failed. Check manually.
    pause
    exit /b 1
)

:end
echo.
echo Done.
pause
