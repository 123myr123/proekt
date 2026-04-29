FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive \
    LANG=C.UTF-8 \
    LC_ALL=C.UTF-8

RUN apt-get update && apt-get install -y --no-install-recommends \
    locales \
    && rm -rf /var/lib/apt/lists/* \
    && locale-gen en_US.UTF-8 \
    && printf '%s\n' \
        'Acquire::Retries "10";' \
        'Acquire::http::Timeout "120";' \
        'Acquire::https::Timeout "120";' \
        > /etc/apt/apt.conf.d/99retry

RUN bash -c 'apt-get update; \
for attempt in 1 2 3; do \
  if apt-get install -y \
    dbus-x11 \
    xfce4 \
    xfce4-terminal \
    tigervnc-standalone-server \
    novnc \
    websockify \
    supervisor \
    ca-certificates \
    curl \
    wget \
    git \
    openssh-client \
    build-essential \
    cmake \
    pkg-config \
    libgpiod-dev \
    libjsoncpp-dev \
    libsqlite3-dev \
    python3-pip \
    gdb \
    nano \
    vim \
    sudo \
  ; then \
    rm -rf /var/lib/apt/lists/*; exit 0; \
  fi; \
  echo "apt install failed (attempt $$attempt/3), waiting and retrying..."; \
  sleep 45; \
  apt-get update; \
done; \
exit 1'

RUN useradd -m -s /bin/bash student \
    && echo "student ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/student \
    && chmod 0440 /etc/sudoers.d/student

COPY docker/entrypoint.sh /entrypoint.sh
COPY docker/start-vnc.sh /usr/local/bin/start-vnc.sh
COPY docker/supervisord.conf /etc/supervisor/conf.d/iot-lab.conf

COPY device_core /opt/iot_workspace/device_core
COPY web /opt/iot_workspace/web
COPY sql /opt/iot_workspace/sql
COPY data /opt/iot_workspace/data
COPY deploy /opt/iot_workspace/deploy
COPY config.json.example /opt/iot_workspace/config.json.example

RUN sed -i 's/\r$//' /entrypoint.sh /usr/local/bin/start-vnc.sh \
    && pip3 install --no-cache-dir -r /opt/iot_workspace/web/requirements.txt \
    && chmod +x /entrypoint.sh /usr/local/bin/start-vnc.sh \
    && mkdir -p /home/student/workspace \
    && chown -R student:student /opt/iot_workspace /home/student \
    && bash -c 'mkdir -p /opt/iot_workspace/device_core/build && cd /opt/iot_workspace/device_core/build && cmake .. && cmake --build . -j"$(nproc)"' \
    && chown -R student:student /opt/iot_workspace

EXPOSE 6080 8080

WORKDIR /home/student

ENTRYPOINT ["/bin/bash", "/entrypoint.sh"]
