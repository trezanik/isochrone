#!/bin/sh
# Does not require bash; compat mode works fine

# Simple check to see if doxygen exists - if anyone has any more 'reliable'
# ideas, please change this!
doxygen_exists=`which doxygen`
if [ "$doxygen_exists" = "" ]
then
  echo "'which' could not find doxygen; this must be installed!"
  exit
fi

# Note the current directory so we can return to it
cur_d="$(readlink -f $(dirname "$0"))"
old_d="$(pwd)"
cd "$cur_d"
# Script is within the doxygen folder; run doxygen with the dedicated config
doxygen ./config.doxygen

echo ">>> Finished"
