@echo off
::
:: This script is Jenkins-compatible; don't make changes that will break it.
::
:: Requirements:
:: * doxygen.exe is in the PATH
:: * dot.exe is in the PATH (if enabled in doxygen config)
::
pushd "%~dp0"
doxygen.exe config.doxygen
popd
