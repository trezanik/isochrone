#!/bin/sh
# Source Scopes: app, core, engine, imgui
if [ -z "$1" ]; then
	echo "Error: source scope must be supplied"
	exit 1
fi
scope=$1

# imgui source is .cpp extension
src_files=$(find src/$scope -name "*.cc" -o -name "*.cpp")
for i in $src_files; do
	echo "$i"
done

# redirection of stderr required as not all scopes have systen-specific sources
#sys_files=$(find sys/linux/src/$scope -name "*.cc" 2>/dev/null)
# actually not - meson only reads from stdout, so stderr items won't be included
sys_files=$(find sys/linux/src/$scope -name "*.cc")
for i in $sys_files; do
	echo "$i"
done
