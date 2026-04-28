#!/bin/bash
set -e
export HOME=/home/student
export USER=student
exec vncserver :1 -geometry "${VNC_RESOLUTION:-1600x900}" -depth 24 -localhost no
