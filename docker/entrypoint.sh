#!/bin/bash
set -e

VNC_PASSWORD="${VNC_PASSWORD:-changeme}"
export VNC_RESOLUTION="${VNC_RESOLUTION:-1600x900}"

export HOME=/home/student
export USER=student

mkdir -p "$HOME/workspace"
if [ ! -f "$HOME/workspace/web/app.py" ]; then
  cp -a /opt/iot_workspace/. "$HOME/workspace/"
  chown -R student:student "$HOME/workspace"
fi
if [ ! -x "$HOME/workspace/device_core/build/iot_device_core" ]; then
  mkdir -p "$HOME/workspace/device_core/build"
  cp -a /opt/iot_workspace/device_core/build/iot_device_core "$HOME/workspace/device_core/build/"
  chown student:student "$HOME/workspace/device_core/build/iot_device_core"
  cp -a /opt/iot_workspace/device_core/config.json "$HOME/workspace/device_core/"
  chown student:student "$HOME/workspace/device_core/config.json"
fi
if [ ! -f "$HOME/workspace/device_core/config.json" ]; then
  mkdir -p "$HOME/workspace/device_core"
  cp -a /opt/iot_workspace/device_core/config.json "$HOME/workspace/device_core/"
  chown student:student "$HOME/workspace/device_core/config.json"
fi
mkdir -p "$HOME/workspace/data"
chown student:student "$HOME/workspace/data"

mkdir -p "$HOME/.vnc"
echo "$VNC_PASSWORD" | vncpasswd -f > "$HOME/.vnc/passwd"
chmod 600 "$HOME/.vnc/passwd"

cat > "$HOME/.vnc/xstartup" << 'XS'
#!/bin/sh
unset SESSION_MANAGER
unset DBUS_SESSION_BUS_ADDRESS
[ -r "$HOME/.Xresources" ] && xrdb "$HOME/.Xresources"
exec dbus-launch --exit-with-session startxfce4
XS
chmod +x "$HOME/.vnc/xstartup"

rm -f /tmp/.X1-lock /tmp/.X11-unix/X1 2>/dev/null || true

chown -R student:student "$HOME"

exec /usr/bin/supervisord -n -c /etc/supervisor/supervisord.conf
