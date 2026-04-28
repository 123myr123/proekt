FROM python:3.12-alpine

LABEL org.opencontainers.image.title="SQL Student Lab"
LABEL org.opencontainers.image.description="Минимальная демонстрация SQL в Docker для учебных целей"

WORKDIR /app

RUN apk add --no-cache sqlite

COPY sql/ ./sql/
COPY app/ ./app/
COPY entrypoint.sh ./
RUN sed -i 's/\r$//' entrypoint.sh && chmod +x entrypoint.sh

ENV DB_PATH=/data/teach.db
ENV PYTHONDONTWRITEBYTECODE=1
ENV PYTHONUNBUFFERED=1

VOLUME ["/data"]

ENTRYPOINT ["./entrypoint.sh"]
