#!/bin/bash

rm -rf /paradigm/ &&
mkdir paradigm &&
rsync -rv --exclude=.git --exclude=/project_files/* --exclude=/builds/* /paradigm_local_git/* /paradigm &&
cd paradigm && 
chmod +x build.py &&
cd ..