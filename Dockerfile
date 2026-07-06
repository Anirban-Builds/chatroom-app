FROM gcc:latest

WORKDIR /app

RUN apt-get update && apt-get install -y\
    gcc make libssl-dev

COPY . .

RUN mkdir -p build
RUN make linux
EXPOSE 7860

CMD ["./build/chatroom"]