FROM python:3.12-slim

ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1

RUN apt-get update && apt-get install -y --no-install-recommends \
    sqlite3 \
    curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

COPY app /workspace/app
COPY sql /workspace/sql
COPY entrypoint.sh /entrypoint.sh

RUN pip install --no-cache-dir -r /workspace/app/requirements.txt \
    && sed -i 's/\r$//' /entrypoint.sh \
    && chmod +x /entrypoint.sh \
    && mkdir -p /workspace/data

EXPOSE 8090

ENTRYPOINT ["/entrypoint.sh"]
